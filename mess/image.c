#include "image.h"
#include "mess.h"
#include "devices/flopdrv.h"
#include "crcfile.h"
#include "utils.h"
#include "pool.h"
#include "snprintf.h"

/* ----------------------------------------------------------------------- */

enum
{
	IMAGE_STATUS_ISLOADING		= 1,
	IMAGE_STATUS_ISLOADED		= 2,
	IMAGE_STATUS_CRCCALCULATED	= 4
};

struct _mess_image
{
	/* variables that persist across image mounts */
	tag_pool tagpool;
	int (*get_open_mode)(mess_image *);

	/* variables that are only non-zero when an image is mounted */
	mame_file *fp;
	UINT32 status;
	char *name;
	char *dir;
	UINT32 crc;
	UINT32 length;
	int effective_mode;
	char *longname;
	char *manufacturer;
	char *year;
	char *playable;
	char *extrainfo;
	memory_pool mempool;
};

static struct _mess_image images[IO_COUNT][MAX_DEV_INSTANCES];

static mame_file *image_fopen_custom(mess_image *img, int filetype, int read_or_write);

#ifdef _MSC_VER
#define ZEXPORT __stdcall
#else
#define ZEXPORT
#endif

extern unsigned int ZEXPORT crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

/* ----------------------------------------------------------------------- */

int image_init(mess_image *img)
{
	int err;
	const struct IODevice *iodev;
	
	memset(img, 0, sizeof(*img));

	tagpool_init(&img->tagpool);

	iodev = image_device(img);
	if (iodev->init)
	{
		err = iodev->init(img);
		if (err != INIT_PASS)
			return err;
	}
	return INIT_PASS;
}

void image_exit(mess_image *img)
{
	const struct IODevice *iodev;	

	iodev = image_device(img);
	if (iodev->exit)
		iodev->exit(img);

	tagpool_exit(&img->tagpool);
}



/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

