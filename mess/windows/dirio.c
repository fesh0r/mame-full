#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dos.h>
#include <unistd.h>
#include <stdlib.h>

/**********************************************************/
/* Functions called from MSDOS.C by MAME for running MESS */
/**********************************************************/
static char startup_dir[260]; /* Max Windows Path? */


/* Go back to the startup dir on exit */
void return_to_startup_dir(void)
{
    chdir(startup_dir);
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
	return 0;
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
		if (!startup_dir[0])
		{
			getcwd(startup_dir,sizeof(startup_dir));
			atexit(return_to_startup_dir);
		}

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

int osd_select_file(int sel, char *filename)
{
	return 0;
}
