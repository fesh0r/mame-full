/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include "driver.h"
#include "config.h"
#include "includes/flopdrv.h"
#include "utils.h"
#include "state.h"
#include "image.h"

extern struct GameOptions options;
extern const struct Devices devices[];

/* Globals */
const char *mess_path;
UINT32 mess_ram_size;
UINT8 *mess_ram;

int DECL_SPEC mess_printf(char *fmt, ...)
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

/*
 * Does the system support cassette (for tapecontrol)
 * TRUE, FALSE return
 */
int system_supports_cassette_device (void)
{
	return device_find(Machine->gamedrv, IO_CASSETTE) ? TRUE : FALSE;
}

int device_count(int type)
{
	const struct IODevice *dev;
	dev = device_find(Machine->gamedrv, type);
	return dev ? dev->count : 0;
}

/*
 * Return a name for the device type (to be used for UI functions)
 */
const char *device_typename(int type)
{
	if (type < IO_COUNT)
		return devices[type].name;
	return "UNKNOWN";
}

const char *device_brieftypename(int type)
{
	if (type < IO_COUNT)
		return devices[type].shortname;
	return "UNKNOWN";
}

/* Return a name for a device of type 'type' with id 'id' */
const char *device_typename_id(int type, int id)
{
	static char typename_id[40][31+1];
	static int which = 0;
	if (type < IO_COUNT)
	{
		which = (which + 1) % 40;
		/* for the average user counting starts at #1 ;-) */
		sprintf(typename_id[which], "%s #%d", devices[type].name, id+1);
		return typename_id[which];
	}
	return "UNKNOWN";
}

/*
 * Return the 'num'th file extension for a device of type 'type',
 * NULL if no file extensions of that type are available.
 */
const char *device_file_extension(int type, int extnum)
{
	const struct IODevice *dev;
	const char *ext;

	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		if( type == dev->type )
		{
			ext = dev->file_extensions;
			while( ext && *ext && extnum-- > 0 )
				ext = ext + strlen(ext) + 1;
			if( ext && !*ext )
				ext = NULL;
			return ext;
		}
	}
	return NULL;
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

		if (type < IO_COUNT)
		{
			/* Do we have too many devices? */
			if (images->count[type] >= MAX_DEV_INSTANCES)
			{
				mess_printf(" Too many devices of type %d\n", type);
				return 1;
			}

			/* Add a filename to the arrays of names */
			if (options.image_files[i].name)
				images->names[type][images->count[type]] = options.image_files[i].name;
			images->count[type]++;
		}
		else
		{
			mess_printf(" Invalid Device type %d for %s\n", type, options.image_files[i].name);
			return 1;
		}
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

#if 0
extern void cpu_setbank_fromram(int bank, UINT32 ramposition, mem_read_handler rhandler, mem_write_handler whandler)
{
	assert(mess_ram_size > 0);
	assert(mess_ram);
	assert((rhandler && whandler) || (!rhandler && !whandler));

	if (ramposition >= mess_ram_size && rhandler)
	{
		memory_set_bankhandler_r(bank, MRA_NOP);
		memory_set_bankhandler_w(bank, MWA_NOP);
	}
	else
	{
		if (rhandler)
		{
			/* this is only necessary if not mirroring */
			memory_set_bankhandler_r(bank, rhandler);
			memory_set_bankhandler_w(bank, whandler);
		}
		else
		{
			/* NULL handlers imply mirroring */
			ramposition %= mess_ram_size;
		}
		cpu_setbank(bank, &mess_ram[ramposition]);
	}
}
#endif

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
		memset(mess_ram, 0xcd, mess_ram_size);

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
int init_devices(const struct GameDriver *gamedrv)
{
	const struct IODevice *dev;
	int i,id;
	struct distributed_images images;

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

	logerror("Initialising Devices...\n");

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (supported_device(Machine->gamedrv, options.image_files[i].type)==FALSE)
		{
			mess_printf(" ERROR: Device [%s] is not supported by this system\n",device_typename(options.image_files[i].type));
			return 1;
		}
	}

	/* Ok! All devices are supported.  Now distribute them to the appropriate device..... */
	memset(&images, 0, sizeof(images));
	if (distribute_images(&images) == 1)
		return 1;

	/* Initialise all floppy drives here if the device is Setting can be overriden by the drivers and UI */
	floppy_drives_init();

	/* Initialize RAM code */
	if (ram_init(gamedrv))
		return 1;

	/* Initialize --all-- devices */
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
		{
			int result;
			mess_printf("Initialising %s device #%d\n",device_typename(dev->type), id + 1);

			/********************************************************************
			 * CALL INITIALISE DEVICE
			 ********************************************************************/
			result = image_load(dev->type, id, images.names[dev->type][id]);

			if (result != INIT_PASS)
			{
				mess_printf("Driver Reports Initialisation [for %s device] failed\n",device_typename(dev->type));
				mess_printf("Ensure image is valid and exists and (if needed) can be created\n");
				mess_printf("Also remember that some systems cannot boot without a valid image!\n");
				return 1;
			}
		}
	}

	mess_printf("Device Initialision Complete!\n");
	images_is_running = 1;
	return 0;
}

/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void exit_devices(void)
{
	/* shutdown all devices */
	image_unload_all();

	/* KT: clean up */
	floppy_drives_exit();

	images_is_running = 0;
}



