/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include "config.h"
#include "includes/flopdrv.h"

extern struct GameOptions options;
extern const struct Devices devices[];

/* CRC database file for this driver, supplied by the OS specific code */
extern const char *crcfile;
extern const char *pcrcfile;


/* Globals */
int mess_keep_going;
char *renamed_image;

struct image_info {
	char *name;
	UINT32 crc;
	UINT32 length;
	char *longname;
	char *manufacturer;
	char *year;
	char *playable;
	char *extrainfo;
};

#define MAX_INSTANCES 5
static struct image_info images[IO_COUNT][MAX_INSTANCES];
static int count[IO_COUNT];


static char* dupe(const char *src)
{
	if( src )
	{
		char *dst = malloc(strlen(src) + 1);
		if( dst )
			strcpy(dst,src);
		return dst;
	}
	return NULL;
}


static char* stripspace(const char *src)
{
	static char buff[512];
	if( src )
	{
		char *dst;
		while( *src && isspace(*src) )
			src++;
		strcpy(buff, src);
		dst = buff + strlen(buff);
		while( dst >= buff && isspace(*--dst) )
			*dst = '\0';
		return buff;
	}
	return NULL;
}

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

static int read_crc_config (const char *file, struct image_info *img, const char* sysname)
{
	int retval;
	void *config = config_open (file);

	retval = 1;
	if( config )
	{
		char line[1024];
		char crc[9+1];

		sprintf(crc, "%08x", img->crc);
		config_load_string(config,sysname,0,crc,line,sizeof(line));
		if( line[0] )
		{
			logerror("found CRC %s= %s\n", crc, line);
			img->longname = dupe(stripspace(strtok(line, "|")));
			img->manufacturer = dupe(stripspace(strtok(NULL, "|")));
			img->year = dupe(stripspace(strtok(NULL, "|")));
			img->playable = dupe(stripspace(strtok(NULL, "|")));
			img->extrainfo = dupe(stripspace(strtok(NULL, "|")));
			retval = 0;
		}
		config_close(config);
	}
	return retval;
}


void *image_fopen(int type, int id, int filetype, int read_or_write)
{
	struct image_info *img = &images[type][id];
	const char *sysname;
	void *file;
	int extnum;
	int original_len;

	if( type >= IO_COUNT )
	{
		logerror("image_fopen: type out of range (%d)\n", type);
		return NULL;
	}

	if( id >= count[type] )
	{
		logerror("image_fopen: id out of range (%d)\n", id);
		return NULL;
	}

	if( img->name == NULL )
		return NULL;

	/* try the supported extensions */
	extnum = 0;

	/* remember original file name */
	original_len = strlen(img->name);

	{
		extern struct GameDriver driver_0;

		sysname = Machine->gamedrv->name;
		logerror("image_fopen: trying %s for system %s\n", img->name, sysname);
		file = osd_fopen(sysname, img->name, filetype, read_or_write);
		/* file found, break out */
		if (!file)
		{
			if( Machine->gamedrv->clone_of &&
				Machine->gamedrv->clone_of != &driver_0 )
			{	/* R Nabet : Shouldn't this be moved to osd code ??? Mac osd code does such a retry
				whenever it makes sense, and I think this is the correct way. */
				sysname = Machine->gamedrv->clone_of->name;
				logerror("image_fopen: now trying %s for system %s\n", img->name, sysname);
				file = osd_fopen(sysname, img->name, filetype, read_or_write);
			}
		}
	}

	if (file)
	{
		void *config;
		const struct IODevice *pc_dev = Machine->gamedrv->dev;

		/* did osd_fopen() rename the image? (yes, I know this is a hack) */
		if (renamed_image)
		{
			free(img->name);
			img->name = renamed_image;
			renamed_image = NULL;
		}

		logerror("image_fopen: found image %s for system %s\n", img->name, sysname);
		img->length = osd_fsize(file);
/* Cowering, partial crcs for NES/A7800/others */
		img->crc = 0;
		while( pc_dev && pc_dev->count && !img->crc)
		{
			logerror("partialcrc() -> %08lx\n",pc_dev->partialcrc);
			if( type == pc_dev->type && pc_dev->partialcrc )
			{
				unsigned char *pc_buf = (unsigned char *)malloc(img->length);
				if( pc_buf )
				{
					osd_fseek(file,0,SEEK_SET);
					osd_fread(file,pc_buf,img->length);
					osd_fseek(file,0,SEEK_SET);
					logerror("Calling partialcrc()\n");
					img->crc = (*pc_dev->partialcrc)(pc_buf,img->length);
					free(pc_buf);
				}
				else
				{
					logerror("failed to malloc(%d)\n", img->length);
				}
			}
			pc_dev++;
		}

		if (!img->crc) img->crc = osd_fcrc(file);
		if( img->crc == 0 && img->length < 0x100000 )
		{
			logerror("image_fopen: calling osd_fchecksum() for %d bytes\n", img->length);
			osd_fchecksum(sysname, img->name, &img->length, &img->crc);
			logerror("image_fopen: CRC is %08x\n", img->crc);
		}

		if (read_crc_config (crcfile, img, sysname) && Machine->gamedrv->clone_of->name)
			read_crc_config (pcrcfile, img, Machine->gamedrv->clone_of->name);

		config = config_open(crcfile);
	}

	return file;
}


