#include "devices/snapquik.h"

/* machine/galaxy.c */
extern MACHINE_INIT( galaxy );
extern INTERRUPT_GEN( galaxy_interrupt );
extern SNAPSHOT_LOAD( galaxy );
extern int galaxy_interrupts_enabled;
extern  READ8_HANDLER( galaxy_kbd_r );
extern WRITE8_HANDLER( galaxy_kbd_w );

/* vidhrdw/galaxy.c */
extern struct GfxLayout galaxy_charlayout;
extern unsigned char galaxy_palette[2*3];
extern unsigned short galaxy_colortable[1][2];
extern PALETTE_INIT( galaxy );
extern VIDEO_START( galaxy );
extern VIDEO_UPDATE( galaxy );
extern WRITE8_HANDLER( galaxy_vh_charram_w );
