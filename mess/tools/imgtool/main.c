#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "imgtool.h"
#include "snprintf.h"
#include "utils.h"
#include "mess.h"
#include "main.h"
#include "fileio.h"
#include "modules.h"

/* ---------------------------------------------------------------------- */

static imgtool_library *library;

static void writeusage(FILE *f, int write_word_usage, struct command *c, char *argv[])
{
	fprintf(f, "%s %s %s %s\n",
		(write_word_usage ? "Usage:" : "      "),
		osd_basename(argv[0]),
		c->name,
		c->usage ? c->usage : "");
}

/* ----------------------------------------------------------------------- */

static int parse_options(int argc, char *argv[], int minunnamed, int maxunnamed, option_resolution *resolution, FILTERMODULE *filter)
{
	int i;
	int lastunnamed = 0;
	char *s;
	char *name = NULL;
	char *value = NULL;
	optreserr_t oerr;

	if (filter)
		*filter = NULL;

	for (i = 0; i < argc; i++)
	{
		/* Named or unamed arg */
		if ((argv[i][0] != '-') || (argv[i][1] != '-'))
		{
			/* Unnamed */
			if (i >= maxunnamed)
				goto error;	/* Too many unnamed */
			lastunnamed = i + 1;
		}
		else
		{
			/* Named */
			name = argv[i] + 2;
			s = strchr(name, '=');
			if (!s)
				goto error;
			*s = 0;
			value = s + 1;
			if (!strcmp(name, "filter"))
			{
				/* Filter option */
				if (!filter)
					goto error; /* This command doesn't use filters */
				if (*filter)
					goto filteralreadyspecified;
				*filter = filter_lookup(value);
				if (!(*filter))
					goto filternotfound;

			}
			else
			{
				/* Other named option */
				if (i < minunnamed)
					goto error; /* Too few unnamed */

				oerr = option_resolution_add_param(resolution, name, value);
				if (oerr)
					goto opterror;
			}
		}
	}
	return lastunnamed;

filternotfound:
	fprintf(stderr, "%s: Unknown filter type\n", value);
	return -1;

filteralreadyspecified:
	fprintf(stderr, "Cannot specify multiple filters\n");
	return -1;

opterror:
	fprintf(stderr, "%s: %s\n", name, option_resolution_error_string(oerr));
	return -1;

error:
	fprintf(stderr, "%s: Unrecognized option\n", argv[i]);
	return -1;
}

void reporterror(imgtoolerr_t err, const struct command *c, const char *format, const char *imagename,
	const char *filename, const char *newname, option_resolution *opts)
{
	const char *src = "imgtool";
	const char *err_name;

	err_name = imgtool_error(err);

	switch(ERRORSOURCE(err)) {
	case IMGTOOLERR_SRC_MODULE:
		src = format;
		break;
	case IMGTOOLERR_SRC_FUNCTIONALITY:
		src = c->name;
		break;
	case IMGTOOLERR_SRC_IMAGEFILE:
		src = imagename;
		break;
	case IMGTOOLERR_SRC_FILEONIMAGE:
		src = filename;
		break;
	case IMGTOOLERR_SRC_NATIVEFILE:
		src = newname ? newname : filename;
		break;
	}
	fflush(stdout);
	fflush(stderr);

	if (!src)
		src = c->name;
	fprintf(stderr, "%s: %s\n", src, err_name);
}

/* ----------------------------------------------------------------------- */

static int cmd_dir(const struct command *c, int argc, char *argv[])
{
	int err, total_count, total_size, freespace_err;
	int freespace;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char buf[512];
	char attrbuf[50];

	/* attempt to open image */
	err = img_open_byname(library, argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	err = img_beginenum(img, &imgenum);
	if (err) {
		img_close(img);
		goto error;
	}

	memset(&ent, 0, sizeof(ent));
	ent.fname = buf;
	ent.fname_len = sizeof(buf);
	ent.attr = attrbuf;
	ent.attr_len = sizeof(attrbuf);

	total_count = 0;
	total_size = 0;

	fprintf(stdout, "Contents of %s:\n", argv[1]);

	img_info(img, buf, sizeof(buf));
	if (buf[0])
		fprintf(stdout, "%s\n", buf);
	fprintf(stdout, "------------------------  ------ ---------------\n");

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof)
	{
		fprintf(stdout, "%-20s\t%8d %15s\n", ent.fname, ent.filesize, ent.attr);
		total_count++;
		total_size += ent.filesize;
	}

	freespace_err = img_freespace(img, &freespace);

	img_closeenum(imgenum);
	img_close(img);

	if (err)
		goto error;

	fprintf(stdout, "------------------------  ------ ---------------\n");
	fprintf(stdout, "%8i File(s)        %8i bytes\n", total_count, total_size);
	if (!freespace_err)
		fprintf(stdout, "                        %8d bytes free\n", freespace);
	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, NULL);
	return -1;
}

