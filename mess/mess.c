/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include "config.h"
#include "includes/flopdrv.h"

extern struct GameOptions options;

/* CRC database file for this driver, supplied by the OS specific code */
extern const char *crcfile;
extern const char *pcrcfile;


/* Globals */
int mess_keep_going;

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

static struct image_info *images[IO_COUNT] = {NULL,};
static int count[IO_COUNT] = {0,};
static const char *typename[IO_COUNT] = {
	"NONE",
	"Cartridge ",
	"Floppydisk",
	"Harddisk  ",
	"Cassette  ",
	"Printer   ",
	"Serial    ",
	"Snapshot  ",
	"Quickload "
};

static const char *brieftypename[IO_COUNT] = {
	"NONE",
	"Cart",
	"Flop",
	"Hard",
	"Cass",
	"Prin",
	"Serl",
	"Dump",
	"Quik"
};

static char *mess_alpha = "";

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

static void free_image_info(struct image_info *img)
{
	if( !img )
		return;
	if( img->longname )
		free(img->longname);
	img->longname = NULL;
	if( img->manufacturer )
		free(img->manufacturer );
	img->manufacturer = NULL;
	if( img->year )
		free(img->year );
	img->year = NULL;
	if( img->playable )
		free(img->playable);
	img->playable = NULL;
	if( img->extrainfo )
		free(img->extrainfo);
	img->extrainfo = NULL;
}

int DECL_SPEC mess_printf(char *fmt, ...)
{
	va_list arg;
	int length = 0;

	if( !options.gui_host )
	{
		va_start(arg,fmt);
		length = vprintf(fmt, arg);
		va_end(arg);
	}

	return length;
}

static int read_crc_config (const char *, struct image_info *, const char*);

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

	while (1)
	{
		extern struct GameDriver driver_0;
		const char *ext;
		char *p;
		int l;

		sysname = Machine->gamedrv->name;
		logerror("image_fopen: trying %s for system %s\n", img->name, sysname);
		file = osd_fopen(sysname, img->name, filetype, read_or_write);
		/* file found, break out */
		if( file )
			break;

		if( Machine->gamedrv->clone_of &&
			Machine->gamedrv->clone_of != &driver_0 )
		{	/* R Nabet : Shouldn't this be moved to osd code ??? Mac osd code does such a retry
			whenever it makes sense, and I think this is the correct way. */
			sysname = Machine->gamedrv->clone_of->name;
			logerror("image_fopen: now trying %s for system %s\n", img->name, sysname);
			file = osd_fopen(sysname, img->name, filetype, read_or_write);
		}
		if( file )
			break;

		if (options.append_no_file_extension)
		{	/* do not try file extension substitutions */
			break;
		}

		if (! ((read_or_write == 0) || (read_or_write == 2) /*|| (read_or_write == 3)*/))
		{	/* Other modes do not require that the file exists, so we must not triffle with extensions */
			/* If fopen ever failed, it was for a completely different reason... */
			break;
		}

		ext = device_file_extension(type,extnum);
		extnum++;

		/* no (more) extensions, break out */
		if( !ext )
			break;

		l = strlen(img->name);
		p = img->name + original_len;	/* overwrite any extension we may have appended so far */
		/* does the file name already have an extension appended ? */
		if( p )
		{
			++p; /* skip the dot */
			/* new extension won't fit? */
			if( strlen(p) < strlen(ext) )
			{
				img->name = realloc(img->name, l - strlen(p) + strlen(ext) + 1);
				if( !img->name )
				{
					logerror("image_fopen: realloc failed.. damn it!\n");
					return NULL;
				}
			}
			strcpy(p, ext);
		}
		else
		{
			img->name = realloc(img->name, l + 1 + strlen(ext) + 1);
			if( !img->name )
			{
				logerror("image_fopen: realloc failed.. damn it!\n");
				return NULL;
			}
			sprintf(img->name + l, ".%s", ext);
		}
	}

	if ((! file) && (strlen(img->name) != original_len))
	{	/* restore original file name, so that we do not mess everything up. (Ha !) */
		if (img->name)	/* safety check */
			img->name = realloc(img->name, original_len);
	}

	if( file )
	{
		void *config;
		const struct IODevice *pc_dev = Machine->gamedrv->dev;

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
		free_image_info(img);

		if (read_crc_config (crcfile, img, sysname) && Machine->gamedrv->clone_of->name)
			read_crc_config (pcrcfile, img, Machine->gamedrv->clone_of->name);

		config = config_open(crcfile);
	}

	return file;
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
 * Return a name for the device type (to be used for UI functions)
 */
