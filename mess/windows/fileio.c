//#include "mamalleg.h"
#include <windows.h>
#include "driver.h"
#include "unzip.h"
#include "rc.h"
#include <signal.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/stat.h>
#elif defined(UNDER_CE)
#include "mamece.h"
#else
#include <sys/stat.h>
#endif

/* Verbose outputs to error.log ? */
#define VERBOSE 	1

/* Use the file cache ? */
#define FILE_CACHE	1

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

/* CRCs */
static char crcdirbuf[256] = "";
const char *crcdir = crcdirbuf;
static char crcfilename[256] = "";
const char *crcfile = crcfilename;
static char pcrcfilename[256] = "";
const char *pcrcfile = pcrcfilename;

/* BIOS */
static char **rompathv = NULL;
static int rompathc = 0;
static int rompath_needs_decomposition = 1;
extern char *rompath_extra;

/* SAMPLES */
static char **samplepathv = NULL;
static int samplepathc = 0;
static int samplepath_needs_decomposition = 1;

/* SOFTWARE */
static char **swpathv = NULL;
static int swpathc = 0;
static int swpath_needs_decomposition = 1;

static const char *rompath = "bios";
static const char *swpath = "software";
static const char *samplepath = "samples";
static const char *cfgdir, *nvdir, *hidir, *inpdir, *stadir;
static const char *memcarddir, *artworkdir, *screenshotdir, *cheatdir;
/* from datafile.c */
extern const char *history_filename;
extern const char *mameinfo_filename;
/* from cheat.c */
extern char *cheatfile;

static int request_decompose_rompath(struct rc_option *option, const char *arg, int priority);
static int request_decompose_samplepath(struct rc_option *option, const char *arg, int priority);
static int request_decompose_swpath(struct rc_option *option, const char *arg, int priority);

struct rc_option fileio_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Windows path and directory options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	/* BIOS and Software */
	{ "biospath",      "bp",  rc_string, &rompath,"bios",     0, 0, request_decompose_rompath, "path to bios sets" },
	{ "softwarepath",  "swp", rc_string, &swpath, "software", 0, 0, request_decompose_swpath,  "path to software" },
	/* Others */
	{ "CRC_directory", "crc", rc_string, &crcdir, "crc"     , 0, 0, NULL,                      "path to CRC files" },
	{ "samplepath", "sp", rc_string, &samplepath, "samples", 0, 0, request_decompose_samplepath, "path to samplesets" },
	{ "cfg_directory", NULL, rc_string, &cfgdir, "cfg", 0, 0, NULL, "directory to save configurations" },
	{ "nvram_directory", NULL, rc_string, &nvdir, "nvram", 0, 0, NULL, "directory to save nvram contents" },
	{ "memcard_directory", NULL, rc_string, &memcarddir, "memcard", 0, 0, NULL, "directory to save memory card contents" },
	{ "input_directory", NULL, rc_string, &inpdir, "inp", 0, 0, NULL, "directory to save input device logs" },
	{ "hiscore_directory", NULL, rc_string, &hidir, "hi", 0, 0, NULL, "directory to save hiscores" },
	{ "state_directory", NULL, rc_string, &stadir, "sta", 0, 0, NULL, "directory to save states" },
	{ "artwork_directory", NULL, rc_string, &artworkdir, "artwork", 0, 0, NULL, "directory for Artwork (Overlays etc.)" },
	{ "snapshot_directory", NULL, rc_string, &screenshotdir, "snap", 0, 0, NULL, "directory for screenshots (.png format)" },
	{ "cheat_file", NULL, rc_string, &cheatfile, "cheat.dat", 0, 0, NULL, "cheat filename" },
	{ "history_file", NULL, rc_string, &history_filename, "sysinfo.dat", 0, 0, NULL, NULL },
	//{ "messinfo_file", NULL, rc_string, &mameinfo_filename, "messinfo.dat", 0, 0, NULL, NULL },
	{ "mameinfo_file", NULL, rc_string, &mameinfo_filename, "mameinfo.dat", 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};


char *alternate_name;	/* for "-romdir" */

typedef enum
{
	kPlainFile,
	kRAMFile,
	kZippedFile
}	eFileType;

typedef struct
{
	FILE *file;
	unsigned char *data;
	unsigned int offset;
	unsigned int length;
	eFileType type;
	unsigned int crc;
	int		eof;	// for kRamFiles only
}	FakeFileHandle;

#ifdef _MSC_VER
#define ZEXPORT WINAPI
#else
#define ZEXPORT
#endif

extern unsigned int ZEXPORT crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);
static int checksum_file (const char *file, unsigned char **p, unsigned int *size, unsigned int *crc);

