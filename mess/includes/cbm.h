#ifndef __CBM_H_
#define __CBM_H_

#include <stdlib.h>

#include "driver.h"

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

#if 0
#include "snprintf.h"
//#else
/* quick (and unsafe as sprintf) snprintf */
#define snprintf cbm_snprintf
int DECL_SPEC cbm_snprintf (char *str, size_t size, const char *format,...);
#endif

#define memset16 cbm_memset16
void *cbm_memset16 (void *dest, int value, size_t size);

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

#ifndef __cplusplus
typedef int bool;
#endif

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

void cbm_quick_exit (int id);
int cbm_quick_init (int id);
/* pet with basic 1 */
int cbm_pet1_quick_open (int id, int mode, void *arg);
int cbm_pet_quick_open (int id, int mode, void *arg);
int cbm_quick_open (int id, int mode, void *arg);
int cbmb_quick_open (int id, int mode, void *arg);
int cbm500_quick_open (int id, int mode, void *arg);
int cbm_c65_quick_open (int id, int mode, void *arg);

#define IODEVICE_CBM_PET1_QUICK \
{\
   IO_QUICKLOAD,	   /* type */\
   1,				   /* count */\
   "p00\0prg\0",       /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_pet1_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_PET_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_pet_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBMB_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbmb_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM500_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm500_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IODEVICE_CBM_C65_QUICK \
{\
   IO_QUICKLOAD,          /* type */\
   1,                                      /* count */\
   "p00\0prg\0",            /*file extensions */\
   IO_RESET_CPU,	   /* reset if file changed */\
   NULL,               /* id */\
   cbm_quick_init,     /* init */\
   cbm_quick_exit,     /* exit */\
   NULL,               /* info */\
   cbm_c65_quick_open,     /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

/* use to functions to parse, load the rom images into memory
   and then use the cbm_rom var */
int cbm_rom_init(int id);
void cbm_rom_exit(int id);

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

#define IODEVICE_CBM_ROM(extensions, idfunc) \
{\
   IO_CARTSLOT,        /* type */\
   2,                  /* in reality 1 *//* count */\
   extensions,            /*file extensions */\
   IO_RESET_ALL,	   /* reset if file changed */\
   idfunc,             /* id */\
   cbm_rom_init,       /* init */\
   cbm_rom_exit,       /* exit */\
   NULL,               /* info */\
   NULL,               /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

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
