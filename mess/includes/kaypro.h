/******************************************************************************
 *
 *	kaypro.h
 *
 *	interface for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *
 ******************************************************************************/

#include "includes/wd179x.h"
#include "machine/cpm_bios.h"

extern void init_kaypro(void);
extern MACHINE_INIT( kaypro );
extern MACHINE_STOP( kaypro );

extern INTERRUPT_GEN( kaypro_interrupt );

#define KAYPRO_FONT_W 	8
#define KAYPRO_FONT_H 	16
#define KAYPRO_SCREEN_W	80
#define KAYPRO_SCREEN_H   25

extern VIDEO_START( kaypro );
extern VIDEO_UPDATE( kaypro );

extern  READ8_HANDLER ( kaypro_const_r );
extern WRITE8_HANDLER ( kaypro_const_w );
extern  READ8_HANDLER ( kaypro_conin_r );
extern WRITE8_HANDLER ( kaypro_conin_w );
extern  READ8_HANDLER ( kaypro_conout_r );
extern WRITE8_HANDLER ( kaypro_conout_w );

extern void kaypro_bell(void);
extern void kaypro_click(void);
