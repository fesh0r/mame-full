#include "snapquik.h"

/* machine/apple1.c */

extern MACHINE_INIT( apple1 );
extern void apple1_interrupt (void);
extern SNAPSHOT_LOAD( apple1 );
extern READ_HANDLER( apple1_pia0_kbdin );
extern READ_HANDLER( apple1_pia0_dsprdy );
extern READ_HANDLER( apple1_pia0_kbdrdy );
extern WRITE_HANDLER( apple1_pia0_dspout );

/* vidhrdw/apple1.c */

extern VIDEO_START( apple1 );
extern VIDEO_UPDATE( apple1 );

extern void apple1_vh_dsp_w (int data);
extern void apple1_vh_dsp_clr (void);

/* systems/apple1.c */

extern struct GfxLayout apple1_charlayout;
