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
  0				/* end of array */
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
#ifdef DEVELOPING
#define TESTDRIVER(NAME) &driver_##NAME,
#else
#define TESTDRIVER(NAME)
#endif
const struct GameDriver *drivers[] =
{
#include "system.c"
  0				/* end of array */
};

#else /* DRIVER_RECURSIVE */

#ifndef NEOMAME

/****************CONSOLES****************************************************/

	/* ATARI */
	DRIVER( a2600 ) 	/* Atari 2600									  */
	DRIVER( a5200 ) 	/* Atari 5200									  */
	DRIVER( a7800 ) 	/* Atari 7800									  */

	/* BALLY */
	DRIVER( astrocde )	/* Bally Astrocade								  */

	/* FAIRCHILD */
	DRIVER( channelf )	/* Channel F									  */

    /* COLECO */
	DRIVER( coleco )	/* ColecoVision (Original BIOS )				  */
#if 0						/* Please dont include these next 2 in a distribution, they are Hacks	*/
	DRIVER( colecofb )	/* ColecoVision (Fast BIOS load)				  */
	DRIVER( coleconb )	/* ColecoVision (No BIOS load)					  */
#endif
	/* NINTENDO */
	DRIVER( nes )		/* Nintendo Entertainment System				  */
	DRIVER( nespal )	/* Nintendo Entertainment System				  */
	DRIVER( famicom )
	DRIVER( gameboy )	/* Nintendo GameBoy Handheld					  */
    DRIVER (snes)		/* Nintendo Super Nintendo                        */
/*	DRIVER (vboy)	*/	/* Nintendo Virtual Boy                           */

	/* NEC */
	DRIVER( pce )		/* PC/Engine - Turbo Graphics-16  NEC 1989-1993   */

	/* SEGA */
	DRIVER( gamegear )	/* Sega Game Gear Handheld						  */
	DRIVER( sms )		/* Sega Sega Master System						  */
	DRIVER( genesis )	/* Sega Genesis/MegaDrive						  */

	/* GCE */
	DRIVER( vectrex )	/* General Consumer Electric Vectrex - 1982-1984  */
						/* (aka Milton-Bradley Vectrex) 				  */
	DRIVER( raaspec )	/* RA+A Spectrum - Modified Vectrex 			  */

	/* ENTEX */
	DRIVER( advision )	/* Adventurevision								  */

	/* Magnavox */
	DRIVER( odyssey2 )	/* Magnavox Odyssey 2 - 1978-1983				  */


	/* CAPCOM */
	DRIVER( sfzch ) 	/* CPS Changer (Street Fighter ZERO)			  */

/****************COMPUTERS****************************************************/

	/* APPLE */
/*
 * CPU Model			 Month				 Year
 * -------------		 -----				 ----
 *
 * Apple I				 July				 1976
 * Apple II 			 April				 1977
 * Apple II Plus		 June				 1979
 * Apple III			 May				 1980
 * Apple IIe			 January			 1983
 * Apple III Plus		 December			 1983
 * Apple IIe Enhanced	 March				 1985
 * Apple IIc			 April				 1984
 * Apple IIc ROM 0		 ?					 1985
 * Apple IIc ROM 3		 September			 1986
 * Apple IIgs			 September			 1986
 * Apple IIe Platinum	 January			 1987
 * Apple IIgs ROM 01	 September			 1987
 * Apple IIc ROM 4		 ?					 198?
 * Apple IIc Plus		 September			 1988
 * Apple IIgs ROM 3 	 August 			 1989
 */
	DRIVER( apple1 )	/* 1976 Apple 1 								  */
	DRIVER( apple2c )	/* 1984 Apple //c								  */
	DRIVER( apple2c0 )	/* 1986 Apple //c (3.5 ROM) 					  */
	DRIVER( apple2cp )	/* 1988 Apple //c+								  */
	DRIVER( apple2e )	/* 1983 Apple //e								  */
	DRIVER( apple2ee )	/* 1985 Apple //e Enhanced						  */
	DRIVER( apple2ep )	/* 1987 Apple //e Platinum						  */