static int image_load_internal(mess_image *img, const char *name, int is_create, int create_format, option_resolution *create_args)
{
	const struct IODevice *dev;
	char *newname;
	int err = INIT_PASS;
	mame_file *file = NULL;
	UINT8 *buffer = NULL;
	UINT64 size;
	int i;
	int open_mode;

	static INT8 file_modes[2][7][4] =
	{
		{
			/* open */
			{ OSD_FOPEN_READ, -1 },										/* OSD_FOPEN_READ */
			{ OSD_FOPEN_WRITE, -1 },									/* OSD_FOPEN_WRITE */
			{ OSD_FOPEN_RW, -1 },										/* OSD_FOPEN_RW */
			{ OSD_FOPEN_RW_CREATE, -1 },								/* OSD_FOPEN_RW_CREATE */
			{ OSD_FOPEN_RW, OSD_FOPEN_READ, -1 },						/* OSD_FOPEN_RW_OR_READ */
			{ OSD_FOPEN_RW, OSD_FOPEN_READ, OSD_FOPEN_RW_CREATE, -1 },	/* OSD_FOPEN_RW_CREATE_OR_READ */
			{ OSD_FOPEN_READ, OSD_FOPEN_WRITE, -1 }						/* OSD_FOPEN_READ_OR_WRITE */
		},
		{
			/* create */
			{ -1 },														/* OSD_FOPEN_READ */
			{ OSD_FOPEN_WRITE, -1 },									/* OSD_FOPEN_WRITE */
			{ -1 },														/* OSD_FOPEN_RW */
			{ OSD_FOPEN_RW_CREATE, -1 },								/* OSD_FOPEN_RW_CREATE */
			{ -1 },														/* OSD_FOPEN_RW_OR_READ */
			{ OSD_FOPEN_RW_CREATE, -1 },								/* OSD_FOPEN_RW_CREATE_OR_READ */
			{ -1 }														/* OSD_FOPEN_READ_OR_WRITE */
		}
	};

	/* unload if we are loaded */
	if (img->status & IMAGE_STATUS_ISLOADED)
		image_unload(img);
	
	/* if we are attempting to "load" NULL, then exit at this point */
	if (!name)
		return INIT_PASS;

	dev = image_device(img);
	assert(dev);

	img->status |= IMAGE_STATUS_ISLOADING;

	if (name && *name)
	{
		newname = image_strdup(img, name);
		if (!newname)
			goto error;
	}
	else
		newname = NULL;

	img->name = newname;
	img->dir = NULL;

	osd_image_load_status_changed(img, 0);

	if ((timer_get_time() > 0) && (dev->flags & DEVICE_LOAD_RESETS_CPU))
		machine_reset();

	open_mode = image_get_open_mode(img);
	if ((open_mode >= 0) && (open_mode < (sizeof(file_modes[0]) / sizeof(file_modes[0][0]))))
	{
		/* attempt to open the file with the various modes */
		i = 0;
		while(!file && (file_modes[is_create][open_mode][i] >= 0))
		{
			img->effective_mode = file_modes[is_create][open_mode][i++];
			file = image_fopen_custom(img, FILETYPE_IMAGE, img->effective_mode);
		}
		if (!file)
			goto error;

		/* if applicable, call device verify */
		if (dev->imgverify && !image_has_been_created(img))
		{
			size = mame_fsize(file);
			buffer = malloc(size);
			if (!buffer)
				goto error;

			if (mame_fread(file, buffer, (UINT32) size) != size)
				goto error;

			err = dev->imgverify(buffer, size);
			if (err)
				goto error;

			mame_fseek(file, 0, SEEK_SET);

			free(buffer);
			buffer = NULL;
		}
	}

	/* call device load or create */
	if (image_has_been_created(img) && dev->create)
	{
		/* using device create */
		err = dev->create(img, file, create_format, create_args);
		if (err)
			goto error;
	}
	else if (dev->load)
	{
		/* using device load */
		err = dev->load(img, file);
		if (err)
			goto error;
	}

	img->status &= ~IMAGE_STATUS_ISLOADING;
	img->status |= IMAGE_STATUS_ISLOADED;
	return INIT_PASS;

error:
	if (file)
		mame_fclose(file);
	if (buffer)
		free(buffer);
	if (img)
	{
		img->fp = NULL;
		img->name = NULL;
		img->status &= ~IMAGE_STATUS_ISLOADING|IMAGE_STATUS_ISLOADED;
	}

	osd_image_load_status_changed(img, 0);

	return INIT_FAIL;
}



int image_load(mess_image *img, const char *name)
{
	return image_load_internal(img, name, 0, 0, NULL);
}



int image_create(mess_image *img, const char *name, int create_format, option_resolution *create_args)
{
	return image_load_internal(img, name, 1, create_format, create_args);
}



static void image_unload_internal(mess_image *img, int is_final_unload)
{
	const struct IODevice *dev;
	int type = image_devtype(img);

	if ((img->status & IMAGE_STATUS_ISLOADED) == 0)
		return;

	dev = device_find(Machine->gamedrv, type);
	assert(dev);

	if (dev->unload)
		dev->unload(img);

	if (img->fp)
	{
		mame_fclose(img->fp);
		img->fp = NULL;
	}
	pool_exit(&img->mempool);

	img->status = 0;
	img->name = NULL;
	img->dir = NULL;
	img->crc = 0;
	img->length = 0;
	img->longname = NULL;
	img->manufacturer = NULL;
	img->year = NULL;
	img->playable = NULL;
	img->extrainfo = NULL;

	osd_image_load_status_changed(img, is_final_unload);
}


void image_unload(mess_image *img)
{
	image_unload_internal(img, FALSE);
}

void image_unload_all(int ispreload)
{
	int id;
	const struct IODevice *dev;
	mess_image *img;

	if (!ispreload)
		osd_begin_final_unloading();

	/* normalize ispreload */
	ispreload = ispreload ? DEVICE_LOAD_AT_INIT : 0;

	/* unload all devices with matching preload */
	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if ((dev->flags & DEVICE_LOAD_AT_INIT) == ispreload)
		{
			/* all instances */
			for( id = 0; id < dev->count; id++ )
			{
				img = image_from_devtype_and_index(dev->type, id);

				/* unload this image */
				image_unload_internal(img, TRUE);
			}
		}
	}
}



