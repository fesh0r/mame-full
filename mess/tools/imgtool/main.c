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

/* ---------------------------------------------------------------------- */
/* HACK */

const struct GameDriver *drivers[1];
int rompath_extra;
int cheatfile;
const char *db_filename;
int history_filename;
int mameinfo_filename;

/* ---------------------------------------------------------------------- */


static void writeusage(FILE *f, int write_word_usage, struct command *c, char *argv[])
{
	fprintf(f, "%s %s %s %s\n",
		(write_word_usage ? "Usage:" : "      "),
		osd_basename(argv[0]),
		c->name,
		c->usage ? c->usage : "");
}

/* ----------------------------------------------------------------------- */

static int parse_options(int argc, char *argv[], int minunnamed, int maxunnamed, struct NamedOption *opts, int optcount, FILTERMODULE *filter)
{
	int i;
	int optpos = 0;
	int lastunnamed = 0;
	char *s;
	char *name = NULL;
	char *value = NULL;

	if (filter)
		*filter = NULL;

	memset(opts, 0, sizeof(struct NamedOption) * optcount);

	for (i = 0; i < argc; i++) {
		/* Named or unamed arg */
		if ((argv[i][0] != '-') || (argv[i][1] != '-')) {
			/* Unnamed */
			if (i >= maxunnamed)
				goto error;	/* Too many unnamed */
			lastunnamed = i + 1;
		}
		else {
			/* Named */
			name = argv[i] + 2;
			s = strchr(name, '=');
			if (!s)
				goto error;
			*s = 0;
			value = s + 1;
			if (!strcmp(name, "filter")) {
				/* Filter option */
				if (!filter)
					goto error; /* This command doesn't use filters */
				if (*filter)
					goto filteralreadyspecified;
				*filter = filter_lookup(value);
				if (!(*filter))
					goto filternotfound;

			}
			else {
				/* Other named option */
				if (i < minunnamed)
					goto error; /* Too few unnamed */
				if (optpos >= optcount)
					goto error; /* Too many */

				opts[optpos].name = name;
				opts[optpos].value = value;
				optpos++;
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

error:
	fprintf(stderr, "%s: Unrecognized option\n", argv[i]);
	return -1;
}

void reporterror(int err, struct command *c, const char *format, const char *imagename,
	const char *filename, const char *newname, const struct NamedOption *namedoptions)
{
	char buf[256];
	const char *src = "imgtool";
	const char *err_name;

	err_name = imageerror(err);

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
	case IMGTOOLERR_SRC_PARAM_USER:
		sprintf(buf, "%s is not a valid option for this command", namedoptions[ERRORPARAM(err)].name);
		err_name = buf;
		break;
	case IMGTOOLERR_SRC_PARAM_FILE:
	case IMGTOOLERR_SRC_PARAM_CREATE:
		{
			int paramnum;
			const struct ImageModule *module;
			const struct OptionTemplate *optiontemplate = NULL;

			module = findimagemodule(format);
			paramnum = ERRORPARAM(err);

			switch(ERRORSOURCE(err)) {
			case IMGTOOLERR_SRC_PARAM_FILE:
				optiontemplate = module->fileoptions_template;
				break;
			case IMGTOOLERR_SRC_PARAM_CREATE:
				optiontemplate = module->createoptions_template;
				break;
			};

			assert(optiontemplate);

			optiontemplate += ERRORPARAM(err);

			switch(optiontemplate->flags & IMGOPTION_FLAG_TYPE_MASK) {
			case IMGOPTION_FLAG_TYPE_INTEGER:
				sprintf(buf, "--%s should be between %i and %i", optiontemplate->name, optiontemplate->min, optiontemplate->max);
				break;
			case IMGOPTION_FLAG_TYPE_CHAR:
				sprintf(buf, "--%s should be between '%c' and '%c'", optiontemplate->name, optiontemplate->min, optiontemplate->max);
				break;
			default:
				sprintf(buf, "--%s: %s", err_name, err_name);
				break;
			}
			err_name = buf;
		}
		break;
	}
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "%s: %s\n", src, err_name);
}

/* ----------------------------------------------------------------------- */

static int cmd_dir(struct command *c, int argc, char *argv[])
{
	int err, total_count, total_size, freespace_err;
	int freespace;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char buf[512];
	char attrbuf[50];

	/* attempt to open image */
	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
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

static int cmd_get(struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;
	char *newfname;
	int unnamedargs;
	FILTERMODULE filter;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	unnamedargs = parse_options(argc, argv, 3, 4, NULL, 0, &filter);
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

static int cmd_put(struct command *c, int argc, char *argv[])
{
	int err, i;
	IMAGE *img;
	const char *fname = NULL;
	int unnamedargs;
	FILTERMODULE filter;
	struct NamedOption nopts[32];

	unnamedargs = parse_options(argc, argv, 3, 0xffff, nopts, sizeof(nopts) / sizeof(nopts[0]), &filter);
	if (unnamedargs < 0)
		return -1;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_RW, &img);
	if (err)
		goto error;

	for (i = 2; i < unnamedargs; i++)
	{
		fname = argv[i];
		printf("Putting file '%s'...\n", fname);
		err = img_putfile(img, NULL, fname, nopts, filter);
		if (err)
			goto error;
	}

	img_close(img);
	return 0;

error:
	if (img)
		img_close(img);
	reporterror(err, c, argv[0], argv[1], fname, NULL, nopts);
	return -1;
}

static int cmd_getall(struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	FILTERMODULE filter;
	int unnamedargs;
	char buf[128];

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	unnamedargs = parse_options(argc, argv, 2, 2, NULL, 0, &filter);
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

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof) {
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

static int cmd_del(struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_RW, &img);
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

static int cmd_info(struct command *c, int argc, char *argv[])
{
	int err;
	imageinfo info;

	err = img_getinfo_byname(argv[0], argv[1], &info);
	if (err)
		goto error;

	fprintf(stdout, "%s:\n----\n", argv[1]);
	if (info.longname)
		fprintf(stdout, "Long Name:    %s\n", info.longname);
	if (info.manufacturer)
		fprintf(stdout, "Manufacturer: %s\n", info.manufacturer);
	if (info.year)
		fprintf(stdout, "Year:         %d\n", info.year);
	if (info.playable)
		fprintf(stdout, "Playable:     %s\n", info.playable);
	fprintf(stdout, "CRC:          %08x\n", (int)info.crc);
	if (info.extrainfo)
		fprintf(stdout, "Extra Info:   %s\n", info.extrainfo);
        fprintf(stdout, "\n");
	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, NULL);
	return -1;
}

static int cmd_crc(struct command *c, int argc, char *argv[])
{
	int err;
	imageinfo info;
	char yearbuf[8];

	err = img_getinfo_byname(argv[0], argv[1], &info);
	if (err)
		goto error;

	if (info.year)
		sprintf(yearbuf, "%d", info.year);

	fprintf(stdout, "%08x = %s | %s | %s | %s\n",
		(int) info.crc,
                info.longname ? info.longname : osd_basename(argv[1]),
		info.year ? yearbuf : "",
		info.manufacturer ? info.manufacturer : "",
		info.playable ? info.playable : "");
	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, NULL);
	return -1;
}

#define GOOD_DIR	"Good"

static int cmd_good(struct command *c, int argc, char *argv[])
{
	int err;
	char *goodname;
	char *s;
	STREAM *input;
	STREAM *output;
	char gooddir[32];
	int len;

	memset(gooddir, 0, sizeof(gooddir));
	strcpy(gooddir, GOOD_DIR);
	s = gooddir + strlen(gooddir);

	strncpy(s, argv[0], sizeof(gooddir) - strlen(gooddir) - 2);
	len = strlen(s);
	s[len] = PATH_SEPARATOR;
	s[len+1] = '\0';
	s[0] = toupper(s[0]);

	osd_mkdir(gooddir);

	err = img_goodname_byname(argv[0], argv[1], gooddir, &goodname);
	if (err)
		goto error;

	if (!goodname) {
		fprintf(stderr, "%s: Unrecognized image\n", argv[1]);
		return -1;
	}

	fprintf(stdout, "%s ==> %s\n", argv[1], goodname);

	input = stream_open(argv[1], OSD_FOPEN_READ);
	if (!input) {
		free(goodname);
		fprintf(stderr, "%s: Cannot open file\n", argv[1]);
		return -1;
	}

	output = stream_open(goodname, OSD_FOPEN_WRITE);
	if (!output) {
		fprintf(stderr, "%s: Cannot create file\n", goodname);
		free(goodname);
		return -1;
	}

	stream_transfer_all(output, input);
	stream_close(input);
	stream_close(output);
	free(goodname);
	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, NULL);
	return -1;
}