/*	DRIVER( mac512k )*/	/* 1984 Apple Macintosh 512k					  */
    DRIVER( mac512ke )  /* 1986 Apple Macintosh 512ke                     */
	DRIVER( macplus )	/* 1986 Apple Macintosh Plus					  */
/*	DRIVER( mac2 )*/	/* 1987 Apple Macintosh II						  */
	DRIVER( lisa2 ) 	/*												  */

	/* ATARI */
	DRIVER( a400 )		/* 1979 Atari 400								  */
	DRIVER( a400pal )	/* 1979 Atari 400 PAL							  */
	DRIVER( a800 )		/* 1979 Atari 800								  */
	DRIVER( a800pal )	/* 1979 Atari 800 PAL							  */
	DRIVER( a800xl )	/* 1983 Atari 800 XL							  */

#ifndef MESS_EXCLUDE_CBM
	/* COMMODORE */
	DRIVER( kim1 )		/* Commodore (MOS) KIM-1 1975					  */

	DRIVER( pet )		/* PET2001/CBM20xx Series (Basic 1) 			  */
	DRIVER( cbm30 ) 	/* Commodore 30xx (Basic 2) 					  */
	DRIVER( cbm30b )	/* Commodore 30xx (Basic 2) (business keyboard)   */
	DRIVER( cbm40 ) 	/* Commodore 40xx FAT (CRTC) 60Hz				  */
	DRIVER( cbm40pal )	/* Commodore 40xx FAT (CRTC) 50Hz				  */
	DRIVER( cbm40b )	/* Commodore 40xx THIN (business keyboard)		  */
	DRIVER( cbm80 ) 	/* Commodore 80xx 60Hz							  */
	DRIVER( cbm80pal )	/* Commodore 80xx 50Hz							  */
	DRIVER( cbm80ger )	/* Commodore 80xx German (50Hz) 				  */
	DRIVER( cbm80swe )	/* Commodore 80xx Swedish (50Hz)				  */
	DRIVER( superpet )	/* Commodore SP9000/MMF9000 (50Hz)				  */

	DRIVER( vic20 ) 	/* Commodore Vic-20 NTSC						  */
	DRIVER( vc20 )		/* Commodore Vic-20 PAL 						  */
	DRIVER( vic20swe )	/* Commodore Vic-20 Sweden						  */
	DRIVER( vic20i )	/* Commodore Vic-20 IEEE488 Interface			  */

	DRIVER( max )		/* Max (Japan)/Ultimax (US)/VC10 (German)		  */
	DRIVER( c64 )		/* Commodore 64 - NTSC							  */
	DRIVER( c64pal )	/* Commodore 64 - PAL							  */
	DRIVER( vic64s )	/* Commodore VIC64S (Swedish)					  */
	DRIVER( cbm4064 )	/* Commodore CBM4064							  */
TESTDRIVER( sx64 )		/* Commodore SX 64 - PAL						  */
	DRIVER( c64gs ) 	/* Commodore 64 - NTSC							  */

	DRIVER( cbm500 )	/* Commodore 500/P128-40						  */
	DRIVER( cbm610 )    /* Commodore 610/B128LP                           */
	DRIVER( cbm620 )    /* Commodore 620/B256LP                           */
	DRIVER( cbm710 )    /* Commodore 710/B128HP                           */
	DRIVER( cbm720 )    /* Commodore 620/B256HP                           */

	DRIVER( c16 )		/* Commodore 16 								  */
	DRIVER( c16hun )	/* Commodore 16 Hungarian Character Set Hack	  */
	DRIVER( c16c )		/* Commodore 16  c1551							  */
TESTDRIVER( c16v )		/* Commodore 16  vc1541 						  */
	DRIVER( plus4 ) 	/* Commodore +4  c1551							  */
	DRIVER( plus4c )	/* Commodore +4  vc1541 						  */