const char *device_typename(int type)
{
	if (type < IO_COUNT)
		return typename[type];
	return "UNKNOWN";
}

const char *briefdevice_typename(int type)
{
	if (type < IO_COUNT)
		return brieftypename[type];
	return "UNKNOWN";
}

const char *device_brieftypename(int type)
{
	if (type < IO_COUNT)
		return brieftypename[type];
	return "UNKNOWN";
}

/* Return a name for a device of type 'type' with id 'id' */
const char *device_typename_id(int type, int id)
{
	static char typename_id[40][31+1];
	static int which = 0;
	if (type < IO_COUNT)
	{
		which = ++which % 40;
		/* for the average user counting starts at #1 ;-) */
		sprintf(typename_id[which], "%s #%d", typename[type], id+1);
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

/*
 * Copy the image names from options.image_files[] to
 * the array of filenames we keep here, depending on the
 * type identifier of each image.
 */
int get_filenames(void)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	int i;

	for( i = 0; i < options.image_count; i++ )
	{
		int type = options.image_files[i].type;

		if (type < IO_COUNT)
		{
			/* Add a filename to the arrays of names */
			if( images[type] )
				images[type] = realloc(images[type],(count[type]+1)*sizeof(struct image_info));
			else
				images[type] = malloc(sizeof(struct image_info));
			if( !images[type] )
				return 1;
			memset(&images[type][count[type]], 0, sizeof(struct image_info));
			if( options.image_files[i].name )
			{
				images[type][count[type]].name = dupe(options.image_files[i].name);
				if( !images[type][count[type]].name )
					return 1;
			}
			logerror("%s #%d: %s\n", typename[type], count[type]+1, images[type][count[type]].name);
			count[type]++;
		}
		else
		{
			logerror("Invalid IO_ type %d for %s\n", type, options.image_files[i].name);
			return 1;
		}
	}

	/* Does the driver have any IODevices defined? */
	if( dev )
	{
		while( dev->count )
		{
			int type = dev->type;
			while( count[type] < dev->count )
			{
				/* Add an empty slot name the arrays of names */
				if( images[type] )
					images[type] = realloc(images[type],(count[type]+1)*sizeof(struct image_info));
				else
					images[type] = malloc(sizeof(struct image_info));
				if( !images[type] )
					return 1;
				memset(&images[type][count[type]], 0, sizeof(struct image_info));
				count[type]++;
			}
			dev++;
		}
	}

	/* everything was fine */
	return 0;
}

/*
 * Call the init() functions for all devices of a driver
 * with all user specified image names.
 */
int init_devices(const void *game)
{
	const struct GameDriver *gamedrv = game;
	const struct IODevice *dev = gamedrv->dev;
	int id;

	/* KT - added floppy drives init here. Drivers can override settings, and
	settings can change in the UI */
	floppy_drives_init();

	/* initialize all devices */
	while( dev->count )
	{

		/* try and check for valid image and compute 'partial' CRC
		   for imageinfo if possible */
		if( dev->id )
		{
			for( id = 0; id < dev->count; id++ )
			{
				int result;

				/* initialize */
				logerror("%s id (%s)\n", device_typename_id(dev->type,id), device_filename(dev->type,id) ? device_filename(dev->type,id) : "NULL");
				result = (*dev->id)(id);
				logerror("%s id returns %d\n", device_typename_id(dev->type,id), result);

				if( result != ID_OK && device_filename(dev->type,id) )
				{
					mess_printf("%s id failed (%s)\n", device_typename_id(dev->type,id), device_filename(dev->type,id) );
/* HJB: I think we can't abort if a device->id function fails _yet_, because
 * we first would have to clean up every driver to use the correct return values.
 * device->init will fail if a file really can't be loaded.
 */
/*					return 1; */
				}
			}
		}
		else
		{
			logerror("%s does not support id!\n", device_typename(dev->type));
		}

		/* if this device supports initialize (it should!) */
		if( dev->init )
		{
			/* all instances */
			for( id = 0; id < dev->count; id++ )
			{
				int result;

				/* initialize */
				logerror("%s init (%s)\n", device_typename_id(dev->type,id), device_filename(dev->type,id) ? device_filename(dev->type,id) : "NULL");
				result = (*dev->init)(id);
				logerror("%s init returns %d\n", device_typename_id(dev->type,id), result);

				if( result != INIT_OK && device_filename(dev->type,id) )
				{
					mess_printf("%s init failed (%s)\n", device_typename_id(dev->type,id), device_filename(dev->type,id) );
					return 1;
				}

				/* init succeeded */
				/* if floppy, perform common init */
				if ((dev->type == IO_FLOPPY) && (device_filename(dev->type, id)))
				{
					floppy_device_common_init(id);
				}

			}
		}
		else
		{
			logerror("%s does not support init!\n", device_typename(dev->type));
		}
		dev++;
	}
	return 0;
}

/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void exit_devices(void)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	int type, id;

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
				images[type][id].name = NULL;
			}
			free(images[type]);
		}
		images[type] = NULL;
		count[type] = 0;
	}

	/* KT: clean up */
	floppy_drives_exit();
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
		if( img->name )
			free(img->name);
		img->name = NULL;
		img->length = 0;
		img->crc = 0;
		free_image_info(img);
		if( name )
		{
			img->name = dupe(name);
			if( !img->name )
				return 1;
		}

		if( dev->reset_depth == IO_RESET_CPU )
			machine_reset();
		else
		if( dev->reset_depth == IO_RESET_ALL )
		{
			mess_keep_going = 1;

		}

		result = (*dev->init)(id);
		if( result != INIT_OK && name )
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

