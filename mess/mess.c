/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include "driver.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "ui_text.h"
#include "state.h"
#include "image.h"
#include "inputx.h"
#include "snprintf.h"
#include "artwork.h"

extern struct GameOptions options;

/* Globals */
const char *mess_path;
int devices_inited;

UINT32 mess_ram_size;
data8_t *mess_ram;
data8_t mess_ram_default_value = 0xCD;

int DECL_SPEC mess_printf(const char *fmt, ...)
{
	va_list arg;
	int length = 0;

	va_start(arg,fmt);

	if (options.mess_printf_output)
		length = options.mess_printf_output(fmt, arg);
	else if (!options.gui_host)
		length = vprintf(fmt, arg);

	va_end(arg);

	return length;
}

struct distributed_images
{
	const char *names[IO_COUNT][MAX_DEV_INSTANCES];
	int count[IO_COUNT];
};

/*****************************************************************************
 *  --Distribute images to their respective Devices--
 *  Copy the Images specified at the CLI from options.image_files[] to the
 *  array of filenames we keep here, depending on the Device type identifier
 *  of each image.  Multiple instances of the same device are allowed
 *  RETURNS 0 on success, 1 if failed
 ****************************************************************************/
static int distribute_images(struct distributed_images *images)
{
	int i;

	logerror("Distributing Images to Devices...\n");
	/* Set names to NULL */

	for( i = 0; i < options.image_count; i++ )
	{
		int type = options.image_files[i].type;

		assert(type < IO_COUNT);

		/* Do we have too many devices? */
		if (images->count[type] >= MAX_DEV_INSTANCES)
		{
			mess_printf(" Too many devices of type %d\n", type);
			return 1;
		}

		/* Add a filename to the arrays of names */
		images->names[type][images->count[type]] = options.image_files[i].name;
		images->count[type]++;
	}

	/* everything was fine */
	return 0;
}



/* Small check to see if system supports device */
static int supported_device(const struct GameDriver *gamedrv, int type)
{
	const struct IODevice *dev;
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		if(dev->type==type)
			return TRUE;	/* Return OK */
	}
	return FALSE;
}



static int ram_init(const struct GameDriver *gamedrv)
{
	int i;

	/* validate RAM option */
	if (options.ram != 0)
	{
		if (!ram_is_valid_option(gamedrv, options.ram))
		{
			char buffer[RAM_STRING_BUFLEN];
			int opt_count;

			opt_count = ram_option_count(gamedrv);
			if (opt_count == 0)
			{
				/* this driver doesn't support RAM configurations */
				mess_printf("Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				mess_printf("%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer, options.ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					mess_printf("%s%s",  i ? " or " : "", ram_string(buffer, ram_option(gamedrv, i)));
				mess_printf(")\n");
			}
			return 1;
		}
		mess_ram_size = options.ram;
	}
	else
	{
		/* none specified; chose default */
		mess_ram_size = ram_default(gamedrv);
	}
	/* if we have RAM, allocate it */
	if (mess_ram_size > 0)
	{
		mess_ram = (UINT8 *) auto_malloc(mess_ram_size);
		if (!mess_ram)
			return 1;
		memset(mess_ram, mess_ram_default_value, mess_ram_size);

		state_save_register_UINT32("mess", 0, "ramsize", &mess_ram_size, 1);
		state_save_register_UINT8("mess", 0, "ram", mess_ram, mess_ram_size);
	}
	else
	{
		mess_ram = NULL;
	}
	return 0;
}

/*****************************************************************************
 *  --Initialise Devices--
 *  Call the init() functions for all devices of a driver
 *  ith all user specified image names.
 ****************************************************************************/
int devices_init(const struct GameDriver *gamedrv)
{
	const struct IODevice *dev;
	int i, id;

	/* convienient place to call this */
	{
		const char *cwd;
		char *s;

		cwd = osd_get_cwd();
		s = auto_malloc(strlen(cwd) + 1);
		if (!s)
			return 1;
		strcpy(s, cwd);
		mess_path = s;
	}

	inputx_init();

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (supported_device(Machine->gamedrv, options.image_files[i].type)==FALSE)
		{
			mess_printf(" ERROR: Device [%s] is not supported by this system\n",device_typename(options.image_files[i].type));
			return 1;
		}
	}

	/* initialize RAM code */
	if (ram_init(gamedrv))
		return 1;

	/* init all devices */
	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
		{
			mess_image *img = image_from_devtype_and_index(dev->type, id);
			image_init(img);
		}
	}
	devices_inited = TRUE;

	return 0;
}

