/***************************************************************************

	imgtool.h

	Main headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "osdepend.h"
#include "mess.h"
#include "formats/flopimg.h"
#include "opresolv.h"
#include "library.h"

/* -----------------------------------------------------------------------
 * Filters
 * ----------------------------------------------------------------------- */

struct filter_info
{
	int (*sendproc)(struct filter_info *fi, void *buf, int buflen);
	void *filterstate;
	void *filterparam;
	void *internalparam;
};

typedef struct ImageModule ImageModule;
typedef const struct ImageModule *ImageModuleConstPtr;

struct filter_module
{
	const char *name;
	const char *humanname;
	void *(*calcreadparam)(const struct ImageModule *imgmod);
	void *(*calcwriteparam)(const struct ImageModule *imgmod);
	int (*filterreadproc)(struct filter_info *fi, void *buf, int buflen);
	int (*filterwriteproc)(struct filter_info *fi, void *buf, int buflen);
	int statesize;
};

typedef struct _imgtool_filter imgtool_filter;

typedef const struct filter_module *FILTERMODULE;

enum {
	PURPOSE_READ,
	PURPOSE_WRITE
};

imgtool_filter *filter_init(FILTERMODULE filter, const struct ImageModule *imgmod, int purpose);
void filter_term(imgtool_filter *f);
int filter_writetostream(imgtool_filter *f, imgtool_stream *s, const void *buf, int buflen);
int filter_readfromstream(imgtool_filter *f, imgtool_stream *s, void *buf, int buflen);
int filter_readintobuffer(imgtool_filter *f, imgtool_stream *s);

extern FILTERMODULE filters[];

FILTERMODULE filter_lookup(const char *name);

imgtool_stream *stream_open_filter(imgtool_stream *s, imgtool_filter *f);

/* ----------------------------------------------------------------------- */

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

enum
{
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE	= 0x0001	/* this means that all filenames are normally upper case */
};

struct tagIMAGE
{
	const struct ImageModule *module;
};

struct tagIMAGEENUM
{
	const struct ImageModule *module;
};

typedef struct tagIMAGE imgtool_image;
typedef struct tagIMAGEENUM imgtool_imageenum;

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

/* img_identify
 *
 * Description:
 *		Attempts to determine the module for any given image
 *
 *	Parameters:
 *		library:			The imgtool_library to search
 *		filename:			The file to check
 */
imgtoolerr_t img_identify(imgtool_library *library, const char *filename,
	ImageModuleConstPtr *modules, size_t count);

/* img_open
 * img_open_byname
 *
 * Description:
 *		Opens an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		filename:			The native filename for the image
 *		read_or_write:		Open mode (use OSD_FOPEN_* constants)
 *		outimg:				Placeholder for image pointer
 */
imgtoolerr_t img_open(const struct ImageModule *module, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t img_open_byname(imgtool_library *library, const char *modulename, const char *filename, int read_or_write, imgtool_image **outimg);

/* img_close
 *
 * Description:
 *		Closes an image
 *
 * Parameters:
 *		img:				The image to close
 */
void img_close(imgtool_image *img);

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
imgtoolerr_t img_info(imgtool_image *img, char *string, size_t len);

/* img_beginenum
 *
 * Description:
 *		Begins enumerating files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		outenum:			The resulting enumeration
 */
imgtoolerr_t img_beginenum(imgtool_image *img, imgtool_imageenum **outenum);

/* img_nextenum
 *
 * Description:
 *		Continues enumerating files within an image
 *
 * Parameters:
 *		enumeration:		The enumeration
 *		ent:				Place to receive directory entry
 */
imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent);

/* img_countfiles
 *
 * Description:
 *		Counts the total amount of files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		totalfiles:			Place to receive the file count
 */
imgtoolerr_t img_countfiles(imgtool_image *img, int *totalfiles);

/* img_filesize
 *
 * Description:
 *		Retrieves the file length of the specified file
 *
 * Parameters:
 *		img:				The image to enumerate
 *		filename			Filename of file on the image
 *		filesize			Place to receive the file length
 */
imgtoolerr_t img_filesize(imgtool_image *img, const char *filename, UINT64 *filesize);

/* img_closeenum
 *
 * Description:
 *		Closes an enumeration
 *
 * Parameters:
 *		enumeration:		The enumeration to close
 */
void img_closeenum(imgtool_imageenum *enumeration);

/* img_freespace
 *
 * Description:
 *		Returns free space on an image, in bytes
 *
 * Parameters:
 *		img:				The image to query
 *		sz					Place to receive free space
 */
imgtoolerr_t img_freespace(imgtool_image *img, UINT64 *sz);

/* img_readfile
 *
 * Description:
 *		Start reading from a file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf,
	FILTERMODULE filter);

/* img_writefile
 *
 * Description:
 *		Start writing to a new file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 *		options/ropts:		Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef,
	option_resolution *resolution, FILTERMODULE filter);

/* img_getfile
 *
 * Description:
 *		Read a file from an image, storing it into a native file
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		dest:				Filename for native file to write to
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_getfile(imgtool_image *img, const char *fname, const char *dest,
	FILTERMODULE filter);

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
 *		opts:				Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_putfile(imgtool_image *img, const char *newfname, const char *source,
	option_resolution *opts, FILTERMODULE filter);

/* img_deletefile
 *
 * Description:
 *		Delete a file on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 */
imgtoolerr_t img_deletefile(imgtool_image *img, const char *fname);

/* img_create
 * img_create_byname
 *
 * Description:
 *		Creates an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		options/ropts:		Options that control how the image is created
 *							(tracks, sectors, etc)
 */
imgtoolerr_t img_create(const struct ImageModule *module, const char *fname, option_resolution *opts);
imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname, option_resolution *opts);

/* img_module
 *
 * Description:
 *		Retrieves the module associated with an image
 */
INLINE const struct ImageModule *img_module(imgtool_image *img)
{
	return img->module;
}


/* img_get_module_features
 *
 * Description:
 *		Retrieves a structure identifying this module's features associated with an image
 */
struct imgtool_module_features
{
	unsigned int supports_create : 1;
	unsigned int supports_open : 1;
	unsigned int supports_reading : 1;
	unsigned int supports_writing : 1;
	unsigned int supports_deleting : 1;
	unsigned int supports_directories : 1;
};

struct imgtool_module_features img_get_module_features(const struct ImageModule *module);

/* imgtool_test
 * imgtool_test_byname
 *
 * Description:
 *		(Only present when MAME_DEBUG is on)
 *		These functions run a test suite on the module
 *
 * Parameters:
 *		module/modulename:	The module for this image format.  If NULL, tests are run on all modules
 */
#ifdef MAME_DEBUG
imgtoolerr_t imgtool_test(imgtool_library *library, const struct ImageModule *module);
imgtoolerr_t imgtool_test_byname(imgtool_library *library, const char *modulename);
#endif /* MAME_DEBUG */

/* imgtool_validitychecks
 *
 * Description:
 *		Runs consistency checks to make sure that all is well
 */
int imgtool_validitychecks(void);

#endif /* IMGTOOL_H */
