#ifndef __CBM_H_
#define __CBM_H_

#include <stdlib.h>

#include "driver.h"
#include "devices/cartslot.h"
#include "devices/snapquik.h"

#ifdef __cplusplus
extern "C" {
#endif

/* must be defined until some driver init problems are solved */
#define NEW_GAMEDRIVER

/* global header file for
 * vc20
 * c16
 * c64
 * c128
 * c65*/

/**************************************************************************
 * Logging
 * call the XXX_LOG with XXX_LOG("info",(errorlog,"%fmt\n",args));
 * where "info" can also be 0 to append .."%fmt",args to a line.
 **************************************************************************/
#define LOG(LEVEL,N,M,A)  \
        { \
	  if(LEVEL>=N) { \
	    if( M ) \
              logerror("%11.6f: %-24s",timer_get_time(), (char*)M );\
	    logerror A; \
	  } \
        }

/* debugging level here for all on or off */
#if 1
# ifdef VERBOSE_DBG
#  undef VERBOSE_DBG
# endif
# if 1
#  define VERBOSE_DBG 0
# else
#  define VERBOSE_DBG 1
# endif
#else
# define PET_TEST_CODE
#endif

#if VERBOSE_DBG
#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)
#else
#define DBG_LOG(n,m,a)
#endif

QUICKLOAD_LOAD( cbm_pet1 );
QUICKLOAD_LOAD( cbm_pet );
QUICKLOAD_LOAD( cbm_c16 );
QUICKLOAD_LOAD( cbm_c64 );
QUICKLOAD_LOAD( cbm_c128 );
QUICKLOAD_LOAD( cbm_vc20 );
QUICKLOAD_LOAD( cbmb );
QUICKLOAD_LOAD( cbm500 );
QUICKLOAD_LOAD( cbm_c65 );

#define CONFIG_DEVICE_CBMPETQUICK	CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_pet)
#define CONFIG_DEVICE_CBMPET1QUICK	CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_pet1)
#define CONFIG_DEVICE_C16QUICK		CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_c16)
#define CONFIG_DEVICE_C64QUICK		CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_c64)
#define CONFIG_DEVICE_VC20QUICK		CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_vc20)
#define CONFIG_DEVICE_CBMBQUICK		CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbmb)
#define CONFIG_DEVICE_CBM500QUICK	CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm500)
#define CONFIG_DEVICE_C65QUICK		CONFIG_DEVICE_QUICKLOAD( "p00\0prg\0", cbm_c65)

/* use to functions to parse, load the rom images into memory
   and then use the cbm_rom var */
int cbm_rom_init(mess_image *img);
int cbm_rom_load(mess_image *img, mame_file *fp, int open_mode);
void cbm_rom_unload(mess_image *img);

typedef struct {
#define CBM_ROM_ADDR_UNKNOWN 0
#define CBM_ROM_ADDR_LO -1
#define CBM_ROM_ADDR_HI -2
	int addr, size;
	UINT8 *chip;
} CBM_ROM;


extern INT8 cbm_c64_game;
extern INT8 cbm_c64_exrom;
extern CBM_ROM cbm_rom[0x20];

#define CONFIG_DEVICE_CBM_CARTSLOT(file_extensions) \
	CONFIG_DEVICE_CARTSLOT_OPT(2, (file_extensions), cbm_rom_init, NULL, cbm_rom_load, cbm_rom_unload, NULL, NULL)

#define CONFIG_DEVICE_CBM_CARTSLOT_REQ(file_extensions) \
	CONFIG_DEVICE_CARTSLOT_REQ(2, (file_extensions), cbm_rom_init, NULL, cbm_rom_load, cbm_rom_unload, NULL, NULL)

/* prg file format
 * sfx file format
 * sda file format
 * 0 lsb 16bit address
 * 2 chip data */

/* p00 file format (p00 .. p63, s00 .. s63, ..)
 * 0x0000 C64File
 * 0x0007 0
 * 0x0008 Name in commodore encoding?
 * 0x0018 0 0
 * 0x001a lsb 16bit address
 * 0x001c data */

#ifdef __cplusplus
}
#endif

#endif
