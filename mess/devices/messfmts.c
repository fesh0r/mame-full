#include "devices/messfmts.h"
#include "osdepend.h"
#include "mess.h"
#include "devices/flopdrv.h"
#include "image.h"
#include "utils.h"

#define BDFTAG		"mess_bdf"

struct mess_bdf
{
	void *bdf;
	int track, sector;
};

/* ----------------------------------------------------------------------- */

static struct mess_bdf *get_messbdf(mess_image *img)
{
	struct mess_bdf *bdf;
	bdf = image_lookuptag(img, BDFTAG);
	return bdf->bdf ? bdf : NULL;
}

static void bdf_seek_callback(mess_image *img, int physical_track)
{
	struct mess_bdf *messbdf;
	
	messbdf = get_messbdf(img);
	if (messbdf)
		messbdf->track = physical_track;
}

static int bdf_get_sectors_per_track(mess_image *img, int side)
{
	struct mess_bdf *messbdf;
	
	messbdf = get_messbdf(img);
	if (!messbdf)
		return 0;
	
	return bdf_get_sector_count(messbdf->bdf, messbdf->track, side);
}

static void bdf_get_id_callback(mess_image *img, chrn_id *id, int id_index, int side)
{
	struct mess_bdf *messbdf;
	UINT16 sector_size;
	UINT8 sector;
	
	messbdf = get_messbdf(img);

	bdf_get_sector_info(messbdf->bdf, messbdf->track, side, id_index, &sector, &sector_size);
	id->C = messbdf->track;
	id->H = side;
	id->R = sector;
	id->data_id = sector;
	id->flags = 0;

	switch(sector_size) {
	case 128:
		id->N = 0;
		break;

	case 256:
		id->N = 1;
		break;

	case 512:
		id->N = 2;
		break;

	case 1024:
		id->N = 3;
		break;

	default:
		assert(0);
		break;
	}
}

static void bdf_read_sector_data_into_buffer(mess_image *img, int side, int index1, char *ptr, int length)
{
	struct mess_bdf *messbdf;
	messbdf = get_messbdf(img);
	bdf_read_sector(messbdf->bdf, messbdf->track, side, index1, 0, ptr, length);
}

static void bdf_write_sector_data_from_buffer(mess_image *img, int side, int index1, char *ptr, int length,int ddam)
{
	struct mess_bdf *messbdf;
	messbdf = get_messbdf(img);
	bdf_write_sector(messbdf->bdf, messbdf->track, side, index1, 0, ptr, length);
}

/* ----------------------------------------------------------------------- */

static int mame_fseek_thunk(void *file, INT64 offset, int whence)
{
	return mame_fseek((mame_file *) file, offset, whence);
}

static UINT32 mame_fread_thunk(void *file, void *buffer, UINT32 length)
{
	return mame_fread((mame_file *) file, buffer, length);
}

static UINT32 mame_fwrite_thunk(void *file, const void *buffer, UINT32 length)
{
	return mame_fwrite((mame_file *) file, buffer, length);
}

static UINT64 mame_fsize_thunk(void *file)
{
	return mame_fsize((mame_file *) file);
}

/* ----------------------------------------------------------------------- */

static struct bdf_procs mess_bdf_procs =
{
	NULL,
	mame_fseek_thunk,
	mame_fread_thunk,
	mame_fwrite_thunk,
	mame_fsize_thunk
};

static floppy_interface bdf_floppy_interface =
{
	bdf_seek_callback,
	bdf_get_sectors_per_track,
	bdf_get_id_callback,
	bdf_read_sector_data_into_buffer,
	bdf_write_sector_data_from_buffer,
	NULL,
	NULL
};

static int bdf_floppy_init(mess_image *img)
{
	if (!image_alloctag(img, BDFTAG, sizeof(struct mess_bdf)))
		return INIT_FAIL;
	return floppy_drive_init(img, &bdf_floppy_interface);
}

static int bdf_floppy_load_internal(mess_image *img, mame_file *file, int mode, const formatdriver_ctor *open_formats, formatdriver_ctor create_format, mame_file *fp, int open_mode)
{
	const char *name;
	const char *ext;
	formatdriver_ctor fmts[2];
	int err;
	struct mess_bdf *messbdf;

	messbdf = image_lookuptag(img, BDFTAG);

	memset(messbdf, 0, sizeof(*messbdf));

	name = image_filename(img);

	if (mode == OSD_FOPEN_RW_CREATE)
	{
		err = bdf_create(&mess_bdf_procs, create_format, file, NULL, &messbdf->bdf);
	}
	else
	{
		if (!open_formats)
		{
			fmts[0] = create_format;
			fmts[1] = NULL;
			open_formats = fmts;				
		}
		ext = image_filetype(img);
		err = bdf_open(&mess_bdf_procs, open_formats, file, (mode == OSD_FOPEN_READ), ext, &messbdf->bdf);
	}
	if (err)
		return INIT_FAIL;

	return INIT_PASS;
}

static int bdf_floppy_load(mess_image *img, mame_file *fp, int open_mode)
{
	const struct IODevice *dev;
	const formatdriver_ctor *open_formats;
	formatdriver_ctor create_format;

	dev = device_find(Machine->gamedrv, IO_FLOPPY);
	assert(dev);

	open_formats = (const formatdriver_ctor *) dev->user1;
	create_format = (formatdriver_ctor) dev->user2;

	return bdf_floppy_load_internal(img, fp, open_mode, open_formats, create_format, fp, open_mode);
}

static void bdf_floppy_unload(mess_image *img)
{
	struct mess_bdf *messbdf;
	messbdf = image_lookuptag(img, BDFTAG);

	if (messbdf->bdf)
	{
		bdf_close(messbdf->bdf);
		memset(messbdf, 0, sizeof(*messbdf));
	}
}

static void specify_extension(char *extbuf, size_t extbuflen, formatdriver_ctor format)
{
	struct InternalBdFormatDriver drv;
	size_t len;

	format(&drv);

	if (drv.extension)
	{
		while(*extbuf)
		{
			/* already have this extension? */
			if (!strcmpi(extbuf, drv.extension))
				return;

			len = strlen(extbuf) + 1;
			extbuf += len;
			extbuflen -= len;
		}

		assert(strlen(drv.extension)+1 <= extbuflen);
		strncpyz(extbuf, drv.extension, extbuflen);
	}
}

const struct IODevice *bdf_device_specify(struct IODevice *iodev, char *extbuf, size_t extbuflen,
	int count, const formatdriver_ctor *open_formats, formatdriver_ctor create_format)
{
	int i;

	assert(count);
	if (iodev->count == 0)
	{
		memset(extbuf, 0, extbuflen);
		if (open_formats)
		{
			for(i = 0; open_formats[i]; i++)
				specify_extension(extbuf, extbuflen, open_formats[i]);
		}
		if (create_format)
			specify_extension(extbuf, extbuflen, create_format);
		assert(extbuf[0]);

		memset(iodev, 0, sizeof(*iodev));
		iodev->type = IO_FLOPPY;
		iodev->count = count;
		iodev->file_extensions = extbuf;
		iodev->flags = DEVICE_LOAD_RESETS_NONE;
		iodev->open_mode = create_format ? OSD_FOPEN_RW_CREATE_OR_READ : OSD_FOPEN_RW_OR_READ;
		iodev->init = bdf_floppy_init;
		iodev->load = bdf_floppy_load;
		iodev->unload = bdf_floppy_unload;
		iodev->user1 = (void *) open_formats;
		iodev->user2 = (void *) create_format;
	}
	return iodev;
}

