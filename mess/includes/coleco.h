#include "driver.h"

/* machine/coleco.c */
extern int coleco_id_rom (int id);
extern int coleco_load_rom (int id);

extern READ_HANDLER  ( coleco_paddle_r );
extern WRITE_HANDLER ( coleco_paddle_toggle_off );
extern WRITE_HANDLER ( coleco_paddle_toggle_on );