/*
 * File stat cache LRU (Last Recently Used)
 */

#if FILE_CACHE
struct file_cache_entry
{
	struct stat stat_buffer;
	int result;
	char *file;
};

/* File cache buffer */
static struct file_cache_entry **file_cache_map = 0;

/* File cache size */
static unsigned int file_cache_max = 0;

/* AM 980919 */
static int cache_stat (const char *path, struct stat *statbuf)
{
	if( file_cache_max )
	{
		unsigned i;
		struct file_cache_entry *entry;

		/* search in the cache */
		for( i = 0; i < file_cache_max; ++i )
		{
			if( file_cache_map[i]->file && strcmp (file_cache_map[i]->file, path) == 0 )
			{	/* found */
				unsigned j;

//				LOG(("File cache HIT  for %s\n", path));
				/* store */
				entry = file_cache_map[i];

				/* shift */
				for( j = i; j > 0; --j )
					file_cache_map[j] = file_cache_map[j - 1];

				/* set the first entry */
				file_cache_map[0] = entry;

				if( entry->result == 0 )
					memcpy (statbuf, &entry->stat_buffer, sizeof (struct stat));

				return entry->result;
			}
		}
//		LOG(("File cache FAIL for %s\n", path));

		/* oldest entry */
		entry = file_cache_map[file_cache_max - 1];
		free (entry->file);

		/* shift */
		for( i = file_cache_max - 1; i > 0; --i )
			file_cache_map[i] = file_cache_map[i - 1];

		/* set the first entry */
		file_cache_map[0] = entry;

		/* file */
		entry->file = (char *) malloc (strlen (path) + 1);
		strcpy (entry->file, path);

		/* result and stat */
		entry->result = stat (path, &entry->stat_buffer);

		if( entry->result == 0 )
			memcpy (statbuf, &entry->stat_buffer, sizeof (struct stat));

		return entry->result;
	}
	else
	{
		return stat (path, statbuf);
	}
}

/* AM 980919 */
static void cache_allocate (unsigned entries)
{
	if( entries )
	{
		unsigned i;

		file_cache_max = entries;
		file_cache_map = (struct file_cache_entry **) malloc (file_cache_max * sizeof (struct file_cache_entry *));

		for( i = 0; i < file_cache_max; ++i )
		{
			file_cache_map[i] = (struct file_cache_entry *) malloc (sizeof (struct file_cache_entry));
			memset (file_cache_map[i], 0, sizeof (struct file_cache_entry));
		}
		LOG(("File cache allocated for %d entries\n", file_cache_max));
	}
}
#else

#define cache_stat(a,b) stat(a,b)

#endif

static int request_decompose_rompath(struct rc_option *option, const char *arg, int priority)
{
	rompath_needs_decomposition = 1;

	option->priority = priority;
	return 0;
}

/* rompath will be decomposed only once after all configuration
 * options are parsed */
static void decompose_rompath(void)
{
	char *token;
	static char* path;

	LOG(("decomposing rompath\n"));
	if (rompath_extra)
		LOG(("  rompath_extra = %s\n", rompath_extra));
	LOG(("  rompath = %s\n", rompath));

	/* run only once */
	rompath_needs_decomposition = 0;

	/* start with zero path components */
	rompathc = 0;

	if (rompath_extra)
	{
		rompathv = malloc (sizeof(char *));
		rompathv[rompathc++] = rompath_extra;
	}

	if (!path)
		path = malloc( strlen(rompath) + 1);
	else
		path = realloc( path, strlen(rompath) + 1);

	if( !path )
	{
		logerror("decompose_rom_path: failed to malloc!\n");
		raise(SIGABRT);
	}

	strcpy (path, rompath);
	token = strtok (path, ";");
	while( token )
	{
		if( rompathc )
			rompathv = realloc (rompathv, (rompathc + 1) * sizeof(char *));
		else
			rompathv = malloc (sizeof(char *));
		if( !rompathv )
			break;
		rompathv[rompathc++] = token;
		token = strtok (NULL, ";");
	}

#if FILE_CACHE
	/* AM 980919 */
	if( file_cache_max == 0 )
	{
		/* (rom path directories + 1 buffer)==rompathc+1 */
		/* (dir + .zip + .zif)==3 */
		/* (clone+parent)==2 */
		cache_allocate ((rompathc + 1) * 3 * 2);
	}
#endif
}

static int request_decompose_samplepath(struct rc_option *option, const char *arg, int priority)
{
	samplepath_needs_decomposition = 1;

	option->priority = priority;
	return 0;
}

/* samplepath will be decomposed only once after all configuration
 * options are parsed */
