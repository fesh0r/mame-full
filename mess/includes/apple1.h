#include "devices/snapquik.h"

/* machine/apple1.c */

DRIVER_INIT( apple1 );
void apple1_interrupt (void);
SNAPSHOT_LOAD( apple1 );
 READ8_HANDLER( apple1_pia0_kbdin );
 READ8_HANDLER( apple1_pia0_dsprdy );
 READ8_HANDLER( apple1_pia0_kbdrdy );
WRITE8_HANDLER( apple1_pia0_dspout );

/* vidhrdw/apple1.c */

VIDEO_START( apple1 );
VIDEO_UPDATE( apple1 );

void apple1_vh_dsp_w (int data);
void apple1_vh_dsp_clr (void);

/* systems/apple1.c */

extern struct GfxLayout apple1_charlayout;
