#include <stdio.h>
#include <string.h>
#include "imgtool.h"

FILE *errorlog;

struct command {
	const char *name;
	const char *usage;
	int (*cmdproc)(struct command *c, int argc, char *argv[]);
	int minargs;
	int maxargs;
};

static void writeusage(FILE *f, int write_word_usage, struct command *c, char *argv[])
{
	fprintf(f, "%s %s %s %s\n",
		(write_word_usage ? "Usage:" : "      "),
		argv[0],
		c->name,
		c->usage ? c->usage : "");
}

/* ----------------------------------------------------------------------- */

static int cmd_dir(struct command *c, int argc, char *argv[])
{
	int err, total_count, total_size, freespace_err;
	size_t freespace;
	IMAGE *img;
	IMAGEENUM *imgenum;
	imgtool_dirent ent;
	char buf[128];

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[(err == IMGTOOLERR_MODULENOTFOUND) ? 0 : 1], imageerror(err));
		return -1;
	}

	err = img_beginenum(img, &imgenum);
	if (err) {
		img_close(img);
		fprintf(stderr, "%s: %s\n", argv[1], imageerror(err));
		return -1;
	}

	ent.eof = 0;
	ent.fname = buf;
	ent.fname_len = sizeof(buf);
	total_count = 0;
	total_size = 0;

	fprintf(stdout, "\nContents of %s:\n", argv[1]);
	fprintf(stdout, "------------------------  ------\n");

	while (((err = img_nextenum(imgenum, &ent)) == 0) && !ent.eof) {
		fprintf(stdout, "%-20s\t%8ld\n", ent.fname, ent.filesize);
		total_count++;
		total_size += ent.filesize;
	}

	freespace_err = img_freespace(img, &freespace);

	img_closeenum(imgenum);
	img_close(img);

	if (err) {
		fprintf(stderr, "%s: %s\n", argv[1], imageerror(err));
		return -1;
	}

	fprintf(stdout, "------------------------  ------\n");
    fprintf(stdout, "%8i File(s)        %8i bytes\n", total_count, total_size);
	if (!freespace_err)
		fprintf(stdout, "                        %8ld bytes free\n", freespace);
	return 0;
}

static int cmd_get(struct command *c, int argc, char *argv[])
{
	int err;
	const char *outfname;
	IMAGE *img;
	STREAM *f;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_READ, &img);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[(err == IMGTOOLERR_MODULENOTFOUND) ? 0 : 1], imageerror(err));
		return -1;
	}

	outfname = (argc == 4) ? argv[3] : argv[2];
	f = stream_open(outfname, OSD_FOPEN_WRITE);
	if (!f) {
		img_close(img);
		fprintf(stderr, "%s: Cannot create file\n", outfname);
		return -1;
	}

	err = img_readfile(img, argv[2], f);
	if (err) {
		stream_close(f);
		img_close(img);
		fprintf(stderr, "%s: %s\n", argv[2], imageerror(err));
		return -1;
	}

	stream_close(f);
	img_close(img);
	return 0;
}

static int cmd_put(struct command *c, int argc, char *argv[])
{
	int err;
	const char *outfname;
	IMAGE *img;
	STREAM *f;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_RW, &img);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[(err == IMGTOOLERR_MODULENOTFOUND) ? 0 : 1], imageerror(err));
		return -1;
	}

	f = stream_open(argv[2], OSD_FOPEN_READ);
	if (!f) {
		img_close(img);
		fprintf(stderr, "%s: Cannot read file\n", argv[2]);
		return -1;
	}

	outfname = (argc == 4) ? argv[3] : argv[2];
	err = img_writefile(img, outfname, f);
	if (err) {
		stream_close(f);
		img_close(img);
		fprintf(stderr, "%s: %s\n", outfname, imageerror(err));
		return -1;
	}

	stream_close(f);
	img_close(img);
	return 0;
}

static int cmd_del(struct command *c, int argc, char *argv[])
{
	int err;
	IMAGE *img;

	err = img_open_byname(argv[0], argv[1], OSD_FOPEN_RW, &img);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[(err == IMGTOOLERR_MODULENOTFOUND) ? 0 : 1], imageerror(err));
		return -1;
	}

	err = img_deletefile(img, argv[2]);
	img_close(img);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[2], imageerror(err));
		return -1;
	}

	return 0;
}

static int cmd_crc(struct command *c, int argc, char *argv[])
{
	int err;
	unsigned long result;

	err = file_crc(argv[0], &result);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[0], imageerror(err));
		return -1;
	}

	fprintf(stdout, "%s: CRC=0x%08lx\n", argv[0], result);
	return 0;
}

static int cmd_create(struct command *c, int argc, char *argv[])
{
	int err;

	err = img_create_byname(argv[0], argv[1]);
	if (err) {
		fprintf(stderr, "%s: %s\n", argv[err == IMGTOOLERR_UNIMPLEMENTED ? 0 : 1], imageerror(err));
		return -1;
	}
	return 0;
}

static int cmd_listformats(struct command *c, int argc, char *argv[])
{
	const struct ImageModule **mod;
	size_t modcount;

	fprintf(stdout, "\nImage formats supported by imgtool:\n\n");

	mod = getmodules(&modcount);
	while(modcount--) {
		fprintf(stdout, "\t%s\t%s\n", (*mod)->name, (*mod)->humanname);
		mod++;
	}

	return 0;
}

/* ----------------------------------------------------------------------- */

static struct command cmds[] = {
	{ "create", "<format> <imagename>", cmd_create, 2, 2},
	{ "dir", "<format> <imagename>", cmd_dir, 2, 2 },
	{ "get", "<format> <imagename> <filename> [newname]", cmd_get, 3, 4 },
	{ "put", "<format> <imagename> <filename> [newname]", cmd_put, 3, 4 },
	{ "del", "<format> <imagename> <filename>", cmd_del, 3, 3 },
	{ "crc", "<imagename>", cmd_crc, 1, 1 },
	{ "listformats", NULL, cmd_listformats, 0, 0 }
};

#ifndef WIN32
#define DECL_SPEC
#endif

int DECL_SPEC main(int argc, char *argv[])
{
	size_t i;

	errorlog = stderr;

	if (argc > 1) {
		for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++) {
			if (!stricmp(cmds[i].name, argv[1])) {
				/* Check argument count */
				if ((cmds[i].minargs > (argc - 2)) || ((cmds[i].maxargs > 0) && (cmds[i].maxargs < (argc - 2)))) {
					writeusage(stdout, 1, &cmds[i], argv);
					return 0;
				}
				return cmds[i].cmdproc(&cmds[i], argc - 2, argv + 2);
			}
		}
	}

	/* Usage */
	fprintf(stderr, "\nimgtool - Generic image manipulation tool for use with MESS\n\n");
	for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++) {
		writeusage(stdout, (i == 0), &cmds[i], argv);
	}

	fprintf(stderr, "\n<format> is the image format, e.g. rsdos\n");
	fprintf(stderr, "<imagename> is the image filename; can specify a ZIP file for image name\n");

	fprintf(stderr, "\nExample usage:\n");
	fprintf(stderr, "\t%s dir rsdos myimageinazip.zip\n", argv[0]);
	fprintf(stderr, "\t%s get rsdos myimage.dsk myfile.bin mynewfile.txt\n", argv[0]);
	fprintf(stderr, "\t%s crc myimage.dsk\n", argv[0]);
	return 0;
}
