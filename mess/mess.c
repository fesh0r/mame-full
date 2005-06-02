/***************************************************************************

	mess.c

	This file is a set of function calls and defs required for MESS

***************************************************************************/

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
#include "artwork.h"

extern struct GameOptions options;

/* Globals */
const char *mess_path;
int devices_inited;

UINT32 mess_ram_size;
data8_t *mess_ram;
data8_t mess_ram_default_value = 0xCD;



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
				printf("Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				printf("%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer, options.ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					printf("%s%s",  i ? " or " : "", ram_string(buffer, ram_option(gamedrv, i)));
				printf(")\n");
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
	int i;

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

	/* initialize natural keyboard support */
	inputx_init();

	/* allocate the IODevice struct */
	Machine->devices = devices_allocate(Machine->gamedrv);
	if (!Machine->devices)
		return 1;

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (!device_find(Machine->devices, options.image_files[i].type))
		{
			printf(" ERROR: Device [%s] is not supported by this system\n",device_typename(options.image_files[i].type));
			return 1;
		}
	}

	/* initialize RAM code */
	if (ram_init(gamedrv))
		return 1;

	/* init all devices */
	image_init();
	devices_inited = TRUE;
	return 0;
}



int devices_initialload(const struct GameDriver *gamedrv, int ispreload)
{
	int i;
	int id;
	int result = INIT_FAIL;
	int devcount;
	int *allocated_slots;
	const struct IODevice *dev;
	const char *image_name;
	mess_image *image;
	iodevice_t devtype;

	/* normalize ispreload */
	ispreload = ispreload ? 1 : 0;

	/* wrong time to preload? */
	if (ispreload != devices_inited)
		return 0;

	/* count number of devices, and record a list of allocated slots */
	devcount = 0;
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		devcount++;
	if (devcount > 0)
	{
		allocated_slots = malloc(devcount * sizeof(*allocated_slots));
		if (!allocated_slots)
			goto error;
		memset(allocated_slots, 0, devcount * sizeof(*allocated_slots));
	}
	else
	{
		allocated_slots = NULL;
	}

	/* distribute images to appropriate devices */
	for (i = 0; i < options.image_count; i++)
	{
		/* get the image type and filename */
		image_name = options.image_files[i].name;
		image_name = (image_name && image_name[0]) ? image_name : NULL;
		devtype = options.image_files[i].type;
		assert(devtype >= 0);
		assert(devtype < IO_COUNT);

		image = NULL;

		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			if (dev->type == devtype)
			{
				id = allocated_slots[dev - Machine->devices];
				if (id < dev->count)
				{
					result = INIT_PASS;
					if (image_name)
					{
						/* try to load this image */
						image = image_from_device_and_index(dev, id);
						result = image_load(image, image_name);
					}
					if (result == INIT_PASS)
					{
						allocated_slots[dev - Machine->devices]++;
						break;
					}
				}
			}
		}
		if (dev->type >= IO_COUNT)
		{
			if (image)
			{
				printf("Device %s load (%s) failed: %s\n",
					device_typename(devtype),
					osd_basename((char *) image_name),
					image_error(image));
			}
			else
			{
				printf("Too many devices of type %d\n", devtype);
			}
			goto error;
		}
	}

	/* make sure that any required devices have been allocated */
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		if (dev->must_be_loaded && (allocated_slots[dev - Machine->devices] != dev->count))
		{
			printf("Driver requires that device %s must have an image to load\n", device_typename(dev->type));
			goto error;
		}
	}
	free(allocated_slots);
	return 0;

error:
	if (allocated_slots)
		free(allocated_slots);
	return 1;
}



/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void devices_exit(void)
{
	/* unload all devices */
	image_unload_all(FALSE);
	image_unload_all(TRUE);

	/* exit all devices */
	image_exit();
	devices_inited = FALSE;
}



int register_device(iodevice_t type, const char *arg)
{
	extern struct GameOptions options;

	/* Check the the device type is valid, otherwise this lookup will be bad*/
	if (type < 0 || type >= IO_COUNT)
	{
		printf("register_device() failed! - device type [%d] is not valid\n",type);
		return -1;
	}

	/* Next, check that we havent loaded too many images					*/
	if (options.image_count >= sizeof(options.image_files) / sizeof(options.image_files[0]))
	{
		printf("Too many image names specified!\n");
		return -1;
	}

	/* All seems ok to add device and argument to options{} struct			*/
	logerror("Image [%s] Registered for Device [%s]\n", arg, device_typename(type));

	/* the user specified a device type */
	options.image_files[options.image_count].type = type;
	options.image_files[options.image_count].name = strdup(arg);
	options.image_count++;
	return 0;

}