/****************************************************************************
  Device callback installation functions

  Called during DEVICE_INIT() to install callbacks to customize certain
  behavior
****************************************************************************/

void image_set_open_mode_callback(mess_image *img, int (*get_open_mode)(mess_image *))
{
	img->get_open_mode = get_open_mode;
}



/****************************************************************************
  Tag management functions.
  
  When devices have private data structures that need to be associated with a
  device, it is recommended that image_alloctag() be called in the device
  init function.  If the allocation succeeds, then a pointer will be returned
  to a block of memory of the specified size that will last for the lifetime
  of the emulation.  This pointer can be retrieved with image_lookuptag().

  Note that since image_lookuptag() is used to index preallocated blocks of
  memory, image_lookuptag() cannot fail legally.  In fact, an assert will be
  raised if this happens
****************************************************************************/

void *image_alloctag(mess_image *img, const char *tag, size_t size)
{
	return tagpool_alloc(&img->tagpool, tag, size);
}

void *image_lookuptag(mess_image *img, const char *tag)
{
	return tagpool_lookup(&img->tagpool, tag);
}


/****************************************************************************
  CRC info loading

  If the CRC is not checked and the relevant info not loaded, force that info
  to be loaded
****************************************************************************/

static int read_crc_config(const char *sysname, mess_image *img)
{
	int rc = 1;
	crc_file *config;
	char line[1024];
	char crc[9+1];

	config = crcfile_open(sysname, sysname, FILETYPE_CRC);
	if (!config)
		goto done;

	snprintf(crc, sizeof(crc) / sizeof(crc[0]), "%08x", img->crc);
	crcfile_load_string(config, sysname, 0, crc, line, sizeof(line));

	if (!line[0])
		goto done;

	logerror("found CRC %s= %s\n", crc, line);
	img->longname		= image_strdup(img, stripspace(strtok(line, "|")));
	img->manufacturer	= image_strdup(img, stripspace(strtok(NULL, "|")));
	img->year			= image_strdup(img, stripspace(strtok(NULL, "|")));
	img->playable		= image_strdup(img, stripspace(strtok(NULL, "|")));
	img->extrainfo		= image_strdup(img, stripspace(strtok(NULL, "|")));
	rc = 0;

done:
	if (config)
		crcfile_close(config);
	return rc;
}

static int image_checkcrc(mess_image *img)
{
	UINT8 static_buf[2048]; 
	UINT8 *alloc_buf;
	UINT8 *buf;
	UINT32 bufsize;
	UINT32 imgsize;
	UINT32 chunksize;
	const struct IODevice *dev;
	const struct GameDriver *drv;
	mame_file *file;
	UINT32 crc;
	int rc;

	/* this call should not be made when the image is not loaded */
	assert(img->status & (IMAGE_STATUS_ISLOADING | IMAGE_STATUS_ISLOADED));

	/* only calculate CRC if it hasn't been calculated, and the open_mode is read only */
	if (!(img->status & IMAGE_STATUS_CRCCALCULATED) && (img->effective_mode == OSD_FOPEN_READ))
	{
		/* initialize key variables */
		file = image_fp(img);
		imgsize = image_length(img);
		dev = image_device(img);
		crc = 0;

		/* decide which buffer we need to use */
		if (dev && dev->partialcrc && (imgsize > sizeof(static_buf)))
		{
			alloc_buf = (UINT8 *) malloc(imgsize);
			if (!alloc_buf)
				return FALSE;
			buf = alloc_buf;
			bufsize = imgsize;
		}
		else
		{
			alloc_buf = NULL;
			buf = static_buf;
			bufsize = sizeof(static_buf);
		}

		/* reset the file */
		mame_fseek(file, 0, SEEK_SET);

		/* loop through the file, calculating the CRC */
		while(imgsize > 0)
		{
			chunksize = (imgsize > bufsize) ? bufsize : imgsize;

			mame_fread(file, buf, chunksize);

			if (dev && dev->partialcrc)
				crc = dev->partialcrc(buf, chunksize);
			else
				crc = crc32(crc, buf, chunksize);

			imgsize -= chunksize;
		}

		/* cleanup */
		if (alloc_buf)
			free(alloc_buf);
		img->crc = crc;
		mame_fseek(file, 0, SEEK_SET);

		/* now read the CRC file */
		drv = Machine->gamedrv;
		do
		{
			rc = read_crc_config(drv->name, img);
			drv = mess_next_compatible_driver(drv);
		}
		while(rc && drv);
		
		img->status |= IMAGE_STATUS_CRCCALCULATED;
	}
	return TRUE;
}