int devices_initialload(const struct GameDriver *gamedrv, int ispreload)
{
	int id;
	int result;
	int count;
	struct distributed_images images;
	const struct IODevice *dev;
	const char *imagename;
	mess_image *img;

	/* normalize ispreload */
	ispreload = ispreload ? DEVICE_LOAD_AT_INIT : 0;

	/* distribute images to appropriate devices */
	memset(&images, 0, sizeof(images));
	if (distribute_images(&images))
		return 1;

	/* load all devices with matching preload */
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		count = images.count[dev->type];

		if ((dev->flags & DEVICE_MUST_BE_LOADED) && (count != dev->count))
		{
			mess_printf("Driver requires that device %s must have an image to load\n", device_typename(dev->type));
			return 1;
		}

		/* all instances */
		for( id = 0; id < count; id++ )
		{
			if ((dev->flags & DEVICE_LOAD_AT_INIT) == ispreload)
			{
				imagename = images.names[dev->type][id];
				if (imagename)
				{
					img = image_from_devtype_and_index(dev->type, id);

					/* load this image */
					result = image_load(img, images.names[dev->type][id]);

					if (result != INIT_PASS)
					{
						mess_printf("Driver reports load for %s device failed\n", device_typename(dev->type));
						mess_printf("Ensure image is valid and exists and (if needed) can be created\n");
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void devices_exit(void)
{
	const struct IODevice *dev;
	int id;
	mess_image *img;

	/* unload all devices */
	image_unload_all(FALSE);
	image_unload_all(TRUE);

	/* exit all devices */
	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
		{
			img = image_from_devtype_and_index(dev->type, id);
			image_exit(img);
		}
	}

	devices_inited = FALSE;
}

void showmessdisclaimer(void)
{
	mess_printf(
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the BIOS, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n");
}

void showmessinfo(void)
{
	mess_printf(
		"M.E.S.S. v%s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2004 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2003 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	showmessdisclaimer();
	mess_printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"        MESS -showusage   to see usage instructions\n"
		"See mess.txt for help, readme.txt for options.\n");
}



void ram_dump(const char *filename)
{
	mame_file *file;

	if (!filename)
		filename = "ram.bin";

	file = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_NVRAM, OSD_FOPEN_WRITE);
	if (file)
	{
		mame_fwrite(file, mess_ram, mess_ram_size);

		/* close file */
		mame_fclose(file);
	}
}



void machine_hard_reset(void)
{
	memset(mess_ram, mess_ram_default_value, mess_ram_size);
	machine_reset();
}



const struct GameDriver *mess_next_compatible_driver(const struct GameDriver *drv)
{
	if (drv->clone_of && !(drv->clone_of->flags & NOT_A_DRIVER))
		drv = drv->clone_of;
	else if (drv->compatible_with && !(drv->compatible_with->flags & NOT_A_DRIVER))
		drv = drv->compatible_with;
	else
		drv = NULL;
	return drv;
}



int mess_count_compatible_drivers(const struct GameDriver *drv)
{
	int count = 0;
	while(drv)
	{
		count++;
		drv = mess_next_compatible_driver(drv);
	}
	return count;
}



UINT32 hash_data_extract_crc32(const char *d)
{
	UINT32 crc = 0;
	UINT8 crc_bytes[4];

	if (hash_data_extract_binary_checksum(d, HASH_CRC, crc_bytes) == 1)
	{
		crc = (((UINT32) crc_bytes[0]) << 24)
			| (((UINT32) crc_bytes[1]) << 16)
			| (((UINT32) crc_bytes[2]) << 8)
			| (((UINT32) crc_bytes[3]) << 0);
	}
	return crc;
}


data32_t read32_with_read8_handler(read8_handler handler, offs_t offset, data32_t mem_mask)
{
	data32_t result = 0;
	if ((mem_mask & 0x000000FF) == 0)
		result |= ((data32_t) handler(offset * 4 + 0)) << 0;
	if ((mem_mask & 0x0000FF00) == 0)
		result |= ((data32_t) handler(offset * 4 + 1)) << 8;
	if ((mem_mask & 0x00FF0000) == 0)
		result |= ((data32_t) handler(offset * 4 + 2)) << 16;
	if ((mem_mask & 0xFF000000) == 0)
		result |= ((data32_t) handler(offset * 4 + 3)) << 24;
	return result;
}



void write32_with_write8_handler(write8_handler handler, offs_t offset, data32_t data, data32_t mem_mask)
{
	if ((mem_mask & 0x000000FF) == 0)
		handler(offset * 4 + 0, data >> 0);
	if ((mem_mask & 0x0000FF00) == 0)
		handler(offset * 4 + 1, data >> 8);
	if ((mem_mask & 0x00FF0000) == 0)
		handler(offset * 4 + 2, data >> 16);
	if ((mem_mask & 0xFF000000) == 0)
		handler(offset * 4 + 3, data >> 24);
}



/***************************************************************************

	Dummy read handlers

***************************************************************************/

READ_HANDLER( return8_00 )	{ return 0x00; }
READ_HANDLER( return8_FE )	{ return 0xFE; }
READ_HANDLER( return8_FF )	{ return 0xFF; }



/***************************************************************************

	MESS debug code

***************************************************************************/

#ifdef MAME_DEBUG
int messvaliditychecks(void)
{
	int i, j;
	int is_invalid;
	int error = 0;
	const struct IODevice *dev;
	long used_devices;
	const char *s1;
	const char *s2;
	extern int device_valididtychecks(void);

	/* MESS specific driver validity checks */
	for(i = 0; drivers[i]; i++)
	{
		/* make sure that there are no clones that reference nonexistant drivers */
		if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
		{
			if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
			{
				printf("%s: both compatbile_with and clone_of are specified\n", drivers[i]->name);
				error = 1;
			}

			for (j = 0; drivers[j]; j++)
			{
				if (drivers[i]->clone_of == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				printf("%s: is a clone of %s, which is not in drivers[]\n", drivers[i]->name, drivers[i]->clone_of->name);
				error = 1;
			}
		}

		/* make sure that there are no clones that reference nonexistant drivers */
		if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
		{
			for (j = 0; drivers[j]; j++)
			{
				if (drivers[i]->compatible_with == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				printf("%s: is compatible with %s, which is not in drivers[]\n", drivers[i]->name, drivers[i]->compatible_with->name);
				error = 1;
			}
		}

		/* check device array */
		used_devices = 0;
		for(dev = device_first(drivers[i]); dev; dev = device_next(drivers[i], dev))
		{
			if (dev->type >= IO_COUNT)
			{
				printf("%s: invalid device type %i\n", drivers[i]->name, dev->type);
				error = 1;
			}

			/* make sure that we can't duplicate devices */
			if (used_devices & (1 << dev->type))
			{
				printf("%s: device type '%s' is specified multiple times\n", drivers[i]->name, device_typename(dev->type));
				error = 1;
			}
			used_devices |= (1 << dev->type);

			/* File Extensions Checks
			 * 
			 * Checks the following
			 *
			 * 1.  Tests the integrity of the string list
			 * 2.  Checks for duplicate extensions
			 * 3.  Makes sure that all extensions are either lower case chars or numbers
			 */
			s1 = dev->file_extensions;
			while(*s1)
			{
				/* check for invalid chars */
				is_invalid = 0;
				for (s2 = s1; *s2; s2++)
				{
					if (!isdigit(*s2) && !islower(*s2))
						is_invalid = 1;
				}
				if (is_invalid)
				{
					printf("%s: device type '%s' has an invalid extension '%s'\n", drivers[i]->name, device_typename(dev->type), s1);
					error = 1;
				}
				s2++;

				/* check for dupes */
				is_invalid = 0;
				while(*s2)
				{
					if (!strcmp(s1, s2))
						is_invalid = 1;
					s2 += strlen(s2) + 1;
				}
				if (is_invalid)
				{
					printf("%s: device type '%s' has duplicate extensions '%s'\n", drivers[i]->name, device_typename(dev->type), s1);
					error = 1;
				}

				s1 += strlen(s1) + 1;
			}

			/* enforce certain rules for certain device types */
			switch(dev->type) {
			case IO_QUICKLOAD:
			case IO_SNAPSHOT:
				if (dev->count != 1)
				{
					printf("%s: there can only be one instance of devices of type '%s'\n", drivers[i]->name, device_typename(dev->type));
					error = 1;
				}
				/* fallthrough */

			case IO_CARTSLOT:
				if (dev->open_mode != OSD_FOPEN_READ)
				{
					printf("%s: devices of type '%s' must have open mode OSD_FOPEN_READ\n", drivers[i]->name, device_typename(dev->type));
					error = 1;
				}
				break;
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);

		/* make sure that our input system likes this driver */
		if (inputx_validitycheck(drivers[i]))
			error = 1;
	}

	if (inputx_validitycheck(NULL))
		error = 1;
	if (device_valididtychecks())
		error = 1;
	return error;
}

enum
{
	TESTERROR_SUCCESS			= 0,

	/* benign errors */
	TESTERROR_NOROMS			= 1,

	/* failures */
	TESTERROR_INITDEVICESFAILED	= -1
};

#if 0
static int try_driver(const struct GameDriver *gamedrv)
{
	int i;
	int error;
	void *mem[128];

	Machine->gamedrv = gamedrv;
	Machine->drv = gamedrv->drv;
	memset(&Machine->memory_region, 0, sizeof(Machine->memory_region));
	if (readroms() != 0)
	{
		error = TESTERROR_NOROMS;
	}
	else
	{
		if (device_init(gamedrv) != 0)
			error = TESTERROR_INITDEVICESFAILED;
		else
			devices_exit();
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
			free_memory_region(i);
	}

	/* hammer away on malloc, to detect any memory errors */
	for (i = 0; i < sizeof(mem) / sizeof(mem[0]); i++)
		mem[i] = malloc((i * 131 % 51) + 64);
	for (i = 0; i < sizeof(mem) / sizeof(mem[0]); i++)
		free(mem[i]);

	return 0;
}
#endif

void messtestdriver(const struct GameDriver *gamedrv, const char *(*getfodderimage)(unsigned int index, int *foddertype))
{
	struct GameOptions saved_options;

	/* preserve old options; the MESS GUI needs this */
	memcpy(&saved_options, &options, sizeof(saved_options));

	/* clear out images in options */
	memset(&options.image_files, 0, sizeof(options.image_files));
	options.image_count = 0;

	/* try running with no attached devices */
#if 0
	error = try_driver(gamedrv);
	if (gamedrv->flags & GAME_COMPUTER)
		assert(error >= 0);	/* computers should succeed when ran with no attached devices */
	else
		assert(error <= 0);	/* consoles can fail; but should never fail due to a no roms error */
#endif

	/* restore old options */
	memcpy(&options, &saved_options, sizeof(options));
}

#endif /* MAME_DEBUG */
