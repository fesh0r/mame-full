/* MODIFIED FOR MESS!!! */
/* (built from the 8/09/98 version of fileio.c) */

#include "driver.h"
#include <sys/stat.h>
#include <allegro.h>
#include <unistd.h>

#define VERBOSE 0

#define MAXPATHC 20 /* at most 20 path entries */
#define MAXPATHL 256 /* at most 255 character path length */

char buf1[MAXPATHL];

char *rompathv[MAXPATHC];
int rompathc;
char *cfgdir, *hidir, *inpdir;

char *alternate_name; /* for "-romdir" */

typedef enum
{
	kPlainFile,
	kZippedFile
} eFileType;

typedef struct
{
	FILE			*file;
	unsigned char	*data;
	int				offset;
	int				length;
	eFileType		type;
} FakeFileHandle;

/* JB 980123 */
int /* error */ load_zipped_file (const char *zipfile,
										 const char *filename,
										 unsigned char **buf, int *length);


/* helper function which decomposes a path list into a vector of paths */
int path2vector (char *path, char *buf, char **pathv)
{
	int i;
	char *token;

	/* copy the path variable, cause strtok will modify it */
	strncpy (buf, path, MAXPATHL-1);
	i = 0;
	token = strtok (buf, ";");
	while ((i < MAXPATHC) && token)
	{
		pathv[i] = token;
		i++;
		token = strtok (NULL, ";");
	}
	return i;
}

void decompose_rom_path (char *rompath)
{
	rompathc = path2vector (rompath, buf1, rompathv);
}

/*
 * file handling routines
 *
 * gamename holds the driver name, filename is only used for ROMs and samples.
 * if 'write' is not 0, the file is opened for write.
 * Otherwise it is opened for read.
 */

/*
 * check if roms/samples for a game exist at all
 * return index+1 of the path vector component on success, otherwise 0
 */
int osd_faccess(const char *newfilename, int filetype)
{
	char name[MAXPATHL];
	char **pathv;
	int  pathc;
	static int indx;
	static const char *filename;
	char *dirname;

	/* if filename == NULL, continue the search */
	if (newfilename)
	{
		indx = 0;
		filename = newfilename;
	}
	else
		indx++;

	if (filetype == OSD_FILETYPE_ROM_CART ||
		filetype == OSD_FILETYPE_IMAGE)
	{
		pathv = rompathv;
		pathc = rompathc;
	}
	else
		return 0;

	for (; indx < pathc; indx++)
	{
		dirname = pathv[indx];

		/* does such a directory (or file) exist? */
		sprintf(name,"%s/%s", dirname, filename);
#if VERBOSE
        if (errorlog) fprintf(errorlog, "osd_faccess indx %d '%s'\n", indx, name);
#endif
        if (access(name, F_OK) == 0)
			return indx+1;

        /* try again with a .zip extension */
		sprintf(name,"%s/%s.zip", dirname, filename);
#if VERBOSE
        if (errorlog) fprintf(errorlog, "osd_faccess indx %d '%s'\n", indx, name);
#endif
        if (access(name, F_OK) == 0)
			return indx+1;

        /* try again with a .zif extension */
		sprintf(name,"%s/%s.zif", dirname, filename);
#if VERBOSE
        if (errorlog) fprintf(errorlog, "osd_faccess indx %d '%s'\n", indx, name);
#endif
        if (access(name, F_OK) == 0)
			return indx+1;
    }

	/* no match */
	return 0;
}

