#include "devices/snapquik.h"

/* from machine/vtech1.c */

/* #define OLD_VIDEO */

extern char vtech1_frame_message[64+1];
extern int vtech1_frame_time;

extern int vtech1_latch;

MACHINE_INIT( laser110 );
MACHINE_INIT( laser210 );
MACHINE_INIT( laser310 );

DEVICE_LOAD( vtech1_floppy );

SNAPSHOT_LOAD( vtech1 );

READ_HANDLER ( vtech1_fdc_r );
WRITE_HANDLER ( vtech1_fdc_w );

READ_HANDLER ( vtech1_joystick_r );
READ_HANDLER ( vtech1_keyboard_r );
WRITE_HANDLER ( vtech1_latch_w );

void vtech1_interrupt(void);

#ifdef OLD_VIDEO
/* from vidhrdw/vtech1.c */
VIDEO_UPDATE( vtech1 );
#else
VIDEO_START( vtech1 );
#endif

