/*********************************************************************

	iflopimg.h

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#ifndef IFLOPIMG_H
#define IFLOPIMG_H

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"


struct ImgtoolFloppyCallbacks
{
	const char *eoln;
	
	int image_extra_bytes;
	int imageenum_extra_bytes;
	
	char path_separator;
	char alternate_path_separator;
	
	/* flags */
	unsigned int prefer_ucase : 1;
	unsigned int initial_path_separator : 1;
	unsigned int open_is_strict : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int tracks_are_called_cylinders : 1;	/* used for hard drivers */
	unsigned int writing_untested : 1;				/* used when we support writing, but not in main build */
	unsigned int creation_untested : 1;				/* used when we support creation, but not in main build */

	imgtoolerr_t	(*create)		(imgtool_image *img, option_resolution *opts);
	imgtoolerr_t	(*open)			(imgtool_image *img);
	imgtoolerr_t	(*begin_enum)	(imgtool_imageenum *enumeration, const char *path);
	imgtoolerr_t	(*next_enum)	(imgtool_imageenum *enumeration, imgtool_dirent *ent);
	void			(*close_enum)	(imgtool_imageenum *enumeration);
	imgtoolerr_t	(*free_space)	(imgtool_image *img, UINT64 *size);
	imgtoolerr_t	(*read_file)	(imgtool_image *img, const char *fname, imgtool_stream *destf);
	imgtoolerr_t	(*write_file)	(imgtool_image *img, const char *fname, imgtool_stream *sourcef, option_resolution *opts);
	imgtoolerr_t	(*delete_file)	(imgtool_image *img, const char *fname);
	imgtoolerr_t	(*create_dir)	(imgtool_image *image, const char *path);
	imgtoolerr_t	(*delete_dir)	(imgtool_image *image, const char *path);

	const struct OptionGuide *writefile_optguide;
	const char *writefile_optspec;
};



/***************************************************************************

	Prototypes

***************************************************************************/

floppy_image *imgtool_floppy(imgtool_image *img);
imgtoolerr_t imgtool_floppy_error(floperr_t err);

imgtoolerr_t imgtool_floppy_read_sector_to_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f);
imgtoolerr_t imgtool_floppy_write_sector_from_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f);


imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyFormat *format,
	imgtoolerr_t (*populate)(imgtool_library *library, struct ImgtoolFloppyCallbacks *module));

void *imgtool_floppy_extrabytes(imgtool_image *img);

#define FLOPPYMODULE(name, description, format, populate)			\
	imgtoolerr_t name##_createmodule(imgtool_library *library)		\
	{																\
		return imgtool_floppy_createmodule(library, #name,			\
			description, floppyoptions_##format, populate);			\
	}																\

#endif /* IFLOPIMG_H */
