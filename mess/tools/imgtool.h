#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "osdepend.h"
#include "mess.h"

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
	IMGTOOLERR_PARAMCORRUPT,
	IMGTOOLERR_BADFILENAME,
	IMGTOOLERR_NOSPACE,
	IMGTOOLERR_INPUTPASTEND
};

/* These error codes are actually modifiers that make it easier to distinguish
 * the cause of an error
 *
 * Note - drivers should not use these modifiers
 */
#define IMGTOOLERR_SRC_MODULE			0x1000
#define	IMGTOOLERR_SRC_FUNCTIONALITY	0x2000
#define	IMGTOOLERR_SRC_IMAGEFILE		0x3000
#define	IMGTOOLERR_SRC_FILEONIMAGE		0x4000
#define	IMGTOOLERR_SRC_NATIVEFILE		0x5000
#define	IMGTOOLERR_SRC_PARAM_USER		0x6000
#define	IMGTOOLERR_SRC_PARAM_FILE		0x7000
#define	IMGTOOLERR_SRC_PARAM_CREATE		0x8000

#define ERRORCODE(err)		((err) & 0x0fff)
#define ERRORSOURCE(err)	((err) & 0xf000)
#define ERRORPARAM(err)		(((err) & 0xf0000) / 0x10000)

#define PARAM_TO_ERROR(errcode, param)	((errcode) | ((param) * 0x10000))

