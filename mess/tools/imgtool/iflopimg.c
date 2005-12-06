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
	imgtoolerr_t (*open)(imgtool_image *img);
};


imgtoolerr_t imgtool_floppy_error(floperr_t err)
{
	switch(err)
	{
		case FLOPPY_ERROR_SUCCESS:
			return IMGTOOLERR_SUCCESS;

		case FLOPPY_ERROR_OUTOFMEMORY:
			return IMGTOOLERR_OUTOFMEMORY;

		case FLOPPY_ERROR_INVALIDIMAGE:
			return IMGTOOLERR_CORRUPTIMAGE;

		case FLOPPY_ERROR_SEEKERROR:
			return IMGTOOLERR_SEEKERROR;

		case FLOPPY_ERROR_UNSUPPORTED:
			return IMGTOOLERR_UNIMPLEMENTED;
			
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
	floppy_image *floppy;
};

static floppy_image *get_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_open_internal(imgtool_image *image, imgtool_stream *f, int noclose)
{
	floperr_t ferr;
	imgtoolerr_t err;
	struct imgtool_floppy_image *fimg;
	const struct ImgtoolFloppyExtra *extra;

	extra = get_extra(img_module(image));
	fimg = (struct imgtool_floppy_image *) img_extrabytes(image);

	/* open up the floppy */
	ferr = floppy_open(f, noclose ? &imgtool_noclose_ioprocs : &imgtool_ioprocs,
		NULL, extra->format, FLOPPY_FLAGS_READWRITE, &fimg->floppy);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		return err;
	}

	if (extra->open)
	{
		err = extra->open(image);
		if (err)
			return err;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t imgtool_floppy_open(imgtool_image *image, imgtool_stream *f)
{
	return imgtool_floppy_open_internal(image, f, FALSE);
}



static imgtoolerr_t imgtool_floppy_create(imgtool_image *image, imgtool_stream *f, option_resolution *opts)
{
	floperr_t ferr;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct FloppyFormat *format;
	const struct ImgtoolFloppyExtra *extra;
	struct imgtool_floppy_image *fimg;

	extra = get_extra(img_module(image));
	format = extra->format;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(image);

	/* open up the floppy */
	ferr = floppy_create(f, &imgtool_ioprocs, format, opts, &fimg->floppy);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		goto done;
	}

	/* do we have to do extra stuff when creating the image? */
	if (extra->create)
	{
		err = extra->create(image, opts);
		if (err)
			goto done;
	}

	/* do we have to do extra stuff when opening the image? */
	if (extra->open)
	{
		err = extra->open(image);
		if (err)
			goto done;
	}

done:
	return err;
}



static void imgtool_floppy_close(imgtool_image *img)
{
	floppy_close(get_floppy(img));
}



imgtoolerr_t imgtool_floppy_get_sector_size(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *sector_size)
{
	floperr_t ferr;

	ferr = floppy_get_sector_length(get_floppy(image), head, track, sector, sector_size);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t imgtool_floppy_read_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len)
{
	floperr_t ferr;

	ferr = floppy_read_sector(get_floppy(image), head, track, sector, 0, buffer, len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t imgtool_floppy_write_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len)
{
	floperr_t ferr;

	ferr = floppy_write_sector(get_floppy(image), head, track, sector, 0, buffer, len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
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
		extra->format = &format[format_index];

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s_%s",
			format[format_index].name, format_name);

		err = imgtool_library_createmodule(library, buffer, &module);
		if (err)
			return err;

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s (%s)",
			format[format_index].description, description);

		module->image_extra_bytes		= sizeof(struct imgtool_floppy_image);
		module->description				= imgtool_library_strdup(library, buffer);
		module->open					= imgtool_floppy_open;
		module->create					= imgtool_floppy_create;
		module->close					= imgtool_floppy_close;
		module->extensions				= format[format_index].extensions;
		module->extra					= extra;
		module->createimage_optguide	= format[format_index].param_guidelines ? floppy_option_guide : NULL;
		module->createimage_optspec		= format[format_index].param_guidelines;
		module->get_sector_size			= imgtool_floppy_get_sector_size;
		module->read_sector				= imgtool_floppy_read_sector;
		module->write_sector			= imgtool_floppy_write_sector;

		if (populate)
		{
			memset(&floppy_callbacks, 0, sizeof(floppy_callbacks));
			floppy_callbacks.image_extra_bytes = module->image_extra_bytes;
			floppy_callbacks.imageenum_extra_bytes = module->imageenum_extra_bytes;
			
			populate(library, &floppy_callbacks);

			extra->create						= floppy_callbacks.create;
			extra->open							= floppy_callbacks.open;
			module->eoln						= floppy_callbacks.eoln;
			module->path_separator				= floppy_callbacks.path_separator;
			module->alternate_path_separator	= floppy_callbacks.alternate_path_separator;
			module->prefer_ucase				= floppy_callbacks.prefer_ucase;
			module->initial_path_separator		= floppy_callbacks.initial_path_separator;
			module->open_is_strict				= floppy_callbacks.open_is_strict;
			module->supports_creation_time		= floppy_callbacks.supports_creation_time;
			module->supports_lastmodified_time	= floppy_callbacks.supports_lastmodified_time;
			module->tracks_are_called_cylinders	= floppy_callbacks.tracks_are_called_cylinders;
			module->writing_untested			= floppy_callbacks.writing_untested;
			module->creation_untested			= floppy_callbacks.creation_untested;
			module->info						= floppy_callbacks.info;
			module->begin_enum					= floppy_callbacks.begin_enum;
			module->next_enum					= floppy_callbacks.next_enum;
			module->close_enum					= floppy_callbacks.close_enum;
			module->free_space					= floppy_callbacks.free_space;
			module->read_file					= floppy_callbacks.read_file;
			module->write_file					= floppy_callbacks.write_file;
			module->delete_file					= floppy_callbacks.delete_file;
			module->list_forks					= floppy_callbacks.list_forks;
			module->create_dir					= floppy_callbacks.create_dir;
			module->delete_dir					= floppy_callbacks.delete_dir;
			module->get_attrs					= floppy_callbacks.get_attrs;
			module->set_attrs					= floppy_callbacks.set_attrs;
			module->suggest_transfer			= floppy_callbacks.suggest_transfer;
			module->get_chain					= floppy_callbacks.get_chain;
			module->writefile_optguide			= floppy_callbacks.writefile_optguide;
			module->writefile_optspec			= floppy_callbacks.writefile_optspec;
			module->image_extra_bytes			= floppy_callbacks.image_extra_bytes;
			module->imageenum_extra_bytes		= floppy_callbacks.imageenum_extra_bytes;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



floppy_image *imgtool_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
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



void *imgtool_floppy_extrabytes(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
	return fimg + 1;
}



