#include "mflopimg.h"
#include "utils.h"
#include "image.h"
#include "devices/flopdrv.h"

#define FLOPPY_TAG		"floptag"
#define LOG_FLOPPY		0

struct mess_flopimg
{
	floppy_image *floppy;
	int track;
};


static struct mess_flopimg *get_flopimg(mess_image *image)
{
	return (struct mess_flopimg *) image_lookuptag(image, FLOPPY_TAG);
}



static void flopimg_seek_callback(mess_image *image, int physical_track)
{
	struct mess_flopimg *flopimg;
	flopimg = get_flopimg(image);
	if (flopimg)
		flopimg->track = physical_track;
}



static int flopimg_get_sectors_per_track(mess_image *image, int side)
{
	struct mess_flopimg *flopimg;
	floperr_t err;
	int sector_count;

	flopimg = get_flopimg(image);
	if (!flopimg)
		return 0;

	err = floppy_get_sector_count(flopimg->floppy, side, flopimg->track, &sector_count);
	if (err)
		return 0;
	return sector_count;
}



static void flopimg_get_id_callback(mess_image *image, chrn_id *id, int id_index, int side)
{
	struct mess_flopimg *flopimg;
	int sector, N;
	UINT32 sector_length;
	
	flopimg = get_flopimg(image);

	floppy_get_indexed_sector_info(flopimg->floppy, side, flopimg->track, id_index, &sector, &sector_length);

	N = compute_log2(sector_length);

	id->C = flopimg->track;
	id->H = side;
	id->R = sector;
	id->data_id = sector;
	id->flags = 0;
	id->N = ((N >= 7) && (N <= 10)) ? N - 7 : 0;
}



#if LOG_FLOPPY
static void log_readwrite(const char *name, int head, int track, int sector, const char *buf, int length)
{
	char membuf[1024];
	int i;
	for (i = 0; i < length; i++)
		sprintf(membuf + i*2, "%02x", (int) (UINT8) buf[i]);
	logerror("%s:  head=%i track=%i sector=%i buffer='%s'\n", name, head, track, sector, membuf);
}
#endif



static void flopimg_read_sector_data_into_buffer(mess_image *image, int side, int index1, char *ptr, int length)
{
	struct mess_flopimg *flopimg;
	flopimg = get_flopimg(image);
	floppy_read_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);
#if LOG_FLOPPY
	log_readwrite("sector_read", side, flopimg->track, index1, ptr, length);
#endif
}



static void flopimg_write_sector_data_from_buffer(mess_image *image, int side, int index1, const char *ptr, int length,int ddam)
{
	struct mess_flopimg *flopimg;
	flopimg = get_flopimg(image);
#if LOG_FLOPPY
	log_readwrite("sector_write", side, flopimg->track, index1, ptr, length);
#endif
	floppy_write_sector(flopimg->floppy, side, flopimg->track, index1, 0, ptr, length);
}



