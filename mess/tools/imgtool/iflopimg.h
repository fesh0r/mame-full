/*********************************************************************

	iflopimg.h

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#ifndef IFLOPIMG_H
#define IFLOPIMG_H

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"


/***************************************************************************

	Prototypes

***************************************************************************/

floppy_image *imgtool_floppy(IMAGE *img);
imgtoolerr_t imgtool_floppy_error(floperr_t err);

imgtoolerr_t imgtool_floppy_read_sector_to_stream(IMAGE *img, int head, int track, int sector, int offset, size_t length, STREAM *f);
imgtoolerr_t imgtool_floppy_write_sector_from_stream(IMAGE *img, int head, int track, int sector, int offset, size_t length, STREAM *f);


imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyFormat *format,
	imgtoolerr_t (*populate)(imgtool_library *library, struct ImageModule *module));


#define FLOPPYMODULE(name, description, format, populate)			\
	imgtoolerr_t name##_createmodule(imgtool_library *library)		\
	{																\
		return imgtool_floppy_createmodule(library, #name,			\
			description, floppyoptions_##format, populate);			\
	}																\

#endif /* IFLOPIMG_H */