static int cmd_create(struct command *c, int argc, char *argv[])
{
	int err;
	int unnamedargs;
	char *label;
	struct NamedOption nopts[32];

	unnamedargs = parse_options(argc, argv, 2, 3, nopts, sizeof(nopts) / sizeof(nopts[0]), NULL);
	if (unnamedargs < 0)
		return -1;

	/* HACK - Need to do something with this! */
	label = (unnamedargs >= 3) ? argv[2] : NULL;

	err = img_create_byname(argv[0], argv[1], nopts);
	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], NULL, NULL, nopts);
	return -1;
}

static int cmd_listformats(struct command *c, int argc, char *argv[])
{
	const struct ImageModule *mod;
	size_t modcount;

	fprintf(stdout, "Image formats supported by imgtool:\n\n");

	mod = getmodules(&modcount);
	while(modcount--) {
		fprintf(stdout, "  %-20s%s\n", mod->name, mod->humanname);
		mod++;
	}

	return 0;
}

static int cmd_listfilters(struct command *c, int argc, char *argv[])
{
	FILTERMODULE *f = filters;

	fprintf(stdout, "Filters supported by imgtool:\n\n");

	while(*f) {
		fprintf(stdout, "  %-10s%s\n", (*f)->name, (*f)->humanname);
		f++;
	}

	return 0;
}

