/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include <ctype.h>
#include "driver.h"
#include "mame.h"
#include "mess/mess.h"

extern struct GameOptions options;

static const char **cartslot_names = NULL;
static const char **floppy_names = NULL;
static const char **harddisk_names = NULL;
static const char **cassette_names = NULL;
static const char **printer_names = NULL;
static const char **serial_names = NULL;

static int cartslot_count = 0;
static int floppy_count = 0;
static int harddisk_count = 0;
static int cassette_count = 0;
static int printer_count = 0;
static int serial_count = 0;

static char *mess_alpha = "";

/* fronthlp functions */
int strwildcmp(const char *sp1, const char *sp2);

/* Add a filename from the GameOptions to the arrays of names */
#define ADD_DEVICE_FILE(dbgmsg,type,name)					\
	if (type##_names)										\
		type##_names = realloc(type##_names,(type##_count+1)*sizeof(char *));  \
	else													\
		type##_names = malloc(sizeof(char *));				\
	if (!type##_names)										\
		return 1;											\
	type##_names[type##_count] = name;						\
	if (errorlog)											\
		fprintf(errorlog, dbgmsg, type##_count, type##_names[type##_count]); \
	type##_count++

/* Zap the array of filenames and the count for a device type */
#define ZAP_DEVICE_FILES(type)								\
	if (type##_names) free(type##_names);					\
	type##_names = NULL;									\
	type##_count = 0;

/*
 * Return a name for the device type (to be used for UI functions)
 */
const char *device_typename(int type)
{
    switch( type )
    {
    case IO_CARTSLOT: return "Cartridge";
    case IO_FLOPPY:   return "Floppydisk";
    case IO_HARDDISK: return "Harddisk";
    case IO_CASSETTE: return "Cassette";
    case IO_PRINTER:  return "Printer";
    case IO_SERIAL:   return "Serial";
    }
    return "UNKNOWN";
}

/*
 * Return the id'th filename for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_filename(int type, int id)
{
    switch( type )
    {
    case IO_CARTSLOT:
		if (id < cartslot_count)
            return cartslot_names[id];
        return NULL;
    case IO_FLOPPY:
		if (id < floppy_count)
            return floppy_names[id];
        return NULL;
    case IO_HARDDISK:
		if (id < harddisk_count)
            return harddisk_names[id];
        return NULL;
    case IO_CASSETTE:
		if (id < cassette_count)
            return cassette_names[id];
        return NULL;
    case IO_PRINTER:
		if (id < printer_count)
            return printer_names[id];
        return NULL;
    case IO_SERIAL:
		if (id < serial_count)
            return serial_names[id];
        return NULL;
    }
    return NULL;
}

/*
 * Return the number of filenames for a device of type 'type'.
 */
int device_count(int type)
{
    switch( type )
    {
    case IO_CARTSLOT: return cartslot_count;
    case IO_FLOPPY:   return floppy_count;
    case IO_HARDDISK: return harddisk_count;
    case IO_CASSETTE: return cassette_count;
    case IO_PRINTER:  return printer_count;
    case IO_SERIAL:   return serial_count;
    }
    return 0;
}

/*
 * Copy the image names from options.image_files[] to
 * the array of filenames we keep here, depending on the
 * type identifier of each image.
 */
int get_filenames(void)
{
	int i;
	for( i = 0; i < options.image_count; i++ )
	{
		const char *name = options.image_files[i].name;
        int type = options.image_files[i].type;

		switch( type )
		{
		case IO_CARTSLOT:
			ADD_DEVICE_FILE("IO_CARTSLOT #%d %s\n", cartslot, name);
            break;
		case IO_FLOPPY:
			ADD_DEVICE_FILE("IO_FLOPPY   #%d %s\n", floppy, name);
			break;
		case IO_HARDDISK:
			ADD_DEVICE_FILE("IO_HARDDISK #%d %s\n", harddisk, name);
			break;
		case IO_CASSETTE:
			ADD_DEVICE_FILE("IO_CASSETTE #%d %s\n", cassette, name);
			break;
		case IO_PRINTER:
			ADD_DEVICE_FILE("IO_PRINTER  #%d %s\n", printer, name);
			break;
        case IO_SERIAL:
			ADD_DEVICE_FILE("IO_SERIAL   #%d %s\n", serial, name);
			break;
		default:
			if(errorlog)
				fprintf(errorlog, "Invalid IO_ type %d for %s\n", type, name);
			return 1;
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

    /* initialize all devices */
	while( dev->count )
	{
		/* if this device supports initialize (it should!) */
        if( dev->init )
		{
			/* all instances */
			for( id = 0; id < dev->count; id++ )
			{
				const char *filename = device_filename(dev->type,id);
				if( filename )
				{
					/* initialize */
					if( (*dev->init)(id,filename) != 0 )
					{
						printf("%s #%d init failed (%s).\n", device_typename(dev->type), id, filename);
						return 1;
					}
				}
			}
		}
		else
		{
			if (errorlog) fprintf(errorlog, "%s does not support init!\n", device_typename(dev->type));
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
    int id;

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
            if (errorlog) fprintf(errorlog, "%s does not support exit!\n", device_typename(dev->type));
        }
        dev++;
    }
	ZAP_DEVICE_FILES(cartslot)
	ZAP_DEVICE_FILES(floppy)
	ZAP_DEVICE_FILES(harddisk)
	ZAP_DEVICE_FILES(cassette)
	ZAP_DEVICE_FILES(printer)
	ZAP_DEVICE_FILES(serial)
}

void showmessdisclaimer(void)
{
	printf("MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		 "several computer and console systems. But hardware is useless without software\n"
		 "so an image of the ROMs, cartridges, discs, and cassettes which run on that\n"
		 "hardware is required. Such images, like any other commercial software, are\n"
		 "copyrighted material and it is therefore illegal to use them if you don't own\n"
		 "the original media from which the images are derived. Needless to say, these\n"
		 "images are not distributed together with MESS. Distribution of MESS together\n"
		 "with these images is a violation of copyright law and should be promptly\n"
		 "reported to the authors so that appropriate legal action can be taken.\n\n");
}

void showmessinfo(void)
{
	printf("M.E.S.S. v%s %s\n"
	       "Multiple Emulation Super System - Copyright (C) 1997-98 by the MESS Team\n"
		   "M.E.S.S. is based on the excellent M.A.M.E. Source code\n"
		   "Copyright (C) 1997-99 by Nicola Salmoria and the MAME Team\n\n",build_version,
                                                                        mess_alpha);
		showmessdisclaimer();
		printf("Usage:  MESS machine [image] [options]\n\n"
				"        MESS -list      for a brief list of supported systems\n"
				"        MESS -listfull  for a full list of supported systems\n"
				"See mess.txt for a complete list of options.\n");

}



/**********************************************************/
/* Functions called from MSDOS.C by MAME for running MESS */
/**********************************************************/

/*
 * Detect the type of image given in 'arg':
 * 1st: user specified type (after -rom, -floppy ect.)
 * 2nd: match extensions specified by the driver
 * default: add image to the list of names for IO_CARTSLOT
 */
static int detect_image_type(int game_index, int type, char *arg)
{
	const struct GameDriver *drv = drivers[game_index];
	char *ext;

	if( options.image_count >= MAX_IMAGES )
	{
		printf("Too many image names specified!\n");
		return 1;
	}

	if( type )
	{
		if (errorlog)
			fprintf(errorlog,"User specified %s for %s\n", device_typename(type), arg);
        /* the user specified a device type */
		options.image_files[options.image_count].type = type;
		options.image_files[options.image_count].name = strdup(arg);
		options.image_count++;
        return 0;
	}

	/* Look up the filename extension in the drivers device list */
    ext = strrchr(arg, '.');
    if( ext )
	{
		const struct IODevice *dev = drv->dev;

        ext++;
		while (dev->type != IO_END)
		{
			const char *dst = dev->file_extensions;
			/* scan supported extensions for this device */
			while (dst && *dst)
			{
				if (stricmp(dst,ext) == 0)
				{
					if (errorlog)
						fprintf(errorlog,"Extension match %s [%s] for %s\n", device_typename(dev->type), dst, arg);
                    options.image_files[options.image_count].type = dev->type;
					options.image_files[options.image_count].name = strdup(arg);
					options.image_count++;
                    return 0;
				}
				/* skip '\0' once in the list of extensions */
				dst += strlen(dst) + 1;
			}
			dev++;
		}
	}

	type = IO_CARTSLOT;
    if (errorlog)
		fprintf(errorlog,"Default %s for %s\n", device_typename(type), arg);
    /* Every unrecognized image type is added here */
	options.image_files[options.image_count].type = type;
	options.image_files[options.image_count].name = strdup(arg);
	options.image_count++;
    return 0;
}

/*
 * Load images from the command line.
 * Detect aliases and substitute the list of images for them.
 */
int load_image(int argc, char **argv, int j, int game_index)
{
	const char *driver = drivers[game_index]->name;
	int i;
    int res = 0;
	int type = 0;	/* unknown image type specified */

	/*
	 * Take all following commandline arguments without "-" as
	 * image names or as an alias name, which is replaced by a list
	 * of images.
	 */
	for (i = j + 1; i < argc; i++)
    {
        /* skip options and their additional arguments */
		if (argv[i][0] == '-')
		{
			if( !stricmp(argv[i], "-rom") || !stricmp(argv[i], "-cart") )
				type = IO_CARTSLOT;
            else
			if( !stricmp(argv[i], "-floppy") )
				type = IO_FLOPPY;
			else
			if( !stricmp(argv[i], "-harddisk") )
				type = IO_HARDDISK;
            else
			if( !stricmp(argv[i], "-cassette") )
				type = IO_CASSETTE;
			else
			if( !stricmp(argv[i], "-printer") )
				type = IO_PRINTER;
			else
			if( !stricmp(argv[i], "-serial") )
				type = IO_SERIAL;
        }
        else
        {
			/* check if this is an alias for a set of images */
			char *alias = get_alias(driver,argv[i]);

			if( alias && strlen(alias) )
            {
				char *arg;
				if( errorlog )
					fprintf(errorlog,"Using alias %s (%s) for driver %s\n", argv[i], alias, driver);
				arg = strtok (alias, ",");
				while (arg)
            	{
					res = detect_image_type(game_index, type, arg);
					arg = strtok(0, ",");
				}
			}
            else
            {
				if (errorlog)
					fprintf(errorlog,"NOTE: No alias found\n");
				res = detect_image_type(game_index, type, argv[i]);
			}
		}
		/* If we had an error bail out now */
		if (res)
			return res;
	}
    return res;
}




/* This function contains all the -list calls from fronthlp.c for MESS */
/* Currently Supported: */
/*   -listextensions    */

void list_mess_info(char *gamename, char *arg)
{

	int i,j;

	/* -listextensions */
	if (!stricmp(arg,"-listextensions"))
	{
		i = 0; j = 0;

        printf("\nSYSTEM    IMAGE FILE EXTENSIONS SUPPORTED\n");
		printf("--------  -------------------------------\n");
		while (drivers[i])
		{

			if ((drivers[i]->clone_of == 0 ||
			    (drivers[i]->clone_of->flags & NOT_A_DRIVER)) &&
			     !strwildcmp(gamename, drivers[i]->name))
			{
                printf("%-10s",drivers[i]->name);

                {
					const char *src = drivers[i]->dev->file_extensions;
                    while (*src)
					{
						printf(".%-5s",src);
						src += strlen(src) + 1;

					}
				}
                printf("\n");
			}
			i++;
		}
	}



}
