/*********************************************************************

	mess.h

	Core MESS headers

*********************************************************************/

#ifndef MESS_H
#define MESS_H

#include <stdarg.h>

#include "osdepend.h"
#include "device.h"
#include "driver.h"
#include "image.h"
#include "artworkx.h"

/***************************************************************************

	Constants

***************************************************************************/

#define LCD_FRAMES_PER_SECOND	30



/**************************************************************************/

extern int devices_inited;

/* MESS_DEBUG is a debug switch (for developers only) for
   debug code, which should not be found in distributions, like testdrivers,...
   contrary to MAME_DEBUG, NDEBUG it should not be found in the makefiles of distributions
   use it in your private root makefile */
/* #define MESS_DEBUG */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	#include <stdbool.h>
#elif (! defined(__bool_true_false_are_defined)) && (! defined(__cplusplus))
	#ifndef bool
		#define bool int
	#endif
	#ifndef true
		#define true 1
	#endif
	#ifndef false
		#define false 0
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

/***************************************************************************/

void showmessinfo(void);
int filemanager(struct mame_bitmap *bitmap, int selected);

UINT32 hash_data_extract_crc32(const char *d);

data32_t read32_with_read8_handler(read8_handler handler, offs_t offset, data32_t mem_mask);
void write32_with_write8_handler(write8_handler handler, offs_t offset, data32_t data, data32_t mem_mask);



#if HAS_WAVE
int tapecontrol(struct mame_bitmap *bitmap, int selected);
void tapecontrol_gettime(char *timepos, size_t timepos_size, mess_image *img, int *curpos, int *endpos);
#endif

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* runs checks to see if device code is proper */
int mess_validitychecks(void);

/* runs a set of test cases on the driver; can pass in an optional callback
 * to provide a way to identify images to test with
 */
void messtestdriver(const struct GameDriver *gamedrv, const char *(*getfodderimage)(unsigned int index, int *foddertype));

/* these are called from mame.c*/
int devices_init(const struct GameDriver *gamedrv);
int devices_initialload(const struct GameDriver *gamedrv, int ispreload);
void devices_exit(void);

char *auto_strlistdup(char *strlist);

int register_device(iodevice_t type, const char *arg);

enum
{
	OSD_FOPEN_READ,
	OSD_FOPEN_WRITE,
	OSD_FOPEN_RW,
	OSD_FOPEN_RW_CREATE
};

/* --------------------------------------------------------------------------------------------- */

/* This call is used to return the next compatible driver with respect to
 * software images.  It is usable both internally and from front ends
 */
const struct GameDriver *mess_next_compatible_driver(const struct GameDriver *drv);
int mess_count_compatible_drivers(const struct GameDriver *drv);

/* --------------------------------------------------------------------------------------------- */

/* RAM configuration calls */
extern UINT32 mess_ram_size;
extern data8_t *mess_ram;
extern data8_t mess_ram_default_value;

/* RAM parsing options */
#define RAM_STRING_BUFLEN 16
UINT32		ram_option(const struct GameDriver *gamedrv, unsigned int i);
int			ram_option_count(const struct GameDriver *gamedrv);
int			ram_is_valid_option(const struct GameDriver *gamedrv, UINT32 ram);
UINT32		ram_default(const struct GameDriver *gamedrv);
UINT32		ram_parse_string(const char *s);
const char *ram_string(char *buffer, UINT32 ram);
int			ram_validate_option(void);
void		ram_dump(const char *filename);

data8_t *memory_install_ram8_handler(int cpunum, int spacenum, offs_t start, offs_t end, offs_t ram_offset, int bank);

/* --------------------------------------------------------------------------------------------- */

/* dummy read handlers */
 READ8_HANDLER(return8_00);
 READ8_HANDLER(return8_FE);
 READ8_HANDLER(return8_FF);

/* gets the path to the MESS executable */
extern const char *mess_path;

void machine_hard_reset(void);

#ifdef __cplusplus
}
#endif

#endif
