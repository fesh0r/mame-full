#ifndef TRS80_H
#define TRS80_H

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/wd179x.h"
#include "devices/snapquik.h"

#define TRS80_FONT_W 6
#define TRS80_FONT_H 12

extern UINT8 trs80_port_ff;


int trs80_cas_load(mess_image *img, mame_file *fp, int open_mode);
void trs80_cas_unload(int id);

QUICKLOAD_LOAD( trs80_cmd );

int trs80_floppy_init(mess_image *img, mame_file *fp, int open_mode);

VIDEO_START( trs80 );
VIDEO_UPDATE( trs80 );

void trs80_sh_sound_init(const char * gamename);

void init_trs80(void);
MACHINE_INIT( trs80 );
MACHINE_STOP( trs80 );

WRITE_HANDLER ( trs80_port_ff_w );
READ_HANDLER ( trs80_port_ff_r );
READ_HANDLER ( trs80_port_xx_r );

INTERRUPT_GEN( trs80_frame_interrupt );
INTERRUPT_GEN( trs80_timer_interrupt );
INTERRUPT_GEN( trs80_fdc_interrupt );

READ_HANDLER( trs80_irq_status_r );
WRITE_HANDLER( trs80_irq_mask_w );

READ_HANDLER( trs80_printer_r );
WRITE_HANDLER( trs80_printer_w );

WRITE_HANDLER( trs80_motor_w );

READ_HANDLER( trs80_keyboard_r );

WRITE_HANDLER( trs80_videoram_w );

#endif	/* TRS80_H */