static int cmd_get(const struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;
	char *newfname;
	int unnamedargs;
	FILTERMODULE filter;

	err = img_open_byname(library, argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	unnamedargs = parse_options(argc, argv, 3, 4, NULL, &filter);
	if (unnamedargs < 0)
		return -1;
	newfname = (unnamedargs == 4) ? argv[3] : NULL;

	err = img_getfile(img, argv[2], newfname, filter);
	img_close(img);
	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], argv[2], argv[3], NULL);
	return -1;
}

static int cmd_put(const struct command *c, int argc, char *argv[])
{
	int err, i;
	IMAGE *img;
	const char *fname = NULL;
	int unnamedargs;
	FILTERMODULE filter;
	const struct ImageModule *module;
	option_resolution *resolution = NULL;

	module = imgtool_library_findmodule(library, argv[0]);
	if (!module)
	{
		err = IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;
		goto error;
	}

	if (module->writefile_optguide && module->writefile_optspec)
	{
		resolution = option_resolution_create(module->writefile_optguide, module->writefile_optspec);
		if (!resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto error;
		}
	}


	unnamedargs = parse_options(argc, argv, 3, 0xffff, resolution, &filter);
	if (unnamedargs < 0)
		return -1;

	err = img_open(module, argv[1], OSD_FOPEN_RW, &img);
	if (err)
		goto error;

	for (i = 2; i < unnamedargs; i++)
	{
		fname = argv[i];
		printf("Putting file '%s'...\n", fname);
		err = img_putfile(img, NULL, fname, resolution, filter);
		if (err)
			goto error;
	}

	img_close(img);
	if (resolution)
		option_resolution_close(resolution);
	return 0;

error:
	if (img)
		img_close(img);
	if (resolution)
		option_resolution_close(resolution);
	reporterror(err, c, argv[0], argv[1], fname, NULL, resolution);
	return -1;
}

static int cmd_getall(const struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	FILTERMODULE filter;
	int unnamedargs;
	char buf[128];

	err = img_open_byname(library, argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	unnamedargs = parse_options(argc, argv, 2, 2, NULL, &filter);
	if (unnamedargs < 0)
		return -1;

	err = img_beginenum(img, &imgenum);
	if (err) {
		img_close(img);
		goto error;
	}

	memset(&ent, 0, sizeof(ent));
	ent.fname = buf;
	ent.fname_len = sizeof(buf);

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof)
	{
		fprintf(stdout, "Retrieving %s (%i bytes)\n", ent.fname, ent.filesize);

		err = img_getfile(img, ent.fname, NULL, filter);
		if (err)
			break;
	}

	img_closeenum(imgenum);
	img_close(img);

	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, NULL);
	return -1;
}

static int cmd_del(const struct command *c, int argc, char *argv[])
{
	imgtoolerr_t err;
	IMAGE *img;

	err = img_open_byname(library, argv[0], argv[1], OSD_FOPEN_RW, &img);
	if (err)
		goto error;

	err = img_deletefile(img, argv[2]);
	img_close(img);
	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], argv[2], argv[3], NULL);
	return -1;
}

static int cmd_create(const struct command *c, int argc, char *argv[])
{
	imgtoolerr_t err;
	int unnamedargs;
	const struct ImageModule *module;
	option_resolution *resolution = NULL;

	module = imgtool_library_findmodule(library, argv[0]);
	if (!module)
	{
		err = IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;
		goto error;
	}

	if (module->createimage_optguide && module->createimage_optspec)
	{
		resolution = option_resolution_create(module->createimage_optguide, module->createimage_optspec);
		if (!resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto error;
		}
	}

	unnamedargs = parse_options(argc, argv, 2, 3, resolution, NULL);
	if (unnamedargs < 0)
		return -1;

	err = img_create(module, argv[1], resolution);
	if (err)
		goto error;

	if (resolution)
		option_resolution_close(resolution);
	return 0;

error:
	if (resolution)
		option_resolution_close(resolution);
	reporterror(err, c, argv[0], argv[1], NULL, NULL, 0);
	return -1;
}

