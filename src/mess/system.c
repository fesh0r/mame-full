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
#define DRIVER(NAME) extern struct GameDriver NAME##_driver;
#define TESTDRIVER(NAME) extern struct GameDriver NAME##_driver;
#include "system.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#undef TESTDRIVER
#define DRIVER(NAME) &NAME##_driver,
#define TESTDRIVER(NAME)
const struct GameDriver *drivers[] =
{
#include "system.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

#ifndef NEOMAME



  /****************THESE DRIVERS WORK OK***************************************/

      DRIVER( a800 )        /* Atari 800                                      */

      DRIVER( a5200 )       /* Atari 5200                                     */
      DRIVER( a7800 )       /* Atari 7800                                     */
      DRIVER( amstrad )     /* Amstrad CPC                                    */
      DRIVER( astrocde )    /* Bally Astrocade                                */

      DRIVER( cgenie )      /* Color Genie                                    */
      DRIVER( coco )        /* Color Computer                                 */
      DRIVER( coleco )      /* ColecoVision (Original BIOS )                  */

      DRIVER( dragon32 )    /* Dragon32                                       */

      DRIVER( ep128 )       /* Enterprise 128 k                               */
      DRIVER( gamegear )    /* Sega Game Gear Handheld                        */
      DRIVER( genesis )     /* Sega Genesis/MegaDrive                         */

      DRIVER( kccomp )      /* KC compact                                     */

      DRIVER( nes )         /* Nintendo Entertainment System                  */

  TESTDRIVER( pc )          /* IBM PC  - parent Driver                        */
      DRIVER( pcmda )
      DRIVER( pccga )

      DRIVER( pce )         /* PC/Engine - Turbo Graphics-16  NEC 1989-1993   */

      DRIVER( pdp1 ) 	    /* DEC PDP1 for SPACEWAR! - 1962                  */

      DRIVER( raaspec )     /* RA+A Spectrum - Modified Vectrex               */

      DRIVER( sms )         /* Sega Sega Master System                        */

  TESTDRIVER( tandy1t )     /* Tandy                                          */

      DRIVER( ti99 )        /* Texas Instruments TI/994a                      */
      DRIVER( trs80 )       /* TRS-80 Model I   - Radio Shack/Tandy           */

      DRIVER( vectrex )     /* General Consumer Electric Vectrex - 1982-1984  */
                            /* aka Milton-Bradley Vectrex)                    */






  /***********THESE DRIVERS EXISTS, BUT HAVE PROBLEMS OF SOME SORT*************/

      DRIVER( apple2e )     /* APPLE                                          */
      DRIVER( apple2ee )    /* APPLE                                          */
      DRIVER( apple2ep )    /* plus? - 1979                                   */
      DRIVER( apple2c )     /* APPLE                                          */
      DRIVER( apple2c0 )    /* APPLE                                          */
      DRIVER( apl2cpls )    /* APPLE                                          */
      DRIVER( amiga )       /* Commodore Amiga                                */

  TESTDRIVER( a2600 )       /* Atari 2600                                     */
      DRIVER( kaypro )      /* APPLE KAYPRO                                   */
      DRIVER( spectrum )    /* Sinclair 48k                                   */

  TESTDRIVER( trs80m3 )     /* TRS-80 Model III - Radio Shack/Tandy           */
  TESTDRIVER( a800xl )      /* Atari 800 XL                                   */
  TESTDRIVER( colecofb )    /* ColecoVision (Fast BIOS load)                  */
  TESTDRIVER( coleconb )    /* ColecoVision (No BIOS load)                    */








  /****THESE DRIVERS DONT EXIST, BUT SHOULD ;-) *******************************/

    //DRIVER( apple )       /* Apple - 1976                                   */
    //DRIVER( applemac )    /* Apple Macintosh                                */
    //DRIVER( arcadia )     /* Arcadia 2001                                   */
    //DRIVER( atarist )     /* Atari ST                                       */

    //DRIVER( bbcmicro )    /* BBC Micro                                      */

    //DRIVER( c64 )         /* Commodore 64                                   */
    //DRIVER( c128 )        /* Commodore 128                                  */
    //DRIVER( channelf )    /* Fairchild Channel F VES - 1976                 */
    //DRIVER( coco2 )       /* Color Computer 2                               */

    //DRIVER( gameboy )     /* Nintendo GameBoy Handheld                      */

  							/* AkA Phillips Videopac                          */
    //DRIVER( intv )        /* Mattel Intellivision - 1979 AKA INTV           */
    //DRIVER( jaguar )      /* Atari Jaguar                                   */

    //DRIVER( lynx )        /* Atari Lynx Handheld                            */
    //DRIVER( msx )         /* MSX                                            */

    //DRIVER( odyssey )     /* Magnavox Odyssey - analogue (1972)             */
    //DRIVER( odyssey2 )    /* Magnavox Odyssey² - 1978-1983                  */

    //DRIVER( snes )        /* Nintendo Super Nintendo                        */

    //DRIVER( trs80_m2 )    /* TRS-80 Model II -                              */


    //DRIVER( vboy )        /* Nintendo Virtual Boy                           */
    //DRIVER( vic20 )       /* Commodore Vic-20                               */

    //DRIVER( x68000 )      /* X68000                                         */




#endif	/* NEOFREE */

#endif	/* DRIVER_RECURSIVE */

#endif	/* TINY_COMPILE */
