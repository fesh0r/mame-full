/****************************************************************************

	library.h

	Code relevant to the Imgtool library; analgous to the MESS/MAME driver
	list.

	Unlike MESS and MAME which have static driver lists, Imgtool has a
	concept of a library and this library is built at startup time.
	dynamic for which modules are added to.  This makes "dynamic" modules
	much easier

****************************************************************************/

#ifndef LIBRARY_H
#define LIBRARY_H

#include "osd_cpu.h"
#include "opresolv.h"
#include "stream.h"


typedef struct _imgtool_image imgtool_image;
typedef struct _imgtool_imageenum imgtool_imageenum;
typedef struct _imgtool_library imgtool_library;

typedef enum
{
	ITLS_NAME,
	ITLS_DESCRIPTION
}
imgtool_libsort_t;

typedef struct
{
	char *filename;
	size_t filename_len;
	char *attr;
	size_t attr_len;
	UINT64 filesize;

	/* flags */
	unsigned int eof : 1;
	unsigned int corrupt : 1;
	unsigned int directory : 1;
}
imgtool_dirent;

struct ImageModule
{
	struct ImageModule *previous;
	struct ImageModule *next;

	const char *name;
	const char *description;
	const char *extensions;
	const char *eoln;

	size_t image_extra_bytes;
	size_t imageenum_extra_bytes;

	char path_separator;
	
	/* flags */
	unsigned int prefer_ucase : 1;
	unsigned int initial_path_separator : 1;

	imgtoolerr_t	(*open)			(imgtool_image *image, imgtool_stream *f);
	void			(*close)		(imgtool_image *image);
	void			(*info)			(imgtool_image *image, char *string, size_t len);
	imgtoolerr_t	(*begin_enum)	(imgtool_imageenum *enumeration, const char *path);
	imgtoolerr_t	(*next_enum)	(imgtool_imageenum *enumeration, imgtool_dirent *ent);
	void			(*close_enum)	(imgtool_imageenum *enumeration);
	imgtoolerr_t	(*free_space)	(imgtool_image *image, UINT64 *size);
	imgtoolerr_t	(*read_file)	(imgtool_image *image, const char *fname, imgtool_stream *destf);
	imgtoolerr_t	(*write_file)	(imgtool_image *image, const char *fname, imgtool_stream *sourcef, option_resolution *opts);
	imgtoolerr_t	(*delete_file)	(imgtool_image *image, const char *fname);
	imgtoolerr_t	(*create)		(imgtool_image *image, imgtool_stream *f, option_resolution *opts);

	const struct OptionGuide *createimage_optguide;
	const char *createimage_optspec;

	const struct OptionGuide *writefile_optguide;
	const char *writefile_optspec;

	const void *extra;
};

/* creates an imgtool library */
imgtool_library *imgtool_library_create(void);

/* closes an imgtool library */
void imgtool_library_close(imgtool_library *library);

/* seeks out and removes a module from an imgtool library */
const struct ImageModule *imgtool_library_unlink(imgtool_library *library,
	const char *module);

/* sorts an imgtool library */
void imgtool_library_sort(imgtool_library *library, imgtool_libsort_t sort);

/* creates an imgtool module; called within module constructors */
imgtoolerr_t imgtool_library_createmodule(imgtool_library *library,
	const char *module_name, struct ImageModule **module);

/* finds a module */
const struct ImageModule *imgtool_library_findmodule(
	imgtool_library *library, const char *module_name);

/* memory allocators for pooled library memory */
void *imgtool_library_alloc(imgtool_library *library, size_t mem);
char *imgtool_library_strdup(imgtool_library *library, const char *s);

const struct ImageModule *imgtool_library_iterate(
	imgtool_library *library, const struct ImageModule *module);
const struct ImageModule *imgtool_library_index(
	imgtool_library *library, int i);

#endif /* LIBRARY_H */
