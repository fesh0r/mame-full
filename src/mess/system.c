/******************************************************************************

  driver.c/system.c for MESS

  The list of all available drivers. Drivers have to be included here to be
  recognized by the executable.

  To save some typing, we use a hack here. This file is recursively #included
  twice, with different definitions of the DRIVER() macro. The first one
  declares external references to the drivers; the second one builds an array
  storing all the drivers.

******************************************************************************/

#include "driver.h"

#ifndef DRIVER_RECURSIVE

/* The "root" driver, defined so we can have &driver_##NAME in macros. */
struct GameDriver driver_0 =
{
	__FILE__,
	0,
	"root",
	0,
	0,
	0,
	0, 0,
	0,
	0,
	0,
	0,
	0, 0, 0, 0,
	0,
	0, 0, 0,
	NOT_A_DRIVER,
	0,0
};

#endif

#ifdef TINY_COMPILE
extern struct GameDriver TINY_NAME;

const struct GameDriver *drivers[] =
{
	&TINY_NAME,
	0	/* end of array */
};

#else

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern struct GameDriver driver_##NAME;
#define TESTDRIVER(NAME) extern struct GameDriver driver_##NAME;
#include "system.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &driver_##NAME,
#define TESTDRIVER(NAME)
const struct GameDriver *drivers[] =
{
#include "system.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

#ifndef NEOMAME


  /****************CONSOLES****************************************************/

      /* ATARI */
  TESTDRIVER( a2600 )       /* Atari 2600                                     */
      DRIVER( a5200 )       /* Atari 5200                                     */
      DRIVER( a7800 )       /* Atari 7800                                     */

      /* BALLY */
      DRIVER( astrocde )    /* Bally Astrocade                                */

      /* COLECO */
      DRIVER( coleco )      /* ColecoVision (Original BIOS )                  */
      /* Please dont include these next 2 in a distribution, they are Hacks   */
  TESTDRIVER( colecofb )    /* ColecoVision (Fast BIOS load)                  */
  TESTDRIVER( coleconb )    /* ColecoVision (No BIOS load)                    */

      /* NINTENDO */
      DRIVER( nes )         /* Nintendo Entertainment System                  */
      DRIVER( gameboy )     /* Nintendo GameBoy Handheld                      */
  TESTDRIVER( snes )        /* Nintendo Super Nintendo                        */
  TESTDRIVER( vboy )        /* Nintendo Virtual Boy                           */

	  /* NEC */
      DRIVER( pce )         /* PC/Engine - Turbo Graphics-16  NEC 1989-1993   */

      /* SEGA */
      DRIVER( gamegear )    /* Sega Game Gear Handheld                        */
      DRIVER( sms )         /* Sega Sega Master System                        */
      DRIVER( genesis )     /* Sega Genesis/MegaDrive                         */

	  /* GCE */
      DRIVER( vectrex )     /* General Consumer Electric Vectrex - 1982-1984  */
                            /* aka Milton-Bradley Vectrex)                    */
      DRIVER( raaspec )     /* RA+A Spectrum - Modified Vectrex               */

      /* Other */
	  DRIVER( advision )






  /****************COMPUTERS****************************************************/

      /* APPLE */
      DRIVER( apple1 )      /* APPLE                                          */
      DRIVER( apple2c )     /* APPLE                                          */
      DRIVER( apple2c0 )    /* APPLE                                          */
      DRIVER( apple2cp )    /* APPLE                                          */
      DRIVER( apple2e )     /* APPLE                                          */
      DRIVER( apple2ee )    /* APPLE                                          */
      DRIVER( apple2ep )    /* plus? - 1979                                   */

      /* ATARI */
      DRIVER( a800 )        /* Atari 800                                      */
  TESTDRIVER( a800xl )      /* Atari 800 XL                                   */

      /* COMMODORE */

      DRIVER( c16 )         /* Commodore 16                                   */
      DRIVER( plus4 )       /* Commodore +4                                   */
  TESTDRIVER( c364 )        /* Commodore ??                                   */

      DRIVER( c64 )         /* Commodore 64 - NTSC                            */
      DRIVER( c64pal )      /* Commodore 64 - PAL                             */
      DRIVER( max )         /* Ulitimax                                       */

      DRIVER( vic20 )       /* Commodore Vic-20 NTSC                          */
      DRIVER( vc20 )        /* Commodore Vic-20 PAL                           */

      DRIVER( amiga )       /* Commodore Amiga                                */




      /* AMSTRAD */
      DRIVER( cpc464 )      /* Amstrad (Schneider in Germany) 1984            */
      DRIVER( cpc664 )      /* Amstrad (Schneider in Germany) 1985            */
      DRIVER( cpc6128 )     /* Amstrad (Schneider in Germany) 1985                                    */
  TESTDRIVER( cpc464p )     /* Amstrad CPC464  Plus - 1987                    */
  TESTDRIVER( cpc6128p )    /* Amstrad CPC6128 Plus - 1987                    */

      /* VEB MIKROELEKTRONIK */
      DRIVER( kccomp )      /* KC compact                                     */
      DRIVER( kc85_4 )      /*                                                */

	  /* CANTAB */
      DRIVER( jupiter )     /*                                                */

      /* Intelligent Software */
      DRIVER( ep128 )       /* Enterprise 128 k                               */

      /* Non Linear Systems */
      DRIVER( kaypro )      /*   KAYPRO                                       */

	  /* Microbee Systems */
	  DRIVER( mbee )		/*	 Microbee									  */
	  DRIVER( mbee56k ) 	/*	 Microbee 56K (CP/M)						  */

      /* Tandy */
      DRIVER( coco )        /* Color Computer                                 */
	  DRIVER( coco3 )       /* Color Computer 3                               */
      DRIVER( cp400 )       /* Prologica CP400                                */
      DRIVER( trs80 )       /* TRS-80 Model I   - Radio Shack/Tandy           */
  TESTDRIVER( trs80m3 )     /* TRS-80 Model III - Radio Shack/Tandy           */

      /* Dragon Data Ltd */
      DRIVER( dragon32 )    /* Dragon32                                       */

	  /* EACA */
      DRIVER( cgenie )      /* Color Genie                                    */

      /* VideoTech */
      DRIVER( laser110 )
      DRIVER( laser210 )
      DRIVER( laser200 )
	  DRIVER( vz200 )       /* laser210?                                      */
	  DRIVER( fellow )
      DRIVER( tx8000 )
      DRIVER( laser310 )
      DRIVER( vz300 )
      DRIVER( laser350 )    /* Video Tech Laser 350                           */
      DRIVER( laser500 )    /* Video Tech Laser 500                           */
      DRIVER( laser700 )    /* Video Tech Laser 700                           */

      /* Tangerine */
	  DRIVER( oric1 )       /* ORIC 1                                         */
	  DRIVER( orica )       /* ORIC Atmos                                     */

      /* Texas Instruments */
  TESTDRIVER( ti99_2_24 )      /* Texas Instruments TI/99 2                      */
  TESTDRIVER( ti99_2_32 )      /* Texas Instruments TI/99 2                      */
      DRIVER( ti99_4 )      /* Texas Instruments TI/99 4                      */
      DRIVER( ti99_4a )     /* Texas Instruments TI/99 4a                     */

      /* IBM & Clones */
  TESTDRIVER( pc )          /* IBM PC  - parent Driver, so no need            */
      DRIVER( pcmda )
      DRIVER( pccga )
      DRIVER( tandy1t )     /* Tandy                                          */

	  /* Sinclair */
	  DRIVER( zx80 )		/*												  */
	  DRIVER( zx81 )		/*												  */
	  DRIVER( ts1000 )		/*												  */
	  DRIVER( aszmic )		/*												  */
      DRIVER( pc8300 )
	  DRIVER( pow3000 )

      DRIVER( spectrum )    /* Sinclair 48k                                   */


	  /* Other */
      DRIVER( msx )         /* MSX                                            */
      DRIVER( nascom1 )     /*                                                */



  /****************OTHERS******************************************************/

      DRIVER( kim1 )        /* Commodore (MOS) KIM-1 1975                     */
      DRIVER( pdp1 ) 	    /* DEC PDP1 for SPACEWAR! - 1962                  */











    //DRIVER( applemac )    /* Apple Macintosh                                */
    //DRIVER( arcadia )     /* Arcadia 2001                                   */
    //DRIVER( atarist )     /* Atari ST                                       */

    //DRIVER( bbcmicro )    /* BBC Micro                                      */

    //DRIVER( channelf )    /* Fairchild Channel F VES - 1976                 */
    //DRIVER( coco2 )       /* Color Computer 2                               */

    //DRIVER( c128 )        /* Commodore 128                                  */

  							/* AkA Phillips Videopac                          */
    //DRIVER( intv )        /* Mattel Intellivision - 1979 AKA INTV           */
    //DRIVER( jaguar )      /* Atari Jaguar                                   */

    //DRIVER( lynx )        /* Atari Lynx Handheld                            */


    //DRIVER( odyssey )     /* Magnavox Odyssey - analogue (1972)             */
    //DRIVER( odyssey2 )    /* Magnavox Odyssey² - 1978-1983                  */



    //DRIVER( trs80_m2 )    /* TRS-80 Model II -                              */


    //DRIVER( x68000 )      /* X68000                                         */


#endif	/* NEOFREE */

#endif	/* DRIVER_RECURSIVE */

#endif	/* TINY_COMPILE */
