#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "imgtool.h"
#include "utils.h"

struct command {
	const char *name;
	const char *usage;
	int (*cmdproc)(struct command *c, int argc, char *argv[]);
	int minargs;
	int maxargs;
	int lastargrepeats;
};

static void writeusage(FILE *f, int write_word_usage, struct command *c, char *argv[])
{
	fprintf(f, "%s %s %s %s\n",
		(write_word_usage ? "Usage:" : "      "),
		osd_basename(argv[0]),
		c->name,
		c->usage ? c->usage : "");
}

/* ----------------------------------------------------------------------- */

static int parse_options(int argc, char *argv[], int minunnamed, int maxunnamed, struct NamedOption *opts, int optcount)
{
	int i;
	int optpos = 0;
	int lastunnamed;
	char *s;

	memset(opts, 0, sizeof(struct NamedOption) * optcount);

	for (i = 0; i < argc; i++) {
		/* Named or unamed arg */
		if ((argv[i][0] != '-') || (argv[i][1] != '-')) {
			/* Unnamed */
			if (i >= maxunnamed)
				goto error;	/* Too many unnamed */
			lastunnamed = i;
		}
		else {
			if (i < minunnamed)
				goto error; /* Too few unnamed */
			if (optpos >= optcount)
				goto error; /* Too many */

			/* Named */
			opts[optpos].name = argv[i] + 2;
			s = strchr(opts[optpos].name, '=');
			if (!s)
				goto error;
			*(s++) = 0;
			opts[optpos].value = s;
			optpos++;
		}
	}
	return 0;

error:
	fprintf(stderr, "%s: Unrecognized option\n", argv[i]);
	return -1;
}

static void reporterror(int err, struct command *c, const char *format, const char *imagename,
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
			default:
				assert(FALSE);
				break;
			};

			optiontemplate += ERRORPARAM(err);

			switch(optiontemplate->flags & IMGOPTION_FLAG_TYPE_MASK) {
			case IMGOPTION_FLAG_TYPE_INTEGER:
				sprintf(buf, "--%s should be between %i and %i", optiontemplate->name, optiontemplate->min, optiontemplate->max);
				break;
			case IMGOPTION_FLAG_TYPE_CHAR:
				sprintf(buf, "--%s should be between '%c' and '%c'", optiontemplate->name, optiontemplate->min, optiontemplate->max);
				break;
			default:
				sprintf(buf, "--%s: %s", err_name);
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
	char buf[128];
	char attrbuf[50];

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
	if (img->module->info) {
		char string[500];
		img->module->info(img, string, sizeof(string));
		fprintf(stdout,"%s\n",string);
	}
	fprintf(stdout, "------------------------  ------ ---------------\n");

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof) {
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

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err)
		goto error;

	err = img_getfile(img, argv[2], (argc == 4) ? argv[3] : NULL);
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
	int err;
	IMAGE *img;
	char *newfname;
	int unnamedargs;
	struct NamedOption nopts[32];

	newfname = (argc >= 4) && (argv[3][0] != '-') ? argv[3] : NULL;

	unnamedargs = parse_options(argc, argv, 3, 4, nopts, sizeof(nopts) / sizeof(nopts[0]));
	if (unnamedargs < 0)
		return -1;
	newfname = (unnamedargs == 4) ? argv[3] : NULL;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_RW, &img);
	if (err)
		goto error;

	err = img_putfile(img, newfname, argv[2], nopts);
	img_close(img);
	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], argv[2], newfname, nopts);
	return -1;
}

static int cmd_getall(struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char buf[128];

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

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof) {
		fprintf(stdout, "Retrieving %s (%i bytes)\n", ent.fname, ent.filesize);

		err = img_getfile(img, ent.fname, NULL);
		if (err)
			break;
	}

	img_closeenum(imgenum);
	img_close(img);

	if (err)
		goto error;

	return 0;

error:
	reporterror(err, c, argv[0], argv[1], argv[2], argv[3], NULL);
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

	unnamedargs = parse_options(argc, argv, 2, 3, nopts, sizeof(nopts) / sizeof(nopts[0]));
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
	const struct ImageModule **mod;
	size_t modcount;

	fprintf(stdout, "Image formats supported by imgtool:\n\n");

	mod = getmodules(&modcount);
	while(modcount--) {
		fprintf(stdout, "  %-10s%s\n", (*mod)->name, (*mod)->humanname);
		mod++;
	}

	return 0;
}

/* ----------------------------------------------------------------------- */

static struct command cmds[] = {
	{ "create", "<format> <imagename> [<internalname>] "
	  "[--heads=HEADS] [--cylinders=CYLINDERS] "
	  "[--sectors=SECTORS] [--sectorsize=SIZE] "
	  "[--htype=HARDWARETYPE] [--exrom=LEVEL] [--game=LEVEL]", 
	  cmd_create, 2, 8, 0},
	{ "dir", "<format> <imagename>...", cmd_dir, 2, 2, 1 },
	{ "get", "<format> <imagename> <filename> [newname]", cmd_get, 3, 4, 0 },
	{ "put", "<format> <imagename> <filename> [newname] [--ftype=FILETYPE] [--ascii=0|1] [--addr=ADDR] [--bank=BANK]", cmd_put, 3, 8, 0 },
	{ "getall", "<format> <imagename>", cmd_getall, 2, 2, 0 },
	{ "del", "<format> <imagename> <filename>...", cmd_del, 3, 3, 1 },
	{ "info", "<format> <imagename>...", cmd_info, 2, 2, 1 },
	{ "crc", "<format> <imagename>...", cmd_crc, 2, 2, 1 },
	{ "good", "<format> <imagename>...", cmd_good, 2, 2, 1 },
	{ "listformats", NULL, cmd_listformats, 0, 0, 0 }
};

int CLIB_DECL main(int argc, char *argv[])
{
	int i;
	int result;
	struct command *c;

	putchar('\n');

	if (argc > 1) {
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
