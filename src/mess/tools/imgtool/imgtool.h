#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "mess/mess.h"

#if !defined(MAX)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Error codes */
enum {
	IMGTOOLERR_OUTOFMEMORY = 1,
	IMGTOOLERR_UNEXPECTED,
	IMGTOOLERR_BUFFERTOOSMALL,
	IMGTOOLERR_READERROR,
	IMGTOOLERR_WRITEERROR,
	IMGTOOLERR_READONLY,
	IMGTOOLERR_CORRUPTIMAGE,
	IMGTOOLERR_FILENOTFOUND,
	IMGTOOLERR_MODULENOTFOUND,
	IMGTOOLERR_UNIMPLEMENTED,
	IMGTOOLERR_BADPARAM,
	IMGTOOLERR_BADFILENAME,
	IMGTOOLERR_NOSPACE
};

typedef struct {
	char *fname;
	size_t fname_len;
	size_t filesize;
	int eof;
	int corrupt;
} imgtool_dirent;

/* ----------------------------------------------------------------------- */

enum {
	IMG_FILE,
	IMG_MEM
};

typedef struct {
	int imgtype;
	int write_protect;
	union {
		FILE *f;
		struct {
			char *buf;
			size_t bufsz;
			size_t pos;
		} m;
	} u;
} STREAM;

STREAM *stream_open(const char *fname, int read_or_write);	/* similar params to osd_fopen */
void stream_close(STREAM *f);
size_t stream_read(STREAM *f, void *buf, size_t sz);
size_t stream_write(STREAM *f, const void *buf, size_t sz);
size_t stream_size(STREAM *f);
int stream_seek(STREAM *f, size_t pos, int where);

/* Transfers sz bytes from source to dest */
size_t stream_transfer(STREAM *dest, STREAM *source, size_t sz);

/* Fills sz bytes with b */
size_t stream_fill(STREAM *f, unsigned char b, size_t sz);

/* Returns the CRC of a file */
int stream_crc(STREAM *f, unsigned long *result);
int file_crc(const char *fname,  unsigned long *result);

/* ----------------------------------------------------------------------- */

typedef struct {
	const struct ImageModule *module;
} IMAGE;

typedef struct {
	const struct ImageModule *module;
} IMAGEENUM;

typedef struct {
	int min_cylinders;
	int max_cylinders;
	int min_heads;
	int max_heads;
} createimgparams;

struct ImageModule {
	const char *name;
	const char *humanname;
	int (*init)(STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	int (*beginenum)(IMAGE *img, IMAGEENUM **outenum);
	int (*nextenum)(IMAGEENUM *enumeration, imgtool_dirent *ent);
	void (*closeenum)(IMAGEENUM *enumeration);
	size_t (*freespace)(IMAGE *img);
	int (*readfile)(IMAGE *img, const char *fname, STREAM *destf);
	int (*writefile)(IMAGE *img, const char *fname, STREAM *sourcef);
	int (*deletefile)(IMAGE *img, const char *fname);
	int (*create)(STREAM *f);
};

#define IMAGEMODULE(name,humanname,init,exit,beginenum,nextenum,closeenum,freespace,readfile,writefile,deletefile,create)	\
	struct ImageModule imgmod_##name = \
		{ #name, (humanname), (init), (exit), (beginenum), (nextenum), (closeenum), (freespace), \
			(readfile), (writefile), (deletefile), (create) };

/* ----------------------------------------------------------------------- */

const struct ImageModule *findimagemodule(const char *name);
const struct ImageModule **getmodules(size_t *len);
const char *imageerror(int err);

/* ----------------------------------------------------------------------- */

int img_open(const struct ImageModule *module, const char *fname, int read_or_write, IMAGE **outimg);
int img_open_byname(const char *modulename, const char *fname, int read_or_write, IMAGE **outimg);
void img_close(IMAGE *img);
int img_beginenum(IMAGE *img, IMAGEENUM **outenum);
int img_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
void img_closeenum(IMAGEENUM *enumeration);
int img_freespace(IMAGE *img, size_t *sz);
int img_readfile(IMAGE *img, const char *fname, STREAM *destf);
int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef);
int img_deletefile(IMAGE *img, const char *fname);

/* These calls are for creating new images */
int img_create(const struct ImageModule *module, const char *fname);
int img_create_byname(const char *modulename, const char *fname);

#endif /* IMGTOOL_H */
