#ifndef TRS80_H
#define TRS80_H

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/wd179x.h"
#include "snapquik.h"

#define TRS80_FONT_W 6
#define TRS80_FONT_H 12

extern UINT8 trs80_port_ff;


extern int trs80_cas_init(int id, void *fp, int open_mode);
extern void trs80_cas_exit(int id);

extern QUICKLOAD_LOAD( trs80_cmd );

extern int trs80_floppy_init(int id, void *fp, int open_mode);

extern VIDEO_START( trs80 );
extern VIDEO_UPDATE( trs80 );

extern void trs80_sh_sound_init(const char * gamename);

extern void init_trs80(void);
extern MACHINE_INIT( trs80 );
extern MACHINE_STOP( trs80 );

extern WRITE_HANDLER ( trs80_port_ff_w );
extern READ_HANDLER ( trs80_port_ff_r );
extern READ_HANDLER ( trs80_port_xx_r );

extern INTERRUPT_GEN( trs80_frame_interrupt );
extern INTERRUPT_GEN( trs80_timer_interrupt );
extern INTERRUPT_GEN( trs80_fdc_interrupt );

extern READ_HANDLER( trs80_irq_status_r );
extern WRITE_HANDLER( trs80_irq_mask_w );

extern READ_HANDLER( trs80_printer_r );
extern WRITE_HANDLER( trs80_printer_w );

extern WRITE_HANDLER( trs80_motor_w );

extern READ_HANDLER( trs80_keyboard_r );

extern WRITE_HANDLER( trs80_videoram_w );

#endif	/* TRS80_H */

