#include "xmame.h"
#include "driver.h"
#include "info.h"
#include "audit.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unzip.h>

#ifdef MESS
#include <dirent.h>
void list_mess_info(const char *gamename, const char *arg, int listclones);
#endif

#ifndef MESS
enum { LIST_SHORT = 1, LIST_INFO, LIST_FULL, LIST_SAMDIR, LIST_ROMS, LIST_SAMPLES,
		LIST_LMR, LIST_DETAILS, LIST_GAMELIST,
		LIST_GAMES, LIST_CLONES,
		LIST_WRONGORIENTATION, LIST_WRONGFPS, LIST_CRC, LIST_SHA1, LIST_MD5, LIST_DUPCRC, 
		LIST_WRONGMERGE, LIST_ROMSIZE, LIST_ROMDISTRIBUTION, LIST_ROMNUMBER, LIST_PALETTESIZE,
		LIST_CPU, LIST_CPUCLASS, LIST_NOSOUND, LIST_SOUND, LIST_NVRAM, LIST_SOURCEFILE,
		LIST_GAMESPERSOURCEFILE };
#else
enum { LIST_SHORT = 1, LIST_INFO, LIST_FULL, LIST_SAMDIR, LIST_ROMS, LIST_SAMPLES,
		LIST_LMR, LIST_DETAILS, LIST_GAMELIST,
		LIST_GAMES, LIST_CLONES,
		LIST_WRONGORIENTATION, LIST_WRONGFPS, LIST_CRC, LIST_SHA1, LIST_MD5, LIST_DUPCRC, LIST_WRONGMERGE,
		LIST_ROMSIZE, LIST_ROMDISTRIBUTION, LIST_ROMNUMBER, LIST_PALETTESIZE,
		LIST_CPU, LIST_CPUCLASS, LIST_NOSOUND, LIST_SOUND, LIST_NVRAM, LIST_SOURCEFILE,
		LIST_GAMESPERSOURCEFILE, LIST_MESSTEXT, LIST_MESSDEVICES, LIST_MESSCREATEDIR };
#endif

#define VERIFY_ROMS		0x00000001
#define VERIFY_SAMPLES	0x00000002
#define VERIFY_VERBOSE	0x00000004
#define VERIFY_TERSE	0x00000008

#define KNOWN_START 0
#define KNOWN_ALL   1
#define KNOWN_NONE  2
#define KNOWN_SOME  3

#ifndef MESS
#define YEAR_BEGIN 1975
#define YEAR_END   2000
#else
#define YEAR_BEGIN 1950
#define YEAR_END   2000
#endif

static int list = 0;
static int listclones = 1;
static int verify = 0;
static int ident = 0;
static int help = 0;
static int sortby = 0;

struct rc_option frontend_opts[] = {
	{ "Frontend Related", NULL,	rc_seperator, NULL, NULL, 0, 0,	NULL, NULL },

	{ "help", "h", rc_set_int, &help, NULL, 1, 0, NULL, "show help message" },
	{ "?", NULL,   rc_set_int, &help, NULL, 1, 0, NULL, "show help message" },

	/* list options follow */
	{ "list", "ls", rc_set_int,	&list, NULL, LIST_SHORT, 0, NULL, "List supported games matching gamename, or all, gamename may contain * and ? wildcards" },
	{ "listfull", "ll", rc_set_int,	&list, NULL, LIST_FULL,	0, NULL, "short name, full name" },
	{ "listgames", NULL, rc_set_int, &list, NULL, LIST_GAMES, 0, NULL, "year, manufacturer and full name" },
	{ "listdetails", NULL, rc_set_int, &list, NULL, LIST_DETAILS, 0, NULL, "detailed info" },
	{ "gamelist", NULL, rc_set_int, &list, NULL, LIST_GAMELIST, 0, NULL, "output gamelist.txt main body" },
	{ "listsourcefile",	NULL, rc_set_int, &list, NULL, LIST_SOURCEFILE, 0, NULL, "driver sourcefile" },
	{ "listgamespersourcefile",	NULL, rc_set_int, &list, NULL, LIST_GAMESPERSOURCEFILE, 0, NULL, "games per sourcefile" },
	{ "listinfo", "li", rc_set_int, &list, NULL, LIST_INFO, 0, NULL, "all available info on driver" },
	{ "listclones", "lc", rc_set_int, &list, NULL, LIST_CLONES, 0, NULL, "show clones" },
	{ "listsamdir", NULL, rc_set_int, &list, NULL, LIST_SAMDIR, 0, NULL, "shared sample directory" },
	{ "listcrc", NULL, rc_set_int, &list, NULL, LIST_CRC, 0, NULL, "CRC-32s" },
	{ "listsha1", NULL, rc_set_int, &list, NULL, LIST_SHA1, 0, NULL, "SHA-1s" },
	{ "listmd5", NULL, rc_set_int, &list, NULL, LIST_MD5, 0, NULL, "MD5s" },
	{ "listdupcrc", NULL, rc_set_int, &list, NULL, LIST_DUPCRC, 0, NULL, "duplicate crc's" },
	{ "listwrongmerge", "lwm", rc_set_int, &list, NULL, LIST_WRONGMERGE, 0, NULL, "wrong merge attempts" },
	{ "listromsize", NULL, rc_set_int, &list, NULL, LIST_ROMSIZE, 0, NULL, "rom size" },
	{ "listromdistribution", NULL, rc_set_int, &list, NULL, LIST_ROMDISTRIBUTION, 0, NULL, "rom distribution" },
	{ "listromnumber", NULL, rc_set_int, &list, NULL, LIST_ROMNUMBER, 0, NULL, "rom size" },
	{ "listpalettesize", "lps", rc_set_int, &list, NULL, LIST_PALETTESIZE, 0, NULL, "palette size" },
	{ "listcpu", NULL, rc_set_int, &list, NULL, LIST_CPU, 0, NULL, "cpu's used" },
	{ "listcpuclass", NULL, rc_set_int, &list, NULL, LIST_CPUCLASS, 0, NULL, "class of cpu's used by year" },
	{ "listnosound", NULL, rc_set_int, &list, NULL, LIST_NOSOUND, 0, NULL, "drivers missing sound support" },
	{ "listsound", NULL, rc_set_int, &list, NULL, LIST_SOUND, 0, NULL, "sound chips used" },
	{ "listnvram",	NULL, rc_set_int, &list, NULL, LIST_NVRAM, 0, NULL, "games with nvram" },
#ifdef MAME_DEBUG /* do not put this into a public release! */
	{ "lmr", NULL, rc_set_int, &list, NULL, LIST_LMR, 0, NULL, "missing roms" },
#endif
	{ "wrongorientation", NULL, rc_set_int, &list, NULL, LIST_WRONGORIENTATION, 0, NULL, "wrong orientation" },
	{ "wrongfps", NULL, rc_set_int, &list, NULL, LIST_WRONGFPS, 0, NULL, "wrong fps" },
	{ "clones", NULL, rc_bool, &listclones, "1", 0, 0, NULL, "enable/disable clones" },
#ifdef MESS
	{ "listdevices", NULL, rc_set_int, &list, NULL, LIST_MESSDEVICES, 0, NULL, "list available devices" },
	{ "listtext", NULL, rc_set_int, &list, NULL, LIST_MESSTEXT, 0, NULL, "list available file extensions" },
	{ "createdir", NULL, rc_set_int, &list, NULL, LIST_MESSCREATEDIR, 0, NULL, NULL },
#endif
	{ "listroms", NULL, rc_set_int, &list, NULL, LIST_ROMS, 0, NULL, "list required roms for a driver" },
	{ "listsamples", NULL, rc_set_int, &list, NULL, LIST_SAMPLES, 0, NULL, "list optional samples for a driver" },
	{ "verifyroms", NULL, rc_set_int, &verify, NULL, VERIFY_ROMS, 0, NULL, "report romsets that have problems" },
	{ "verifysets", NULL, rc_set_int, &verify, NULL, VERIFY_ROMS|VERIFY_VERBOSE|VERIFY_TERSE, 0, NULL, "verify checksums of romsets (terse)" },
	{ "vset", NULL, rc_set_int, &verify, NULL, VERIFY_ROMS|VERIFY_VERBOSE, 0, NULL, "verify checksums of a romset (verbose)" },
	{ "verifysamples", NULL, rc_set_int, &verify, NULL, VERIFY_SAMPLES|VERIFY_VERBOSE, 0, NULL, "report samplesets that have problems" },
	{ "vsam", NULL, rc_set_int, &verify, NULL, VERIFY_SAMPLES|VERIFY_VERBOSE, 0, NULL, "verify a sampleset" },
	{ "romident", NULL, rc_set_int, &ident, NULL, 1, 0, NULL, "compare files with known MAME roms" },
	{ "isknown", NULL, rc_set_int, &ident, NULL, 2, 0, NULL, "compare files with known MAME roms (brief)" },
	{ "sortname", NULL, rc_set_int, &sortby, NULL, 1, 0, NULL, "sort by descriptive name" },
	{ "sortdriver", NULL, rc_set_int, &sortby, NULL, 2, 0, NULL, "sort by driver" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};


static int silentident,knownstatus;

#ifdef _MSC_VER
#define ZEXPORT __stdcall
#else
#define ZEXPORT
#endif

extern unsigned int ZEXPORT crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

void get_rom_sample_path (int argc, char **argv, int game_index, char *override_default_rompath);

static const struct GameDriver *gamedrv;

/* compare string[8] using standard(?) DOS wildchars ('?' & '*')      */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled. Look into msdos.c */

int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return stricmp(s1, s2);
}


