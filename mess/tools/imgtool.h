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
#define	IMGTOOLERR_SRC_PARAM_CYLINDERS	0x6000
#define	IMGTOOLERR_SRC_PARAM_HEADS		0x7000

#define	IMGTOOLERR_SRC_MASK				0xf000

#define ERRORSOURCE(err)	((err) & IMGTOOLERR_SRC_MASK)

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

/* Flags for ImageModule */
enum {
	IMAGE_USES_LABEL			= 0x001,

	/* floppy disk */
	IMAGE_USES_SECTOR_SIZE		= 0x002,
	IMAGE_USES_SECTORS			= 0x004, // in track

	/* harddisk */
	IMAGE_USES_CYLINDERS		= 0x008,
	IMAGE_USES_HEADS			= 0x010,

	/* c64 tape */
	IMAGE_USES_ENTRIES			= 0x020,

	/* c64 cartridge */
	IMAGE_USES_HARDWARE_TYPE	= 0x040,
	IMAGE_USES_GAME_LINE		= 0x080,
	IMAGE_USES_EXROM_LINE		= 0x100,

	/* rsdos */
	IMAGE_USES_FTYPE			= 0x200,
	IMAGE_USES_FASCII			= 0x400,
	/* c64 cartridge */
	IMAGE_USES_FADDR			= 0x800,
	IMAGE_USES_FBANK			= 0x1000
};

typedef struct {
	/* floppy disk */
	int sector_size;
	int sectors;
	/* pc harddisks */
	int cylinders;
	int heads;
	/* c64 cartridges */
	int hardware_type;
	int game_line;
	int exrom_line;
	/* c64 tape */
	int entries;

	char *label;
} geometry_options;

typedef struct {
	/* rsdos */
	int ftype;
	int fascii;
	/* c64 cartridges */
	int faddr;
	int fbank;
} file_options;

typedef struct {
	geometry_options minimum;
	geometry_options maximum;
} geometry_ranges;

struct ImageModule {
	const char *name;
	const char *humanname;
	const char *fileextension;
	int flags;
	const char *crcfile;
	const char *crcsysname;
	const geometry_ranges *ranges;
	int (*init_by_name)(const char *name, IMAGE **outimg); /* used for directory and archiver access */
	int (*init)(STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	void (*info)(IMAGE *img, char *string, const int len);
	int (*beginenum)(IMAGE *img, IMAGEENUM **outenum);
	int (*nextenum)(IMAGEENUM *enumeration, imgtool_dirent *ent);
	void (*closeenum)(IMAGEENUM *enumeration);
	size_t (*freespace)(IMAGE *img);
	int (*readfile)(IMAGE *img, const char *fname, STREAM *destf);
	int (*writefile)(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options);
	int (*deletefile)(IMAGE *img, const char *fname);
	int (*create)(STREAM *f, const geometry_options *options);
	int (*extract)(IMAGE *img, STREAM *f);
	/* size must be set with the size of the buffer,
	   if the buffer is too small it will be relocated with realloc */
	int (*read_sector)(IMAGE *img, int head, int track, int sector, char **buffer, int *size);
	int (*write_sector)(IMAGE *img, int head, int track, int sector, char *buffer, int size);
	void *extra;
};

/* ----------------------------------------------------------------------- */

/* Use IMAGEMODULE for (potentially) full featured images */
#define IMAGEMODULE(name,humanname,ext,flags,crcfile,crcsysname,ranges,\
		initbyname, init,exit,info,beginenum,nextenum,closeenum,freespace,readfile,writefile,deletefile,create,extract,read_sector, write_sector)	\
struct ImageModule imgmod_##name = \
{					\
	#name,			\
	(humanname),	\
	(ext),			\
	(flags),		\
	(crcfile),		\
	(crcsysname),	\
	(ranges),		\
	(initbyname),	\
	(init),			\
	(exit),			\
	(info),			\
	(beginenum),	\
	(nextenum),		\
	(closeenum),	\
	(freespace),	\
	(readfile),		\
	(writefile),	\
	(deletefile),	\
	(create),		\
	(extract),		\
	(read_sector),	\
	(write_sector),	\
	NULL			\
};

/* Use CARTMODULE for cartriges (where the only relevant option is CRC checking */
#define CARTMODULE(name,humanname,ext)	\
struct ImageModule imgmod_##name = \
{					\
	#name,			\
	(humanname),	\
	(ext),			\
	0,				\
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

/* img_extract
 *
 * Description:
 *		???
 *
 * Parameters:
 *		img:				The image to extract
 *		fname:				???
 */
int img_extract(IMAGE *img, const char *fname);

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
int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options);

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
int img_putfile(IMAGE *img, const char *newfname, const char *source, const file_options *options);

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
	int (*nextfile)(IMAGE *img, imgtool_dirent *ent);
	int (*readfile)(IMAGE *img, STREAM *destf);
	int zeropulse;
	int threshpulse;
	int onepulse;
	int waveflags;
	const UINT8 *blockheader;
	int blockheadersize;

};

#define WAVEMODULE(name,humanname,ext,zeropulse,onepulse,threshpulse,waveflags,blockheader,blockheadersize,\
		nextfile,readfilechunk)	\
static int imgmodinit_##name(STREAM *f, IMAGE **outimg); \
static struct WaveExtra waveextra_##name = \
{						\
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
	0,					\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
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
