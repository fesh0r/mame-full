/*
This file is a set of function calls and defs required for MESS.
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