static void decompose_samplepath(void)
{
	char *token;
	static char *path;

	LOG(("decomposing samplepath\n  samplepath = %s\n", samplepath));

	/* run only once */
	samplepath_needs_decomposition = 0;

	/* start with zero path components */
	samplepathc = 0;

	if (!path)
		path = malloc( strlen(samplepath) + 1);
	else
		path = realloc( path, strlen(samplepath) + 1);

	if( !path )
	{
		logerror("decompose_sample_path: failed to malloc!\n");
		raise(SIGABRT);
	}

	strcpy (path, samplepath);
	token = strtok (path, ";");
	while( token )
	{
		if( samplepathc )
			samplepathv = realloc (samplepathv, (samplepathc + 1) * sizeof(char *));
		else
			samplepathv = malloc (sizeof(char *));
		if( !samplepathv )
			break;
		samplepathv[samplepathc++] = token;
		token = strtok (NULL, ";");
	}
}


static int request_decompose_swpath(struct rc_option *option, const char *arg, int priority)
{
	swpath_needs_decomposition = 1;

	option->priority = priority;
	return 0;
}

/* swpath will be decomposed only once after all configuration
 * options are parsed */
static void decompose_swpath(void)
{
	char *token;
	static char *path;
	LOG(("decomposing swpath\n  swpath = %s\n", swpath));

	/* run only once */
	swpath_needs_decomposition = 0;

	/* start with zero path components */
	swpathc = 0;

	if (!path)
		path = malloc( strlen(swpath) + 1);
	else
		path = realloc( path, strlen(swpath) + 1);

	if( !path )
	{
		logerror("decompose_swpath: failed to malloc!\n");
		raise(SIGABRT);
	}

	strcpy (path, swpath);
	token = strtok (path, ";");
	while( token )
	{
		if( swpathc )
			swpathv = realloc (swpathv, (swpathc + 1) * sizeof(char *));
		else
			swpathv = malloc (sizeof(char *));
		if( !swpathv )
			break;
		swpathv[swpathc++] = token;
		token = strtok (NULL, ";");
	}
}

static inline void decompose_paths_if_needed(void)
{
	if (rompath_needs_decomposition)
		decompose_rompath();
	if (samplepath_needs_decomposition)
		decompose_samplepath();
	if (swpath_needs_decomposition)
		decompose_swpath();

}

/*
 * file handling routines
 *
 * gamename holds the driver name, filename is only used for ROMs and samples.
 * if 'write' is not 0, the file is opened for write. Otherwise it is opened
 * for read.
 */

/*
 * check if roms/samples for a game exist at all
 * return index+1 of the path vector component on success, otherwise 0
 */
int osd_faccess (const char *newfilename, int filetype)
{
	static int indx;
	static const char *filename;
	char name[256];
	char **pathv;
	int pathc;
	char *dir_name;

	/* update path info */
	decompose_paths_if_needed();

	/* if filename == NULL, continue the search */
	if( newfilename != NULL )
	{
		indx = 0;
		filename = newfilename;
	}
	else
		indx++;

	if( filetype == OSD_FILETYPE_ROM )
	{
		pathv = rompathv;
		pathc = rompathc;
	}
	else
	if( filetype == OSD_FILETYPE_SAMPLE )
	{
		pathv = samplepathv;
		pathc = samplepathc;
	}
	else
	if (filetype == OSD_FILETYPE_IMAGE)
	{
		pathv = swpathv;
		pathc = swpathc;
	}
	else
	if( filetype == OSD_FILETYPE_SCREENSHOT )
	{
		void *f;

		sprintf (name, "%s/%s.png", screenshotdir, newfilename);
		f = fopen (name, "rb");
		if( f )
		{
			fclose (f);
			return 1;
		}
		else
			return 0;
	}
	else
		return 0;

	for( ; indx < pathc; indx++ )
	{
		struct stat stat_buffer;

		dir_name = pathv[indx];

		/* does such a directory (or file) exist? */
		sprintf (name, "%s/%s", dir_name, filename);
		if( cache_stat (name, &stat_buffer) == 0 )
			return indx + 1;

		/* try again with a .zip extension */
		sprintf (name, "%s/%s.zip", dir_name, filename);
		if( cache_stat (name, &stat_buffer) == 0 )
			return indx + 1;

		/* try again with a .zif extension */
		sprintf (name, "%s/%s.zif", dir_name, filename);
		if( cache_stat (name, &stat_buffer) == 0 )
			return indx + 1;
	}

	/* no match */
	return 0;
}

#ifdef MESS
static int is_zipfile(const char *filename)
{
	const char *extension;
	extension = strrchr(filename, '.');
	return extension && !stricmp(extension, ".zip");
}

static int is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
}

