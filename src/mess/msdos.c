/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include "driver.h"
#include "mess/msdos.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dos.h>
#include <unistd.h>

extern struct GameOptions options;

/* fronthlp functions */
extern int strwildcmp(const char *sp1, const char *sp2);

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

	if (options.image_count >= MAX_IMAGES)
	{
		printf("Too many image names specified!\n");
		return 1;
	}

	if (type)
	{
		if (errorlog)
			fprintf(errorlog, "User specified %s for %s\n", device_typename(type), arg);
		/* the user specified a device type */
		options.image_files[options.image_count].type = type;
		options.image_files[options.image_count].name = strdup(arg);
		options.image_count++;
		return 0;
	}

	/* Look up the filename extension in the drivers device list */
	ext = strrchr(arg, '.');
	if (ext)
	{
		const struct IODevice *dev = drv->dev;

		ext++;
		while (dev->type != IO_END)
		{
			const char *dst = dev->file_extensions;

			/* scan supported extensions for this device */
			while (dst && *dst)
			{
				if (stricmp(dst, ext) == 0)
				{
					if (errorlog)
						fprintf(errorlog, "Extension match %s [%s] for %s\n", device_typename(dev->type), dst, arg);
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
		fprintf(errorlog, "Default %s for %s\n", device_typename(type), arg);
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
	int type = IO_END;

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
			if (!stricmp(argv[i], "-rom") || !stricmp(argv[i], "-cart"))
				type = IO_CARTSLOT;
			else if (!stricmp(argv[i], "-floppy"))
				type = IO_FLOPPY;
			else if (!stricmp(argv[i], "-harddisk"))
				type = IO_HARDDISK;
			else if (!stricmp(argv[i], "-cassette"))
				type = IO_CASSETTE;
			else if (!stricmp(argv[i], "-printer"))
				type = IO_PRINTER;
			else if (!stricmp(argv[i], "-serial"))
				type = IO_SERIAL;
		}
		else
		{
			/* check if this is an alias for a set of images */
			char *alias = get_alias(driver, argv[i]);

			if (alias && strlen(alias))
			{
				char *arg;

				if (errorlog)
					fprintf(errorlog, "Using alias %s (%s) for driver %s\n", argv[i], alias, driver);
				arg = strtok(alias, ",");
				while (arg)
				{
					res = detect_image_type(game_index, type, arg);
					arg = strtok(0, ",");
				}
			}
			else
			{
				if (errorlog)
					fprintf(errorlog, "NOTE: No alias found\n");
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

	int i, j;

	/* -listextensions */
	if (!stricmp(arg, "-listextensions"))
	{

		i = 0;
		j = 0;

		printf("\nSYSTEM    DEVICE       IMAGE FILE EXTENSIONS SUPPORTED\n");
		printf("--------  ----------   -------------------------------\n");

		while (drivers[i])
		{
			const struct IODevice *dev = drivers[i]->dev;

			if (!strwildcmp(gamename, drivers[i]->name))
			{
				int devcount = 1;

				printf("%-10s", drivers[i]->name);

				/* if IODevice not used, print UNKNOWN */
				if (dev->type == IO_END)
					printf("%-12s\n", "UNKNOWN");

				/* else cycle through Devices */
				while (dev->type != IO_END)
				{
					const char *src = dev->file_extensions;

					if (devcount == 1)
						printf("%-12s", device_typename(dev->type));
					else
						printf("%-10s%-12s", "    ", device_typename(dev->type));
					devcount++;

					while (src && *src)
					{

						printf(".%-5s", src);
						src += strlen(src) + 1;
					}
					dev++;			   /* next IODevice struct */
					printf("\n");
				}


			}
			i++;

		}

	}

}

/*****************************************************************************
 * device, directory and file functions
 *****************************************************************************/

static int num_devices = 0;
static char dos_devices[32*2];
static char dos_device[2];
static char dos_filemask[260];

static int fnmatch(const char *f1, const char *f2)
{
	while (*f1 && *f2)
	{
		if (*f1 == '*')
		{
			/* asterisk is not the last character? */
			if (f1[1])
			{
				/* skip until first occurance of the character after the asterisk */
                while (*f2 && toupper(f1[1]) != toupper(*f2))
					f2++;
				/* skip repetitions of the character after the asterisk */
				while (*f2 && toupper(f1[1]) == toupper(f2[1]))
					f2++;
			}
			else
			{
				/* skip until end of string */
                while (*f2)
					f2++;
			}
        }
		else
		if (*f1 == '?')
		{
			/* skip one character */
            f2++;
		}
		else
		{
			/* mismatch? */
            if (toupper(*f1) != toupper(*f2))
				return 0;
            /* skip one character */
			f2++;
		}
		/* skip mask */
        f1++;
	}
	/* no match if anything is left */
	if (*f1 || *f2)
		return 0;
    return 1;
}

int osd_num_devices(void)
{
	if (num_devices == 0)
	{
		union REGS r;
        int dev, previous_dev;
		r.h.ah = 0x19;	/* get current drive */
		intdos(&r,&r);
		previous_dev = r.h.al;	/* save current drive */
		for (dev = 0; dev < 26; dev++)
		{
			r.h.ah = 0x0e;		/* select drive */
			r.h.dl = dev;		/* DL */
			intdos(&r,&r);
			r.h.ah = 0x19;		/* get current drive */
			intdos(&r,&r);
			if (r.h.al == dev)	/* successful? */
			{
				dos_devices[num_devices*2+0] = 'A' + dev;
				dos_devices[num_devices*2+1] = '\0';
				num_devices++;
			}
        }
		r.h.ah = 0x0e;		/* select previous drive again */
		r.h.dl = previous_dev;
		intdos(&r,&r);
    }
	return num_devices;
}

const char *osd_get_device_name(int idx)
{
	if (idx < num_devices)
        return &dos_devices[idx*2];
    return "";
}

void osd_change_device(const char *device)
{
        char chdir_device[4];

	dos_device[0] = device[0];
	dos_device[1] = '\0';

        chdir_device[0] = device[0];
        chdir_device[1] = ':';
        chdir_device[2] = '/';
        chdir_device[3] = '\0';


        chdir(chdir_device);

}

void osd_change_directory(const char *directory)
{
        chdir(directory);

}

static char dos_cwd[260];

const char *osd_get_cwd(void)
{
        getcwd(dos_cwd, 260);

        return dos_cwd;
}

void *osd_dir_open(const char *mess_dirname, const char *filemask)
{
	DIR *dir;

	strcpy(dos_filemask, filemask);

    dir = opendir(".");

    return dir;
}

int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir)
{
	int len;
    struct dirent *d;

    name[0] = '\0';
	*is_dir = 0;

    if (!dir)
		return 0;

    d = readdir(dir);
	while (d)
	{
		struct stat st;

		strncpy(name, d->d_name, namelength-1);
		name[namelength-1]='\0';

		len = strlen(name);

		if( stat(d->d_name, &st) == 0 )
			*is_dir = S_ISDIR(st.st_mode);

		if (*is_dir)
			return len;
		else
		if (fnmatch(dos_filemask, d->d_name))
			return len;
		else
		{
			/* no match, zap the name and type again */
			name[0] = '\0';
			*is_dir = 0;
        }
		d = readdir(dir);
	}
	return 0;
}

void osd_dir_close(void *dir)
{
	if (dir)
		closedir(dir);
}


