/******************************************************************************

  driver.c

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
/* The "root" driver, defined so we can have &driver_##NAME in macros. */
struct GameDriver driver_0 =
{
	__FILE__,
	0,
	"root",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NOT_A_DRIVER,
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
	  DRIVER( apple1 )		/* Apple 1										  */
	  DRIVER( apple2c ) 	/* Apple //c									  */
	  DRIVER( apple2c0 )	/* Apple //c (3.5 ROM)							  */
	  DRIVER( apple2cp )	/* Apple //c+									  */
	  DRIVER( apple2e ) 	/* Apple //e									  */
	  DRIVER( apple2ee )	/* Apple //e Enhanced							  */
	  DRIVER( apple2ep )	/* Apple //e Platinum							  */
      DRIVER( macplus )	    /* Apple MacIntosh Plus      					  */

				/*
				CPU Model             Month               Year
				-------------         -----               ----

				Apple I               July                1976
				Apple II              April               1977
				Apple II Plus         June                1979
				Apple III             May                 1980
				Apple IIe             January             1983
				Apple III Plus        December            1983
				Apple IIe Enhanced    March               1985
				Apple IIc             April               1984
				Apple IIc ROM 0       ?                   1985
				Apple IIc ROM 3       September           1986
				Apple IIgs            September           1986
				Apple IIe Platinum    January             1987
				Apple IIgs ROM 01     September           1987
				Apple IIc ROM 4       ?                   198?
				Apple IIc Plus        September           1988
				Apple IIgs ROM 3      August              1989
				*/



      /* ATARI */
      DRIVER( a400 )        /* Atari 400                                      */
      DRIVER( a400pal )     /* Atari 400 PAL                                  */
      DRIVER( a800 )        /* Atari 800                                      */
      DRIVER( a800pal )     /* Atari 800 PAL                                  */
	  DRIVER( a800xl )		/* Atari 800 XL 								  */

      /* COMMODORE */
      DRIVER( c16 )         /* Commodore 16                                   */
      DRIVER( c16c )        /* Commodore 16  c1551                            */
  TESTDRIVER( c16v )        /* Commodore 16  vc1541                           */
      DRIVER( plus4 )       /* Commodore +4  c1551                            */
      DRIVER( plus4c )      /* Commodore +4  vc1541                           */
  TESTDRIVER( plus4v )      /* Commodore +4                                   */
	  DRIVER( c364 )		/* Commodore 364 - Prototype					  */

      DRIVER( c64 )         /* Commodore 64 - NTSC                            */
      DRIVER( c64pal )      /* Commodore 64 - PAL                             */
      DRIVER( c64gs )       /* Commodore 64 - NTSC                            */
	  DRIVER( cbm4064 ) 	/* Commodore CBM4064							  */
  TESTDRIVER( sx64 )		/* Commodore SX 64 - PAL						  */
      DRIVER( max )         /* Ulitimax                                       */
      DRIVER( c65 ) 		/* Commodore 65 - NTSC							  */
      DRIVER( c65ger )		/* Commodore 65 - PAL (german)					  */
	  DRIVER( c128 )		/* Commodore 128 - NTSC 						  */
	  DRIVER( c128ger ) 	/* Commodore 128 - PAL (german) 				  */
	  DRIVER( c128fra ) 	/* Commodore 128 - PAL (french) 				  */
      DRIVER( vic20 )       /* Commodore Vic-20 NTSC                          */
      DRIVER( vc20 )        /* Commodore Vic-20 PAL                           */

      DRIVER( pet )
	  DRIVER( cbm30 )
	  DRIVER( cbm30b )
	  DRIVER( cbm40 )
	  DRIVER( cbm40b )
	  DRIVER( cbm80 )
	  DRIVER( cbm80ger )
	  DRIVER( cbm80swe )

      DRIVER( cbm710 )
      DRIVER( cbm720 )
      DRIVER( cbm610 )
      DRIVER( cbm620 )
      DRIVER( cbm500 )

      DRIVER( amiga )       /* Commodore Amiga                                */

      /* AMSTRAD */
      DRIVER( cpc464 )      /* Amstrad (Schneider in Germany) 1984            */
      DRIVER( cpc664 )      /* Amstrad (Schneider in Germany) 1985            */
      DRIVER( cpc6128 )     /* Amstrad (Schneider in Germany) 1985                                    */
  TESTDRIVER( cpc464p )     /* Amstrad CPC464  Plus - 1987                    */
  TESTDRIVER( cpc6128p )    /* Amstrad CPC6128 Plus - 1987                    */

      /* VEB MIKROELEKTRONIK */
      DRIVER( kccomp )      /* KC compact                                     */
	  DRIVER( kc85_4 )		/* KC 85/4										  */

	  /* CANTAB */
	  DRIVER( jupiter ) 	/* Jupiter Ace									  */

	  /* INTELLIGENT SOFTWARE */
      DRIVER( ep128 )       /* Enterprise 128 k                               */

	  /* NON LINEAR SYSTEMS */
	  DRIVER( kaypro )		/* Kaypro 2X									  */

	  /* MICROBEE SYSTEMS */
	  DRIVER( mbee )		/* Microbee 									  */
	  DRIVER( mbee56k ) 	/* Microbee 56K (CP/M)							  */

