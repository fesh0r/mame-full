#include "driver.h"
#include "devices/snapquik.h"

//#define AMSTRAD_VIDEO_USE_EVENT_LIST

void amstrad_setup_machine(void);
void amstrad_shutdown_machine(void);

extern SNAPSHOT_LOAD( amstrad );

int amstrad_floppy_load(int);
void amstrad_floppy_exit(int);

void Amstrad_Reset(void);

void AmstradCPC_GA_Write(int);
void AmstradCPC_SetUpperRom(int);
void Amstrad_RethinkMemory(void);
void Amstrad_Init(void);
void amstrad_handle_snapshot(unsigned char *);
void AmstradCPC_PALWrite(int);

extern int amstrad_cassette_init(mess_image *img, mame_file *fp, int open_mode);


/* On the Amstrad, any part of the 64k memory can be access by the video
hardware (GA and CRTC - the CRTC specifies the memory address to access,
and the GA fetches 2 bytes of data for each 1us cycle.

The Z80 must also access the same ram.

To maintain the screen display, the Z80 is halted on each memory access.

The result is that timing for opcodes, appears to fall into a nice pattern,
where the time for each opcode can be measured in NOP cycles. NOP cycles is
the name I give to the time taken for one NOP command to execute.

This happens to be 1us.

From measurement, there are 64 NOPs per line, with 312 lines per screen.
This gives a total of 19968 NOPs per frame. */

/* number of us cycles per frame (measured) */
#define AMSTRAD_US_PER_FRAME	19968
#define AMSTRAD_FPS 			50



/* These are the measured visible screen dimensions in CRTC characters.
50 CRTC chars in X, 35 CRTC chars in Y (8 lines per char assumed) */
#define AMSTRAD_SCREEN_WIDTH	(50*16)
#define AMSTRAD_SCREEN_HEIGHT	(35*8)
#define AMSTRAD_MONITOR_SCREEN_WIDTH	(64*16)
#define AMSTRAD_MONITOR_SCREEN_HEIGHT	(39*8)

#ifdef AMSTRAD_VIDEO_USE_EVENT_LIST
/* codes for eventlist */
enum
{
	/* change pen colour with gate array */
	EVENT_LIST_CODE_GA_COLOUR = 0,
	/* change mode with gate array */
	EVENT_LIST_CODE_GA_MODE = 1,
	/* change CRTC register data */
	EVENT_LIST_CODE_CRTC_WRITE = 2,
	/* change CRTC register selection */
        EVENT_LIST_CODE_CRTC_INDEX_WRITE = 3
};
#endif

extern VIDEO_START( amstrad );
extern VIDEO_UPDATE( amstrad );
void amstrad_update_scanline(void);
void amstrad_vh_execute_crtc_cycles(int crtc_execute_cycles);
void amstrad_vh_update_colour(int,int);
void amstrad_vh_update_mode(int);

extern int amstrad_vsync;

/* update interrupt timer */
void amstrad_interrupt_timer_update(void);
/* if start of vsync sound, wait to reset interrupt counter 2 hsyncs later */
void amstrad_interrupt_timer_trigger_reset_by_vsync(void);

/*** AMSTRAD CPC SPECIFIC ***/

/* initialise palette for CPC464, CPC664 and CPC6128 */
extern PALETTE_INIT( amstrad_cpc );

/**** KC COMPACT SPECIFIC ***/

/* initialise palette for KC Compact */
extern PALETTE_INIT( kccomp );

/**** AMSTRAD PLUS SPECIFIC ***/

/* initialise palette for 464plus, 6128plus */
extern PALETTE_INIT( amstrad_plus );

int amstrad_plus_cartridge_load(mess_image *img, mame_file *fp, int open_mode);