static void listoptions(const struct OptionTemplate *opttemplate)
{
	const char *attrtypename;
	const char *minval;
	const char *maxval;
	char buf1[256];
	char buf2[16];
	char buf3[16];

	assert(opttemplate);

	fprintf(stdout, "Option   Type      Min  Max  Default Description\n");
	fprintf(stdout, "-------- --------- ---- ---- ------- -----------\n");

	while(opttemplate->name) {
		minval = maxval = "";

		/* Min/Max/Default representations are dependant on type */
		switch(opttemplate->flags & IMGOPTION_FLAG_TYPE_MASK) {
		case IMGOPTION_FLAG_TYPE_INTEGER:
			attrtypename = "(integer)";
			snprintf(buf2, sizeof(buf2) / sizeof(buf2[0]), "%i", opttemplate->min);
			snprintf(buf3, sizeof(buf3) / sizeof(buf3[0]), "%i", opttemplate->max);
			minval = buf2;
			maxval = buf3;
			break;
		case IMGOPTION_FLAG_TYPE_CHAR:
			attrtypename = "(char)";
			buf2[0] = opttemplate->min;
			buf2[1] = '\0';
			buf3[0] = opttemplate->max;
			buf3[1] = '\0';
			minval = buf2;
			maxval = buf3;
			break;
		case IMGOPTION_FLAG_TYPE_STRING:
			attrtypename = "(string)";
			break;
		default:
			assert(0);
			attrtypename = NULL;
			break;
		}

		snprintf(buf1, sizeof(buf1) / sizeof(buf1[0]), "--%s", opttemplate->name);

		fprintf(stdout, "%8s %-9s %4s %4s %7s %s\n", buf1, attrtypename, minval, maxval,
			opttemplate->defaultvalue ? opttemplate->defaultvalue : "",
			opttemplate->description ? opttemplate->description : "");
		opttemplate++;
	}
}

static int cmd_listdriveroptions(struct command *c, int argc, char *argv[])
{
	const struct ImageModule *mod;
	const struct OptionTemplate *opttemplate;

	mod = findimagemodule(argv[0]);
	if (!mod)
		goto error;

	fprintf(stdout, "Driver specific options for module '%s':\n\n", argv[0]);

	opttemplate = mod->fileoptions_template;
	if (opttemplate) {
		fprintf(stdout, "Image specific file options (usable on the 'put' command): %s\n\n", argv[0]);
		listoptions(opttemplate);
		puts("\n");
	}
	else {
		fprintf(stdout, "No image specific file options\n\n");
	}

	opttemplate = mod->createoptions_template;
	if (opttemplate) {
		fprintf(stdout, "Image specific creation options (usable on the 'create' command):%s\n\n", argv[0]);
		listoptions(opttemplate);
		puts("\n");
	}
	else {
		fprintf(stdout, "No image specific creation options\n\n");
	}

	return 0;

error:
	reporterror(IMGTOOLERR_MODULENOTFOUND|IMGTOOLERR_SRC_MODULE, c, argv[0], NULL, NULL, NULL, NULL);
	return -1;
}