/* common init for all IO_FLOPPY devices */
static void	floppy_device_common_init(int id)
{
	logerror("floppy device common init: id: %02x\n",id);
	/* disk inserted */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_INSERTED, 1);
	/* drive connected */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_CONNECTED, 1);
}

/* common exit for all IO_FLOPPY devices */
static void floppy_device_common_exit(int id)
{
	logerror("floppy device common exit: id: %02x\n",id);
	/* disk removed */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_DISK_INSERTED, 0);
	/* drive disconnected */
	floppy_drive_set_flag_state(id, FLOPPY_DRIVE_CONNECTED, 0);
}


/*
 * Does the system support cassette (for tapecontrol)
 * TRUE, FALSE return
 */
int system_supports_cassette_device (void)
{
	const struct IODevice *dev = Machine->gamedrv->dev;

	/* Cycle through all devices for this system */
	while(dev->type != IO_END)
	{
		if (dev->type == IO_CASSETTE)
			return TRUE;
		dev++;
	}

	return FALSE;
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
 * Return the number of filenames for a device of type 'type'.
 */
int device_count(int type)
{
	if (type >= IO_COUNT)
		return 0;
	return count[type];
}

/*
 * Return the 'id'th filename for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_filename(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].name;
	return NULL;
}

/*
 * Return the 'num'th file extension for a device of type 'type',
 * NULL if no file extensions of that type are available.
 */
const char *device_file_extension(int type, int extnum)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	const char *ext;
	if (type >= IO_COUNT)
		return NULL;
	while( dev->count )
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
		dev++;
	}
	return NULL;
}

/*
 * Return the 'id'th crc for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
unsigned int device_crc(int type, int id)
{
	if (type >= IO_COUNT)
		return 0;
	if (id < count[type])
		return images[type][id].crc;
	return 0;
}

/*
 * Set the 'id'th crc for a device of type 'type',
 * this is to be used if only a 'partial crc' shall be used.
 */
void device_set_crc(int type, int id, UINT32 new_crc)
{
	if (type >= IO_COUNT)
	{
		logerror("device_set_crc: type out of bounds (%d)\n", type);
		return;
	}
	if (id < count[type])
	{
		images[type][id].crc = new_crc;
		logerror("device_set_crc: new_crc %08x\n", new_crc);
	}
	else
		logerror("device_set_crc: id out of bounds (%d)\n", id);
}

/*
 * Return the 'id'th length for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
unsigned int device_length(int type, int id)
{
	if (type >= IO_COUNT)
		return 0;
	if (id < count[type])
		return images[type][id].length;
	return 0;
}

/*
 * Return the 'id'th long name for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_longname(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].longname;
	return NULL;
}

/*
 * Return the 'id'th manufacturer name for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_manufacturer(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].manufacturer;
	return NULL;
}

/*
 * Return the 'id'th release year for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_year(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].year;
	return NULL;
}

/*
 * Return the 'id'th playable info for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_playable(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].playable;
	return NULL;
}


/*
 * Return the 'id'th extrainfo info for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_extrainfo(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
	if (id < count[type])
		return images[type][id].extrainfo;
	return NULL;
}


/*****************************************************************************
 *  --Distribute images to their respective Devices--
 *  Copy the Images specified at the CLI from options.image_files[] to the
 *  array of filenames we keep here, depending on the Device type identifier
 *  of each image.  Multiple instances of the same device are allowed
 *  RETURNS 0 on success, 1 if failed
 ****************************************************************************/