/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

mame_file *image_fp(mess_image *img)
{
	return img->fp;
}

const struct IODevice *image_device(mess_image *img)
{
	return device_find(Machine->gamedrv, image_devtype(img));
}

int image_exists(mess_image *img)
{
	return image_filename(img) != NULL;
}

int image_slotexists(mess_image *img)
{
	return image_index_in_devtype(img) < device_count(image_devtype(img));
}

const char *image_filename(mess_image *img)
{
	return img->name;
}

const char *image_basename(mess_image *img)
{
	return osd_basename((char *) image_filename(img));
}

const char *image_filetype(mess_image *img)
{
	const char *s;
	s = image_filename(img);
	if (s)
		s = strrchr(s, '.');
	return s ? s+1 : NULL;
}

const char *image_filedir(mess_image *img)
{
	char *s;

	if (!img->dir)
	{
		img->dir = image_strdup(img, img->name);
		if (img->dir)
		{
			s = img->dir + strlen(img->dir);
			while(--s > img->dir)
			{
				if (strchr("\\/:", *s))
				{
					*s = '\0';
					if (osd_get_path_info(FILETYPE_IMAGE, 0, img->dir) == PATH_IS_DIRECTORY)
						break;
				}
			}
		}
	}
	return img->dir;
}



unsigned int image_length(mess_image *img)
{
	return img->length;
}



unsigned int image_crc(mess_image *img)
{
	image_checkcrc(img);
	return img->crc;
}



int image_is_writable(mess_image *img)
{
	return is_effective_mode_writable(img->effective_mode);
}



int image_has_been_created(mess_image *img)
{
	return is_effective_mode_create(img->effective_mode);
}



int image_get_open_mode(mess_image *img)
{
	int open_mode;
	if (img->get_open_mode)
		open_mode = img->get_open_mode(img);
	else
		open_mode = image_device(img)->open_mode;
	return open_mode;
}



void image_make_readonly(mess_image *img)
{
	img->effective_mode = OSD_FOPEN_READ;
}


/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(mess_image *img, size_t size)
{
	assert(img->status & (IMAGE_STATUS_ISLOADING | IMAGE_STATUS_ISLOADED));
	return pool_malloc(&img->mempool, size);
}

void *image_realloc(mess_image *img, void *ptr, size_t size)
{
	assert(img->status & (IMAGE_STATUS_ISLOADING | IMAGE_STATUS_ISLOADED));
	return pool_realloc(&img->mempool, ptr, size);
}

char *image_strdup(mess_image *img, const char *src)
{
	assert(img->status & (IMAGE_STATUS_ISLOADING | IMAGE_STATUS_ISLOADED));
	return pool_strdup(&img->mempool, src);
}

void image_freeptr(mess_image *img, void *ptr)
{
	pool_freeptr(&img->mempool, ptr);
}

/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(mess_image *img)
{
	image_checkcrc(img);
	return img->longname;
}

const char *image_manufacturer(mess_image *img)
{
	image_checkcrc(img);
	return img->manufacturer;
}

const char *image_year(mess_image *img)
{
	image_checkcrc(img);
	return img->year;
}

const char *image_playable(mess_image *img)
{
	image_checkcrc(img);
	return img->playable;
}

const char *image_extrainfo(mess_image *img)
{
	image_checkcrc(img);
	return img->extrainfo;
}

/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

static char *battery_nvramfilename(mess_image *img)
{
	const char *filename;
	filename = image_filename(img);
	return strip_extension(osd_basename((char *) filename));
}

