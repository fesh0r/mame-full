/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include <ctype.h>
#include "driver.h"
#include "mame.h"
#include "mess/mess.h"


char rom_name[MAX_ROM][MAX_PATHLEN]; /* MESS */
char floppy_name[MAX_FLOPPY][MAX_PATHLEN]; /* MESS */
char hard_name[MAX_HARD][MAX_PATHLEN]; /* MESS */
char cassette_name[MAX_CASSETTE][MAX_PATHLEN]; /* MESS */


extern struct GameOptions options;
static int num_roms = 0;
static int num_floppies = 0;
static int num_harddisks = 0;
static int num_cassettes = 0;

static char *mess_alpha = "";




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

/* HJB 12/11/99
 * this will go away, once we have a cleaner definition of the extensions
 * in the proposed PERIPHERAL struct.
 */

static int slot = -1;

int parse_image_types(char *arg)
{
    char e1 = 0, e2 = 0, e3 = 0;
	char *ext = strrchr(arg, '.');

    if( ext )
	{
		e1 = toupper(ext[1]);
		e2 = toupper(ext[2]);
		e3 = toupper(ext[3]);
    }

    /* cassette images */
	if( slot == 3 ||
		( slot < 0 &&
			(
				(e1=='C' && e2=='A' && e3=='S') ||
				(e1=='C' && e2=='M' && e3=='D') ||
				(e1=='T' && e2=='A' && e3=='P') ||
				(e1=='T' && e2=='6' && e3=='4') ||
				(e1=='V' && e2=='Z' && e3==  0)
			)
		  )
	  )
	{
		if( num_cassettes >= MAX_CASSETTE )
		{
			printf("Too many cassette image names specified!\n");
			return 1;
		}
		strcpy(options.cassette_name[num_cassettes++], arg);
		printf("Using cassette image #%d %s\n", num_cassettes, arg);
		return 0;
	}
	else
	/* harddisk images */
	if( slot == 2 ||
		( slot < 0 &&
			(
				(e1=='I' && e2=='M' && e3=='G')
			)
		)
	)
    {
        if( num_harddisks >= MAX_HARD )
        {
            printf("Too many hard disk image names specified!\n");
            return 1;
        }
        strcpy(options.hard_name[num_harddisks++], arg);
		printf("Using hard disk image #%d %s\n", num_harddisks, arg);
        return 0;
    }
    else
	/* are floppy disk images */
	if( slot == 1 ||
		( slot < 0 &&
			(
				(e1=='D' && e2=='6' && e3=='4') ||
				(e1=='D' && e2=='S' && e3=='K') ||
				(e1=='A' && e2=='D' && e3=='F') ||
				(e1=='A' && e2=='T' && e3=='R') ||
				(e1=='X' && e2=='F' && e3=='D') ||
				(e1=='V' && e2=='Z' && e3=='D')
			)
		)
	)
	{
		if (num_floppies >= MAX_FLOPPY)
		{
			printf("Too many floppy image names specified!\n");
			return 1;
		}
		strcpy(options.floppy_name[num_floppies++], arg);
		printf("Using floppy image #%d %s\n", num_floppies, arg);
		return 0;
	}
	else
	{
		if( num_roms >= MAX_ROM )
		{
			printf("Too many ROM image names specified!\n");
			return 1;
		}
		strcpy(options.rom_name[num_roms++], arg);
		printf("Using ROM image #%d %s\n", num_roms, arg);
	}
	return 0;
}






int load_image(int argc, char **argv, char *driver, int j)
{

	/*
	* Take all additional commandline arguments without "-" as image
	* names. This is an ugly hack that will hopefully eventually be
	* replaced with an online version that lets you "hot-swap" images.
	* HJB 08/13/98 for now the hack is extended even more :-/
	* Skip arguments to options starting with "-" too and accept
	* aliases for a set of ROMs/images.
    */
    int i;
    int res=0;
	char *alias;

	/* unknown image type specified */
    slot = -1;

    for (i = j+1; i < argc; i++)
    {
        /* skip options and their additional arguments */
		if (argv[i][0] == '-')
        {
         	/* Need to skip all options which are not followed by a "-" */
			if( !stricmp(argv[i],"-vgafreq")     ||
				!stricmp(argv[i],"-depth")       ||
				!stricmp(argv[i],"-skiplines")   ||
				!stricmp(argv[i],"-skipcolumns") ||
				!stricmp(argv[i],"-beam")        ||
				!stricmp(argv[i],"-flicker")     ||
				!stricmp(argv[i],"-gamma")       ||
				!stricmp(argv[i],"-frameskip")   ||
				!stricmp(argv[i],"-soundcard")   ||
				!stricmp(argv[i],"-samplerate")  ||
				!stricmp(argv[i],"-sr")          ||
				!stricmp(argv[i],"-samplebits")  ||
				!stricmp(argv[i],"-sb")          ||
				!stricmp(argv[i],"-joystick")    ||
				!stricmp(argv[i],"-joy")         ||
				!stricmp(argv[i],"-resolution") ) i++;
			/* check for explicit slots for the image names following */
			if( !stricmp(argv[i], "-rom") )
                slot = 0;
            else
			if( !stricmp(argv[i], "-floppy") )
				slot = 1;
			else
			if( !stricmp(argv[i], "-harddisk") )
				slot = 2;
            else
			if( !stricmp(argv[i], "-cassette") )
				slot = 3;
        }
        else
        {
			/* check if this is an alias for a set of images */

			alias = get_alias(driver,argv[i]);

			if( alias && strlen(alias) )
            {
				char *arg;
				if( errorlog )
					fprintf(errorlog,"Using alias %s (%s) for driver %s\n", argv[i],alias, driver);
				arg = strtok (alias, ",");
				while (arg)
            	{
					res = parse_image_types(arg);
					arg = strtok(0, ",");
				}
			}
            else
            {
				if (errorlog)
					fprintf(errorlog,"NOTE: No alias found\n");
				res = parse_image_types(argv[i]);
			}
		}
		/* If we had an error leave now */
		if (res)
			return res;
	}
    return res;
}