static int cmd_listformats(const struct command *c, int argc, char *argv[])
{
	const struct ImageModule *mod;

	fprintf(stdout, "Image formats supported by imgtool:\n\n");

	mod = imgtool_library_findmodule(library, NULL);
	while(mod)
	{
		fprintf(stdout, "  %-20s%s\n", mod->name, mod->description);
		mod = mod->next;
	}

	return 0;
}

static int cmd_listfilters(const struct command *c, int argc, char *argv[])
{
	FILTERMODULE *f = filters;

	fprintf(stdout, "Filters supported by imgtool:\n\n");

	while(*f) {
		fprintf(stdout, "  %-10s%s\n", (*f)->name, (*f)->humanname);
		f++;
	}

	return 0;
}

static void listoptions(const struct OptionGuide *opt_guide, const char *opt_spec)
{
	char opt_name[32];
	const char *opt_desc;
	struct OptionRange range[32];
	char range_buffer[512];
	char buf[32];
	int i;

	assert(opt_guide);

	fprintf(stdout, "Option           Allowed values                 Description\n");
	fprintf(stdout, "---------------- ------------------------------ -----------\n");

	while(opt_guide->option_type != OPTIONTYPE_END)
	{
		range_buffer[0] = 0;
		snprintf(opt_name, sizeof(opt_name) / sizeof(opt_name[0]), "--%s", opt_guide->identifier);
		opt_desc = opt_guide->display_name;

		/* is this option relevant? */
		if (!strchr(opt_spec, opt_guide->parameter))
		{
			opt_guide++;
			continue;
		}

		switch(opt_guide->option_type) {
		case OPTIONTYPE_INT:
			option_resolution_listranges(opt_spec, opt_guide->parameter,
				range, sizeof(range) / sizeof(range[0]));

			for (i = 0; range[i].max >= 0; i++)
			{
				snprintf(buf, sizeof(buf) / sizeof(buf[0]),
					(range[i].min == range[i].max) ? "%i" : "%i-%i",
					range[i].min,
					range[i].max);

				if (i > 0)
					strncatz(range_buffer, "/", sizeof(range_buffer));
				strncatz(range_buffer, buf, sizeof(range_buffer));
			}
			break;

		case OPTIONTYPE_ENUM_BEGIN:
			i = 0;
			while(opt_guide[1].option_type == OPTIONTYPE_ENUM_VALUE)
			{
				if (i > 0)
					strncatz(range_buffer, "/", sizeof(range_buffer));
				strncatz(range_buffer, opt_guide[1].identifier, sizeof(range_buffer));

				opt_guide++;
				i++;
			}
			break;

		case OPTIONTYPE_STRING:
			snprintf(range_buffer, sizeof(range_buffer), "(string)");
			break;
		default:
			assert(0);
			break;
		}

		fprintf(stdout, "%16s %-30s %s\n",
			opt_name,
			range_buffer,
			opt_desc);
		opt_guide++;
	}
}



static int cmd_listdriveroptions(const struct command *c, int argc, char *argv[])
{
	const struct ImageModule *mod;
	const struct OptionGuide *opt_guide;

	mod = imgtool_library_findmodule(library, argv[0]);
	if (!mod)
		goto error;

	fprintf(stdout, "Driver specific options for module '%s':\n\n", argv[0]);

	/* list write options */
	opt_guide = mod->writefile_optguide;
	if (opt_guide)
	{
		fprintf(stdout, "Image specific file options (usable on the 'put' command):\n\n");
		listoptions(opt_guide, mod->writefile_optspec);
		puts("\n");
	}
	else
	{
		fprintf(stdout, "No image specific file options\n\n");
	}

	/* list create options */
	opt_guide = mod->createimage_optguide;
	if (opt_guide)
	{
		fprintf(stdout, "Image specific creation options (usable on the 'create' command):\n\n");
		listoptions(opt_guide, mod->createimage_optspec);
		puts("\n");
	}
	else
	{
		fprintf(stdout, "No image specific creation options\n\n");
	}

	return 0;

error:
	reporterror(IMGTOOLERR_MODULENOTFOUND|IMGTOOLERR_SRC_MODULE, c, argv[0], NULL, NULL, NULL, NULL);
	return -1;
}

/* ----------------------------------------------------------------------- */