static void namecopy(char *name_ref,const char *desc)
{
	char name[200];

	strcpy(name,desc);

	/* remove details in parenthesis */
	if (strstr(name," (")) *strstr(name," (") = 0;

	/* Move leading "The" to the end */
	if (strncmp(name,"The ",4) == 0)
	{
		sprintf(name_ref,"%s, The",name+4);
	}
	else
		sprintf(name_ref,"%s",name);
}


/* Identifies a rom from from this checksum */
static void match_roms(const struct GameDriver *driver,const char* hash,int *found)
{
	const struct RomModule *region, *rom;

	for (region = rom_first_region(driver); region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			if (hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0))
			{
				char baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

				if (!silentident)
				{
					if (*found != 0)
						printf("             ");
					printf("= %s%-12s  %s\n",baddump ? "(BAD) " : "",ROM_GETNAME(rom),driver->description);
				}
				(*found)++;
			}
		}
	}
}


void identify_rom(const char* name, const char* hash, int length)
{
	int found = 0;

	/* remove directory name */
	int i;
	for (i = strlen(name)-1;i >= 0;i--)
	{
		if (name[i] == '/' || name[i] == '\\')
		{
			i++;
			break;
		}
	}
	if (!silentident)
		printf("%s ",&name[0]);

	for (i = 0; drivers[i]; i++)
		match_roms(drivers[i],hash,&found);

	for (i = 0; test_drivers[i]; i++)
		match_roms(test_drivers[i],hash,&found);

	if (found == 0)
	{
		unsigned size = length;
		while (size && (size & 1) == 0) size >>= 1;
		if (size & ~1)
		{
			if (!silentident)
				printf("NOT A ROM\n");
		}
		else
		{
			if (!silentident)
				printf("NO MATCH\n");
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
}

/* Identifies a file from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	char* data;
	char hash[HASH_BUF_SIZE];

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

	/* Compute checksum of all the available functions. Since MAME for
	   now carries inforamtions only for CRC and SHA1, we compute only
	   these */
	if (options.crc_only)
		hash_compute(hash, data, length, HASH_CRC);
	else
		hash_compute(hash, data, length, HASH_CRC|HASH_SHA1);
	
	/* Try to identify the ROM */
	identify_rom(name, hash, length);

	free(data);
}

void identify_zip(const char* zipname)
{
	struct zipent* ent;

	ZIP* zip = openzip( FILETYPE_RAW, 0, zipname );
	if (!zip)
		return;

	while ((ent = readzip(zip))) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			char hash[HASH_BUF_SIZE];
			UINT8 crcs[4];

//			sprintf(buf,"%s/%s",zipname,ent->name);
			sprintf(buf,"%-12s",ent->name);

			/* Decompress the ROM from the ZIP, and compute all the needed 
			   checksums. Since MAME for now carries informations only for CRC and
			   SHA1, we compute only these (actually, CRC is extracted from the
			   ZIP header) */
			hash_data_clear(hash);

			if (!options.crc_only)
			{
				UINT8* data =  (UINT8*)malloc(ent->uncompressed_size);
				readuncompresszip(zip, ent, data);
				hash_compute(hash, data, ent->uncompressed_size, HASH_SHA1);
				free(data);
			}
			
			crcs[0] = (UINT8)(ent->crc32 >> 24);
			crcs[1] = (UINT8)(ent->crc32 >> 16);
			crcs[2] = (UINT8)(ent->crc32 >> 8);
			crcs[3] = (UINT8)(ent->crc32 >> 0);
			hash_data_insert_binary_checksum(hash, HASH_CRC, crcs);

			/* Try to identify the ROM */
			identify_rom(buf, hash, ent->uncompressed_size);

			free(buf);
		}
	}

	closezip(zip);
}

void romident(const char* name, int enter_dirs);