/* ----------------------------------------------------------------------- */

#ifdef MAME_DEBUG
static int cmd_test(struct command *c, int argc, char *argv[])
{
	int err;
	const char *module_name;

	module_name = (argc > 0) ? argv[0] : NULL;

	if (module_name)
		fprintf(stdout, "Running test suite for module '%s':\n\n", module_name);
	else
		fprintf(stdout, "Running test suite for all modules\n\n");

	err = imgtool_test_byname(module_name);
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

static struct command cmds[] = {
#ifdef MAME_DEBUG
	{ "test", "<format>", cmd_test, 0, 1, 0 },
	{ "testsuite", "<testsuitefile>", cmd_testsuite, 1, 1, 0 },
#endif
	{ "create", "<format> <imagename>", cmd_create, 2, 8, 0},
	{ "dir", "<format> <imagename>...", cmd_dir, 2, 2, 1 },
	{ "get", "<format> <imagename> <filename> [newname] [--filter=filter]", cmd_get, 3, 4, 0 },
	{ "put", "<format> <imagename> <filename>...[--(fileoption)==value] [--filter=filter]", cmd_put, 3, 0xffff, 0 },
	{ "getall", "<format> <imagename> [--filter=filter]", cmd_getall, 2, 2, 0 },
	{ "del", "<format> <imagename> <filename>...", cmd_del, 3, 3, 1 },
	{ "info", "<format> <imagename>...", cmd_info, 2, 2, 1 },
	{ "crc", "<format> <imagename>...", cmd_crc, 2, 2, 1 },
	{ "good", "<format> <imagename>...", cmd_good, 2, 2, 1 },
	{ "listformats", NULL, cmd_listformats, 0, 0, 0 },
	{ "listfilters", NULL, cmd_listfilters, 0, 0, 0 },
	{ "listdriveroptions", "<format>", cmd_listdriveroptions, 1, 1, 0 }
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

#ifdef WIN32
	win_expand_wildcards(&argc, &argv);
#endif

	putchar('\n');

	if (argc > 1) {
		/* figure out what command they are running, and run it */
		for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++) {
			c = &cmds[i];
			if (!stricmp(c->name, argv[1])) {
				/* Check argument count */
				if (c->minargs > (argc - 2))
					goto cmderror;

				if (c->lastargrepeats && (argc > c->maxargs)) {
					for (i = c->maxargs+1; i < argc; i++) {
						argv[c->maxargs+1] = argv[i];

						result = c->cmdproc(c, c->maxargs, argv + 2);
						if (result)
							return result;
					}
					return 0;
				}
				else {
					if ((c->maxargs > 0) && (c->maxargs < (argc - 2)))
						goto cmderror;
					return c->cmdproc(c, argc - 2, argv + 2);
				}
			}
		}
	}

	/* Usage */
	fprintf(stderr, "imgtool - Generic image manipulation tool for use with MESS\n\n");
	for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++) {
		writeusage(stdout, (i == 0), &cmds[i], argv);
	}

	fprintf(stderr, "\n<format> is the image format, e.g. rsdos\n");
	fprintf(stderr, "<imagename> is the image filename; can specify a ZIP file for image name\n");

	fprintf(stderr, "\nExample usage:\n");
	fprintf(stderr, "\t%s dir rsdos myimageinazip.zip\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s get rsdos myimage.dsk myfile.bin mynewfile.txt\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s getall rsdos myimage.dsk\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s info nes myimage.dsk\n", osd_basename(argv[0]));
	fprintf(stderr, "\t%s good nes mynescart.zip\n", osd_basename(argv[0]));
	return 0;

cmderror:
	writeusage(stdout, 1, &cmds[i], argv);
	return -1;
}

/* ----------------------------------------------------------------------- */
/* total hack */

mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite)
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "crc/%s", filename);
	return (mame_file *) fopen(buffer, "r");
}

char *mame_fgets(char *s, int n, mame_file *file)
{
	return fgets(s, n, (FILE *) file);
}

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	return fwrite(buffer, 1, length, (FILE *) file);
}

void mame_fclose(mame_file *file)
{
	fclose((FILE *) file);
}