static int distribute_images(void)
{
	int i,j;

	logerror("Distributing Images to Devices...\n");
	/* Set names to NULL */
	for (i=0;i<IO_COUNT;i++)
		for (j=0;j<MAX_INSTANCES;j++)
			images[i][j].name = NULL;

	for( i = 0; i < options.image_count; i++ )
	{
		int type = options.image_files[i].type;

		if (type < IO_COUNT)
		{
			/* Do we have too many devices? */
			if( count[type] >= MAX_INSTANCES )
			{
				mess_printf(" Too many devices of type %d\n", type);
				return 1;
			}

			/* Add a filename to the arrays of names */
			if( options.image_files[i].name )
			{
				images[type][count[type]].name = dupe(options.image_files[i].name);
				if( !images[type][count[type]].name )
				{
					mess_printf(" ERROR - dupe() failed\n");
					return 1;
				}
			}
			count[type]++;
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
static int supported_device(const struct IODevice *dev, int type)
{
	while(dev->type!=IO_END)
	{
		if(dev->type==type)
			return TRUE;	/* Return OK */
		dev++;
	}
	return FALSE;
}

/*****************************************************************************
 *  --Initialise Devices--
 *  Call the init() functions for all devices of a driver
 *  ith all user specified image names.
 ****************************************************************************/
int init_devices(const void *game)
{
	const struct GameDriver *gamedrv = game;
	const struct IODevice *dev = gamedrv->dev;
	int i,id;

	logerror("Initialising Devices...\n");

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (supported_device(dev, options.image_files[i].type)==FALSE)
		{
			mess_printf(" ERROR: Device [%s] is not supported by this system\n",device_typename(options.image_files[i].type));
			return 1;
		}
	}

	/* Ok! All devices are supported.  Now distribute them to the appropriate device..... */
	if (distribute_images() == 1)
		return 1;

	/* Initialise all floppy drives here if the device is Setting can be overriden by the drivers and UI */
	floppy_drives_init();

	/* initialize --all-- devices */
	while( dev->count )
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
		{
			mess_printf("Initialising %s device #%d\n",device_typename(dev->type), id + 1);
			/********************************************************************
			 * CALL INITIALISE DEVICE
			 ********************************************************************/
			/* if this device supports initialize (it should!) */
			if( dev->init )
			{
				int result;

				/* initialize */
				result = (*dev->init)(id);

				if( result != INIT_PASS)
				{
					mess_printf("Driver Reports Initialisation [for %s device] failed\n",device_typename(dev->type));
					mess_printf("Ensure image is valid and exists and (if needed) can be created\n");
					mess_printf("Also remember that some systems cannot boot without a valid image!\n");
					return 1;
				}

				/* init succeeded */
				/* if floppy, perform common init */
				if ((dev->type == IO_FLOPPY) && (device_filename(dev->type, id)))
				{
					floppy_device_common_init(id);
				}
			}
			else
			{
				mess_printf(" %s does not support init!\n", device_typename(dev->type));
			}
		}
		dev++;
	}
	mess_printf("Device Initialision Complete!\n");
	return 0;
}

/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void exit_devices(void)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	int id;
	int type;

	/* shutdown all devices */
	while( dev->count )
	{
		/* all instances */
		if( dev->exit)
		{
			/* shutdown */
			for( id = 0; id < device_count(dev->type); id++ )
				(*dev->exit)(id);

		}
		else
		{
			logerror("%s does not support exit!\n", device_typename(dev->type));
		}

		/* The following is always executed for a IO_FLOPPY exit */
		/* KT: if a image is removed:
			1. Disconnect drive
			2. Remove disk from drive */
		/* This is done here, so if a device doesn't support exit, the status
		will still be correct */
		if (dev->type == IO_FLOPPY)
		{
			for (id = 0; id< device_count(dev->type); id++)
			{
				floppy_device_common_exit(id);
			}
		}

		dev++;
	}

	for( type = 0; type < IO_COUNT; type++ )
	{
		if( images[type] )
		{
			for( id = 0; id < device_count(dev->type); id++ )
			{
				if( images[type][id].name )
					free(images[type][id].name);
				if( images[type][id].longname )
					free(images[type][id].longname);
				if( images[type][id].manufacturer )
					free(images[type][id].manufacturer);
				if( images[type][id].year )
					free(images[type][id].year);
				if( images[type][id].playable )
					free(images[type][id].playable);
				if( images[type][id].extrainfo )
					free(images[type][id].extrainfo);
			}
		}
		count[type] = 0;
	}

	/* KT: clean up */
	floppy_drives_exit();

#ifdef MAME_DEBUG
	for( type = 0; type < IO_COUNT; type++ )
	{
		if (count[type])
			mess_printf("OOPS!!!  Appears not all images free!\n");

	}
#endif

}