void *osd_fopen(const char *game, const char *filename, int filetype, int wrmode)
{
	char name[MAXPATHL];
	char file[MAXPATHL];
	char *extension;
	char *dirname;
	char *gamename;

	int indx;
	struct stat stat_buffer;
	FakeFileHandle *f;

	f = (FakeFileHandle *)malloc(sizeof(FakeFileHandle));
	if (f == 0)
		return f;
	f->type = kPlainFile;
	f->file = 0;

	/* game = driver name, filename = file to load */
	gamename = (char *)game;

    switch (filetype)
	{
		case OSD_FILETYPE_ROM_CART:
			if (errorlog) fprintf(errorlog,"Opening ROM '%s' for driver %s...\n", filename, gamename);
			if (wrmode)
			{
				if (errorlog) fprintf(errorlog, "ROMs cannot be openend other than read-only!\n");
				break;
			}

            strcpy(name, filename);
#if VERBOSE
            if (errorlog) fprintf(errorlog, "- trying %s\n", name);
#endif
            f->file = fopen(name, "rb");
			if (f->file)
			{
#if VERBOSE
                if (errorlog)
                    fprintf(errorlog, "Using plain file %s\n", name);
#endif
                break;
            }

			/* strip off the extension */
            strcpy(file, filename);
			extension = strrchr(file, '.');
			if (extension)
				*extension = '\0';

			/* try to access the filename */
			indx = osd_faccess (filename, filetype);
			if (!indx)
				/* or if that fails gamename */
				indx = osd_faccess (gamename, filetype);
			if (!indx)
				/* and finally the file without extension */
				indx = osd_faccess (file, filetype);

			while (indx)
			{
				dirname = rompathv[indx-1];

				sprintf(name,"%s/%s", dirname, filename);
#if VERBOSE
				if (errorlog) fprintf(errorlog, "- trying %s\n", name);
#endif
                f->file = fopen(name, wrmode ? "r+b" : "rb");
				if (f->file)
				{
                    if (errorlog)
						fprintf(errorlog, "Using plain file %s for %s\n", name, filename);
                    break;
				}

                /* try in a subdirectory gamename/ */
				sprintf(name,"%s/%s/%s", dirname, gamename, filename);
#if VERBOSE
                if (errorlog) fprintf(errorlog, "- trying %s\n", name);
#endif
                f->file = fopen(name, "rb");
				if (f->file)
				{
                    if (errorlog)
						fprintf(errorlog, "Using plain file %s for %s\n", name, filename);
                    break;
				}
				/* try with a .zip extension for the filename */
				sprintf(name,"%s/%s/%s.zip", dirname, gamename, file);
#if VERBOSE
                if (errorlog) fprintf(errorlog, "- trying ROM zip %s\n", name);
#endif
                f->file = fopen(name, "rb");
				stat(name, &stat_buffer);
				if ((stat_buffer.st_mode & S_IFDIR))
				{
#if VERBOSE
                    if (errorlog)
						fprintf(errorlog, "ROM zip %s is a directory\n", name);
#endif
                    /* it's a directory */
					fclose(f->file);
					f->file = 0;
				}
				if (f->file)
				{
                    fclose(f->file);
					f->type = kZippedFile;
					if (load_zipped_file(name, filename, &f->data, &f->length))
					{
						f->type = kPlainFile;
						f->data = 0;
						f->file = 0;
					}
					else
					{
						if (errorlog)
							fprintf(errorlog, "Using ROM zip file %s for %s\n", name, filename);
						f->offset = 0;
						break;
					}
				}
				/* try with a .zip extension for the gamename */
				sprintf(name,"%s/%s.zip", dirname, gamename);
#if VERBOSE
				if (errorlog) fprintf(errorlog, "- trying driver zip %s\n", name);
#endif
                f->file = fopen(name, "rb");
				stat(name, &stat_buffer);
				if ((stat_buffer.st_mode & S_IFDIR))
				{
#if VERBOSE
                    if (errorlog)
						fprintf(errorlog, "Driver zip %s is a directory\n", name);
#endif
                    /* it's a directory */
					fclose(f->file);
					f->file = 0;
				}
				if (f->file)
				{
					fclose(f->file);
					f->type = kZippedFile;
					if (load_zipped_file(name, filename, &f->data, &f->length))
					{
						f->type = kPlainFile;
						f->data = 0;
						f->file = 0;
					}
					else
					{
						if (errorlog)
							fprintf(errorlog, "Using driver zip file %s for %s\n", name, filename);
						f->offset = 0;
						break;
					}
				}
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s/%s/%s.zip/%s", dirname, gamename, file, filename);
#if VERBOSE
                if (errorlog)
					fprintf(errorlog, "- trying ZipMagic %s\n", name);
#endif
                f->file = fopen(name,wrmode ? "r+b" : "rb");
				if (f->file)
				{
					if (errorlog)
						fprintf(errorlog, "Using ZipMagic file %s for %s\n", name, filename);
					break;
				}
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s/%s/%s.zif/%s", dirname, gamename, file, filename);
#if VERBOSE
                if (errorlog)
					fprintf(errorlog, "- trying ZipFolders %s\n", name);
#endif
                f->file = fopen(name,wrmode ? "r+b" : "rb");
				if (f->file)
				{
					if (errorlog)
						fprintf(errorlog, "Using ZipFolders file %s for %s\n", name, filename);
					break;
				}

				/* check next path entry */
				indx = osd_faccess (NULL, filetype);
			}

            if (!f->file)
			{
				if (errorlog)
					fprintf(errorlog, "Found no ROM '%s' for driver %s\n", filename, gamename);
			}
            break;

        case OSD_FILETYPE_IMAGE:
			if (errorlog) fprintf(errorlog,"Opening image '%s' for driver %s...\n", filename, gamename);

			strcpy(name, filename);
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "- trying %s\n", name);
#endif
            f->file = fopen(name, wrmode ? "r+b" : "rb");
			if (f->file)
			{
				if (errorlog)
					fprintf(errorlog, "Using plain file %s\n", name);
				break;
            }

			/* strip off the extension */
            strcpy(file, filename);
			extension = strrchr(file, '.');
			if (extension)
				*extension = '\0';

			/* try to access the filename */
			indx = osd_faccess (filename, filetype);
			if (!indx)
				/* or if that fails gamename */
				indx = osd_faccess (gamename, filetype);
			if (!indx)
				/* and finally the file without extension */
				indx = osd_faccess (file, filetype);

			while (indx)
			{
				dirname = rompathv[indx-1];

				sprintf(name,"%s/%s", dirname, filename);
#if VERBOSE
                if (errorlog)
					fprintf(errorlog, "- trying %s\n", name);
#endif
                f->file = fopen(name, wrmode ? "r+b" : "rb");
				if (f->file)
				{
					if (errorlog)
						fprintf(errorlog, "Using plain file %s for %s\n", name, filename);
					break;
				}

				/* try in a subdirectory gamename/ */
				sprintf(name,"%s/%s/%s", dirname, gamename, filename);
#if VERBOSE
                if (errorlog) fprintf(errorlog, "- trying %s\n", name);
#endif
                f->file = fopen(name, wrmode ? "r+b" : "rb");
				if (f->file)
				{
					if (errorlog)
						fprintf(errorlog, "Using plain file %s for %s\n", name, filename);
					break;
				}

				/* if not wrmode mode */
				if (!wrmode)
				{
					/* try with a .zip extension for the filename */
					sprintf(name,"%s/%s/%s.zip", dirname, gamename, file);
#if VERBOSE
                    if (errorlog) fprintf(errorlog, "- trying image zip %s\n", name);
#endif
                    f->file = fopen(name, "rb");
					stat(name, &stat_buffer);
					if ((stat_buffer.st_mode & S_IFDIR))
					{
#if VERBOSE
                        if (errorlog)
							fprintf(errorlog, "Image zip %s is a directory\n", name);
#endif
                        /* it's a directory */
						fclose(f->file);
						f->file = 0;
					}
					if (f->file)
					{
						fclose(f->file);
						f->type = kZippedFile;
						if (load_zipped_file(name, filename, &f->data, &f->length))
						{
							f->type = kPlainFile;
							f->data = 0;
							f->file = 0;
						}
						else
						{
                            if (errorlog)
								fprintf(errorlog, "Using image zip file %s for %s\n", name, filename);
							f->offset = 0;
							break;
						}
					}
				}
				/* if not wrmode mode */
				if (!wrmode)
				{
					/* try with a .zip extension for the gamename */
					sprintf(name,"%s/%s.zip", dirname, gamename);
#if VERBOSE
                    if (errorlog)
						fprintf(errorlog, "- trying driver zip %s\n", name);
#endif
                    f->file = fopen(name, "rb");
					stat(name, &stat_buffer);
					if ((stat_buffer.st_mode & S_IFDIR))
					{
#if VERBOSE
                        if (errorlog)
							fprintf(errorlog, "Driver zip %s is a directory\n", name);
#endif
                        /* it's a directory */
						fclose(f->file);
						f->file = 0;
					}
					if (f->file)
					{
						fclose(f->file);
						f->type = kZippedFile;
						if (load_zipped_file(name, filename, &f->data, &f->length))
						{
							f->type = kPlainFile;
							f->data = 0;
							f->file = 0;
						}
						else
						{
							if (errorlog)
								fprintf(errorlog, "Using driver zip file %s for %s\n", name, filename);
							f->offset = 0;
							break;
						}
					}
				}
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s/%s/%s.zip/%s", dirname, gamename, file, filename);
#if VERBOSE
                if (errorlog)
					fprintf(errorlog, "- trying ZipMagic %s\n", name);
#endif
                f->file = fopen(name,wrmode ? "r+b" : "rb");
				if (f->file)
				{
                    if (errorlog)
                        fprintf(errorlog, "Using ZipMagic file %s for %s\n", name, filename);
					break;
				}

				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s/%s/%s.zif/%s", dirname, gamename, file, filename);
#if VERBOSE
                if (errorlog)
					fprintf(errorlog, "- trying ZipFolders %s\n", name);
#endif
                f->file = fopen(name,wrmode ? "r+b" : "rb");
				if (f->file)
				{
					if (errorlog)
						fprintf(errorlog, "Using ZipFolders file %s for %s\n", name, filename);
					break;
				}
				/* check next path entry */
				indx = osd_faccess (NULL, filetype);
			}

            if (!f->file)
			{
				if (errorlog)
					fprintf(errorlog, "Found no image '%s' for driver %s\n", filename, gamename);
            }
            break;

        case OSD_FILETYPE_HIGHSCORE:
			if (mame_highscore_enabled())
			{
				if (errorlog)
					fprintf(errorlog,"Opening high score for driver %s...\n", gamename);
                sprintf(name,"%s/%s.hi", hidir, gamename);
				f->file = fopen(name,wrmode ? "wb" : "rb");
				if (f->file == 0)
				{
					/* try with a .zip directory (if ZipMagic is installed) */
					sprintf(name,"%s.zip/%s.hi", hidir, gamename);
					f->file = fopen(name,wrmode ? "wb" : "rb");
				}
				if (f->file == 0)
				{
					/* try with a .zif directory (if ZipFolders is installed) */
					sprintf(name,"%s.zif/%s.hi", hidir, gamename);
					f->file = fopen(name,wrmode ? "wb" : "rb");
				}
			}
			break;
		case OSD_FILETYPE_CONFIG:
			if (errorlog)
				fprintf(errorlog,"Opening config for driver %s...\n", gamename);
            sprintf(name,"%s/%s.cfg", cfgdir, gamename);
			f->file = fopen(name,wrmode ? "wb" : "rb");
			if (!f->file)
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s.zip/%s.cfg", cfgdir, gamename);
				f->file = fopen(name,wrmode ? "wb" : "rb");
			}
			if (!f->file)
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s.zif/%s.cfg", cfgdir, gamename);
				f->file = fopen(name,wrmode ? "wb" : "rb");
			}
			break;
		case OSD_FILETYPE_INPUTLOG:
			if (errorlog)
				fprintf(errorlog,"Opening input log for driver %s...\n", gamename);
            sprintf(name,"%s/%s.inp", inpdir, gamename);
			f->file = fopen(name,wrmode ? "wb" : "rb");
			break;
		default:
			break;
	}

	if (f->file == 0)
	{
		free(f);
		return 0;
	}

	return f;
}



