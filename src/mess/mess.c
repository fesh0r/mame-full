/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include "driver.h"
#include "mame.h"
#include "mess/mess.h"

extern struct GameOptions options;

static const char **names[IO_COUNT] = {NULL,};
static int count[IO_COUNT] = {0,};
static const char *typename[IO_COUNT] = {
	"NONE",
    "Cartridge",
    "Floppydisk",
    "Harddisk",
    "Cassette",
    "Printer",
    "Serial",
    "Snapshot"
};

static char *mess_alpha = "";



/*
 * Return a name for the device type (to be used for UI functions)
 */
const char *device_typename(int type)
{
	if (type < IO_COUNT)
		return typename[type];
    return "UNKNOWN";
}

/*
 * Return the id'th filename for a device of type 'type',
 * NULL if not enough image names of that type are available.
 */
const char *device_filename(int type, int id)
{
	if (type >= IO_COUNT)
		return NULL;
    if (id < count[type])
		return names[type][id];
    return NULL;
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
 * Copy the image names from options.image_files[] to
 * the array of filenames we keep here, depending on the
 * type identifier of each image.
 */
int get_filenames(void)
{
	int i;
	for( i = 0; i < options.image_count; i++ )
	{
        int type = options.image_files[i].type;

		if (type < IO_COUNT)
		{
			/* Add a filename from the GameOptions to the arrays of names */
			if (names[type])
				names[type] = realloc(names[type],(count[type]+1)*sizeof(char *));
			else
				names[type] = malloc(sizeof(char *));
			if (!names[type])
				return 1;
			names[type][count[type]] = options.image_files[i].name;
			if (errorlog)
				fprintf(errorlog, "%-16s #%d %s", typename[type], count[type], names[type][count[type]]);
			count[type]++;
		}
		else
		{
			if(errorlog)
				fprintf(errorlog, "Invalid IO_ type %d for %s\n", type, options.image_files[i].name);
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
                /* initialize */
				int result = (*dev->init)(id,filename);

				printf("Device #%d (%s) Init %d\n", id, device_typename(dev->type), result);
				if( result != INIT_OK && filename )
				{
					printf("%s #%d init failed (%s).\n", device_typename(dev->type), id, filename);
					return 1;
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
            if (errorlog) fprintf(errorlog, "%s does not support exit!\n", device_typename(dev->type));
        }
        dev++;
    }
	for( type = IO_CARTSLOT; type < IO_COUNT; type++ )
	{
		if( names[type] )
			free(names[type]);
		names[type] = NULL;
		count[type] = 0;
	}
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