int device_open(int type, int id, int mode, void *args)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->open )
			return (*dev->open)(id,mode,args);

		dev++;
	}
	return 1;
}

void device_close(int type, int id)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->close )
		{
			(*dev->close)(id);
			return;
		}
		dev++;
	}
}

int device_seek(int type, int id, int offset, int whence)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->seek )
			return (*dev->seek)(id,offset,whence);
		dev++;
	}
	return 0;
}

int device_tell(int type, int id)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->tell )
			return (*dev->tell)(id);
		dev++;
	}
	return 0;
}

int device_status(int type, int id, int newstatus)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->status )
			return (*dev->status)(id,newstatus);
		dev++;
	}
	return 0;
}

int device_input(int type, int id)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->input )
			return (*dev->input)(id);
		dev++;
	}
	return 0;
}

void device_output(int type, int id, int data)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->output )
		{
			(*dev->output)(id,data);
			return;
		}
		dev++;
	}
}

int device_input_chunk(int type, int id, void *dst, int chunks)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->input_chunk )
			return (*dev->input_chunk)(id,dst,chunks);
		dev++;
	}
	return 1;
}

void device_output_chunk(int type, int id, void *src, int chunks)
{
	const struct IODevice *dev = Machine->gamedrv->dev;
	while( dev && dev->count )
	{
		if( type == dev->type && dev->output )
		{
			(*dev->output_chunk)(id,src,chunks);
			return;
		}
		dev++;
	}
}






int displayimageinfo(struct osd_bitmap *bitmap, int selected)
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
				dst += sprintf(dst,"%s: %s\n", device_typename_id(type,id), device_filename(type,id));
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
		"so a file dump of the ROMs, cartridges, discs, and cassettes which run on that\n"
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
		"M.E.S.S. v%s %s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2000 by the MESS Team\n"
		"M.E.S.S. is based on the excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2000 by Nicola Salmoria and the MAME Team\n\n",
		build_version, mess_alpha);
	showmessdisclaimer();
	mess_printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listfull    for a full list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"See mess.txt for help, readme.txt for options.\n");

}

