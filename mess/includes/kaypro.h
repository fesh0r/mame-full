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

extern int kaypro_floppy_init(int id);
extern void init_kaypro(void);
extern void kaypro_init_machine(void);
extern void kaypro_stop_machine(void);

extern int kaypro_interrupt(void);

#define KAYPRO_FONT_W 	8
#define KAYPRO_FONT_H 	16
#define KAYPRO_SCREEN_W	80
#define KAYPRO_SCREEN_H   25

extern int	kaypro_vh_start(void);
extern void kaypro_vh_stop(void);
extern void kaypro_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh);

extern READ_HANDLER ( kaypro_const_r );
extern WRITE_HANDLER ( kaypro_const_w );
extern READ_HANDLER ( kaypro_conin_r );
extern WRITE_HANDLER ( kaypro_conin_w );
extern READ_HANDLER ( kaypro_conout_r );
extern WRITE_HANDLER ( kaypro_conout_w );

extern int	kaypro_sh_start(const struct MachineSound *msound);
extern void kaypro_sh_stop(void);
extern void kaypro_sh_update(void);

extern void kaypro_bell(void);
extern void kaypro_click(void);