typedef struct {
	char *fname;
	size_t fname_len;
	char *attr;
	int attr_len;
	int filesize;
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
	const char *name; // needed for clear
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
STREAM *stream_open_write_stream(int filesize);
STREAM *stream_open_mem(void *buf, size_t sz);
void stream_close(STREAM *f);
size_t stream_read(STREAM *f, void *buf, size_t sz);
size_t stream_write(STREAM *f, const void *buf, size_t sz);
size_t stream_size(STREAM *f);
int stream_seek(STREAM *f, size_t pos, int where);
/* works currently only for IMG_FILE
   clears FILE on harddisk! */
void stream_clear(STREAM *f);
/* truncate with ansi c library not easy to implement */
//void stream_truncate(STREAM *f, size_t newsize);

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

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

enum {
	IMGOPTION_FLAG_TYPE_INTEGER	= 0x0000,
	IMGOPTION_FLAG_TYPE_CHAR	= 0x0001,	/* case insensitive; upper case forced */
	IMGOPTION_FLAG_TYPE_STRING	= 0x0002,
	IMGOPTION_FLAG_TYPE_MASK	= 0x000f,

	IMGOPTION_FLAG_HASDEFAULT	= 0x0010
};

struct OptionTemplate {
	const char *name;
	int flags;
	int min;
	int max;
	const char *defaultvalue;
};

struct NamedOption {
	const char *name;
	const char *value;
};

typedef union {
	int i;			/* integer or char */
	const char *s;	/* string */
} ResolvedOption;

enum {
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE	= 0x0001	/* this means that all filenames are normally upper case */
};

struct ImageModule {
	const char *name;
	const char *humanname;
	const char *fileextension;
	const char *crcfile;
	const char *crcsysname;
	const char *eoln;
	int flags;

	int (*init)(STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	void (*info)(IMAGE *img, char *string, const int len);
	int (*beginenum)(IMAGE *img, IMAGEENUM **outenum);
	int (*nextenum)(IMAGEENUM *enumeration, imgtool_dirent *ent);
	void (*closeenum)(IMAGEENUM *enumeration);
	size_t (*freespace)(IMAGE *img);
	int (*readfile)(IMAGE *img, const char *fname, STREAM *destf);
	int (*writefile)(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *writeoptions);
	int (*deletefile)(IMAGE *img, const char *fname);
	int (*create)(STREAM *f, const ResolvedOption *createoptions);

	/* size must be set with the size of the buffer,
	   if the buffer is too small it will be relocated with realloc */
	int (*read_sector)(IMAGE *img, int head, int track, int sector, char **buffer, int *size);
	int (*write_sector)(IMAGE *img, int head, int track, int sector, char *buffer, int size);

	const struct OptionTemplate *fileoptions_template;
	const struct OptionTemplate *createoptions_template;

	void *extra;
};

/* ----------------------------------------------------------------------- */

/* Use IMAGEMODULE for (potentially) full featured images */
#define IMAGEMODULE(name,humanname,ext,crcfile,crcsysname,eoln,flags,\
		init,exit,info,beginenum,nextenum,closeenum,freespace,readfile,writefile, \
		deletefile,create,read_sector,write_sector,fileoptions_template,createoptions_template)	\
struct ImageModule imgmod_##name = \
{								\
	#name,						\
	(humanname),				\
	(ext),						\
	(crcfile),					\
	(crcsysname),				\
	(eoln),						\
	(flags),					\
	(init),						\
	(exit),						\
	(info),						\
	(beginenum),				\
	(nextenum),					\
	(closeenum),				\
	(freespace),				\
	(readfile),					\
	(writefile),				\
	(deletefile),				\
	(create),					\
	(read_sector),				\
	(write_sector),				\
	(fileoptions_template),		\
	(createoptions_template),	\
	NULL			\
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
	0,				\
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
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL			\
};

/* ---------------------------------------------------------------------------
 * Image calls
 *
 * These are the calls that front ends should use for manipulating images. You
 * should never call the module functions directly because they may not be
 * implemented (i.e. - the function pointers are NULL). The img_* functions are
 * aware of these issues and will make the appropriate checks as well as
 * marking up return codes with the source.  In addition, some of the img_*
 * calls are high level calls that simply image manipulation
 *
 * Calls that return 'int' that are not explictly noted otherwise return
 * imgtool error codes
 * ---------------------------------------------------------------------------
 */

/* findimagemodule
 *
 * Description:
 *		Looks up the ImageModule with a certain name
 *
 * Parameters:
 *		name:				The name of the module
 *
 * Returns:
 *		The module with that name.  Returns NULL if the name is unknown
 */
const struct ImageModule *findimagemodule(const char *name);

/* getmodules
 *
 * Description:
 *		Retrieves the list of modules
 *
 * Parameters:
 *		len:				Place to receive the length of the list
 *
 * Returns:
 *		An array of pointers to the modules
 */
const struct ImageModule **getmodules(size_t *len);

/* imageerror
 *
 * Description:
 *		Returns a human readable string
 *
 * Parameters:
 *		err:				The error return code
 */
const char *imageerror(int err);

/* img_open
 * img_open_byname
 *
 * Description:
 *		Opens an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		read_or_write:		Open mode (use OSD_FOPEN_* constants)
 *		outimg:				Placeholder for image pointer
 */
int img_open(const struct ImageModule *module, const char *fname, int read_or_write, IMAGE **outimg);
int img_open_byname(const char *modulename, const char *fname, int read_or_write, IMAGE **outimg);

/* img_close
 *
 * Description:
 *		Closes an image
 *
 * Parameters:
 *		img:				The image to close
 */
void img_close(IMAGE *img);

/* img_info
 *
 * Description:
 *		Returns format specific information about an image
 *
 * Parameters:
 *		img:				The image to query the info of
 *		string:				Buffer to place info in
 *		len:				Length of buffer
 */
int img_info(IMAGE *img, char *string, const int len);

/* img_beginenum
 *
 * Description:
 *		Begins enumerating files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		outenum:			The resulting enumeration
 */
int img_beginenum(IMAGE *img, IMAGEENUM **outenum);

/* img_nextenum
 *
 * Description:
 *		Continues enumerating files within an image
 *
 * Parameters:
 *		enumeration:		The enumeration
 *		ent:				Place to receive directory entry
 */
int img_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);

/* img_closeenum
 *
 * Description:
 *		Closes an enumeration
 *
 * Parameters:
 *		enumeration:		The enumeration to close
 */
void img_closeenum(IMAGEENUM *enumeration);

/* img_freespace
 *
 * Description:
 *		Returns free space on an image, in bytes
 *
 * Parameters:
 *		img:				The image to query
 *		sz					Place to receive free space
 */
int img_freespace(IMAGE *img, int *sz);

/* img_readfile
 *
 * Description:
 *		Start reading from a file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 */
int img_readfile(IMAGE *img, const char *fname, STREAM *destf);

/* img_writefile
 *
 * Description:
 *		Start writing to a new file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 *		options:			Options to specify on the new file
 */
int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const struct NamedOption *_options);

/* img_getfile
 *
 * Description:
 *		Read a file from an image, storing it into a native file
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		dest:				Filename for native file to write to
 */
int img_getfile(IMAGE *img, const char *fname, const char *dest);

