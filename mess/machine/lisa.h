#ifndef LISA_H
#define LISA_H

#include "osdepend.h"

extern VIDEO_START( lisa );
extern VIDEO_UPDATE( lisa );

int lisa_floppy_load(mess_image *img, mame_file *fp, int open_mode);
void lisa_floppy_unload(mess_image *img);
extern NVRAM_HANDLER(lisa);

void init_lisa2(void);
void init_lisa210(void);
void init_mac_xl(void);

extern MACHINE_INIT( lisa );

extern void lisa_interrupt(void);

READ_HANDLER ( lisa_fdc_io_r );
WRITE_HANDLER ( lisa_fdc_io_w );
READ_HANDLER ( lisa_fdc_r );
READ_HANDLER ( lisa210_fdc_r );
WRITE_HANDLER ( lisa_fdc_w );
WRITE_HANDLER ( lisa210_fdc_w );
READ16_HANDLER ( lisa_r );
WRITE16_HANDLER ( lisa_w );

#endif /* LISA_H */

