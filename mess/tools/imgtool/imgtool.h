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
#include "filter.h"

/* ----------------------------------------------------------------------- */

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

#define FILENAME_BOOTBLOCK	((const char *) 1)

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

/* ----- image management ----- */
imgtoolerr_t img_identify(imgtool_library *library, const char *filename, imgtool_module **modules, size_t count);
imgtoolerr_t img_open(const imgtool_module *module, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t img_open_byname(imgtool_library *library, const char *modulename, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t img_create(const imgtool_module *module, const char *fname, option_resolution *opts, imgtool_image **image);
imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname, option_resolution *opts, imgtool_image **image);
void img_close(imgtool_image *image);
imgtoolerr_t img_info(imgtool_image *image, char *string, size_t len);
imgtoolerr_t img_getsectorsize(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *length);
imgtoolerr_t img_readsector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len);
imgtoolerr_t img_writesector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len);
imgtoolerr_t img_getblocksize(imgtool_image *image, UINT32 *length);
imgtoolerr_t img_readblock(imgtool_image *image, UINT64 block, void *buffer);
imgtoolerr_t img_writeblock(imgtool_image *image, UINT64 block, const void *buffer);
void *img_malloc(imgtool_image *image, size_t size);
const imgtool_module *img_module(imgtool_image *img);
void *img_extrabytes(imgtool_image *img);

/* ----- stuff that will eventually operate on partitions ----- */
imgtoolerr_t img_beginenum(imgtool_image *image, const char *path, imgtool_imageenum **outenum);
imgtoolerr_t img_getdirent(imgtool_image *image, const char *path, int index, imgtool_dirent *ent);
imgtoolerr_t img_filesize(imgtool_image *image, const char *filename, UINT64 *filesize);
imgtoolerr_t img_freespace(imgtool_image *image, UINT64 *sz);
imgtoolerr_t img_readfile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf, filter_getinfoproc filter);
imgtoolerr_t img_writefile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *resolution, filter_getinfoproc filter);
imgtoolerr_t img_getfile(imgtool_image *image, const char *filename, const char *fork, const char *dest, filter_getinfoproc filter);
imgtoolerr_t img_putfile(imgtool_image *image, const char *newfname, const char *fork, const char *source, option_resolution *opts, filter_getinfoproc filter);
imgtoolerr_t img_deletefile(imgtool_image *image, const char *fname);
imgtoolerr_t img_listforks(imgtool_image *image, const char *path, imgtool_forkent *ents, size_t len);
imgtoolerr_t img_createdir(imgtool_image *img, const char *path);
imgtoolerr_t img_deletedir(imgtool_image *img, const char *path);
imgtoolerr_t img_listattrs(imgtool_image *image, const char *path, UINT32 *attrs, size_t len);
imgtoolerr_t img_getattrs(imgtool_image *image, const char *path, const UINT32 *attrs, imgtool_attribute *values);
imgtoolerr_t img_setattrs(imgtool_image *image, const char *path, const UINT32 *attrs, const imgtool_attribute *values);
imgtoolerr_t img_getattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute *value);
imgtoolerr_t img_setattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute value);
void img_attrname(const imgtool_module *module, UINT32 attribute, const imgtool_attribute *attr_value, char *buffer, size_t buffer_len);
imgtoolerr_t img_geticoninfo(imgtool_image *image, const char *path, imgtool_iconinfo *iconinfo);
imgtoolerr_t img_suggesttransfer(imgtool_image *image, const char *path, imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);
imgtoolerr_t img_getchain(imgtool_image *img, const char *path, imgtool_chainent *chain, size_t chain_size);
imgtoolerr_t img_getchain_string(imgtool_image *img, const char *path, char *buffer, size_t buffer_len);

/* ----- directory management ----- */
imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent);
void img_closeenum(imgtool_imageenum *enumeration);
const imgtool_module *img_enum_module(imgtool_imageenum *enumeration);
void *img_enum_extrabytes(imgtool_imageenum *enumeration);
imgtool_image *img_enum_image(imgtool_imageenum *enumeration);

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
	unsigned int supports_deletefile : 1;
	unsigned int supports_directories : 1;
	unsigned int supports_freespace : 1;
	unsigned int supports_createdir : 1;
	unsigned int supports_deletedir : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int supports_readsector : 1;
	unsigned int supports_writesector : 1;
	unsigned int supports_forks : 1;
	unsigned int supports_geticoninfo : 1;
	unsigned int is_read_only : 1;
};

struct imgtool_module_features img_get_module_features(const imgtool_module *module);

/* imgtool_validitychecks
 *
 * Description:
 *		Runs consistency checks to make sure that all is well
 */
int imgtool_validitychecks(void);

#endif /* IMGTOOL_H */
