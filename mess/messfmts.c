#include "messfmts.h"
#include "osdepend.h"
#include "mess.h"
#include "includes/flopdrv.h"
#include "image.h"

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
	return bdf_get_geometry(messbdf->bdf)->sectors;
}

static void bdf_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	struct mess_bdf *messbdf;
	const struct disk_geometry *geo;
	
	messbdf = get_messbdf(drive);
	geo = bdf_get_geometry(messbdf);

	id->C = messbdf->track;
	id->H = side;
	id->R = geo->first_sector_id + id_index;
	id->data_id = geo->first_sector_id + id_index;
	id->flags = 0;

	switch(geo->sector_size) {
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

static int bdf_floppy_init_internal(int id, const formatdriver_ctor *open_formats, formatdriver_ctor create_format)
{
	void *file;
	const char *name;
	int mode;
	const char *ext;
	formatdriver_ctor fmts[2];
	int device_type = IO_FLOPPY;
	int err;
	
	assert(id < (sizeof(bdfs) / sizeof(bdfs[0])));
	memset(&bdfs[id], 0, sizeof(bdfs[id]));

	name = image_filename(device_type, id);
	if (name)
	{
		mode = OSD_FOPEN_RW;
		file = image_fopen(device_type, id, OSD_FILETYPE_IMAGE, mode);
		if (!file)
		{
			mode = OSD_FOPEN_READ;
			file = image_fopen(device_type, id, OSD_FILETYPE_IMAGE, mode);
			if (!file)
			{
				mode = OSD_FOPEN_RW_CREATE;
				file = image_fopen(device_type, id, OSD_FILETYPE_IMAGE, mode);
				if (!file)
					return INIT_FAIL;
			}
		}

		/* its lame that I have to do this */
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
			ext = strrchr(name, '.');
			err = bdf_open(&mess_bdf_procs, open_formats, file, (mode == OSD_FOPEN_READ), ext, &bdfs[id].bdf);
		}
		if (err)
			return INIT_FAIL;

		floppy_drive_set_disk_image_interface(id, &bdf_floppy_interface);
	}
	return INIT_PASS;
}

int bdf_floppy_init(int id)
{
	const struct IODevice *dev;
	const formatdriver_ctor *open_formats;
	formatdriver_ctor create_format;

	dev = device_find(Machine->gamedrv, IO_FLOPPY);
	assert(dev);

	open_formats = (const formatdriver_ctor *) dev->input_chunk;
	create_format = (formatdriver_ctor) dev->output_chunk;

	return bdf_floppy_init_internal(id, open_formats, create_format);
}

void bdf_floppy_exit(int id)
{
	if (bdfs[id].bdf)
	{
		bdf_close(bdfs[id].bdf);
		memset(&bdfs[id], 0, sizeof(bdfs[id]));
	}
}
