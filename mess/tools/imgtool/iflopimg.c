/*********************************************************************

	iflopimg.c

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"
#include "iflopimg.h"

imgtoolerr_t imgtool_floppy_error(floperr_t err)
{
	switch(err) {
	case FLOPPY_ERROR_SUCCESS:
		return IMGTOOLERR_SUCCESS;

	case FLOPPY_ERROR_OUTOFMEMORY:
		return IMGTOOLERR_OUTOFMEMORY;

	case FLOPPY_ERROR_INVALIDIMAGE:
		return IMGTOOLERR_CORRUPTIMAGE;
		
	default:
		return IMGTOOLERR_UNEXPECTED;
	}
}



/*********************************************************************
	Imgtool ioprocs
*********************************************************************/

static void imgtool_floppy_closeproc(void *file)
{
	stream_close((STREAM *) file);
}

static int imgtool_floppy_seekproc(void *file, INT64 offset, int whence)
{
	stream_seek((STREAM *) file, offset, whence);
	return 0;
}

static size_t imgtool_floppy_readproc(void *file, void *buffer, size_t length)
{
	return stream_read((STREAM *) file, buffer, length);
}

static size_t imgtool_floppy_writeproc(void *file, const void *buffer, size_t length)
{
	stream_write((STREAM *) file, buffer, length);
	return length;
}

static UINT64 imgtool_floppy_filesizeproc(void *file)
{
	return stream_size((STREAM *) file);
}

static struct io_procs imgtool_ioprocs_procs =
{
	imgtool_floppy_closeproc,
	imgtool_floppy_seekproc,
	imgtool_floppy_readproc,
	imgtool_floppy_writeproc,
	imgtool_floppy_filesizeproc
};



/*********************************************************************
	Imgtool handlers
*********************************************************************/

struct imgtool_floppy_image
{
	IMAGE base;
	floppy_image *floppy;
};

static floppy_image *get_floppy(IMAGE *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_open(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	floperr_t ferr;
	imgtoolerr_t err;
	struct imgtool_floppy_image *fimg;
	const struct FloppyOption *format;

	format = (const struct FloppyOption *) mod->extra;

	/* allocate space for our image */
	fimg = (struct imgtool_floppy_image *) malloc(sizeof(struct imgtool_floppy_image));
	if (!fimg)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	/* open up the floppy */
	ferr = floppy_open(f, &imgtool_ioprocs_procs, NULL, format, FLOPPY_FLAGS_READWRITE, &fimg->floppy);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		goto error;
	}

	fimg->base.module = mod;
	*outimg = &fimg->base;
	return IMGTOOLERR_SUCCESS;

error:
	if (fimg)
		free(fimg);
	return err;
}



static imgtoolerr_t imgtool_floppy_create(const struct ImageModule *mod, STREAM *f, option_resolution *opts)
{
	floperr_t ferr;
	const struct FloppyOption *format;

	format = (const struct FloppyOption *) mod->extra;

	/* open up the floppy */
	ferr = floppy_create(f, &imgtool_ioprocs_procs, format, opts, NULL);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



static void imgtool_floppy_close(IMAGE *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	floppy_close(fimg->floppy);
}



imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyOption *format,
	imgtoolerr_t (*populate)(imgtool_library *library, struct ImageModule *module))
{
	imgtoolerr_t err;
	struct ImageModule *module;
	int format_index;
	char buffer[512];

	for (format_index = 0; format[format_index].construct; format_index++)
	{
		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s_%s",
			format[format_index].name, format_name);

		err = imgtool_library_createmodule(library, buffer, &module);
		if (err)
			return err;

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s (%s)",
			format[format_index].description, description);

		module->description				= imgtool_library_strdup(library, buffer);
		module->open					= imgtool_floppy_open;
		module->create					= imgtool_floppy_create;
		module->close					= imgtool_floppy_close;
		module->extensions				= format[format_index].extensions;
		module->extra					= &format[format_index];
		module->createimage_optguide	= floppy_option_guide;
		module->createimage_optspec		= format[format_index].param_guidelines;

		if (populate)
			populate(library, module);
	}
	return IMGTOOLERR_SUCCESS;
}



floppy_image *imgtool_floppy(IMAGE *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_transfer_sector_tofrom_stream(IMAGE *img, int head, int track, int sector, int offset, size_t length, STREAM *f, int direction)
{
	floperr_t err;
	floppy_image *floppy;
	void *buffer = NULL;

	floppy = imgtool_floppy(img);

	buffer = malloc(length);
	if (!buffer)
	{
		err = FLOPPY_ERROR_OUTOFMEMORY;
		goto done;
	}

	if (direction)
	{
		err = floppy_read_sector(floppy, head, track, sector, offset, buffer, length);
		if (err)
			goto done;
		stream_write(f, buffer, length);
	}
	else
	{
		stream_read(f, buffer, length);
		err = floppy_write_sector(floppy, head, track, sector, offset, buffer, length);
		if (err)
			goto done;
	}
	
	err = FLOPPY_ERROR_SUCCESS;

done:
	if (buffer)
		free(buffer);
	return imgtool_floppy_error(err);
}



imgtoolerr_t imgtool_floppy_read_sector_to_stream(IMAGE *img, int head, int track, int sector, int offset, size_t length, STREAM *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 1);
}



imgtoolerr_t imgtool_floppy_write_sector_from_stream(IMAGE *img, int head, int track, int sector, int offset, size_t length, STREAM *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 0);
}