/*
 * Change the associated image filename for a device.
 * Returns 0 if successful.
 */
int device_filename_change(int type, int id, const char *name)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	struct image_info *img = &images[type][id];

	if( type >= IO_COUNT )
		return 1;

	while( dev->count && dev->type != type )
		dev++;

	if( id >= dev->count )
		return 1;

	if( dev->exit )
		dev->exit(id);

	/* if floppy, perform common exit */
	if (dev->type == IO_FLOPPY)
	{
		floppy_device_common_exit(id);
	}

	if( dev->init )
	{
		int result;
		/*
		 * set the new filename and reset all addition info, it will
		 * be inserted by osd_fopen() and the crc handling
		 */
		img->name = NULL;
		img->length = 0;
		img->crc = 0;
		if( name )
		{
			img->name = dupe(name);
			/* Check the name */
			if( !img->name )
				return 1;
			/* check the count - if it equals id, add new! */
			if (device_count(type) == id)
				count[type]++;
		}

		if( dev->reset_depth == IO_RESET_CPU )
			machine_reset();
		else
		if( dev->reset_depth == IO_RESET_ALL )
		{
			mess_keep_going = 1;

		}

		result = (*dev->init)(id);
		if( result != INIT_PASS)
			return 1;

		/* init succeeded */
		/* if floppy, perform common init */
		if (dev->type == IO_FLOPPY)
		{
			floppy_device_common_init(id);
		}

	}
	return 0;
}

int displayimageinfo(struct mame_bitmap *bitmap, int selected)
{
	char buf[2048], *dst = buf;
	int type, id, sel = selected - 1;

	dst += sprintf(dst,"%s\n\n",Machine->gamedrv->description);

	for (type = 0; type < IO_COUNT; type++)
	{
		for( id = 0; id < device_count(type); id++ )
		{
			const char *name = device_filename(type,id);
			if( name )
			{
				const char *info;
				char *filename;

				filename = (char *) device_filename(type, id);

				dst += sprintf(dst,"%s: %s\n", device_typename_id(type,id), osd_basename(filename));
				info = device_longname(type,id);
				if( info )
					dst += sprintf(dst,"%s\n", info);
				info = device_manufacturer(type,id);
				if( info )
				{
					dst += sprintf(dst,"%s", info);
					info = stripspace(device_year(type,id));
					if( info && strlen(info))
						dst += sprintf(dst,", %s", info);
					dst += sprintf(dst,"\n");
				}
				info = device_playable(type,id);
				if( info )
					dst += sprintf(dst,"%s\n", info);
// why is extrainfo printed? only MSX and NES use it that i know of ... Cowering
//				info = device_extrainfo(type,id);
//				if( info )
//					dst += sprintf(dst,"%s\n", info);
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
		if (code_read_async() != KEYCODE_NONE ||
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
		"Multiple Emulation Super System - Copyright (C) 1997-2001 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2001 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	showmessdisclaimer();
	mess_printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"        MESS -showusage   to see usage instructions\n"
		"See mess.txt for help, readme.txt for options.\n");

}

#ifdef MAME_DEBUG

int messvaliditychecks(void)
{
	int i;
	int error = 0;

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
	return error;
}

void messtestdriver(const void *game)
{
	const struct GameDriver *gamedrv = game;
	Machine->gamedrv = gamedrv;
	Machine->drv = gamedrv->drv;
	memset(&Machine->memory_region, 0, sizeof(Machine->memory_region));
	if (readroms() != 0)
		return;
	if (init_devices(gamedrv) != 0)
		return;
	exit_devices();
}

#endif /* MAME_DEBUG */
