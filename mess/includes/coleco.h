#include "driver.h"

/* machine/coleco.c */
extern int coleco_id_rom (int id);
extern int coleco_load_rom (int id);

extern READ_HANDLER  ( coleco_paddle_1_r );
extern READ_HANDLER  ( coleco_paddle_2_r );
extern WRITE_HANDLER ( coleco_paddle_toggle_off );
extern WRITE_HANDLER ( coleco_paddle_toggle_on );

extern READ_HANDLER  ( coleco_VDP_reg_r );
extern WRITE_HANDLER ( coleco_VDP_reg_w );
extern READ_HANDLER  ( coleco_VDP_ram_r );
extern WRITE_HANDLER ( coleco_VDP_ram_w );

/* vidhrdw/coleco.c */
extern int coleco_vh_start(void);
extern void coleco_vh_stop(void);
extern void coleco_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);