void showmessdisclaimer(void)
{
	printf(
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
	printf(
		"M.E.S.S. v%s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2004 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2004 by Nicola Salmoria and the MAME Team\n\n",
		build_version);

	showmessdisclaimer();

	printf(
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



char *auto_strlistdup(char *strlist)
{
	int i;
	char *s;

	for (i = 2; strlist[i - 2] || strlist[i - 1]; i++)
		;
	s = auto_malloc(i * sizeof(*strlist));
	if (!s)
		return NULL;

	memcpy(s, strlist, i * sizeof(*strlist));
	return s;
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


/***************************************************************************

	Reading read8 handlers with other types

***************************************************************************/

data32_t read32le_with_read8_handler(read8_handler handler, offs_t offset, data32_t mem_mask)
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



void write32le_with_write8_handler(write8_handler handler, offs_t offset, data32_t data, data32_t mem_mask)
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



data64_t read64be_with_read8_handler(read8_handler handler, offs_t offset, data64_t mem_mask)
{
	data64_t result = 0;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (((mem_mask >> (56 - i * 8)) & 0xFF) == 0)
			result |= ((data64_t) handler(offset * 8 + i)) << (56 - i * 8);
	}
	return result;
}



void write64be_with_write8_handler(write8_handler handler, offs_t offset, data64_t data, data64_t mem_mask)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (((mem_mask >> (56 - i * 8)) & 0xFF) == 0)
			handler(offset * 8 + i, data >> (56 - i * 8));
	}
}



data64_t read64le_with_32le_handler(read32_handler handler, offs_t offset, data64_t mem_mask)
{
	data64_t result = 0;
	if ((mem_mask & U64(0x00000000FFFFFFFF)) != U64(0x00000000FFFFFFFF))
		result |= ((data64_t) handler(offset * 2 + 0, (data32_t) (mem_mask >> 0))) << 0;
	if ((mem_mask & U64(0xFFFFFFFF00000000)) != U64(0xFFFFFFFF00000000))
		result |= ((data64_t) handler(offset * 2 + 1, (data32_t) (mem_mask >> 0))) << 32;
	return result;
}



void write64le_with_32le_handler(write32_handler handler, offs_t offset, data64_t data, data64_t mem_mask)
{
	if ((mem_mask & U64(0x00000000FFFFFFFF)) != U64(0x00000000FFFFFFFF))
		handler(offset * 2 + 0, data >>  0, mem_mask >>  0);
	if ((mem_mask & U64(0xFFFFFFFF00000000)) != U64(0xFFFFFFFF00000000))
		handler(offset * 2 + 1, data >> 32, mem_mask >> 32);
}



data64_t read64be_with_32le_handler(read32_handler handler, offs_t offset, data64_t mem_mask)
{
	data64_t result;
	mem_mask = FLIPENDIAN_INT64(mem_mask);
	result = read64le_with_32le_handler(handler, offset, mem_mask);
	return FLIPENDIAN_INT64(result);
}



void write64be_with_32le_handler(write32_handler handler, offs_t offset, data64_t data, data64_t mem_mask)
{
	data = FLIPENDIAN_INT64(data);
	mem_mask = FLIPENDIAN_INT64(mem_mask);
	write64le_with_32le_handler(handler, offset, data, mem_mask);
}



/***************************************************************************

	Dummy read handlers

***************************************************************************/

READ8_HANDLER( return8_00 )	{ return 0x00; }
READ8_HANDLER( return8_FE )	{ return 0xFE; }
READ8_HANDLER( return8_FF )	{ return 0xFF; }



/***************************************************************************

	Dummy read handlers

***************************************************************************/

void mess_config_save_xml(int type, mame_file *file)
{
	osd_config_save_xml(type, file);
}



/***************************************************************************

	MESS debug code

***************************************************************************/

int mess_validitychecks(void)
{
	int i, j;
	int is_invalid;
	int error = 0;
	struct IODevice *devices;
	const struct IODevice *dev;
	const char *s1;
	const char *s2;
	extern int device_valididtychecks(void);

	/* MESS specific driver validity checks */
	for (i = 0; drivers[i]; i++)
	{
		begin_resource_tracking();
		devices = devices_allocate(drivers[i]);

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
		for (j = 0; devices[j].type < IO_COUNT; j++)
		{
			dev = &devices[j];

			if (dev->type >= IO_COUNT)
			{
				printf("%s: invalid device type %i\n", drivers[i]->name, dev->type);
				error = 1;
			}

			/* File Extensions Checks
			 * 
			 * Checks the following
			 *
			 * 1.  Tests the integrity of the string list
			 * 2.  Checks for duplicate extensions
			 * 3.  Makes sure that all extensions are either lower case chars or numbers
			 */
			if (!dev->file_extensions)
			{
				printf("%s: device type '%s' has null file extensions\n", drivers[i]->name, device_typename(dev->type));
				error = 1;
			}
			else
			{
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
			}

			/* enforce certain rules for certain device types */
			switch(dev->type)
			{
				case IO_QUICKLOAD:
				case IO_SNAPSHOT:
					if (dev->count != 1)
					{
						printf("%s: there can only be one instance of devices of type '%s'\n", drivers[i]->name, device_typename(dev->type));
						error = 1;
					}
					/* fallthrough */

				case IO_CARTSLOT:
					if (!dev->readable || dev->writeable || dev->creatable)
					{
						printf("%s: devices of type '%s' has invalid open modes\n", drivers[i]->name, device_typename(dev->type));
						error = 1;
					}
					break;
					
				default:
					break;
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);

		/* make sure that our input system likes this driver */
		if (inputx_validitycheck(drivers[i]))
			error = 1;

		end_resource_tracking();
	}

	if (inputx_validitycheck(NULL))
		error = 1;
	if (device_valididtychecks())
		error = 1;
#ifdef WIN32
	{
		extern int win_mess_validitychecks(void);
		if (win_mess_validitychecks())
			error = 1;
	}
#endif /* WIN32 */
	return error;
}
