#include "xmame.h"
#include "audit.h"
#include "unzip.h"
#include "driver.h"
#include <dirent.h>

#ifdef BSD43 /* old style directory handling */
#include <sys/types.h>
#include <sys/dir.h>
#define dirent direct
#endif

/* #define IDENT_DEBUG */

unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);
void romident(const char* name, int enter_dirs);

enum { KNOWN_START, KNOWN_ALL, KNOWN_NONE, KNOWN_SOME };

static int silentident = 0;
static int knownstatus = KNOWN_START;
static int ident = 0;

enum { IDENT_IDENT = 1, IDENT_ISKNOWN };

struct rc_option frontend_ident_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Rom Identification Related", NULL,	rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "ident",		"id",			rc_set_int,	&ident,
     NULL,		IDENT_IDENT,		0,		NULL,
     "Identify unknown romdump, or unknown romdumps in dir/zip" },
   { "isknown",		"ik",			rc_set_int,	&ident,
     NULL,		IDENT_ISKNOWN,		0,		NULL,
     "Check if romdump or romdumps in dir/zip are known"} ,
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};


/* Identifies a rom from from this checksum */
void identify_rom(const char* name, int checksum, int length)
{
/* Nicola output format */
#if 1
	int found = 0;
	int i;

	/* remove directory name */
	for (i = strlen(name)-1;i >= 0;i--)
	{
		if (name[i] == '/' || name[i] == '\\')
		{
			i++;
			break;
		}
	}
	if (!silentident)
		fprintf(stdout_file, "%-12s ",&name[i]);

	for (i = 0; drivers[i]; i++)
	{
		const struct RomModule *region, *rom;

                for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
                    for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
                    {
                        if (checksum == ROM_GETCRC(rom))
                        {
                            if (!silentident)
                            {
                                if (found != 0)
                                   fprintf(stdout_file, "             ");
				fprintf(stdout_file, "= %-12s %s\n",ROM_GETNAME(rom),drivers[i]->description);
                            }
                            found++;
                        }
		    }
	}
	if (found == 0)
	{
		unsigned size = length;
		while (size && (size & 1) == 0) size >>= 1;
		if (size & ~1)
		{
			if (!silentident)
				fprintf(stdout_file, "NOT A ROM\n");
		}
		else
		{
			if (!silentident)
				fprintf(stdout_file, "NO MATCH\n");
			if (knownstatus == KNOWN_START)
				knownstatus = KNOWN_NONE;
			else if (knownstatus == KNOWN_ALL)
				knownstatus = KNOWN_SOME;
		}
	}
	else
	{
		if (knownstatus == KNOWN_START)
			knownstatus = KNOWN_ALL;
		else if (knownstatus == KNOWN_NONE)
			knownstatus = KNOWN_SOME;
	}
#else
/* New output format */
	int i;
	fprintf(stdout_file, "%s\n",name);

	for (i = 0; drivers[i]; i++)
        {
		const struct RomModule *region, *rom;

                for (region = rom_first_region(drivers[i]; region; region = rom_next_region(region))
                    for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
                       if (checksum == ROM_GETCRC(rom)) 
                       {
				fprintf(stdout_file, "\t%s/%s %s, %s, %s\n",drivers[i]->name,romp->name,
					drivers[i]->description,
					drivers[i]->manufacturer,
					drivers[i]->year);
                       }
	}
#endif
}

/* Identifies a file from from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	char* data;
#ifdef IDENT_DEBUG
	fprintf(stderr_file, "identify_file(%s) called\n", name);
#endif

	f = fopen(name,"rb");
	if (!f) {
		return;
	}

	/* determine length of file */
	if (fseek (f, 0L, SEEK_END)!=0)	{
		fclose(f);
		return;
	}

	length = ftell(f);
	if (length == -1L) {
		fclose(f);
		return;
	}

	/* empty file */
	if (!length) {
		fclose(f);
		return;
	}

	/* allocate space for entire file */
	data = (char*)malloc(length);
	if (!data) {
		fclose(f);
		return;
	}

	if (fseek (f, 0L, SEEK_SET)!=0) {
		free(data);
		fclose(f);
		return;
	}

	if (fread(data, 1, length, f) != length) {
		free(data);
		fclose(f);
		return;
	}

	fclose(f);

	identify_rom(name, crc32(0L,(const unsigned char*)data,length), length);

	free(data);
}

void identify_zip(const char* zipname)
{
	struct zipent* ent;
	ZIP* zip = openzip( zipname );
#ifdef IDENT_DEBUG
	fprintf(stderr_file, "identify_zip(%s) called\n", zipname);
#endif

	if (!zip)
		return;

	while ((ent = readzip(zip))) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			sprintf(buf,"%s/%s",zipname,ent->name);
			identify_rom(buf,ent->crc32,ent->uncompressed_size);
			free(buf);
		}
	}

	closezip(zip);
}

void identify_dir(const char* dirname)
{
	DIR *dir;
	struct dirent *ent;
#ifdef IDENT_DEBUG
	fprintf(stderr_file, "identdir(%s) called\n", dirname);
#endif

	dir = opendir(dirname);
	if (!dir) {
		return;
	}

	ent = readdir(dir);
	while (ent) {
		/* Skip special files */
		if (ent->d_name[0]!='.') {
			char* buf = (char*)malloc(strlen(dirname)+1+strlen(ent->d_name)+1);
			sprintf(buf,"%s/%s",dirname,ent->d_name);
			romident(buf,0);
			free(buf);
		}

		ent = readdir(dir);
	}
	closedir(dir);
}

void romident(const char* name,int enter_dirs) {
	struct stat s;
#ifdef IDENT_DEBUG
	fprintf(stderr_file, "romident(%s, %d) called\n", name, enter_dirs);
#endif

	if (stat(name,&s) != 0)	{
		fprintf(stdout_file, "%s: %s\n",name,strerror(errno));
		return;
	}

#ifdef BSD43
	if (S_IFDIR & s.st_mode) {
#else
	if (S_ISDIR(s.st_mode)) {
#endif
		if (enter_dirs)
			identify_dir(name);
	} else {
		unsigned l = strlen(name);
		if (l>=4 && stricmp(name+l-4,".zip")==0)
			identify_zip(name);
		else
			identify_file(name);
		return;
	}
}

int frontend_ident(char *gamename)
{
#ifdef IDENT_DEBUG
   fprintf(stderr_file, "frontend_ident(%d, %s) called\n", ident, gamename);
#endif

   if (!ident)
      return 1234;
   
   if (!gamename)
   {
      fprintf(stderr_file, "-ident / -isknown requires a game- or filename as second argument\n");
      return OSD_NOT_OK;
   }
   
   if (ident == IDENT_ISKNOWN)
         silentident = 1;
         
   romident(gamename, 1);
   
   if (ident == IDENT_ISKNOWN)
   {
      switch (knownstatus)
      {
         case KNOWN_START: fprintf(stdout_file, "ERROR     %s\n",gamename); break;
         case KNOWN_ALL:   fprintf(stdout_file, "KNOWN     %s\n",gamename); break;
         case KNOWN_NONE:  fprintf(stdout_file, "UNKNOWN   %s\n",gamename); break;
         case KNOWN_SOME:  fprintf(stdout_file, "PARTKNOWN %s\n",gamename); break;
      }
   }
   return OSD_OK;
}