int displayimageinfo(struct mame_bitmap *bitmap, int selected)
{
	char buf[2048], *dst = buf;
	int type, id, sel = selected - 1;

	dst += sprintf(dst,"%s\n\n",Machine->gamedrv->description);

	if (options.ram)
	{
		char buf2[RAM_STRING_BUFLEN];
		dst += sprintf(dst, "RAM: %s\n\n", ram_string(buf2, options.ram));
	}

	for (type = 0; type < IO_COUNT; type++)
	{
		for( id = 0; id < device_count(type); id++ )
		{
			const char *name = image_filename(type,id);
			if( name )
			{
				const char *filename;
				const char *base_filename;
				const char *info;
				char *base_filename_noextension;

				filename = image_filename(type, id);
				base_filename = osd_basename((char *) filename);
				base_filename_noextension = osd_strip_extension(base_filename);

				/* display device type and filename */
				dst += sprintf(dst,"%s: %s\n", device_typename_id(type,id), base_filename);

				/* display long filename, if present and doesn't correspond to name */
				info = image_longname(type,id);
				if (info && (!base_filename_noextension || strcmpi(info, base_filename_noextension)))
					dst += sprintf(dst,"%s\n", info);

				/* display manufacturer, if available */
				info = image_manufacturer(type,id);
				if (info)
				{
					dst += sprintf(dst,"%s", info);
					info = stripspace(image_year(type,id));
					if (info && *info)
						dst += sprintf(dst,", %s", info);
					dst += sprintf(dst,"\n");
				}

				/* display playable information, if available */
				info = image_playable(type,id);
				if (info)
					dst += sprintf(dst,"%s\n", info);

// why is extrainfo printed? only MSX and NES use it that i know of ... Cowering
//				info = device_extrainfo(type,id);
//				if( info )
//					dst += sprintf(dst,"%s\n", info);

				if (base_filename_noextension)
					free(base_filename_noextension);
			}
			else
			{
				dst += sprintf(dst,"%s: ---\n", device_typename_id(type,id));
			}
		}
	}

	if (sel == -1)
	{
		/* startup info, print MAME version and ask for any key */

		strcat(buf,"\n\tPress any key to Begin");
		ui_drawbox(bitmap,0,0,Machine->uiwidth,Machine->uiheight);
		ui_displaymessagewindow(bitmap, buf);

		sel = 0;
		if (input_ui_posted() ||
			code_read_async() != KEYCODE_NONE ||
			code_read_async() != JOYCODE_NONE)
			sel = -1;
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t\x1a Return to Main Menu \x1b");

		ui_displaymessagewindow(bitmap,buf);

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		schedule_full_refresh();
	}

	return sel + 1;
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
		"Multiple Emulation Super System - Copyright (C) 1997-2002 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2002 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	showmessdisclaimer();
	mess_printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"        MESS -showusage   to see usage instructions\n"
		"See mess.txt for help, readme.txt for options.\n");

}

static char *battery_nvramfilename(const char *filename)
{
	return osd_strip_extension(osd_basename((char *) filename));
}

/* load battery backed nvram from a driver subdir. in the nvram dir. */
int battery_load(const char *filename, void *buffer, int length)
{
	void *f;
	int bytes_read = 0;
	int result = FALSE;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(filename);
		if (nvram_filename)
		{
			f = osd_fopen(Machine->gamedrv->name, nvram_filename, OSD_FILETYPE_NVRAM, 0);
			if (f)
			{
				bytes_read = osd_fread(f, buffer, length);
				osd_fclose(f);
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
int battery_save(const char *filename, void *buffer, int length)
{
	void *f;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(filename);
		if (nvram_filename)
		{
			f = osd_fopen(Machine->gamedrv->name, nvram_filename, OSD_FILETYPE_NVRAM, 1);
			if (f)
			{
				osd_fwrite(f, buffer, length);
				osd_fclose(f);
				return TRUE;
			}
			free(nvram_filename);
		}
	}
	return FALSE;
}

void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count)
{
	while(color_count--)
	{
		palette_set_color(color_base++, colors[0], colors[1], colors[2]);
		colors += 3;
	}
}

#ifdef MAME_DEBUG

int messvaliditychecks(void)
{
	int i;
	int error = 0;
	const struct RomModule *region, *rom;

	/* Check the device struct array */
	i=0;
	while (devices[i].id != IO_COUNT)
	{
		if (devices[i].id != i)
		{
			mess_printf("MESS Validity Error - Device struct array order mismatch\n");
			error = 1;
		}
		i++;
	}
	if (i < IO_COUNT)
	{
		mess_printf("MESS Validity Error - Device struct entry missing\n");
		error = 1;
	}

	/* MESS specific driver validity checks */
	for(i = 0; drivers[i]; i++)
	{
		/* check device array */
	    const struct IODevice *dev;
		for(dev = device_first(drivers[i]); dev; dev = device_next(drivers[i], dev))
		{
			assert(dev->type < IO_COUNT);
		}

		/* this detects some inconsistencies in the ROM structures */
		for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
		{
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				char name[100];
				sprintf(name,ROM_GETNAME(rom));
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);
	}

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
		if (init_devices(gamedrv) != 0)
			error = TESTERROR_INITDEVICESFAILED;
		else
			exit_devices();
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