TESTDRIVER( plus4v )	/* Commodore +4 								  */
	DRIVER( c364 )		/* Commodore 364 - Prototype					  */

	DRIVER( c128 )		/* Commodore 128 - NTSC 						  */
	DRIVER( c128ger )	/* Commodore 128 - PAL (german) 				  */
	DRIVER( c128fra )	/* Commodore 128 - PAL (french) 				  */
	DRIVER( c128ita )	/* Commodore 128 - PAL (italian)				  */

	DRIVER( amiga ) 	/* Commodore Amiga								  */

	DRIVER( c65 )		/* C65 / C64DX (Prototype, NTSC, 911001)		  */
	DRIVER( c65e )		/* C65 / C64DX (Prototype, NTSC, 910828)		  */
	DRIVER( c65d )		/* C65 / C64DX (Prototype, NTSC, 910626)		  */
	DRIVER( c65c )		/* C65 / C64DX (Prototype, NTSC, 910523)		  */
	DRIVER( c65ger )	/* C65 / C64DX (Prototype, German PAL, 910429)	  */
	DRIVER( c65a )		/* C65 / C64DX (Prototype, NTSC, 910111)		  */

#endif

#ifndef MESS_EXCLUDE_AMSTRAD
	DRIVER( cpc464 )	/* Amstrad (Schneider in Germany) 1984			  */
	DRIVER( cpc664 )	/* Amstrad (Schneider in Germany) 1985			  */
	DRIVER( cpc6128 )	/* Amstrad (Schneider in Germany) 1985			  */
/*	DRIVER( cpc464p )*/	/* Amstrad CPC464  Plus - 1987					  */
/*	DRIVER( cpc6128p )*//* Amstrad CPC6128 Plus - 1987					  */
	DRIVER( pcw8256 )	/* 198? PCW8256 								  */
	DRIVER( pcw8512 )	/* 198? PCW8512 								  */
	DRIVER( pcw9256 )	/* 198? PCW9256 								  */
	DRIVER( pcw9512 )	/* 198? PCW9512 (+) 							  */
	DRIVER( pcw10 ) 	/* 198? PCW10									  */
	DRIVER( pcw16 ) 	/* 1995 PCW16									  */
	DRIVER( nc100 ) 	/* 19?? NC100									  */
#endif
#ifndef MESS_EXCLUDE_ACORN
	DRIVER( z88 )		/*												  */
	DRIVER( avigo ) 	/*												  */
#endif
#ifndef MESS_EXCLUDE_AMSTRAD
	/* VEB MIKROELEKTRONIK */
	DRIVER( kccomp )	/* KC compact									  */
	DRIVER( kc85_4 )	/* KC 85/4										  */
#endif

	/* CANTAB */
	DRIVER( jupiter )	/* Jupiter Ace									  */

	/* INTELLIGENT SOFTWARE */
	DRIVER( ep128 ) 	/* Enterprise 128 k 							  */
	DRIVER( ep128a )	/* Enterprise 128 k 							  */

	/* NON LINEAR SYSTEMS */
	DRIVER( kaypro )	/* Kaypro 2X									  */

	/* MICROBEE SYSTEMS */
	DRIVER( mbee )		/* Microbee 									  */
	DRIVER( mbee56k )	/* Microbee 56K (CP/M)							  */

	/* TANDY RADIO SHACK */
	DRIVER( trs80l1 )	/* TRS-80 Model I	- Radio Shack Level I BASIC   */
	DRIVER( trs80 ) 	/* TRS-80 Model I	- Radio Shack Level II BASIC  */
	DRIVER( trs80alt )	/* TRS-80 Model I	- R/S L2 BASIC				  */