static int is_absolute_path(const char *filename)
{
	if ((filename[0] == '.') || (is_path_separator(filename[0])))
		return 1;
#ifndef UNDER_CE
	if (filename[0] && (filename[1] == ':'))
		return 1;
#endif
	return 0;
}
#endif

/* JB 980920 update */
/* AM 980919 update */
void *osd_fopen (const char *game, const char *filename, int filetype, int openforwrite)
{
	char *scratch1 = NULL;
	char *scratch2 = NULL;
	char name[256];
	char *gamename;
	int found = 0;
	int indx;
	struct stat stat_buffer;
	FakeFileHandle *f;
	int pathc;
	char **pathv;

	decompose_paths_if_needed();

	f = (FakeFileHandle *) malloc (sizeof (FakeFileHandle));

	if( !f )
	{
		logerror("osd_fopen: failed to malloc FakeFileHandle!\n");
		return 0;
	}

	memset (f, 0, sizeof (FakeFileHandle));

	gamename = (char *) game;


	/* Support "-romdir" yuck. */
	if( alternate_name )
	{
		/* check for DEFAULT.CFG file request */
		if( filetype == OSD_FILETYPE_CONFIG && gamename == "default" )
		{
			LOG(("osd_fopen: default input configuration file requested; -romdir switch not applied\n"));
		} else {
			LOG(("osd_fopen: -romdir overrides '%s' by '%s'\n", gamename, alternate_name));
 			gamename = alternate_name;
		}
	}

	switch( filetype )
	{
	case OSD_FILETYPE_ROM:
	case OSD_FILETYPE_SAMPLE:

		/* only for reading */
		if( openforwrite )
		{
			logerror("osd_fopen: type %02x write not supported\n",filetype);
			break;
		}

		if( filetype == OSD_FILETYPE_SAMPLE )
		{
			LOG(("osd_fopen: using samplepath\n"));
			pathc = samplepathc;
			pathv = samplepathv;
		}
		else
		{
			LOG(("osd_fopen: using rompath\n"));
			pathc = rompathc;
			pathv = rompathv;
		}

		for( indx = 0; indx < pathc && !found; ++indx )
		{
			const char *dir_name = pathv[indx];

			if( !found )
			{
				sprintf (name, "%s/%s", dir_name, gamename);
				LOG(("Trying %s\n", name));
				if( cache_stat (name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR) )
				{
					sprintf (name, "%s/%s/%s", dir_name, gamename, filename);
					if( filetype == OSD_FILETYPE_ROM )
					{
						if( checksum_file (name, &f->data, &f->length, &f->crc) == 0 )
						{
							f->type = kRAMFile;
							f->offset = 0;
							found = 1;
						}
					}
					else
					{
						f->type = kPlainFile;
						f->file = fopen (name, "rb");
						found = f->file != 0;
					}
				}
			}

			if( !found )
			{
				/* try with a .zip extension */
				sprintf (name, "%s/%s.zip", dir_name, gamename);
				LOG(("Trying %s file\n", name));
				if( cache_stat (name, &stat_buffer) == 0 )
				{
					if( load_zipped_file (name, filename, &f->data, &f->length) == 0 )
					{
						LOG(("Using (osd_fopen) zip file for %s\n", filename));
						f->type = kZippedFile;
						f->offset = 0;
						f->crc = crc32 (0L, f->data, f->length);
						found = 1;
					}
				}
			}

			if( !found )
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf (name, "%s/%s.zip", dir_name, gamename);
				LOG(("Trying %s directory\n", name));
				if( cache_stat (name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR) )
				{
					sprintf (name, "%s/%s.zip/%s", dir_name, gamename, filename);
					if( filetype == OSD_FILETYPE_ROM )
					{
						if( checksum_file (name, &f->data, &f->length, &f->crc) == 0 )
						{
							f->type = kRAMFile;
							f->offset = 0;
							found = 1;
						}
					}
					else
					{
						f->type = kPlainFile;
						f->file = fopen (name, "rb");
						found = f->file != 0;
					}
				}
			}
		}

		break;

#ifdef MESS
	case OSD_FILETYPE_IMAGE:
		{
			extern char *renamed_image;	/* HACK */
			static char *write_modes[] = {"rb","wb","r+b","r+b","w+b"};
			ZIP *z;
			struct zipent *ent;
			int file_exists;
			const char *first_entry_name;

			if (renamed_image)
			{
				free(renamed_image);
				renamed_image = NULL;
			}

			if (is_absolute_path(filename))
			{
				LOG(("osd_fopen: using absolute path\n"));
				pathc = 1;
				pathv = NULL;
			}
			else
			{
				LOG(("osd_fopen: using rompath\n"));
				pathc = swpathc;
				pathv = swpathv;
			}

			LOG(("Open IMAGE '%s' for %s\n", filename, game));

			/* use the software paths */
			for (indx = 0; indx < pathc && !found; indx++)
			{
				if (pathv)
					sprintf(name, "%s%s/%s/%s", is_absolute_path(pathv[indx]) ? "" : "./", pathv[indx], game, filename);
				else
					strcpy(name, filename);

				/* we have a name; does this match a real file? */
				file_exists = (cache_stat(name,&stat_buffer) == 0) && !(stat_buffer.st_mode & S_IFDIR);

				if (is_zipfile(name))
				{
					/* we can only open bare zip paths for reading, and the zip has to be there */
					if (openforwrite || !file_exists)
						continue;

					/* a bare zip file was selected; so all we can do here is tranform
					 * the name to a full zip name (i.e. a name with a zip path at the end
					 */
					z = openzip(name);
					if (!z)
						continue;

					ent = readzip(z);
					if (!ent)
					{
						closezip(z);
						continue;
					}

					first_entry_name = ent->name;

					/* if the first entry name has a './' in the front of it, remove all occurances */
					while((first_entry_name[0] == '.') && (first_entry_name[1] == '/'))
						first_entry_name += 2;

					strcat(name, "/");
					strcat(name, first_entry_name);
					closezip(z);

					renamed_image = strdup(name);
					LOG(("osd_fopen: opened a raw zip file; renaming as '%s'\n", name));
				}
				else
				{
					/* a real, non-zip file */
					if (openforwrite)
					{
						/* a real, non-zip file, for writing! */
						f->file = fopen(name, write_modes[openforwrite]);
						found = f->file != 0;
						if( !found && openforwrite == OSD_FOPEN_RW_CREATE )
						{
							f->type = kPlainFile;
							f->file = fopen(name, write_modes[4]);
							found = f->file != 0;
						}
					}
					else if (file_exists)
					{
						/* a real, non-zip file, read only */
						if (checksum_file(name, &f->data, &f->length, &f->crc) == 0)
						{
							f->type = kRAMFile;
							f->offset = 0;
							found = 1;
						}
					}
				}

				/* at this point, we have a path that does not correspond to a real file; move
				 * up the filepath to see if we can find a zip (assuming we are opening for 
				 * read only).
				 */
				if (!found && !openforwrite)
				{
					char *s;
					char c;
					int done = 0;

					s = name + strlen(name) - 1;

					while(!done)
					{
						while(!is_path_separator(*s) && (s > name))
							s--;

						if (s <= name)
						{
							/* can't go up anymore */
							done = 1;
						}
						else
						{
							c = *s;
							*s = '\0';

							if (cache_stat(name, &stat_buffer) == 0)
							{
								/* we found something; we are done with the tranversal */
								done = 1;
								if (!(stat_buffer.st_mode & S_IFDIR) && is_zipfile(name))
								{
									/* we have a zipfile, with a partial path */
									if (load_zipped_file(name, s + 1, &f->data, &f->length) == 0)
									{
										LOG(("Using (osd_fopen) zip file for %s\n", filename));
										f->type = kZippedFile;
										f->offset = 0;
										f->crc = crc32 (0L, f->data, f->length);
										found = 1;
									}
								}
							}
							else
							{
								*(s--) = c;
							}
						}
					}
				}
			}
		}
		break;

#endif	/* MESS */


	case OSD_FILETYPE_NVRAM:
		if( !found )
		{
			sprintf (name, "%s/%s.nv", nvdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf (name, "%s.zip/%s.nv", nvdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s.zif/%s.nv", nvdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}
		break;

	case OSD_FILETYPE_HIGHSCORE:
		if( mame_highscore_enabled () )
		{
			if( !found )
			{
				sprintf (name, "%s/%s.hi", hidir, gamename);
				f->type = kPlainFile;
				f->file = fopen (name, openforwrite ? "wb" : "rb");
				found = f->file != 0;
			}

			if( !found )
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf (name, "%s.zip/%s.hi", hidir, gamename);
				f->type = kPlainFile;
				f->file = fopen (name, openforwrite ? "wb" : "rb");
				found = f->file != 0;
			}

			if( !found )
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf (name, "%s.zif/%s.hi", hidir, gamename);
				f->type = kPlainFile;
				f->file = fopen (name, openforwrite ? "wb" : "rb");
				found = f->file != 0;
			}
		}
		break;

	case OSD_FILETYPE_CONFIG:
		sprintf (name, "%s/%s.cfg", cfgdir, gamename);
		f->type = kPlainFile;
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = f->file != 0;

		if( !found )
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf (name, "%s.zip/%s.cfg", cfgdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s.zif/%s.cfg", cfgdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}
		break;

	case OSD_FILETYPE_INPUTLOG:
		sprintf (name, "%s/%s.inp", inpdir, gamename);
		f->type = kPlainFile;
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = f->file != 0;

		if( !found )
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf (name, "%s.zip/%s.cfg", inpdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s.zif/%s.cfg", inpdir, gamename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !openforwrite )
		{
			char file[256];
			sprintf (file, "%s.inp", gamename);
			sprintf (name, "%s/%s.zip", inpdir, gamename);
			LOG(("Trying %s in %s\n", file, name));
			if( cache_stat (name, &stat_buffer) == 0 )
			{
				if( load_zipped_file (name, file, &f->data, &f->length) == 0 )
				{
					LOG(("Using (osd_fopen) zip file %s for %s\n", name, file));
					f->type = kZippedFile;
					f->offset = 0;
					found = 1;
				}
			}
		}

		break;

	case OSD_FILETYPE_STATE:
		sprintf (name, "%s/%s.sta", stadir, gamename);
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = !(f->file == 0);
		if( !found )
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf (name, "%s.zip/%s.sta", stadir, gamename);
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = !(f->file == 0);
		}
		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s.zif/%s.sta", stadir, gamename);
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = !(f->file == 0);
		}
		break;

	case OSD_FILETYPE_ARTWORK:
		/* only for reading */
		if( openforwrite )
		{
			logerror("osd_fopen: type %02x write not supported\n",filetype);
			break;
		}
		sprintf (name, "%s/%s", artworkdir, filename);
		f->type = kPlainFile;
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = f->file != 0;
		if( !found )
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf (name, "%s.zip/%s.png", artworkdir, filename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s.zif/%s.png", artworkdir, filename);
			f->type = kPlainFile;
			f->file = fopen (name, openforwrite ? "wb" : "rb");
			found = f->file != 0;
		}

		if( !found )
		{
			char file[256], *extension;
			sprintf(file, "%s", filename);
			sprintf(name, "%s/%s", artworkdir, filename);
			extension = strrchr(name, '.');
			if( extension )
				strcpy (extension, ".zip");
			else
				strcat (name, ".zip");
			LOG(("Trying %s in %s\n", file, name));
			if( cache_stat (name, &stat_buffer) == 0 )
			{
				if( load_zipped_file (name, file, &f->data, &f->length) == 0 )
				{
					LOG(("Using (osd_fopen) zip file %s\n", name));
					f->type = kZippedFile;
					f->offset = 0;
					found = 1;
				}
			}
			if( !found )
			{
				sprintf(name, "%s/%s.zip", artworkdir, game);
				LOG(("Trying %s in %s\n", file, name));
				if( cache_stat (name, &stat_buffer) == 0 )
				{
					if( load_zipped_file (name, file, &f->data, &f->length) == 0 )
					{
						LOG(("Using (osd_fopen) zip file %s\n", name));
						f->type = kZippedFile;
						f->offset = 0;
						found = 1;
					}
				}
			}
		}
		break;

	case OSD_FILETYPE_MEMCARD:
		sprintf (name, "%s/%s", memcarddir, filename);
		f->type = kPlainFile;
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = f->file != 0;
		break;

	case OSD_FILETYPE_SCREENSHOT:
		/* only for writing */
		if( !openforwrite )
		{
			logerror("osd_fopen: type %02x read not supported\n",filetype);
			break;
		}

		sprintf (name, "%s/%s.png", screenshotdir, filename);
		f->type = kPlainFile;
		f->file = fopen (name, openforwrite ? "wb" : "rb");
		found = f->file != 0;
		break;

	case OSD_FILETYPE_HIGHSCORE_DB:
		/* only for reading */
		if( openforwrite )
		{
			logerror("osd_fopen: type %02x write not supported\n",filetype);
			break;
		}
		f->type = kPlainFile;
		/* open as ASCII files, not binary like the others */
		f->file = fopen (filename, openforwrite ? "w" : "r");
		found = f->file != 0;
		break;

	case OSD_FILETYPE_HISTORY:
		/* only for reading */
		if( openforwrite )
		{
			logerror("osd_fopen: type %02x write not supported\n",filetype);
			break;
		}
		f->type = kPlainFile;
		/* open as _binary_ like the others */
		f->file = fopen (filename, openforwrite ? "wb" : "rb");
		found = f->file != 0;
		break;


	/* Steph */
	case OSD_FILETYPE_CHEAT:
		sprintf (name, "%s/%s", cheatdir, filename);
		f->type = kPlainFile;
		/* open as ASCII files, not binary like the others */
		f->file = fopen (filename, openforwrite ? "a" : "r");
		found = f->file != 0;
		break;

	case OSD_FILETYPE_LANGUAGE:
		/* only for reading */
		if( openforwrite )
		{
			logerror("osd_fopen: type %02x write not supported\n",filetype);
			break;
		}
		sprintf (name, "%s.lng", filename);
		f->type = kPlainFile;
		/* open as ASCII files, not binary like the others */
		f->file = fopen (name, openforwrite ? "w" : "r");
		found = f->file != 0;
		logerror("fopen %s = %08x\n",name,(int)f->file);
		break;

	default:
		logerror("osd_fopen(): unknown filetype %02x\n",filetype);
	}

	if( !found )
	{
		free (f);
		return 0;
	}

	if( scratch1 )
		free( scratch1 );
	if( scratch2 )
		free( scratch2 );

	return f;
}

