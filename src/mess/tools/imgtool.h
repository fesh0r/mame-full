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
	IMGTOOLERR_PARAMTOOSMALL,
	IMGTOOLERR_PARAMTOOLARGE,
	IMGTOOLERR_PARAMNEEDED,
	IMGTOOLERR_PARAMNOTNEEDED,
	IMGTOOLERR_BADFILENAME,
	IMGTOOLERR_NOSPACE
};

/* These error codes are actually modifiers that make it easier to distinguish
 * the cause of an error
 *
 * Note - drivers should not use these modifiers
 */
enum {
	IMGTOOLERR_SRC_MODULE			= 0x1000,
	IMGTOOLERR_SRC_FUNCTIONALITY	= 0x2000,
	IMGTOOLERR_SRC_IMAGEFILE		= 0x3000,
	IMGTOOLERR_SRC_FILEONIMAGE		= 0x4000,
	IMGTOOLERR_SRC_NATIVEFILE		= 0x5000,
	IMGTOOLERR_SRC_PARAM_CYLINDERS	= 0x6000,
	IMGTOOLERR_SRC_PARAM_HEADS		= 0x7000,

	IMGTOOLERR_SRC_MASK				= 0xffff
};

#define ERRORSOURCE(err)	((err) & IMGTOOLERR_SRC_MASK)

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
size_t stream_transfer_all(STREAM *dest, STREAM *source);

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

enum {
	IMAGE_USES_CYLINDERS	= 0x01,
	IMAGE_USES_HEADS		= 0x02
};

typedef struct {
	int cylinders;
	int heads;
} geometry_options;

typedef struct {
	int flags;
	geometry_options minimum;
	geometry_options maximum;
} geometry_ranges;

struct ImageModule {
	const char *name;
	const char *humanname;
	const char *fileextension;
	const char *crcfile;
	const char *crcsysname;
	const geometry_ranges *ranges;
	int (*init)(STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	int (*beginenum)(IMAGE *img, IMAGEENUM **outenum);
	int (*nextenum)(IMAGEENUM *enumeration, imgtool_dirent *ent);
	void (*closeenum)(IMAGEENUM *enumeration);
	size_t (*freespace)(IMAGE *img);
	int (*readfile)(IMAGE *img, const char *fname, STREAM *destf);
	int (*writefile)(IMAGE *img, const char *fname, STREAM *sourcef);
	int (*deletefile)(IMAGE *img, const char *fname);
	int (*create)(STREAM *f, const geometry_options *options);
};

/* ----------------------------------------------------------------------- */

/* Use IMAGEMODULE for (potentially) full featured images */
#define IMAGEMODULE(name,humanname,ext,crcfile,crcsysname,ranges,init,exit,beginenum,nextenum,closeenum,freespace,readfile,writefile,deletefile,create)	\
struct ImageModule imgmod_##name = \
{					\
	#name,			\
	(humanname),	\
	(ext),			\
	(crcfile),		\
	(crcsysname),	\
	(ranges),		\
	(init),			\
	(exit),			\
	(beginenum),	\
	(nextenum),		\
	(closeenum),	\
	(freespace),	\
	(readfile),		\
	(writefile),	\
	(deletefile),	\
	(create)		\
};

/* Use CARTMODULE for cartriges (where the only relevant option is CRC checking */
#define CARTMODULE(name,humanname,ext)	\
struct ImageModule imgmod_##name = \
{					\
	#name,			\
	(humanname),	\
	(ext),			\
	(#name ".crc"),	\
	#name,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL			\
};

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
int img_getfile(IMAGE *img, const char *fname, const char *dest);
int img_putfile(IMAGE *img, const char *fname, const char *source);
int img_deletefile(IMAGE *img, const char *fname);

/* These calls are for creating new images */
int img_create(const struct ImageModule *module, const char *fname, const geometry_options *options);
int img_create_byname(const char *modulename, const char *fname, const geometry_options *options);

typedef struct {
	unsigned long crc;
	char *longname;
	char *manufacturer;
	int year;
	char *playable;
	char *extrainfo;
	char buffer[1024];
} imageinfo;

/* img_getinfo loads a given image, and returns information about that image.
 * Any unknown info is NULL or zero
 */
int img_getinfo(const struct ImageModule *module, const char *fname, imageinfo *info);
int img_getinfo_byname(const char *modulename, const char *fname, imageinfo *info);

/* img_goodname loads an image and figures out what its "goodname" is and
 * returns the result in result (the callee needs to call free() on result)
 */
int img_goodname(const struct ImageModule *module, const char *fname, const char *base, char **result);
int img_goodname_byname(const char *modulename, const char *fname, const char *base, char **result);

#endif /* IMGTOOL_H */