TESTDRIVER( trs80m3 )	/* TRS-80 Model III - Radio Shack/Tandy 		  */
	DRIVER( coco )		/* Color Computer								  */
	DRIVER( coco3 ) 	/* Color Computer 3 							  */
	DRIVER( coco3h ) /* Hacked Color Computer 3 (6309)						  */
	DRIVER( cp400 ) 	/* Prologica CP400								  */
	DRIVER( mc10 )		/* MC-10									      */

	/* DRAGON DATA LTD */
	DRIVER( dragon32 )	/* Dragon32 									  */

	/* EACA */
	DRIVER( cgenie )	/* Colour Genie 								  */
	DRIVER( sys80 ) 	/* System 80									  */

	/* VIDEO TECHNOLOGY */
	DRIVER( laser110 )	/* Laser 110									  */
	DRIVER( laser200 )	/* Laser 200									  */
	DRIVER( laser210 )	/* Laser 210 (indentical to Laser 200 ?)		  */
	DRIVER( laser310 )	/* Laser 310 (210 with diff. keyboard and RAM)	  */
	DRIVER( vz200 ) 	/* Dick Smith Electronics / Sanyo VZ200 		  */
	DRIVER( vz300 ) 	/* Dick Smith Electronics / Sanyo VZ300 		  */
	DRIVER( fellow )	/* Salora Fellow (Finland)						  */
	DRIVER( tx8000 )	/* Texet TX-8000 (U.K.) 						  */
	DRIVER( laser350 )	/* Laser 350									  */
	DRIVER( laser500 )	/* Laser 500									  */
	DRIVER( laser700 )	/* Laser 700									  */

	/* TANGERINE */
	DRIVER( microtan )	/* Microtan 65									  */
	DRIVER( oric1 ) 	/* Oric 1										  */
	DRIVER( orica ) 	/* Oric Atmos									  */

	/* TEXAS INSTRUMENTS */
/*DRIVER( ti99_2_24 )*/ /* Texas Instruments TI 99/2					  */
/*DRIVER( ti99_2_32 )*/ /* Texas Instruments TI 99/2					  */
	DRIVER( ti99_4 )	/* Texas Instruments TI 99/4					  */
	DRIVER( ti99_4e )	/* Texas Instruments TI 99/4E					  */
	DRIVER( ti99_4a )	/* Texas Instruments TI 99/4A					  */
	DRIVER( ti99_4ae )	/* Texas Instruments TI 99/4AE					  */

#ifndef MESS_EXCLUDE_IBMPC
	/* IBM & Clones */
	DRIVER( pc )		/* PC											  */
	DRIVER( pcmda ) 	/* PC/XT with MDA (MGA aka Hercules)		      */
	DRIVER( pccga ) 	/* PC/XT with CGA							      */

	DRIVER( t1000hx )	/* Tandy 1000HX (similiar to PCJr)				  */

	DRIVER( pc1512 )	/* Amstrad PC1512 (XT, CGA compatible)			  */
	DRIVER( pc1640 )    /* Amstrad PC1640 (XT, EGA compatible)			  */

TESTDRIVER( xtcga ) 	/* 												  */
	DRIVER( xtvga ) 	/*												  */
	DRIVER( atcga ) 	/*												  */
TESTDRIVER( atvga ) 	/*												  */
#endif

	/* PHILIPS */
	DRIVER( p2000t )	/* Philips - P2000T 							  */
	DRIVER( p2000m )	/* Philips - P2000M 							  */

	/* COMPUKIT */
	DRIVER( uk101 ) 	/*												  */

	/* OHIO SCIENTIFIC */
	DRIVER( superbrd )	/*												  */