/* JB 980920 update */
int osd_fread (void *file, void *buffer, int length)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch( f->type )
	{
	case kPlainFile:
		return fread (buffer, 1, length, f->file);
		break;
	case kZippedFile:
	case kRAMFile:
		/* reading from the RAM image of a file */
		if( f->data )
		{
			if ((f->eof=(length + f->offset > f->length)) )
				length = f->length - f->offset;
			memcpy (buffer, f->offset + f->data, length);
			f->offset += length;
			return length;
		}
		break;
	}

	return 0;
}

int osd_fread_swap (void *file, void *buffer, int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;


	res = osd_fread (file, buffer, length);

	buf = buffer;
	for( i = 0; i < length; i += 2 )
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}


/* AM 980919 update */
int osd_fwrite (void *file, const void *buffer, int length)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch( f->type )
	{
	case kPlainFile:
		return fwrite (buffer, 1, length, ((FakeFileHandle *) file)->file);
	default:
		return 0;
	}
}

int osd_fwrite_swap (void *file, const void *buffer, int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;


	buf = (unsigned char *) buffer;
	for( i = 0; i < length; i += 2 )
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	res = osd_fwrite (file, buffer, length);

	for( i = 0; i < length; i += 2 )
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}

int osd_fread_scatter (void *file, void *buffer, int length, int increment)
{
	unsigned char *buf = buffer;
	FakeFileHandle *f = (FakeFileHandle *) file;
	unsigned char tempbuf[4096];
	int totread, r, i;

	switch( f->type )
	{
	case kPlainFile:
		totread = 0;
		while (length)
		{
			r = length;
			if( r > 4096 )
				r = 4096;
			r = fread (tempbuf, 1, r, f->file);
			if( r == 0 )
				return totread;		/* error */
			for( i = 0; i < r; i++ )
			{
				*buf = tempbuf[i];
				buf += increment;
			}
			totread += r;
			length -= r;
		}
		return totread;
		break;
	case kZippedFile:
	case kRAMFile:
		/* reading from the RAM image of a file */
		if( f->data )
		{
			if( length + f->offset > f->length )
				length = f->length - f->offset;
			for( i = 0; i < length; i++ )
			{
				*buf = f->data[f->offset + i];
				buf += increment;
			}
			f->offset += length;
			return length;
		}
		break;
	}

	return 0;
}