#ifdef MAME_DEBUG
static int cmd_test(const struct command *c, int argc, char *argv[])
{
	int err;
	const char *module_name;

	module_name = (argc > 0) ? argv[0] : NULL;

	if (module_name)
		fprintf(stdout, "Running test suite for module '%s':\n\n", module_name);
	else
		fprintf(stdout, "Running test suite for all modules\n\n");

	err = imgtool_test_byname(library, module_name);
	if (err)
		goto error;

	fprintf(stdout, "...Test succeeded!\n");
	return 0;

error:
	reporterror(err, c, argv[0], NULL, NULL, NULL, NULL);
	return -1;
}
#endif

/* ----------------------------------------------------------------------- */

static struct command cmds[] =
{
#ifdef MAME_DEBUG
	{ "test",				cmd_test,				"<format>", 0, 1, 0 },
//	{ "testsuite",			cmd_testsuite,			"<testsuitefile>", 1, 1, 0 },
#endif
	{ "create",				cmd_create,				"<format> <imagename>", 2, 8, 0},
	{ "dir",				cmd_dir,				"<format> <imagename>...", 2, 2, 1 },
	{ "get",				cmd_get,				"<format> <imagename> <filename> [newname] [--filter=filter]", 3, 4, 0 },
	{ "put",				cmd_put,				"<format> <imagename> <filename>...[--(fileoption)==value] [--filter=filter]", 3, 0xffff, 0 },
	{ "getall",				cmd_getall,				"<format> <imagename> [--filter=filter]", 2, 2, 0 },
	{ "del",				cmd_del,				"<format> <imagename> <filename>...", 3, 3, 1 },
	{ "listformats",		cmd_listformats,		NULL, 0, 0, 0 },
	{ "listfilters",		cmd_listfilters,		NULL, 0, 0, 0 },
	{ "listdriveroptions",	cmd_listdriveroptions, "<format>", 1, 1, 0 }
};

#ifdef WIN32
#include "glob.h"
void win_expand_wildcards(int *argc, char **argv[])
{
	int i;
	glob_t g;

	memset(&g, 0, sizeof(g));

	for (i = 0; i < *argc; i++)
		glob((*argv)[i], (g.gl_pathc > 0) ? GLOB_APPEND|GLOB_NOCHECK : GLOB_NOCHECK, NULL, &g);

	*argc = g.gl_pathc;
	*argv = g.gl_pathv;
}
#endif

int CLIB_DECL main(int argc, char *argv[])
{
	int i;
	int result;
	struct command *c;
	imgtoolerr_t err;

#ifdef WIN32
	win_expand_wildcards(&argc, &argv);
#endif

	putchar('\n');

	if (argc > 1)
	{
		/* figure out what command they are running, and run it */
		for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++)
		{
			c = &cmds[i];
			if (!stricmp(c->name, argv[1]))
			{
				/* check argument count */
				if (c->minargs > (argc - 2))
					goto cmderror;

				/* build module library */
				err = imgtool_create_cannonical_library(&library);
				if (err)
				{
					reporterror(err, NULL, NULL, NULL, NULL, NULL, NULL);
					result = -1;
					goto done;
				}

				if (c->lastargrepeats && (argc > c->maxargs))
				{
					for (i = c->maxargs+1; i < argc; i++)
					{
						argv[c->maxargs+1] = argv[i];

						result = c->cmdproc(c, c->maxargs, argv + 2);
						if (result)
							goto done;
					}
					result = 0;
					goto done;
				}
				else
				{
					if ((c->maxargs > 0) && (c->maxargs < (argc - 2)))
						goto cmderror;

					result = c->cmdproc(c, argc - 2, argv + 2);
					goto done;
				}
			}
		}
	}

	/* Usage */
	fprintf(stderr, "imgtool - Generic image manipulation tool for use with MESS\n\n");
	for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++)
	{
		writeusage(stdout, (i == 0), &cmds[i], argv);
	}

	fprintf(stderr, "\n<format> is the image format, e.g. rsdos\n");
	fprintf(stderr, "<imagename> is the image filename; can specify a ZIP file for image name\n");

	fprintf(stderr, "\nExample usage:\n");
	fprintf(stderr, "\t%s dir rsdos myimageinazip.zip\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s get rsdos myimage.dsk myfile.bin mynewfile.txt\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s getall rsdos myimage.dsk\n", osd_basename(argv[0]));
	result = 0;
	goto done;

cmderror:
	writeusage(stdout, 1, &cmds[i], argv);
	result = -1;

done:
	if (library)
		imgtool_library_close(library);
	return result;
}

