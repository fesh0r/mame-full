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


struct tagIMAGE;
struct tagIMAGEENUM;
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
	int flags;

	imgtoolerr_t	(*open)			(const struct ImageModule *mod, imgtool_stream *f, struct tagIMAGE **outimg);
	void			(*close)		(struct tagIMAGE *img);
	void			(*info)			(struct tagIMAGE *img, char *string, size_t len);
	imgtoolerr_t	(*begin_enum)	(struct tagIMAGE *img, struct tagIMAGEENUM **outenum);
	imgtoolerr_t	(*next_enum)	(struct tagIMAGEENUM *enumeration, imgtool_dirent *ent);
	void			(*close_enum)	(struct tagIMAGEENUM *enumeration);
	imgtoolerr_t	(*free_space)	(struct tagIMAGE *img, UINT64 *size);
	imgtoolerr_t	(*read_file)	(struct tagIMAGE *img, const char *fname, imgtool_stream *destf);
	imgtoolerr_t	(*write_file)	(struct tagIMAGE *img, const char *fname, imgtool_stream *sourcef, option_resolution *opts);
	imgtoolerr_t	(*delete_file)	(struct tagIMAGE *img, const char *fname);
	imgtoolerr_t	(*create)		(const struct ImageModule *mod, imgtool_stream *f, option_resolution *opts);

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
