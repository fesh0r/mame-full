/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

//#include "msdos/mamalleg.h"
#include "driver.h"
#include "mame.h"
#include "mess/mess.h"

#ifdef MESS
char rom_name[MAX_ROM][2048]; /* MESS */
char floppy_name[MAX_FLOPPY][2048]; /* MESS */
char hard_name[MAX_HARD][2048]; /* MESS */
char cassette_name[MAX_CASSETTE][2048]; /* MESS */
//unsigned int dispensed_tickets = 0; /* MESS */ in common.c
#endif

extern struct GameOptions options;
static int num_roms = 0;
static int num_floppies = 0;
static int num_harddisks = 0;

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

int parse_image_types(char *arg)
{
	/* Is it a floppy or a rom? */
	char *ext = strrchr(arg, '.');
	if (ext && (!stricmp(ext,".DSK") || !stricmp(ext,".ATR") || !stricmp(ext,".XFD"))) {
		if (num_floppies >= MAX_FLOPPY) {
			printf("Too many floppy names specified!\n");
			return 1;
		}
		strcpy(options.floppy_name[num_floppies++], arg);
		if (errorlog) fprintf(errorlog,"Using floppy image #%d %s\n", num_floppies, arg);
		return 0;
	}
	if (ext && !stricmp(ext,".IMG")) {
		if (num_harddisks >= MAX_HARD) {
			printf("Too many hard disk image names specified!\n");
			return 1;
		}
		strcpy(options.hard_name[num_harddisks++], arg);
		if (errorlog) fprintf(errorlog,"Using hard disk image #%d %s\n", num_harddisks, arg);
		return 0;
	}
	if (num_roms >= MAX_ROM) {
		printf("Too many ROM image names specified!\n");
		return 1;
	}
	strcpy(options.rom_name[num_roms++], arg);
	if (errorlog) fprintf(errorlog,"Using ROM image #%d %s\n", num_roms, arg);
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
		for (i = j+1; i < argc; i++)
      {
			//char * alias;

			/* skip options and their additional arguments */
			if (argv[i][0] == '-')
         {
         /*
            if (	!stricmp(argv[i],"-vgafreq") ||
                	!stricmp(argv[i],"-depth") ||
                	!stricmp(argv[i],"-skiplines") ||
                	!stricmp(argv[i],"-skipcolumns") ||
                	!stricmp(argv[i],"-beam") ||
                	!stricmp(argv[i],"-flicker") ||
                	!stricmp(argv[i],"-gamma") ||
                	!stricmp(argv[i],"-frameskip") ||
                	!stricmp(argv[i],"-soundcard") ||
                	!stricmp(argv[i],"-samplerate") ||
                	!stricmp(argv[i],"-sr") ||
                	!stricmp(argv[i],"-samplebits") ||
                	!stricmp(argv[i],"-sb") ||
                	!stricmp(argv[i],"-joystick") ||
						!stricmp(argv[i],"-joy") ||
            		!stricmp(argv[i],"-resolution")) i++;
			*/

         }
         else
         {
				/* check if this is an alias for a set of images */

				//alias = get_config_string(driver, argv[i], "");
				//if (alias && strlen(alias))
            //{
				//	char *arg;
				//	if (errorlog) fprintf(errorlog,"Using alias %s for driver %s\n", argv[i], driver);
				//	arg = strtok (alias, ",");
				//	while (arg)
            //   {
				//		res = parse_image_types(arg);
				//		arg = strtok(0, ",");
				//	}
				//}
            //else
            //{
					res = parse_image_types(argv[i]);
				//}
			}
			/* If we had an error leave now */
			if (res)
				return res;
		}
    		return res;
}