static void flopimg_read_track_data_info_buffer(mess_image *image, int side, void *ptr, int *length)
{
	struct mess_flopimg *flopimg;
	flopimg = get_flopimg(image);
	floppy_read_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static void flopimg_write_track_data_info_buffer(mess_image *image, int side, const void *ptr, int *length)
{
	struct mess_flopimg *flopimg;
	flopimg = get_flopimg(image);
	floppy_write_track_data(flopimg->floppy, side, flopimg->track, ptr, *length);
}



static floppy_interface mess_floppy_interface =
{
	flopimg_seek_callback,
	flopimg_get_sectors_per_track,
	flopimg_get_id_callback,
	flopimg_read_sector_data_into_buffer,
	flopimg_write_sector_data_from_buffer,
	flopimg_read_track_data_info_buffer,
	flopimg_write_track_data_info_buffer,
	NULL
};

/* ----------------------------------------------------------------------- */

static int mame_fseek_thunk(void *file, INT64 offset, int whence)
{
	return mame_fseek((mame_file *) file, offset, whence);
}

static size_t mame_fread_thunk(void *file, void *buffer, size_t length)
{
	return mame_fread((mame_file *) file, buffer, length);
}

static size_t mame_fwrite_thunk(void *file, const void *buffer, size_t length)
{
	return mame_fwrite((mame_file *) file, buffer, length);
}

static UINT64 mame_fsize_thunk(void *file)
{
	return mame_fsize((mame_file *) file);
}

/* ----------------------------------------------------------------------- */

struct io_procs mess_ioprocs =
{
	NULL,
	mame_fseek_thunk,
	mame_fread_thunk,
	mame_fwrite_thunk,
	mame_fsize_thunk
};




/* ----------------------------------------------------------------------- */


static DEVICE_INIT(floppy)
{
	if (!image_alloctag(image, FLOPPY_TAG, sizeof(struct mess_flopimg)))
		return INIT_FAIL;
	return floppy_drive_init(image, &mess_floppy_interface);
}



static DEVICE_LOAD(floppy)
{
	floperr_t err;
	struct mess_flopimg *flopimg;
	const struct IODevice *dev;
	const struct FloppyFormat *floppy_options;
	int floppy_flags;
	const char *extension;

	/* look up instance data */
	flopimg = get_flopimg(image);

	/* figure out the floppy options */
	dev = device_find(Machine->gamedrv, IO_FLOPPY);
	assert(dev);
	floppy_options = dev->user1;

	if (image_has_been_created(image))
	{
		/* creating an image */
		err = floppy_create(file, &mess_ioprocs, floppy_options, NULL, &flopimg->floppy);
		if (err)
			goto error;
	}
	else
	{
		/* opening an image */
		floppy_flags = image_is_writable(image) ? FLOPPY_FLAGS_READWRITE : FLOPPY_FLAGS_READONLY;
		extension = image_filetype(image);
		err = floppy_open_choices(file, &mess_ioprocs, extension, floppy_options, floppy_flags, &flopimg->floppy);
		if (err)
			goto error;
	}
	return INIT_PASS;

error:
	return INIT_FAIL;
}



static DEVICE_UNLOAD(floppy)
{
	struct mess_flopimg *flopimg;
	flopimg = image_lookuptag(image, FLOPPY_TAG);
	floppy_close(flopimg->floppy);
	flopimg->floppy = NULL;
}



void specify_extension(char *extbuf, size_t extbuflen, const char *extension)
{
	char *s;

	/* loop through the extensions that we are adding */
	while(extension && *extension)
	{
		/* loop through the already specified extensions; and check for dupes */
		for (s = extbuf; *s; s += strlen(s) + 1)
		{
			if (!strcmp(extension, s))
				break;
		}

		/* only write if there are no dupes */
		if (*s == '\0')
		{
			/* out of room?  this should never happen */
			if ((s - extbuf + strlen(extension) + 1) >= extbuflen)
			{
				assert(FALSE);
				continue;
			}
	
			/* copy the extension */
			strncpyz(s, extension, extbuflen);
		}

		/* next extension */
		extension += strlen(extension) + 1;
	}
}



const struct IODevice *floppy_device_specify(struct IODevice *iodev, char *extbuf, size_t extbuflen,
	int count, const struct FloppyFormat *floppy_options)
{
	int i;

	assert(count);
	if (iodev->count == 0)
	{
		memset(extbuf, 0, extbuflen);
		for(i = 0; floppy_options[i].construct; i++)
			specify_extension(extbuf, extbuflen, floppy_options[i].extensions);
		assert(extbuf[0]);

		memset(iodev, 0, sizeof(*iodev));
		iodev->type = IO_FLOPPY;
		iodev->count = count;
		iodev->file_extensions = extbuf;
		iodev->flags = DEVICE_LOAD_RESETS_NONE;
		iodev->open_mode = floppy_options->param_guidelines ? OSD_FOPEN_RW_CREATE_OR_READ : OSD_FOPEN_RW_OR_READ;
		iodev->init = device_init_floppy;
		iodev->load = device_load_floppy;
		iodev->unload = device_unload_floppy;
		iodev->user1 = (void *) floppy_options;
	}
	return iodev;
}