int osd_fread(void *file,void *buffer,int length)
{
	FakeFileHandle *f = (FakeFileHandle *)file;
	int gotbytes = 0;

	switch (f->type)
	{
		case kPlainFile:
			gotbytes = fread(buffer,1,length,f->file);
			break;
		case kZippedFile:
			/* reading from the uncompressed image of a zipped file */
			if (f->data)
			{
				gotbytes = length;
                if (length + f->offset > f->length)
					gotbytes = f->length - f->offset;
				memcpy(buffer, f->offset + f->data, gotbytes);
				f->offset += gotbytes;
			}
			break;
	}

	return gotbytes;
}



int osd_fwrite(void *file,const void *buffer,int length)
{
	return fwrite(buffer,1,length,((FakeFileHandle *)file)->file);
}



int osd_fseek(void *file,int offset,int whence)
{
	FakeFileHandle *f = (FakeFileHandle *)file;
	int err = 0;

	switch (f->type)
	{
		case kPlainFile:
			return fseek(((FakeFileHandle *)file)->file,offset,whence);
			break;
		case kZippedFile:
			/* seeking within the uncompressed image of a zipped file */
			switch (whence)
			{
				case SEEK_SET:
					f->offset = offset;
					break;
				case SEEK_CUR:
					f->offset += offset;
					break;
				case SEEK_END:
					f->offset = f->length + offset;
					break;
			}
			break;
	}

	return err;
}



void osd_fclose(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch(f->type)
	{
		case kPlainFile:
			fclose(f->file);
			break;
		case kZippedFile:
			if (f->data)
				free(f->data);
			break;
	}
	free(f);
}