/* img_putfile
 *
 * Description:
 *		Read a native file and store it on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		newfname:			The filename on the image to store (if NULL, then
 *							the file will be named basename(source)
 *		source:				Native filename for source
 *		options:			Options to specify on the new file
 */
int img_putfile(IMAGE *img, const char *newfname, const char *source, const struct NamedOption *_options);

/* img_deletefile
 *
 * Description:
 *		Delete a file on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 */
int img_deletefile(IMAGE *img, const char *fname);

/* img_create
 * img_create_byname
 *
 * Description:
 *		Creates an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		options:			Options that control how the image is created
 *							(tracks, sectors, etc)
 */
int img_create(const struct ImageModule *module, const char *fname, const struct NamedOption *_options);
int img_create_byname(const char *modulename, const char *fname, const struct NamedOption *_options);

typedef struct {
	unsigned long crc;
	char *longname;
	char *manufacturer;
	int year;
	char *playable;
	char *extrainfo;
	char buffer[1024];
} imageinfo;

/* img_getinfo
 * img_getinfo_byname
 *
 * Description:
 *		Retrieves information about an image from the CRC databases
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		info				Place to receive information about the image
 */
int img_getinfo(const struct ImageModule *module, const char *fname, imageinfo *info);
int img_getinfo_byname(const char *modulename, const char *fname, imageinfo *info);

/* img_goodname
 * img_goodname_byname
 *
 * Description:
 *		Figures out what the "Good" name of a particular image is
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		base:				Arbitrary string to prefix the good name with
 *		result:				Placeholder to receive allocated string (i.e. you
 *							must call free() with the "Good" name)
 */
int img_goodname(const struct ImageModule *module, const char *fname, const char *base, char **result);
int img_goodname_byname(const char *modulename, const char *fname, const char *base, char **result);

/* ---------------------------------------------------------------------------
 * Wave/Cassette calls
 * ---------------------------------------------------------------------------
 */

enum {
	WAVEIMAGE_LSB_FIRST = 0,
	WAVEIMAGE_MSB_FIRST = 1
};

struct WaveExtra
{
	int (*initalt)(STREAM *instream, STREAM **outstream, int *basepos, int *length, int *channels, int *frequency, int *resolution);
	int (*nextfile)(IMAGE *img, imgtool_dirent *ent);
	int (*readfile)(IMAGE *img, STREAM *destf);
	int zeropulse;
	int threshpulse;
	int onepulse;
	int waveflags;
	const UINT8 *blockheader;
	int blockheadersize;

};

#define WAVEMODULE(name,humanname,ext,eoln,flags,zeropulse,onepulse,threshpulse,waveflags,blockheader,blockheadersize,\
		initalt,nextfile,readfilechunk)	\
static int imgmodinit_##name(STREAM *f, IMAGE **outimg); \
static struct WaveExtra waveextra_##name = \
{						\
	(initalt),			\
	(nextfile),			\
	(readfilechunk),	\
	(zeropulse),		\
	(onepulse),			\
	(threshpulse),		\
	(waveflags),		\
	(blockheader),		\
	(blockheadersize),	\
}; \
struct ImageModule imgmod_##name = \
{						\
	#name,				\
	(humanname),		\
	(ext),				\
	NULL,				\
	NULL,				\
	(eoln),				\
	(flags),			\
	imgmodinit_##name,	\
	imgwave_exit,		\
	NULL,				\
	imgwave_beginenum,	\
	imgwave_nextenum,	\
	imgwave_closeenum,	\
	NULL,				\
	imgwave_readfile,	\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	(void *) &waveextra_##name \
}; \
static int imgmodinit_##name(STREAM *f, IMAGE **outimg) \
{ \
	return imgwave_init(&imgmod_##name, f, outimg); \
}

int imgwave_init(struct ImageModule *mod, STREAM *f, IMAGE **outimg);
void imgwave_exit(IMAGE *img);
int imgwave_beginenum(IMAGE *img, IMAGEENUM **outenum);
int imgwave_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
void imgwave_closeenum(IMAGEENUM *enumeration);
int imgwave_readfile(IMAGE *img, const char *fname, STREAM *destf);

int imgwave_seek(IMAGE *img, int pos);
int imgwave_forward(IMAGE *img);
int imgwave_read(IMAGE *img, UINT8 *buf, int bufsize);

#endif /* IMGTOOL_H */
