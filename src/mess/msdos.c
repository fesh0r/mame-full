/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include "driver.h"
#include "mess/msdos.h"

extern struct GameOptions options;


/* fronthlp functions */
int strwildcmp(const char *sp1, const char *sp2);

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

		printf("\nSYSTEM    DEVICE       IMAGE FILE EXTENSIONS SUPPORTED\n");
		printf(  "--------  ----------   -------------------------------\n");

		while (drivers[i])
		{
			const struct IODevice *dev = drivers[i]->dev;

			if (!strwildcmp(gamename, drivers[i]->name))
			{
				int devcount=1;
				printf("%-10s",drivers[i]->name);

				/* if IODevice not used, print UNKNOWN */
				if (dev->type == IO_END)
					printf("%-12s\n","UNKNOWN");

				/* else cycle through Devices */
				while (dev->type != IO_END)
				{
					const char *src = dev->file_extensions;

					if (devcount == 1)
						printf("%-12s",device_typename(dev->type));
					else
						printf("%-10s%-12s","    ",device_typename(dev->type));
					devcount++;

					while (*src && src!=NULL)
					{

						printf(".%-5s",src);
						src += strlen(src) + 1;
					}
					dev++; /* next IODevice struct */
					printf("\n");
				}


			}
			i++;

		}

	}

}
