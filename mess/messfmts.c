#include "messfmts.h"
#include "osdepend.h"
#include "mess.h"
#include "includes/flopdrv.h"
#include "image.h"
#include "utils.h"

struct mess_bdf
{
	void *bdf;
	int track, sector;
};

struct mess_bdf bdfs[5];

/* ----------------------------------------------------------------------- */

static struct mess_bdf *get_messbdf(int drive)
{
	if ((drive >= (sizeof(bdfs) / sizeof(bdfs[0]))) || !bdfs[drive].bdf)
		return NULL;
	else
		return &bdfs[drive];
}

static void bdf_seek_callback(int drive, int physical_track)
{
	struct mess_bdf *messbdf;
	
	messbdf = get_messbdf(drive);
	if (messbdf)
		messbdf->track = physical_track;
}

static int bdf_get_sectors_per_track(int drive, int side)
{
	struct mess_bdf *messbdf;
	
	messbdf = get_messbdf(drive);
	if (!messbdf)
		return 0;
	
	return bdf_get_sector_count(messbdf->bdf, messbdf->track, side);
}

static void bdf_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	struct mess_bdf *messbdf;
	UINT16 sector_size;
	UINT8 sector;
	
	messbdf = get_messbdf(drive);

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

static void bdf_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length)
{
	struct mess_bdf *messbdf;
	messbdf = get_messbdf(drive);
	bdf_read_sector(messbdf->bdf, messbdf->track, side, index1, 0, ptr, length);
}

static void bdf_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length,int ddam)
{
	struct mess_bdf *messbdf;
	messbdf = get_messbdf(drive);
	bdf_write_sector(messbdf->bdf, messbdf->track, side, index1, 0, ptr, length);
}

/* ----------------------------------------------------------------------- */

static struct bdf_procs mess_bdf_procs =
{
	osd_fclose,
	osd_fseek,
	osd_fread,
	osd_fwrite,
	osd_fsize
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

static int bdf_floppy_init_internal(int id, void *file, int mode, const formatdriver_ctor *open_formats, formatdriver_ctor create_format, void *fp, int open_mode)
{
	const char *name;
	const char *ext;
	formatdriver_ctor fmts[2];
	int device_type = IO_FLOPPY;
	int err;
	
	assert(id < (sizeof(bdfs) / sizeof(bdfs[0])));
	memset(&bdfs[id], 0, sizeof(bdfs[id]));

	if (file)
	{
		name = image_filename(device_type, id);

		if (mode == OSD_FOPEN_RW_CREATE)
		{
			err = bdf_create(&mess_bdf_procs, create_format, file, NULL, &bdfs[id].bdf);
		}
		else
		{
			if (!open_formats)
			{
				fmts[0] = create_format;
				fmts[1] = NULL;
				open_formats = fmts;				
			}
			ext = image_filetype(device_type, id);
			err = bdf_open(&mess_bdf_procs, open_formats, file, (mode == OSD_FOPEN_READ), ext, &bdfs[id].bdf);
		}
		if (err)
			return INIT_FAIL;

		floppy_drive_set_disk_image_interface(id, &bdf_floppy_interface);
	}
	return INIT_PASS;
}

static int bdf_floppy_init(int id, void *fp, int open_mode)
{
	const struct IODevice *dev;
	const formatdriver_ctor *open_formats;
	formatdriver_ctor create_format;

	dev = device_find(Machine->gamedrv, IO_FLOPPY);
	assert(dev);

	open_formats = (const formatdriver_ctor *) dev->input;
	create_format = (formatdriver_ctor) dev->output;

	return bdf_floppy_init_internal(id, fp, open_mode, open_formats, create_format, fp, open_mode);
}

static void bdf_floppy_exit(int id)
{
	if (bdfs[id].bdf)
	{
		bdf_close(bdfs[id].bdf);
		memset(&bdfs[id], 0, sizeof(bdfs[id]));
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
		iodev->reset_depth = IO_RESET_NONE;
		iodev->open_mode = create_format ? OSD_FOPEN_RW_CREATE_OR_READ : OSD_FOPEN_RW_OR_READ;
		iodev->init = bdf_floppy_init;
		iodev->exit = bdf_floppy_exit;
		iodev->input = (int (*)(int)) (void *) open_formats;
		iodev->output = (void (*)(int, int)) (void *) create_format;
	}
	return iodev;
}