#ifndef MESS_EXCLUDE_SINCLAIR
	/* SINCLAIR */
	DRIVER( zx80 )		/* Sinclair ZX-80								  */
	DRIVER( zx81 )		/* Sinclair ZX-81								  */
	DRIVER( ts1000 )	/* Timex Sinclair 1000							  */
	DRIVER( aszmic )	/* ASZMIC ZX-81 ROM swap						  */
	DRIVER( pc8300 )	/* Your Computer - PC8300						  */
	DRIVER( pow3000 )	/* Creon Enterprises - Power 3000				  */

	DRIVER( spectrum )	/* Sinclair ZX Spectrum 48k 					  */
	DRIVER( specbusy )	/*												  */
	DRIVER( specgrot )	/*												  */
	DRIVER( specimc )	/*												  */
	DRIVER( speclec )	/*												  */
	DRIVER( inves ) 	/*												  */
	DRIVER( tk90x ) 	/*												  */
	DRIVER( tk95 )		/*												  */
	DRIVER( tc2048 )	/*												  */
	DRIVER( ts2068 )	/*												  */

	DRIVER( spec128 )	/* Spectrum 									  */
	DRIVER( spec128s )	/* Spectrum 									  */
	DRIVER( specpls2 )	/* Spectrum 									  */
	DRIVER( specpl2a )	/* Spectrum 									  */
	DRIVER( specp2fr )	/*												  */
	DRIVER( specp2sp )	/*												  */
	DRIVER( specpls3 )	/* Spectrum Plus 3								  */
	DRIVER( specp3sp )	/*												  */
	DRIVER( specpl3e )	/*												  */
	DRIVER( specpls4 )	/*												  */
#endif

	/* ASCII & MICROSOFT */
	DRIVER( msx )		/* MSX											  */
	DRIVER( msxj )		/* MSX Jap										  */
	DRIVER( msxkr ) 	/* MSX Korean									  */
	DRIVER( msxuk ) 	/* MSX UK										  */

	/* NASCOM MICROCOMPUTERS */
	DRIVER( nascom1 )	/* Nascom 1 									  */
	DRIVER( nascom2 )	/* Nascom 2 									  */

	/* ACORN */
#ifndef MESS_EXCLUDE_ACORN
	DRIVER( atom )		/* Acorn Atom									  */
	DRIVER( bbca )		/* BBC Micro									  */
	DRIVER( bbcb )		/* BBC Micro									  */
TESTDRIVER( a310 )		/* Acorn Archimedes 310 						  */
#endif

	/* MILES GORDON TECHNOLOGY */
	DRIVER( coupe ) 	/*												  */

#ifndef MESS_EXCLUDE_SHARP
	/* SHARP */
	DRIVER( pc1251 )	/* Pocket Computer 1251 						  */
	DRIVER( pc1401 )	/* Pocket Computer 1401 						  */
	DRIVER( pc1402 )	/* Pocket Computer 1402 						  */
	DRIVER( pc1350 )	/* Pocket Computer 1350 						  */

	DRIVER( mz700 ) 	/* Sharp MZ700									  */
	DRIVER( mz700j )	/* Sharp MZ700 Japan							  */
#endif

	/* MOTOROLA */
/*	DRIVER( mekd2 )*/ 	/* Motorola Evaluation Kit						  */

	/* DEC */
	DRIVER( pdp1 )		/* DEC PDP1 for SPACEWAR! - 1962				  */

	/* MEMOTECH */
	DRIVER( mtx512 )	/* Memotech MTX512								  */

	/* MATTEL */
	DRIVER( aquarius )	/*												  */

/****************OTHERS******************************************************/

#if 0
    DRIVER( arcadia )   /* Arcadia 2001                                   */
	DRIVER( atarist )	/* Atari ST 									  */
	DRIVER( channelf )	/* Fairchild Channel F VES - 1976				  */
	DRIVER( coco2 ) 	/* Color Computer 2 							  */
	DRIVER( intv )		/* Mattel Intellivision - 1979 AKA INTV 		  */
	DRIVER( jaguar )	/* Atari Jaguar 								  */
	DRIVER( lynx )		/* Atari Lynx Handheld							  */
	DRIVER( odyssey )	/* Magnavox Odyssey - analogue (1972)			  */
	DRIVER( trs80_m2 )	/* TRS-80 Model II -							  */
	DRIVER( x68000 )	/* X68000										  */
#endif

#endif /* NEOMAME */

#endif /* DRIVER_RECURSIVE */

#endif /* TINY_COMPILE */