	  /* TANDY */
      DRIVER( coco )        /* Color Computer                                 */
	  DRIVER( coco3 )       /* Color Computer 3                               */
      DRIVER( cp400 )       /* Prologica CP400                                */
      DRIVER( trs80 )       /* TRS-80 Model I   - Radio Shack/Tandy           */
	  DRIVER( mc10 )		/* MC-10										  */
  TESTDRIVER( trs80m3 )     /* TRS-80 Model III - Radio Shack/Tandy           */

	  /* DRAGON DATA LTD */
      DRIVER( dragon32 )    /* Dragon32                                       */

	  /* EACA */
	  DRIVER( cgenie )		/* Colour Genie 								  */
	  DRIVER( sys80 )		/* System 80									  */

	  /* VIDEO TECHNOLOGY */
	  DRIVER( laser110 )	/* Laser 110									  */
	  DRIVER( laser200 )	/* Laser 200									  */
	  DRIVER( laser210 )	/* Laser 210 (indentical to Laser 200 ?)		  */
	  DRIVER( vz200 )		/* Dick Smith Electronics / Sanyo VZ200 		  */
	  DRIVER( fellow )		/* Salora Fellow (Finland)						  */
	  DRIVER( tx8000 )		/* Texet TX-8000 (U.K.) 						  */
	  DRIVER( laser310 )	/* Laser 310 (210 with diff. keyboard and RAM)	  */
	  DRIVER( vz300 )		/* Dick Smith Electronics / Sanyo VZ300 		  */
	  DRIVER( laser350 )	/* Laser 350									  */
	  DRIVER( laser500 )	/* Laser 500									  */
	  DRIVER( laser700 )	/* Laser 700									  */

      /* Tangerine */
      DRIVER( oric1 )       /* Oric 1                                         */
	  DRIVER( orica )		/* Oric Atmos									  */

      /* Texas Instruments */
  TESTDRIVER( ti99_2_24 )	/* Texas Instruments TI 99/2					  */
  TESTDRIVER( ti99_2_32 )	/* Texas Instruments TI 99/2					  */
	  DRIVER( ti99_4 )		/* Texas Instruments TI 99/4					  */
	  DRIVER( ti99_4e ) 	/* Texas Instruments TI 99/4E					  */
	  DRIVER( ti99_4a ) 	/* Texas Instruments TI 99/4A					  */
	  DRIVER( ti99_4ae )	/* Texas Instruments TI 99/4AE					  */

      /* IBM & Clones */
  TESTDRIVER( pc )          /* IBM PC  - parent Driver, so no need            */
	  DRIVER( pcmda )		/* IBM PC/XT with MDA (MGA aka Hercules)		  */
	  DRIVER( pccga )		/* IBM PC/XT with CGA							  */
	  DRIVER( tandy1t ) 	/* Tandy 1000TX (similiar to PCJr)				  */

	  DRIVER( p2000t )		/* Philips - P2000T 							  */
	  DRIVER( p2000m )		/*                  							  */
	  DRIVER( uk101 )		/*                  							  */
	  DRIVER( superbrd )    /*                  							  */

      /* Sinclair */
	  DRIVER( zx80 )		/* Sinclair ZX-80								  */
	  DRIVER( zx81 )		/* Sinclair ZX-81								  */
	  DRIVER( ts1000 )		/* Timex Sinclair 1000							  */
	  DRIVER( aszmic )		/* ASZMIC ZX-81 ROM swap						  */
	  DRIVER( pc8300 )		/* Your Computer - PC8300						  */
	  DRIVER( pow3000 ) 	/* Creon Enterprises - Power 3000				  */

	  DRIVER( spectrum )	/* Sinclair ZX Spectrum 48k 					  */
      DRIVER( specpls3 )    /* Spectrum Plus 3                                */

	  /* ASCII & Microsoft */
      DRIVER( msx )         /* MSX                                            */
      DRIVER( msxj )        /* MSX Jap                                        */
      DRIVER( msxkr )       /* MSX Korean                                     */
      DRIVER( msxuk )       /* MSX UK                                         */

      /* Nascom */
	  DRIVER( nascom1 ) 	/* Nascom 1 									  */


  TESTDRIVER( mekd2 ) 	    /* Motorola 									  */


  /****************OTHERS******************************************************/

      DRIVER( kim1 )        /* Commodore (MOS) KIM-1 1975                     */
      DRIVER( pdp1 ) 	    /* DEC PDP1 for SPACEWAR! - 1962                  */

      DRIVER( sfzch )       /* CPS Changer (Street Fighter ZERO)              */


  TESTDRIVER( bbc  )        /* BBC Micro                                      */



    //DRIVER( applemac )    /* Apple Macintosh                                */
    //DRIVER( arcadia )     /* Arcadia 2001                                   */
    //DRIVER( atarist )     /* Atari ST                                       */



    //DRIVER( channelf )    /* Fairchild Channel F VES - 1976                 */
    //DRIVER( coco2 )       /* Color Computer 2                               */

  							/* AkA Phillips Videopac                          */
    //DRIVER( intv )        /* Mattel Intellivision - 1979 AKA INTV           */
    //DRIVER( jaguar )      /* Atari Jaguar                                   */

    //DRIVER( lynx )        /* Atari Lynx Handheld                            */


    //DRIVER( odyssey )     /* Magnavox Odyssey - analogue (1972)             */
    //DRIVER( odyssey2 )    /* Magnavox Odyssey² - 1978-1983                  */


    //DRIVER( trs80_m2 )    /* TRS-80 Model II -                              */


    //DRIVER( x68000 )      /* X68000                                         */


#endif  /* NEOMAME */

#endif	/* DRIVER_RECURSIVE */

#endif	/* TINY_COMPILE */
