/*********************************************************************

	iflopimg.c

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"
#include "iflopimg.h"

struct ImgtoolFloppyExtra
{
	const struct FloppyFormat *format;
	imgtoolerr_t (*create)(imgtool_image *image, option_resolution *opts);
};


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
	stream_close((imgtool_stream *) file);
}

static int imgtool_floppy_seekproc(void *file, INT64 offset, int whence)
{
	stream_seek((imgtool_stream *) file, offset, whence);
	return 0;
}

static size_t imgtool_floppy_readproc(void *file, void *buffer, size_t length)
{
	return stream_read((imgtool_stream *) file, buffer, length);
}

static size_t imgtool_floppy_writeproc(void *file, const void *buffer, size_t length)
{
	stream_write((imgtool_stream *) file, buffer, length);
	return length;
}

static UINT64 imgtool_floppy_filesizeproc(void *file)
{
	return stream_size((imgtool_stream *) file);
}

static struct io_procs imgtool_ioprocs =
{
	imgtool_floppy_closeproc,
	imgtool_floppy_seekproc,
	imgtool_floppy_readproc,
	imgtool_floppy_writeproc,
	imgtool_floppy_filesizeproc
};

static struct io_procs imgtool_noclose_ioprocs =
{
	NULL,
	imgtool_floppy_seekproc,
	imgtool_floppy_readproc,
	imgtool_floppy_writeproc,
	imgtool_floppy_filesizeproc
};



/*********************************************************************
	Imgtool handlers
*********************************************************************/

static const struct ImgtoolFloppyExtra *get_extra(const struct ImageModule *module)
{
	return (const struct ImgtoolFloppyExtra *) module->extra;
}



struct imgtool_floppy_image
{
	imgtool_image base;
	floppy_image *floppy;
};

static floppy_image *get_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_open_internal(const struct ImageModule *mod, imgtool_stream *f,
	int noclose, imgtool_image **outimg)
{
	floperr_t ferr;
	imgtoolerr_t err;
	struct imgtool_floppy_image *fimg;
	const struct FloppyFormat *format;

	format = get_extra(mod)->format;

	/* allocate space for our image */
	fimg = (struct imgtool_floppy_image *) malloc(sizeof(struct imgtool_floppy_image));
	if (!fimg)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	/* open up the floppy */
	ferr = floppy_open(f, noclose ? &imgtool_noclose_ioprocs : &imgtool_ioprocs,
		NULL, format, FLOPPY_FLAGS_READWRITE, &fimg->floppy);
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



static imgtoolerr_t imgtool_floppy_open(const struct ImageModule *mod, imgtool_stream *f,
	imgtool_image **outimg)
{
	return imgtool_floppy_open_internal(mod, f, FALSE, outimg);
}



static void imgtool_floppy_close(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	floppy_close(fimg->floppy);
}



static imgtoolerr_t imgtool_floppy_create(const struct ImageModule *mod, imgtool_stream *f, option_resolution *opts)
{
	floperr_t ferr;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct FloppyFormat *format;
	imgtool_image *image = NULL;

	format = get_extra(mod)->format;

	/* open up the floppy */
	ferr = floppy_create(f, &imgtool_ioprocs, format, opts, NULL);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		goto done;
	}

	if (get_extra(mod)->create)
	{
		err = imgtool_floppy_open_internal(mod, f, TRUE, &image);
		if (err)
			goto done;

		err = get_extra(mod)->create(image, opts);
		if (err)
			goto done;
	}

done:
	if (image)
		img_close(image);
	return err;
}



imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyFormat *format,
	imgtoolerr_t (*populate)(imgtool_library *library, struct ImgtoolFloppyCallbacks *module))
{
	imgtoolerr_t err;
	struct ImageModule *module;
	int format_index;
	char buffer[512];
	struct ImgtoolFloppyCallbacks floppy_callbacks;
	struct ImgtoolFloppyExtra *extra;

	for (format_index = 0; format[format_index].construct; format_index++)
	{
		extra = imgtool_library_alloc(library, sizeof(*extra));
		if (!extra)
			return IMGTOOLERR_OUTOFMEMORY;
		memset(extra, 0, sizeof(*extra));

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s_%s",
			format[format_index].name, format_name);

		err = imgtool_library_createmodule(library, buffer, &module);
		if (err)
			return err;

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s (%s)",
			format[format_index].description, description);

		extra->format = &format[format_index];

		module->description				= imgtool_library_strdup(library, buffer);
		module->open					= imgtool_floppy_open;
		module->create					= imgtool_floppy_create;
		module->close					= imgtool_floppy_close;
		module->extensions				= format[format_index].extensions;
		module->extra					= extra;
		module->createimage_optguide	= floppy_option_guide;
		module->createimage_optspec		= format[format_index].param_guidelines;

		if (populate)
		{
			memset(&floppy_callbacks, 0, sizeof(floppy_callbacks));
			populate(library, &floppy_callbacks);

			extra->create					= floppy_callbacks.create;
			module->eoln					= floppy_callbacks.eoln;
			module->path_separator			= floppy_callbacks.path_separator;
			module->prefer_ucase			= floppy_callbacks.prefer_ucase;
			module->initial_path_separator	= floppy_callbacks.initial_path_separator;
			module->begin_enum				= floppy_callbacks.begin_enum;
			module->next_enum				= floppy_callbacks.next_enum;
			module->close_enum				= floppy_callbacks.close_enum;
			module->free_space				= floppy_callbacks.free_space;
			module->read_file				= floppy_callbacks.read_file;
			module->write_file				= floppy_callbacks.write_file;
			module->delete_file				= floppy_callbacks.delete_file;
			module->writefile_optguide		= floppy_callbacks.writefile_optguide;
			module->writefile_optspec		= floppy_callbacks.writefile_optspec;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



floppy_image *imgtool_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img;
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_transfer_sector_tofrom_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f, int direction)
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



imgtoolerr_t imgtool_floppy_read_sector_to_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 1);
}



imgtoolerr_t imgtool_floppy_write_sector_from_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 0);
}

