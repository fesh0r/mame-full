#include "driver.h"

/* machine/coleco.c */
extern int coleco_init_cart (int id, mame_file *fp, int open_mode);

extern READ_HANDLER  ( coleco_paddle_r );
extern WRITE_HANDLER ( coleco_paddle_toggle_off );
extern WRITE_HANDLER ( coleco_paddle_toggle_on );