void identify_dir(const char* dirname)
{
	DIR *dir;
	struct dirent *ent;

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

	if (stat(name,&s) != 0)	{
		printf("%s: %s\n",name,strerror(errno));
		return;
	}

	if (S_ISDIR(s.st_mode)) {
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


void CLIB_DECL terse_printf(const char *fmt,...)
{
	/* no-op */
}


int CLIB_DECL compare_names(const void *elem1, const void *elem2)
{
	struct GameDriver *drv1 = *(struct GameDriver **)elem1;
	struct GameDriver *drv2 = *(struct GameDriver **)elem2;
	char name1[200],name2[200];
	namecopy(name1,drv1->description);
	namecopy(name2,drv2->description);
	return strcmp(name1,name2);
}


int CLIB_DECL compare_driver_names(const void *elem1, const void *elem2)
{
	struct GameDriver *drv1 = *(struct GameDriver **)elem1;
	struct GameDriver *drv2 = *(struct GameDriver **)elem2;
	return strcmp(drv1->name, drv2->name);
}


int frontend_help (const char *gamename)
{
	struct InternalMachineDriver drv;
	int i, j;
	const char *all_games = "*";

	/* display help unless a game or an utility are specified */
	if (!gamename && !help && !list && !ident && !verify)
		help = 1;

	if (help)  /* brief help - useful to get current version info */
	{
		#ifndef MESS
		printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
				"Copyright (C) 1997-2003 by Nicola Salmoria and the MAME Team\n\n",build_version);
		showdisclaimer();
		printf("Usage:  MAME gamename [options]\n\n"
				"        MAME -list         for a brief list of supported games\n"
				"        MAME -listfull     for a full list of supported games\n"
				"        MAME -showusage    for a brief list of options\n"
				"        MAME -showconfig   for a list of configuration options\n"
				"        MAME -createconfig to create a mame.ini\n\n"
				"See readme.txt for a complete list of options.\n");
		#else
		showmessinfo();
		#endif
		return 0;
	}

	/* HACK: some options REQUIRE gamename field to work: default to "*" */
	if (!gamename || (strlen(gamename) == 0))
		gamename = all_games;

	/* sort the list if requested */
	if (sortby)
	{
		int count = 0;

		/* first count the drivers */
		while (drivers[count]) count++;

		/* qsort as appropriate */
		if (sortby == 1)
			qsort(drivers, count, sizeof(drivers[0]), compare_names);
		else if (sortby == 2)
			qsort(drivers, count, sizeof(drivers[0]), compare_driver_names);
	}

	switch (list)  /* front-end utilities ;) */
	{

        #ifdef MESS
		case LIST_MESSTEXT: /* all mess specific calls here */
		{
					/* send the gamename and arg to mess.c */
			list_mess_info(gamename, "-listtext", listclones);
			return 0;
			break;
		}
		case LIST_MESSDEVICES:
			{
					/* send the gamename and arg to mess.c */
			list_mess_info(gamename, "-listdevices", listclones);
			return 0;
			break;
		}
		case LIST_MESSCREATEDIR:
			 	{
					/* send the gamename and arg to mess.c */
			list_mess_info(gamename, "-createdir", listclones);
			return 0;
			break;
		}
		#endif

		case LIST_SHORT: /* simple games list */
			#ifndef MESS
			printf("\nMAME currently supports the following games:\n\n");
			#else
			printf("\nMESS currently supports the following systems:\n\n");
			#endif
			for (i = j = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					printf("%-8s",drivers[i]->name);
					j++;
					if (!(j % 8)) printf("\n");
					else printf("  ");
				}
			if (j % 8) printf("\n");
			printf("\n");
			if (j != i) printf("Total ROM sets displayed: %4d - ", j);
			#ifndef MESS
			printf("Total ROM sets supported: %4d\n", i);
			#else
			printf("Total Systems supported: %4d\n", i);
			#endif
            return 0;
			break;

		case LIST_FULL: /* games list with descriptions */
			printf("Name:     Description:\n");
			for (i = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					char name[200];

					printf("%-10s",drivers[i]->name);

					namecopy(name,drivers[i]->description);
						printf("\"%s",name);

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							printf(" %s",strchr(drivers[i]->description,'('));
					}
					printf("\"\n");
				}
			return 0;
			break;

		case LIST_SAMDIR: /* games list with samples directories */
			printf("Name:     Samples dir:\n");
			for (i = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					expand_machine_driver(drivers[i]->drv, &drv);
#if (HAS_SAMPLES || HAS_VLM5030)
					for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
					{
						const char **samplenames = NULL;
#if (HAS_SAMPLES)
						if( drv.sound[j].sound_type == SOUND_SAMPLES )
							samplenames = ((struct Samplesinterface *)drv.sound[j].sound_interface)->samplenames;
#endif
						if (samplenames != 0 && samplenames[0] != 0)
						{
							printf("%-10s",drivers[i]->name);
							if (samplenames[0][0] == '*')
								printf("%s\n",samplenames[0]+1);
							else
								printf("%s\n",drivers[i]->name);
						}
					}
#endif
				}
			return 0;
			break;

		case LIST_ROMS: /* game roms list or */
		case LIST_SAMPLES: /* game samples list */
			j = 0;
			while (drivers[j] && (stricmp(gamename,drivers[j]->name) != 0))
				j++;
			if (drivers[j] == 0)
			{
				printf("Game \"%s\" not supported!\n",gamename);
				return 1;
			}
			gamedrv = drivers[j];
			if (list == LIST_ROMS)
				printromlist(gamedrv->rom,gamename);
			else
			{
#if (HAS_SAMPLES || HAS_VLM5030)
				int k;
				expand_machine_driver(gamedrv->drv, &drv);
				for( k = 0; drv.sound[k].sound_type && k < MAX_SOUND; k++ )
				{
					const char **samplenames = NULL;
#if (HAS_SAMPLES)
					if( drv.sound[k].sound_type == SOUND_SAMPLES )
							samplenames = ((struct Samplesinterface *)drv.sound[k].sound_interface)->samplenames;
#endif
					if (samplenames != 0 && samplenames[0] != 0)
					{
						i = 0;
						while (samplenames[i] != 0)
						{
							printf("%s\n",samplenames[i]);
							i++;
						}
					}
                }
#endif
			}
			return 0;
			break;

		case LIST_LMR:
			{
				int total;

				total = 0;
				for (i = 0; drivers[i]; i++)
						total++;
				for (i = 0; drivers[i]; i++)
				{
					static int first_missing = 1;
//					get_rom_sample_path (argc, argv, i, NULL);
					if (RomsetMissing (i))
					{
						if (first_missing)
						{
							first_missing = 0;
							printf ("game      clone of  description\n");
							printf ("--------  --------  -----------\n");
						}
						printf ("%-10s%-10s%s\n",
								drivers[i]->name,
								(drivers[i]->clone_of) ? drivers[i]->clone_of->name : "",
								drivers[i]->description);
					}
					fprintf(stderr,"%d%%\r",100 * (i+1) / total);
				}
			}
			return 0;
			break;

		case LIST_DETAILS: /* A detailed MAMELIST.TXT type roms lister */

			/* First, we shall print the header */

			printf(" romname driver     ");
			for(j=0;j<MAX_CPU;j++) printf("cpu %d    ",j+1);
			for(j=0;j<MAX_SOUND;j++) printf("sound %d     ",j+1);
			printf("name\n");
			printf("-------- ---------- ");
			for(j=0;j<MAX_CPU;j++) printf("-------- ");
			for(j=0;j<MAX_SOUND;j++) printf("----------- ");
			printf("--------------------------\n");

			/* Let's cycle through the drivers */

			for (i = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					/* Dummy structs to fetch the information from */

					const struct MachineCPU *x_cpu;
					const struct MachineSound *x_sound;
					struct InternalMachineDriver x_driver;

					expand_machine_driver(drivers[i]->drv, &x_driver);
					x_cpu = x_driver.cpu;
					x_sound = x_driver.sound;

					/* First, the rom name */

					printf("%-8s ",drivers[i]->name);

					#ifndef MESS
					/* source file (skip the leading "src/drivers/" */
					printf("%-10s ",&drivers[i]->source_file[12]);
					#else
					/* source file (skip the leading "src/mess/systems/" */
					printf("%-10s ",&drivers[i]->source_file[17]);
					#endif

					/* Then, cpus */

					for(j=0;j<MAX_CPU;j++)
					{
						if (x_cpu[j].cpu_flags & CPU_AUDIO_CPU)
							printf("[%-6s] ",cputype_name(x_cpu[j].cpu_type));
						else
							printf("%-8s ",cputype_name(x_cpu[j].cpu_type));
					}

					/* Then, sound chips */

					for(j=0;j<MAX_SOUND;j++)
					{
						if (sound_num(&x_sound[j]))
						{
							printf("%dx",sound_num(&x_sound[j]));
							printf("%-9s ",sound_name(&x_sound[j]));
						}
						else
							printf("%-11s ",sound_name(&x_sound[j]));
					}

					/* Lastly, the name of the game and a \newline */

					printf("%s\n",drivers[i]->description);
				}
			return 0;
			break;

		case LIST_GAMELIST: /* GAMELIST.TXT */
			printf("This is the complete list of games supported by MAME %s.\n",build_version);
			if (!listclones)
				printf("Variants of the same game are not included, you can use the -listclones command\n"
					"to get a list of the alternate versions of a given game.\n");
			printf("\n"
				"This list is generated automatically and is not 100%% accurate (particularly in\n"
				"the Screen Flip column). Please let us know of any errors so we can correct\n"
				"them.\n"
				"\n"
				"Here are the meanings of the columns:\n"
				"\n"
				"Working\n"
				"=======\n"
				"  NO: Emulation is still in progress; the game does not work correctly. This\n"
				"  means anything from major problems to a black screen.\n"
				"\n"
				"Correct Colors\n"
				"==============\n"
				"    YES: Colors should be identical to the original.\n"
				"  CLOSE: Colors are nearly correct.\n"
				"     NO: Colors are completely wrong. \n"
				"  \n"
				"  Note: In some cases, the color PROMs for some games are not yet available.\n"
				"  This causes a NO GOOD DUMP KNOWN message on startup (and, of course, the game\n"
				"  has wrong colors). The game will still say YES in this column, however,\n"
				"  because the code to handle the color PROMs has been added to the driver. When\n"
				"  the PROMs are available, the colors will be correct.\n"
				"\n"
				"Sound\n"
				"=====\n"
				"  PARTIAL: Sound support is incomplete or not entirely accurate. \n"
				"\n"
				"  Note: Some original games contain analog sound circuitry, which is difficult\n"
				"  to emulate. Therefore, these emulated sounds may be significantly different.\n"
				"\n"
				"Screen Flip\n"
				"===========\n"
				"  Many games were offered in cocktail-table models, allowing two players to sit\n"
				"  across from each other; the game's image flips 180 degrees for each player's\n"
				"  turn. Some games also have a \"Flip Screen\" DIP switch setting to turn the\n"
				"  picture (particularly useful with vertical games).\n"
				"  In many cases, this feature has not yet been emulated.\n"
				"\n"
				"Internal Name\n"
				"=============\n"
				"  This is the unique name that must be used when running the game from a\n"
				"  command line.\n"
				"\n"
				"  Note: Each game's ROM set must be placed in the ROM path, either in a .zip\n"
				"  file or in a subdirectory with the game's Internal Name. The former is\n"
				"  suggested, because the files will be identified by their CRC instead of\n"
				"  requiring specific names.\n\n");
			printf("+----------------------------------+-------+-------+-------+-------+----------+\n");
			printf("|                                  |       |Correct|       |Screen | Internal |\n");
			printf("| Game Name                        |Working|Colors | Sound | Flip  |   Name   |\n");
			printf("+----------------------------------+-------+-------+-------+-------+----------+\n");

			for (i = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					char name_ref[200];

					namecopy(name_ref,drivers[i]->description);

					strcat(name_ref," ");

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							strcat(name_ref,strchr(drivers[i]->description,'('));
					}

					printf("| %-33.33s",name_ref);

					if (drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
					{
						const struct GameDriver *maindrv;
						int foundworking;

						if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
							maindrv = drivers[i]->clone_of;
						else maindrv = drivers[i];

						foundworking = 0;
						j = 0;
						while (drivers[j])
						{
							if (drivers[j] == maindrv || drivers[j]->clone_of == maindrv)
							{
								if ((drivers[j]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
								{
									foundworking = 1;
									break;
								}
							}
							j++;
						}

						if (foundworking)
							printf("| No(1) ");
						else
							printf("|   No  ");
					}
					else
						printf("|  Yes  ");

					if (drivers[i]->flags & GAME_WRONG_COLORS)
						printf("|   No  ");
					else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
						printf("| Close ");
					else
						printf("|  Yes  ");

					{
						const char **samplenames = NULL;
						expand_machine_driver(drivers[i]->drv, &drv);
#if (HAS_SAMPLES || HAS_VLM5030)
						for (j = 0;drv.sound[j].sound_type && j < MAX_SOUND; j++)
						{
#if (HAS_SAMPLES)
							if (drv.sound[j].sound_type == SOUND_SAMPLES)
							{
								samplenames = ((struct Samplesinterface *)drv.sound[j].sound_interface)->samplenames;
								break;
							}
#endif
						}
#endif
						if (drivers[i]->flags & GAME_NO_SOUND)
							printf("|   No  ");
						else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
						{
							if (samplenames)
								printf("|Part(2)");
							else
								printf("|Partial");
						}
						else
						{
							if (samplenames)
								printf("| Yes(2)");
							else
								printf("|  Yes  ");
						}
					}

					if (drivers[i]->flags & GAME_NO_COCKTAIL)
						printf("|   No  ");
					else
						printf("|  Yes  ");

					printf("| %-8s |\n",drivers[i]->name);
				}

			printf("+----------------------------------+-------+-------+-------+-------+----------+\n\n");
			printf("(1) There are variants of the game (usually bootlegs) that work correctly\n");
#if (HAS_SAMPLES)
			printf("(2) Needs samples provided separately\n");
#endif
			return 0;
			break;

		case LIST_GAMES: /* list games, production year, manufacturer */
			for (i = 0; drivers[i]; i++)
				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->description))
				{
					char name[200];

					printf("%-5s%-36s ",drivers[i]->year,drivers[i]->manufacturer);

					namecopy(name,drivers[i]->description);
						printf("%s",name);

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							printf(" %s",strchr(drivers[i]->description,'('));
					}
					printf("\n");
				}
			return 0;
			break;

		case LIST_CLONES: /* list clones */
			printf("Name:    Clone of:\n");
			for (i = 0; drivers[i]; i++)
				if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER) &&
						(!strwildcmp(gamename,drivers[i]->name)
								|| !strwildcmp(gamename,drivers[i]->clone_of->name)))
					printf("%-8s %-8s\n",drivers[i]->name,drivers[i]->clone_of->name);
			return 0;
			break;

		case LIST_WRONGORIENTATION: /* list drivers which incorrectly use the orientation and visible area fields */
			for (i = 0; drivers[i]; i++)
			{
				expand_machine_driver(drivers[i]->drv, &drv);
				if ((drv.video_attributes & VIDEO_TYPE_VECTOR) == 0 &&
						(drivers[i]->clone_of == 0
								|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)) &&
						drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1 <=
						drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1)
				{
					if (strcmp(drivers[i]->name,"crater") &&
						strcmp(drivers[i]->name,"mpatrol") &&
						strcmp(drivers[i]->name,"troangel") &&
						strcmp(drivers[i]->name,"travrusa") &&
						strcmp(drivers[i]->name,"kungfum") &&
						strcmp(drivers[i]->name,"battroad") &&
						strcmp(drivers[i]->name,"vigilant") &&
						strcmp(drivers[i]->name,"sonson") &&
						strcmp(drivers[i]->name,"brkthru") &&
						strcmp(drivers[i]->name,"darwin") &&
						strcmp(drivers[i]->name,"exprraid") &&
						strcmp(drivers[i]->name,"sidetrac") &&
						strcmp(drivers[i]->name,"targ") &&
						strcmp(drivers[i]->name,"spectar") &&
						strcmp(drivers[i]->name,"venture") &&
						strcmp(drivers[i]->name,"mtrap") &&
						strcmp(drivers[i]->name,"pepper2") &&
						strcmp(drivers[i]->name,"hardhat") &&
						strcmp(drivers[i]->name,"fax") &&
						strcmp(drivers[i]->name,"circus") &&
						strcmp(drivers[i]->name,"robotbwl") &&
						strcmp(drivers[i]->name,"crash") &&
						strcmp(drivers[i]->name,"ripcord") &&
						strcmp(drivers[i]->name,"starfire") &&
						strcmp(drivers[i]->name,"fireone") &&
						strcmp(drivers[i]->name,"renegade") &&
						strcmp(drivers[i]->name,"battlane") &&
						strcmp(drivers[i]->name,"megatack") &&
						strcmp(drivers[i]->name,"killcom") &&
						strcmp(drivers[i]->name,"challeng") &&
						strcmp(drivers[i]->name,"kaos") &&
						strcmp(drivers[i]->name,"formatz") &&
						strcmp(drivers[i]->name,"bankp") &&
						strcmp(drivers[i]->name,"liberatr") &&
						strcmp(drivers[i]->name,"toki") &&
						strcmp(drivers[i]->name,"stactics") &&
						strcmp(drivers[i]->name,"sprint1") &&
						strcmp(drivers[i]->name,"sprint2") &&
						strcmp(drivers[i]->name,"nitedrvr") &&
						strcmp(drivers[i]->name,"punchout") &&
						strcmp(drivers[i]->name,"spnchout") &&
						strcmp(drivers[i]->name,"armwrest") &&
						strcmp(drivers[i]->name,"route16") &&
						strcmp(drivers[i]->name,"stratvox") &&
						strcmp(drivers[i]->name,"irobot") &&
						strcmp(drivers[i]->name,"leprechn") &&
						strcmp(drivers[i]->name,"starcrus") &&
						strcmp(drivers[i]->name,"astrof") &&
						strcmp(drivers[i]->name,"tomahawk") &&
						1)
						printf("%s %dx%d\n",drivers[i]->name,
								drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
								drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1);
				}
			}
			return 0;
			break;

		case LIST_WRONGFPS: /* list drivers with too high frame rate */
			for (i = 0; drivers[i]; i++)
			{
				expand_machine_driver(drivers[i]->drv, &drv);
				if ((drv.video_attributes & VIDEO_TYPE_VECTOR) == 0 &&
						(drivers[i]->clone_of == 0
								|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)) &&
						drv.frames_per_second > 57 &&
						drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1 > 244 &&
						drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1 <= 256)
				{
					printf("%s %dx%d %fHz\n",drivers[i]->name,
							drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
							drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1,
							drv.frames_per_second);
				}
			}
			return 0;
			break;

		case LIST_SOURCEFILE:
			for (i = 0; drivers[i]; i++)
				if (!strwildcmp(gamename,drivers[i]->name))
					printf("%-8s %s\n",drivers[i]->name,drivers[i]->source_file);
			return 0;
			break;

		case LIST_GAMESPERSOURCEFILE:
			{
				#define MAXCOUNT 8

				int numcount[MAXCOUNT],gamescount[MAXCOUNT];

				for (i = 0;i < MAXCOUNT;i++) numcount[i] = gamescount[i] = 0;

				for (i = 0; drivers[i]; i++)
				{
					if (drivers[i]->clone_of == 0 ||
							(drivers[i]->clone_of->flags & NOT_A_DRIVER))
					{
						const char *sf = drivers[i]->source_file;
						int total = 0;

						for (j = 0; drivers[j]; j++)
						{
							if (drivers[j]->clone_of == 0 ||
									(drivers[j]->clone_of->flags & NOT_A_DRIVER))
							{
								if (drivers[j]->source_file == sf)
								{
									if (j < i) break;

									total++;
								}
							}
						}

						if (total)
						{
							if (total == 1)							{ numcount[0]++; gamescount[0] += total; }
							else if (total >= 2 && total <= 3)		{ numcount[1]++; gamescount[1] += total; }
							else if (total >= 4 && total <= 7)		{ numcount[2]++; gamescount[2] += total; }
							else if (total >= 8 && total <= 15)		{ numcount[3]++; gamescount[3] += total; }
							else if (total >= 16 && total <= 31)	{ numcount[4]++; gamescount[4] += total; }
							else if (total >= 32 && total <= 63)	{ numcount[5]++; gamescount[5] += total; }
							else if (total >= 64)					{ numcount[6]++; gamescount[6] += total; }
						}
					}
				}

				printf("1\t%d\t%d\n",		numcount[0],gamescount[0]);
				printf("2-3\t%d\t%d\n",		numcount[1],gamescount[1]);
				printf("4-7\t%d\t%d\n",		numcount[2],gamescount[2]);
				printf("8-15\t%d\t%d\n",	numcount[3],gamescount[3]);
				printf("16-31\t%d\t%d\n",	numcount[4],gamescount[4]);
				printf("32-63\t%d\t%d\n",	numcount[5],gamescount[5]);
				printf("64+\t%d\t%d\n",		numcount[6],gamescount[6]);

				#undef MAXCOUNT
			}
			return 0;
			break;

		case LIST_CRC: /* list all crc-32 */
		case LIST_SHA1: /* list all sha-1 */
		case LIST_MD5:  /* list all md5 */

			if (list == LIST_SHA1)
				j = HASH_SHA1;
			else if (list == LIST_MD5)
				j = HASH_MD5;
			else
				j = HASH_CRC;

			for (i = 0; drivers[i]; i++)
			{
				const struct RomModule *region, *rom;

				for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					{
						char chksum[256];

						if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), j, chksum))
							printf("%s %-12s %s\n",chksum,ROM_GETNAME(rom),drivers[i]->description);
					}
			}
			return 0;
			break;

		case LIST_DUPCRC: /* list duplicate crc-32 (with different ROM name) */
			for (i = 0; drivers[i]; i++)
			{
				const struct RomModule *region, *rom;

				for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
						/* compare all the ROMS that we have a dump for */
						if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
						{
							char first_match = 1;

							for (j = i + 1; drivers[j]; j++)
							{
								const struct RomModule *region1, *rom1;

								for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
									for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
										if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
										{
											/* Dump checksum infos only on the first match for a given
											   ROM. This reduces the output size and makes it more
											   readable. */
											if (first_match)
										{
												char buf[512];

												first_match = 0;
				
												hash_data_print(ROM_GETHASHDATA(rom), 0, buf);
												printf("%s\n", buf);
												printf("    %-12s %-8s\n", ROM_GETNAME(rom),drivers[i]->name);

											}

											printf("    %-12s %-8s\n", ROM_GETNAME(rom1),drivers[j]->name);
										}
										}
							}
			}
			return 0;
			break;


		case LIST_WRONGMERGE:	/* list duplicate crc-32 with different ROM name */
								/* and different crc-32 with duplicate ROM name */
								/* in clone sets */
			for (i = 0; drivers[i]; i++)
			{
				const struct RomModule *region, *rom;

				for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
				{
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					{
						if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
						{
							for (j = 0; drivers[j]; j++)
							{
								if (j != i &&
									drivers[j]->clone_of &&
									(drivers[j]->clone_of->flags & NOT_A_DRIVER) == 0 &&
									(drivers[j]->clone_of == drivers[i] ||
									(i < j && drivers[j]->clone_of == drivers[i]->clone_of)))
								{
									const struct RomModule *region1, *rom1;
									int match = 0;

									for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
									{
										for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
										{
											if (!strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)))
											{
												if (!hash_data_has_info(ROM_GETHASHDATA(rom1), HASH_INFO_NO_DUMP) &&
													!hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
												{
													char temp[512];

													/* Print only the checksums available for both the roms */
													unsigned int functions = 
														hash_data_used_functions(ROM_GETHASHDATA(rom)) &
														hash_data_used_functions(ROM_GETHASHDATA(rom1));

													printf("%s:\n", ROM_GETNAME(rom));
													
													hash_data_print(ROM_GETHASHDATA(rom), functions, temp);
													printf("  %-8s: %s\n", drivers[i]->name, temp);

													hash_data_print(ROM_GETHASHDATA(rom1), functions, temp);
													printf("  %-8s: %s\n", drivers[j]->name, temp);
												}
												else
													match = 1;
											}
										}
									}

									if (match == 0)
									{
										for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
										{
											for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
											{
												if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && 
													hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
												{
													char temp[512];

													/* Print only the checksums available for both the roms */
													unsigned int functions = 
														hash_data_used_functions(ROM_GETHASHDATA(rom)) &
														hash_data_used_functions(ROM_GETHASHDATA(rom1));

													hash_data_print(ROM_GETHASHDATA(rom), functions, temp);
													printf("%s\n", temp);
													printf("  %-12s %-8s\n", ROM_GETNAME(rom), drivers[i]->name);
													printf("  %-12s %-8s\n", ROM_GETNAME(rom1),drivers[j]->name);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return 0;
			break;

		case LIST_ROMSIZE: /* I used this for statistical analysis */
			for (i = 0; drivers[i]; i++)
			{
				if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
				{
					const struct RomModule *region, *rom, *chunk;
					int romtotal = 0,romcpu = 0,romgfx = 0,romsound = 0;

					for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
					{
						int type = ROMREGION_GETTYPE(region);

						for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
						{
							for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
							{
								romtotal += ROM_GETLENGTH(chunk);
								if (type >= REGION_CPU1 && type <= REGION_CPU8) romcpu += ROM_GETLENGTH(chunk);
								if (type >= REGION_GFX1 && type <= REGION_GFX8) romgfx += ROM_GETLENGTH(chunk);
								if (type >= REGION_SOUND1 && type <= REGION_SOUND8) romsound += ROM_GETLENGTH(chunk);
							}
						}
					}

//					printf("%-8s\t%-5s\t%u\t%u\t%u\t%u\n",drivers[i]->name,drivers[i]->year,romtotal,romcpu,romgfx,romsound);
					printf("%-8s\t%-5s\t%u\n",drivers[i]->name,drivers[i]->year,romtotal);
				}
			}
			return 0;
			break;

		case LIST_ROMDISTRIBUTION: /* I used this for statistical analysis */
			{
				int year;

				for (year = 1975;year <= 2000;year++)
				{
					int gamestotal = 0,romcpu = 0,romgfx = 0,romsound = 0;

					for (i = 0; drivers[i]; i++)
					{
						if (atoi(drivers[i]->year) == year)
						{
							if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
							{
								const struct RomModule *region, *rom, *chunk;

								gamestotal++;

								for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
								{
									int type = ROMREGION_GETTYPE(region);

									for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
									{
										for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
										{
											if (type >= REGION_CPU1 && type <= REGION_CPU8) romcpu += ROM_GETLENGTH(chunk);
											if (type >= REGION_GFX1 && type <= REGION_GFX8) romgfx += ROM_GETLENGTH(chunk);
											if (type >= REGION_SOUND1 && type <= REGION_SOUND8) romsound += ROM_GETLENGTH(chunk);
										}
									}
								}
							}
						}
					}

					printf("%-5d\t%u\t%u\t%u\t%u\n",year,gamestotal,romcpu,romgfx,romsound);
				}
			}
			return 0;
			break;

		case LIST_ROMNUMBER: /* I used this for statistical analysis */
			{
				#define MAXCOUNT 100

				int numcount[MAXCOUNT];

				for (i = 0;i < MAXCOUNT;i++) numcount[i] = 0;

				for (i = 0; drivers[i]; i++)
				{
					if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
					{
						const struct RomModule *region, *rom;
						int romnum = 0;

						for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
						{
							for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
							{
								romnum++;
							}
						}

						if (romnum)
						{
							if (romnum > MAXCOUNT) romnum = MAXCOUNT;
							numcount[romnum-1]++;
						}
					}
				}

				for (i = 0;i < MAXCOUNT;i++)
					printf("%d\t%d\n",i+1,numcount[i]);

				#undef MAXCOUNT
				}
			return 0;
			break;

		case LIST_PALETTESIZE: /* I used this for statistical analysis */
			for (i = 0; drivers[i]; i++)
				if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
				{
					expand_machine_driver(drivers[i]->drv, &drv);
					printf("%-8s\t%-5s\t%u\n",drivers[i]->name,drivers[i]->year,drv.total_colors);
				}
			return 0;
			break;

		case LIST_CPU: /* I used this for statistical analysis */
			{
				int type;

				for (type = 1;type < CPU_COUNT;type++)
				{
					int count_main = 0,count_slave = 0;

					i = 0;
					while (drivers[i])
					{
						if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							struct InternalMachineDriver x_driver;
							const struct MachineCPU *x_cpu;

							expand_machine_driver(drivers[i]->drv, &x_driver);
							x_cpu = x_driver.cpu;

							for (j = 0;j < MAX_CPU;j++)
							{
								if (x_cpu[j].cpu_type == type)
								{
									if (j == 0) count_main++;
									else count_slave++;
									break;
								}
							}
						}

						i++;
					}

					printf("%s\t%d\n",cputype_name(type),count_main+count_slave);
//					printf("%s\t%d\t%d\n",cputype_name(type),count_main,count_slave);
				}
			}

			return 0;
			break;


		case LIST_CPUCLASS: /* I used this for statistical analysis */
			{
				int year;

//				for (j = 1;j < CPU_COUNT;j++)
//					printf("\t%s",cputype_name(j));
				for (j = 0;j < 3;j++)
					printf("\t%d",8<<j);
				printf("\n");

				for (year = YEAR_BEGIN;year <= YEAR_END;year++)
				{
					int count[CPU_COUNT];
					int count_buswidth[3];

					for (j = 0;j < CPU_COUNT;j++)
						count[j] = 0;
					for (j = 0;j < 3;j++)
						count_buswidth[j] = 0;

					i = 0;
					while (drivers[i])
					{
						if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							struct InternalMachineDriver x_driver;
							const struct MachineCPU *x_cpu;

							expand_machine_driver(drivers[i]->drv, &x_driver);
							x_cpu = x_driver.cpu;

							if (atoi(drivers[i]->year) == year)
							{
//								for (j = 0;j < MAX_CPU;j++)
j = 0;	// count only the main cpu
								{
									count[x_cpu[j].cpu_type]++;
									switch(cputype_databus_width(x_cpu[j].cpu_type))
									{
										case  8: count_buswidth[0]++; break;
										case 16: count_buswidth[1]++; break;
										case 32: count_buswidth[2]++; break;
									}
									}
							}
						}

						i++;
					}

					printf("%d",year);
//					for (j = 1;j < CPU_COUNT;j++)
//						printf("\t%d",count[j]);
					for (j = 0;j < 3;j++)
						printf("\t%d",count_buswidth[j]);
					printf("\n");
				}
			}

			return 0;
			break;


		case LIST_NOSOUND: /* I used this for statistical analysis */
			{
				int year;

				for (year = 1975;year <= 2000;year++)
				{
					int games=0,nosound=0;

					i = 0;
					while (drivers[i])
					{
						if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							if (atoi(drivers[i]->year) == year)
							{
								games++;
								if (drivers[i]->flags & GAME_NO_SOUND) nosound++;
							}
						}

						i++;
					}

					printf("%d\t%d\t%d\n",year,nosound,games);
				}
			}

			return 0;
			break;


		case LIST_SOUND: /* I used this for statistical analysis */
			{
				int type;

				for (type = 1;type < SOUND_COUNT;type++)
				{
					int count = 0,minyear = 3000,maxyear = 0;

					i = 0;
					while (drivers[i])
					{
						if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							struct InternalMachineDriver x_driver;
							const struct MachineSound *x_sound;

							expand_machine_driver(drivers[i]->drv, &x_driver);
							x_sound = x_driver.sound;

							for (j = 0;j < MAX_SOUND;j++)
							{
								if (x_sound[j].sound_type == type)
								{
									int year = atoi(drivers[i]->year);

									count++;

									if (year > 1900)
									{
										if (year > maxyear) maxyear = year;
										if (year < minyear) minyear = year;
									}
								}
							}
						}

						i++;
					}

					if (count)
//						printf("%s (%d-%d)\t%d\n",soundtype_name(type),minyear,maxyear,count);
						printf("%s\t%d\n",soundtype_name(type),count);
				}
			}

			return 0;
			break;


		case LIST_NVRAM: /* I used this for statistical analysis */
			{
				int year;

				for (year = 1975;year <= 2000;year++)
				{
					int games=0,nvram=0;

					i = 0;
					while (drivers[i])
					{
						if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							struct InternalMachineDriver x_driver;

							expand_machine_driver(drivers[i]->drv, &x_driver);

							if (atoi(drivers[i]->year) == year)
							{
								games++;
								if (x_driver.nvram_handler) nvram++;
							}
						}

						i++;
					}

					printf("%d\t%d\t%d\n",year,nvram,games);
				}
			}

			return 0;
			break;


		case LIST_INFO: /* list all info */
			print_mame_info( stdout, drivers );
			return 0;
	}

	if (verify)  /* "verify" utilities */
	{
		int err = 0;
		int correct = 0;
		int incorrect = 0;
		int res = 0;
		int total = 0;
		int checked = 0;
		int notfound = 0;


		for (i = 0; drivers[i]; i++)
		{
			if (!strwildcmp(gamename, drivers[i]->name))
				total++;
		}

		for (i = 0; drivers[i]; i++)
		{
			if (strwildcmp(gamename, drivers[i]->name))
				continue;

			/* set rom and sample path correctly */
//			get_rom_sample_path (argc, argv, i, NULL);

			if (verify & VERIFY_ROMS)
			{
				res = VerifyRomSet (i,(verify & VERIFY_TERSE) ? terse_printf : (verify_printf_proc)printf);

				if (res == CLONE_NOTFOUND || res == NOTFOUND)
				{
					notfound++;
					goto nextloop;
				}

				if (res == INCORRECT || res == BEST_AVAILABLE || (verify & VERIFY_VERBOSE))
				{
					printf ("romset %s ", drivers[i]->name);
					if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
						printf ("[%s] ", drivers[i]->clone_of->name);
				}
			}
			if (verify & VERIFY_SAMPLES)
			{
				const char **samplenames = NULL;
				expand_machine_driver(drivers[i]->drv, &drv);
#if (HAS_SAMPLES || HAS_VLM5030)
 				for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
				{
#if (HAS_SAMPLES)
 					if( drv.sound[j].sound_type == SOUND_SAMPLES )
 						samplenames = ((struct Samplesinterface *)drv.sound[j].sound_interface)->samplenames;
#endif
				}
#endif
				/* ignore games that need no samples */
				if (samplenames == 0 || samplenames[0] == 0)
					goto nextloop;

				res = VerifySampleSet (i,(verify_printf_proc)printf);
				if (res == NOTFOUND)
				{
					notfound++;
					goto nextloop;
				}
				printf ("sampleset %s ", drivers[i]->name);
			}

			if (res == NOTFOUND)
			{
				printf ("oops, should never come along here\n");
			}
			else if (res == INCORRECT)
			{
				printf ("is bad\n");
				incorrect++;
			}
			else if (res == CORRECT)
			{
				if (verify & VERIFY_VERBOSE)
					printf ("is good\n");
				correct++;
			}
			else if (res == BEST_AVAILABLE)
			{
				printf ("is best available\n");
				correct++;
			}
			if (res)
				err = res;

nextloop:
			checked++;
			fprintf(stderr,"%d%%\r",100 * checked / total);
		}

		if (correct+incorrect == 0)
		{
			printf ("%s ", (verify & VERIFY_ROMS) ? "romset" : "sampleset" );
			if (notfound > 0)
				printf("\"%8s\" not found!\n",gamename);
			else
				printf("\"%8s\" not supported!\n",gamename);
			return 1;
		}
		else
		{
			printf("%d %s found, %d were OK.\n", correct+incorrect,
					(verify & VERIFY_ROMS)? "romsets" : "samplesets", correct);
			if (incorrect > 0)
				return 2;
			else
				return 0;
		}
		return 0;
	}
	if (ident)
	{
		if (ident == 2) silentident = 1;
		else silentident = 0;

		knownstatus = KNOWN_START;
		romident(gamename,1);
		if (ident == 2)
		{
			switch (knownstatus)
			{
				case KNOWN_START: printf("ERROR     %s\n",gamename); break;
				case KNOWN_ALL:   printf("KNOWN     %s\n",gamename); break;
				case KNOWN_NONE:  printf("UNKNOWN   %s\n",gamename); break;
				case KNOWN_SOME:  printf("PARTKNOWN %s\n",gamename); break;
			}
		}
		return 0;
	}

	/* FIXME: horrible hack to tell that no frontend option was used */
	return 1234;
}

#ifdef MESS

/* This function contains all the -list calls from fronthlp.c for MESS */
/* Currently Supported: */
/*   -listdevices       */
/*   -listtext       	*/

void list_mess_info(const char *gamename, const char *arg, int listclones)
{

	int i, j;

	/* -listdevices */
	if (!stricmp(arg, "-listdevices"))
	{

		i = 0;
		j = 0;


		printf(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
		printf("----------  --------------------  ------------------------------------\n");

		while (drivers[i])
		{
			const struct IODevice *dev = device_first(drivers[i]);

			if (!strwildcmp(gamename, drivers[i]->name))
			{
				int devcount = 1;

				printf("%-13s", drivers[i]->name);

				/* if IODevice not used, print UNKNOWN */
				if (!dev)
					printf("%-12s\n", "UNKNOWN");

				/* else cycle through Devices */
				while (dev)
				{
					const char *src = dev->file_extensions;

					if (devcount == 1)
						printf("%-12s(%s)   ", device_typename(dev->type), device_brieftypename(dev->type));
					else
						printf("%-13s%-12s(%s)   ", "    ", device_typename(dev->type), device_brieftypename(dev->type));
					devcount++;

					while (src && *src)
					{
						printf(".%-5s", src);
						src += strlen(src) + 1;
					}
					dev = device_next(drivers[i], dev);			   /* next IODevice struct */
					printf("\n");
				}
			}
			i++;
		}
	}

	/* -listtext */
	else if (!stricmp(arg, "-listtext"))
	{
		printf("                   ==========================================\n" );
		printf("                    M.E.S.S.  -  Multi-Emulator Super System\n"  );
		printf("                             Copyright (C) 1998-2003\n");
		printf("                                by the MESS team\n"    );
		printf("                    Official Page at: http://www.mess.org\n");
		printf("                   ==========================================\n\n" );

		printf("This document is generated for MESS %s\n\n",build_version);

		printf("Please note that many people helped with this project, either directly or by\n"
		       "releasing source code which was used to write the drivers. We are not trying to\n"
		       "appropriate merit which isn't ours. See the acknowledgents section for a list\n"
			   "of contributors, however please note that the list is largely incomplete. See\n"
			   "also the CREDITS section in the emulator to see the people who contributed to a\n"
			   "specific driver. Again, that list might be incomplete. We apologize in advance\n"
			   "for any omission.\n\n"

			   "All trademarks cited in this document are property of their respective owners.\n"

			   "Especially, the MESS team would like to thank Nicola Salmoria and the MAME team\n"
			   "for letting us play with their code and, in fact, incorporating MESS specific\n"
			   "code into MAME.  Without it, MESS would be substantially less than what it is\n"
			   "right now! ;-)\n\n"

			   "Usage and Distribution Licence:\n"
			   "===============================\n"
			   "- MESS usage and distribution follows that of MAME.  Please read the MAME\n"
			   "  readme.txt file distributed with MESS for further information.\n\n"

			   "How to Contact The MESS Team\n"
			   "============================\n"
			   "Visit the web page at http://www.mess.org to see a list of contributers\n"
			   "If you have comments, suggestions or bug reports about an existing driver, check\n"
			   "the page contacts section to find who has worked on it, and send comments to that \n"
			   "person. If you are not sure who to contact, write to Ben (ben@mame.net)\n"
			   "who is the current coordinator of the MESS project [Win Console]. \n\n"

			   "PLEASE DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST! \n"

			   "THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses\n"
			   "*will* be ignored. Please understand that this is a *free* project, mostly\n"
			   "targeted at experienced users. We don't have the resources to provide end user\n"
			   "support. Basically, if you can't get the emulator to work, you are on your own.\n"
			   "First of all, read this doc carefully. If you still can't find an answer to\n"
			   "your question, try checking the beginner's sections that many emulation pages\n"
			   "have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc)\n"
			   "or on the many emulation message boards.  The official MESS message board is at:\n"
			   "   http://www.mess.org\n\n");


		printf("Also, please DO NOT SEND REQUESTS FOR NEW SYSTEMS TO ADD, unless you have some original\n");
		printf("info on the hardware or, even better, have the technical expertise needed to\n");
		printf("help us. Please don't send us information widely available on the Internet -\n");
		printf("we are perfectly capable of finding it ourselves, thank you.\n\n\n");


		printf("Complete Emulated System List\n");
		printf("=============================\n");
		printf("Here is the list of systems supported by MESS %s\n",build_version);
		if (!listclones)
			printf("Variants of the same system are not included, you can use the -listclones command\n"
				"to get a list of the alternate versions of a given system.\n");
		printf("\n"
			   "The meanings of the columns are as follows:\n"
			   "Working - \"No\" means that the emulation has shortcomings that cause the system\n"
			   "  not to work correctly. This can be anywhere from just showing a black screen\n"
			   "  to not being playable with major problems.\n"
			   "Correct Colors - \"Yes\" means that colors should be identical to the original,\n"
			   "  \"Close\" that they are very similar but wrong in places, \"No\" that they are\n"
			   "  completely wrong. \n"
			   "Sound - \"Partial\" means that sound support is either incomplete or not entirely\n"
			   "  accurate. \n"
			   "Internal Name - This is the unique name that should be specified on the command\n"
			   "  line to run the system. ROMs must be placed in the ROM path, either in a .zip\n"
			   "  file or in a subdirectory of the same name. The former is suggested, because\n"
			   "  the files will be identified by their CRC instead of requiring specific\n"
			   "  names.  NOTE! that as well as required ROM files to emulate the system, you may\n"
			   "  also attach IMAGES of files created for system specific devices (some examples of \n"
			   "  devices are cartridges, floppydisks, harddisks, etc).  See below for a complete list\n"
			   "  of a systems supported devices and common file formats used for that device\n\n");

		printf("System Information can be obtained from the SysInfo.dat file (online in the MESS UI\n"
			   "from the Machine history) or sysinfo.htm.  To generate sysinfo.htm, execute \n"
			   "dat2html.exe.\n\n\n");

		printf("+-----------------------------------------+-------+-------+-------+----------+\n");
		printf("|                                         |       |Correct|       | Internal |\n");
		printf("| System Name                             |Working|Colors | Sound |   Name   |\n");
		printf("+-----------------------------------------+-------+-------+-------+----------+\n");



			/* Generate the System List */

			 i = 0;
			while (drivers[i])
			{

				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					char name[200],name_ref[200];

					strcpy(name,drivers[i]->description);

					/* Move leading "The" to the end */
					if (strstr(name," (")) *strstr(name," (") = 0;
					if (strncmp(name,"The ",4) == 0)
					{
						sprintf(name_ref,"%s, The ",name+4);
					}
					else
						sprintf(name_ref,"%s ",name);

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							strcat(name_ref,strchr(drivers[i]->description,'('));
					}

					//printf("| %-33.33s",name_ref);
					printf("| %-40.40s",name_ref);

					if (drivers[i]->flags & GAME_NOT_WORKING)
					{
						const struct GameDriver *maindrv;
						int foundworking;

						if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
							maindrv = drivers[i]->clone_of;
						else maindrv = drivers[i];

						foundworking = 0;
						j = 0;
						while (drivers[j])
						{
							if (drivers[j] == maindrv || drivers[j]->clone_of == maindrv)
							{
								if ((drivers[j]->flags & GAME_NOT_WORKING) == 0)
								{
									foundworking = 1;
									break;
								}
							}
							j++;
						}

						if (foundworking)
							printf("| No(1) ");
						else
							printf("|   No  ");
					}
					else
						printf("|  Yes  ");

					if (drivers[i]->flags & GAME_WRONG_COLORS)
						printf("|   No  ");
					else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
						printf("| Close ");
					else
						printf("|  Yes  ");

					{
						const char **samplenames = 0;
/*						for (j = 0;drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++)
						{

						#if (HAS_SAMPLES)
							if (drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES)
							{
								samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
								break;
							}
						#endif
						}
*/						if (drivers[i]->flags & GAME_NO_SOUND)
							printf("|   No  ");
						else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
						{
							if (samplenames)
								printf("|Part(2)");
							else
								printf("|Partial");
						}
						else
						{
							if (samplenames)
								printf("| Yes(2)");
							else
								printf("|  Yes  ");
						}
					}

					printf("| %-8s |\n",drivers[i]->name);
				}
				i++;
			}

			printf("+-----------------------------------------+-------+-------+-------+----------+\n");
			printf("(1) There are variants of the system that work correctly\n\n\n");

		printf("QUICK MESS USAGE GUIDE!\n"
		       "=======================\n"
		       "In order to use MESS, you must at least specify at the command line\n\n"
               "      MESS <system>\n\n"
			   "This will emulate the system requested.  Note that most systems require the BIOS for\n"
			   "emulation.  These system BIOS ROM files are copyright and ARE NOT supplied with MESS.\n\n"
			   "To use files created for the system emulated (SOFTWARE), MESS works by attaching these\n"
			   "files created for the particular device of that system, for example, a cartridge,\n"
               "floppydisk, harddisk, cassette, software etc.  Therefore, in order to attach software to the\n"
			   "system, you must specify at the command line:\n\n"
               "      MESS <system> <device> <software_name>\n\n"
			   "To manually manipulate the emulation options, you must specify:\n\n"
               "      MESS <system> <device> <software_name> <options>\n\n");
		printf("*For a complete list of systems emulated,  use: MESS -listfull\n"
			   "*For system files (BIOS) required by each system, use: MESS <system> -listroms\n"
			   "*See below for valid device names and usage.\n"
			   "*See the MAME readme.txt and below for a detailed list of options.\n\n"
			   "Make sure you have BIOS and SOFTWARE in a subdirectory specified in mess.cfg\n\n\n");
		printf("Examples:\n\n"
			   "    MESS nes -cart zelda.nes\n"
			   "        will attach zelda.nes to the cartridge device and run MESS in\n"
			   "        the following way:\n"
			   "        <system>        = nes             (Nintendo Entertainment System)\n"
			   "        <device>        = CARTRIDGE\n"
			   "        <software_name> = zelda.nes       (Zelda cartridge)\n"
			   "        <options>       = none specified, so default options (see mess.cfg)\n\n"
			   "    MESS coleco -cart dkong -soundcard 0\n"
			   "        will run MESS in the following way:\n"
			   "        <system>        = coleco          (ColecoVision)\n"
			   "        <device>        = CARTRIDGE\n"
			   "        <software_name> = dkong.rom       (Donkey Kong cartridge)\n"
			   "        <options>       = default options without sound (see mess.cfg)\n\n"
			   "    MESS trs80 -flop boot.dsk -flop arcade1.dsk\n"
			   "        will run MESS in the following way:\n"
			   "        <system>         = trs80           (TRs-80 model 1)\n"
			   "        <device1>        = FLOPPYDISK\n"
			   "        <software_name1> = boot.dsk        (The Trs80 boot floppy diskl)\n"
			   "        <device2>        = FLOPPYDISK\n"
			   "        <software_name2> = arcade1.dsk     (floppy Disk which contains games)\n"
			   "        <options>        = default options (all listed in mess.cfg)\n\n"
			   "    MESS cgenie -flop games1\n"
			   "        will run the system Colour Genie with one disk loaded,\n"
			   "        automatically appending the file extension .dsk.\n\n\n");


		printf("To EXIT the emulator, press ESC.  If the emulated system is a computer, \n"
		       "you may need to toggle the UI mode (use the SCR_LOCK key).\n"
		       "The on-screen display shows the current UI mode when SCR_LOCK is pressed.\n\n");


		printf("To automatically create the individual system directories in the \n"
		       "SOFTWARE folder, use:\n"
		       "    MESS -createdir\n\n\n");

		printf("DEVICE support list\n");
		printf("===================\n");
		printf("As mentioned, in order to fully utilise MESS, you will need to attach software files\n"
			   "to the system devices.  To obtain a full list for all the devices and software \n"
			   "file extensions currently supported by a system in MESS, Use:\n"
			   "    MESS -listdevices\n\n\n");

		printf("KEYS: see readme.txt\n"
		       "====================\n"
		       "      ESC     - Exit emulator (providing UI is enabled - see below)\n"
		       "      ScrLOCK - Toggle UI for computer systems\n");


	}

    else if (!stricmp(arg, "-createdir"))
	{
	/***************************************************/
	/* To create the SOFTWARE directories */
		const char *sys_rom_path  = "SOFTWARE";
		char buf[128];
		int d=0;

			/* ensure the roms directory exists! */
			sprintf(buf,"%s %s","md",sys_rom_path);
			printf("%s\n",buf);
			system(buf);

			/* create directory for all currently supported drivers */
			while(drivers[d])
			{
				/* create the systems directory */
				sprintf(buf,"%s %s\\%s","md",sys_rom_path,drivers[d]->name);
				printf("%s\n",buf);
				system(buf);
				d++;
			}
	}


}

#endif