/* load battery backed nvram from a driver subdir. in the nvram dir. */
int image_battery_load(mess_image *img, void *buffer, int length)
{
	mame_file *f;
	int bytes_read = 0;
	int result = FALSE;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(img);
		if (nvram_filename)
		{
			f = mame_fopen(Machine->gamedrv->name, nvram_filename, FILETYPE_NVRAM, 0);
			if (f)
			{
				bytes_read = mame_fread(f, buffer, length);
				mame_fclose(f);
				result = TRUE;
			}
			free(nvram_filename);
		}

		/* fill remaining bytes (if necessary) */
		memset(((char *) buffer) + bytes_read, '\0', length - bytes_read);
	}
	return result;
}

/* save battery backed nvram to a driver subdir. in the nvram dir. */
int image_battery_save(mess_image *img, const void *buffer, int length)
{
	mame_file *f;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(img);
		if (nvram_filename)
		{
			f = mame_fopen(Machine->gamedrv->name, nvram_filename, FILETYPE_NVRAM, 1);
			if (f)
			{
				mame_fwrite(f, buffer, length);
				mame_fclose(f);
				return TRUE;
			}
			free(nvram_filename);
		}
	}
	return FALSE;
}

/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(mess_image *image)
{
	return image - &images[0][0];
}

mess_image *image_from_absolute_index(int absolute_index)
{
	return &images[0][absolute_index];
}

/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

int image_index_in_device(mess_image *img)
{
	return image_index_in_devtype(img);
}

mess_image *image_from_device_and_index(const struct IODevice *dev, int id)
{
	return image_from_devtype_and_index(dev->type, id);
}

mess_image *image_from_devtype_and_index(int type, int id)
{
	assert(id < device_count(type));
	return &images[type][id];
}

int image_devtype(mess_image *img)
{
	return (img - &images[0][0]) / MAX_DEV_INSTANCES;
}



int image_index_in_devtype(mess_image *img)
{
	assert(img);
	return (img - &images[0][0]) % MAX_DEV_INSTANCES;
}



static mame_file *image_fopen_custom(mess_image *img, int filetype, int read_or_write)
{
	const char *sysname;
	mame_file *file;
	char buffer[512];
	const struct GameDriver *gamedrv = Machine->gamedrv;

	assert(img);

	if (!img->name)
		return NULL;

	if (img->fp)
		/* If already open, we won't open the file again until it is closed. */
		return NULL;

	do
	{
		sysname = gamedrv->name;
		logerror("image_fopen: trying %s for system %s\n", img->name, sysname);
		img->fp = file = mame_fopen(sysname, img->name, filetype, read_or_write);

		gamedrv = mess_next_compatible_driver(gamedrv);
	}
	while(!img->fp && gamedrv);

	if ((file) && ! is_effective_mode_create(read_or_write))
	{
		/* is this file actually a zip file? */
		if ((mame_fread(file, buffer, 4) == 4) && (buffer[0] == 0x50)
			&& (buffer[1] == 0x4B) && (buffer[2] == 0x03) && (buffer[3] == 0x04))
		{
			mame_fseek(file, 26, SEEK_SET);
			if (mame_fread(file, buffer, 2) == 2)
			{
				int fname_length = buffer[0];
				char *newname;

				mame_fseek(file, 30, SEEK_SET);
				mame_fread(file, buffer, fname_length);
				mame_fclose(file);
				img->fp = file = NULL;

				buffer[fname_length] = '\0';

				newname = image_malloc(img, strlen(img->name) + 1 + fname_length + 1);
				if (!newname)
					return NULL;

				strcpy(newname, img->name);
				strcat(newname, osd_path_separator());
				strcat(newname, buffer);
				img->fp = file = mame_fopen(sysname, newname, filetype, read_or_write);
				if (!file)
					return NULL;

				image_freeptr(img, img->name);
				img->name = newname;
			}
		}
		mame_fseek(file, 0, SEEK_SET);

		logerror("image_fopen: found image %s for system %s\n", img->name, sysname);
		img->length = mame_fsize(file);
		img->crc = 0;
		img->status &= ~IMAGE_STATUS_CRCCALCULATED;
	}

	return file;
}

