#include "devices/snapquik.h"

/* machine/apple1.c */

DRIVER_INIT( apple1 );
MACHINE_INIT( apple1 );
SNAPSHOT_LOAD( apple1 );

/* vidhrdw/apple1.c */

VIDEO_START( apple1 );
VIDEO_UPDATE( apple1 );

void apple1_vh_dsp_w (int data);
void apple1_vh_dsp_clr (void);
double apple1_vh_dsp_time_to_ready (void);

extern int apple1_vh_clrscrn_pressed;

/* systems/apple1.c */

extern struct GfxLayout apple1_charlayout;