/* JB 980920 update */
int osd_fseek (void *file, int offset, int whence)
{
	FakeFileHandle *f = (FakeFileHandle *) file;
	int err = 0;

	switch( f->type )
	{
	case kPlainFile:
		return fseek (f->file, offset, whence);
		break;
	case kZippedFile:
	case kRAMFile:
		/* seeking within the RAM image of a file */
		switch( whence )
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

/* JB 980920 update */
void osd_fclose (void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch( f->type )
	{
	case kPlainFile:
		fclose (f->file);
		break;
	case kZippedFile:
	case kRAMFile:
		if( f->data )
			free (f->data);
		break;
	}
	free (f);
}

/* JB 980920 update */
/* AM 980919 */
static int checksum_file (const char *file, unsigned char **p, unsigned int *size, unsigned int *crc)
{
	int length;
	unsigned char *data;
	FILE *f;

	f = fopen (file, "rb");
	if( !f )
		return -1;

	/* determine length of file */
	if( fseek (f, 0L, SEEK_END) != 0 )
	{
		fclose (f);
		return -1;
	}

	length = ftell (f);
	if( length == -1L )
	{
		fclose (f);
		return -1;
	}

	/* allocate space for entire file */
	data = (unsigned char *) malloc (length);
	if( !data )
	{
		fclose (f);
		return -1;
	}

	/* read entire file into memory */
	if( fseek (f, 0L, SEEK_SET) != 0 )
	{
		free (data);
		fclose (f);
		return -1;
	}

	if( fread (data, sizeof (unsigned char), length, f) != length )
	{
		free (data);
		fclose (f);
		return -1;
	}

	*size = length;
	*crc = crc32 (0L, data, length);
	if( p )
		*p = data;
	else
		free (data);

	fclose (f);

	return 0;
}

/* JB 980920 updated */
/* AM 980919 updated */
int osd_fchecksum (const char *game, const char *filename, unsigned int *length, unsigned int *sum)
{
	char name[256];
	int indx;
	struct stat stat_buffer;
	int found = 0;
	const char *gamename = game;

	decompose_paths_if_needed();

	/* Support "-romdir" yuck. */
	if( alternate_name )
		gamename = alternate_name;

	for( indx = 0; indx < rompathc && !found; ++indx )
	{
		const char *dir_name = rompathv[indx];

		if( !found )
		{
			sprintf (name, "%s/%s", dir_name, gamename);
			if( cache_stat (name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR) )
			{
				sprintf (name, "%s/%s/%s", dir_name, gamename, filename);
				if( checksum_file (name, 0, length, sum) == 0 )
				{
					found = 1;
				}
			}
		}

		if( !found )
		{
			/* try with a .zip extension */
			sprintf (name, "%s/%s.zip", dir_name, gamename);
			if( cache_stat (name, &stat_buffer) == 0 )
			{
				if( checksum_zipped_file (name, filename, length, sum) == 0 )
				{
					LOG(("Using (osd_fchecksum) zip file for %s\n", filename));
					found = 1;
				}
			}
		}

		if( !found )
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf (name, "%s/%s.zif", dir_name, gamename);
			if( cache_stat (name, &stat_buffer) == 0 )
			{
				sprintf (name, "%s/%s.zif/%s", dir_name, gamename, filename);
				if( checksum_file (name, 0, length, sum) == 0 )
				{
					found = 1;
				}
			}
		}
	}

	if( !found )
		return -1;

	return 0;
}

/* JB 980920 */
int osd_fsize (void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if( f->type == kRAMFile || f->type == kZippedFile )
		return f->length;

	if( f->file )
	{
		int size, offs;
		offs = ftell( f->file );
		fseek( f->file, 0, SEEK_END );
		size = ftell( f->file );
		fseek( f->file, offs, SEEK_SET );
		return size;
	}

	return 0;
}

/* JB 980920 */
unsigned int osd_fcrc (void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	return f->crc;
}

int osd_fgetc(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return fgetc(f->file);
	else
		return EOF;
}

int osd_ungetc(int c, void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return ungetc(c,f->file);
	else
		return EOF;
}

char *osd_fgets(char *s, int n, void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return fgets(s,n,f->file);
	else
		return NULL;
}

int osd_feof(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return feof(f->file);
	else if (f->type == kRAMFile)
	{
		if (!f->data) return 1;
		return f->eof;
	}
	else
		return 1;
}

int osd_ftell(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	if (f->type == kPlainFile && f->file)
		return ftell(f->file);
	else
		return -1L;
}

char *osd_basename (char *filename)
{
	char *c;

	if (!filename)
		return NULL;

	c = filename + strlen(filename);

	while (c != filename)
	{
		c--;
		if (*c == '\\' || *c == '/' || *c == ':')
			return (c+1);
	}

	return filename;
}

char *osd_dirname (char *filename)
{
	char *dirname;
	char *c;
	int found = 0;

	if (!filename)
		return NULL;

	if ( !( dirname = malloc(strlen(filename)+1) ) )
	{
		fprintf(stderr, "error: malloc failed in osd_dirname\n");
		return 0;
	}

	strcpy (dirname, filename);

	c = dirname + strlen(dirname);
	while (c != dirname)
	{
		--c;
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			*(c+1)=0;
			found = 1;
			break;
		}
	}

	/* did we find a path seperator? */
	if (!found)
		dirname[0]=0;

	return dirname;
}

char *osd_strip_extension (char *filename)
{
	char *newname;
	char *c;

	if (!filename)
		return NULL;

	if ( !( newname = malloc(strlen(filename)+1) ) )
	{
		fprintf(stderr, "error: malloc failed in osd_newname\n");
		return 0;
	}

	strcpy (newname, filename);

	c = newname + strlen(newname);
	while (c != newname)
	{
		--c;
		if (*c == '.')
			*c = 0;
		if (*c == '\\' || *c == '/' || *c == ':')
			break;
	}

	return newname;
}


void build_crc_database_filename(int game_index)
{
	/* Build the CRC database filename */
	sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
	if (drivers[game_index]->clone_of->name)
		sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
	else
		pcrcfilename[0] = 0;
}
