#include "devices/snapquik.h"

/* machine/ti85.c */

extern UINT8 ti85_LCD_memory_base;
extern UINT8 ti85_LCD_contrast;
extern UINT8 ti85_LCD_status;
extern UINT8 ti85_timer_interrupt_mask;

MACHINE_INIT( ti81 );
MACHINE_INIT( ti85 );
MACHINE_STOP( ti85 );
MACHINE_INIT( ti86 );
MACHINE_STOP( ti86 );

NVRAM_HANDLER( ti81 );
NVRAM_HANDLER( ti85 );
NVRAM_HANDLER( ti86 );

SNAPSHOT_LOAD( ti8x );

DEVICE_INIT( ti85_serial );
DEVICE_LOAD( ti85_serial );
DEVICE_UNLOAD( ti85_serial );

WRITE_HANDLER( ti81_port_0007_w);
READ_HANDLER( ti85_port_0000_r);
READ_HANDLER( ti85_port_0001_r);
READ_HANDLER( ti85_port_0002_r);
READ_HANDLER( ti85_port_0003_r);
READ_HANDLER( ti85_port_0004_r);
READ_HANDLER( ti85_port_0005_r);
READ_HANDLER( ti85_port_0006_r);
READ_HANDLER( ti85_port_0007_r);
READ_HANDLER( ti86_port_0005_r);
READ_HANDLER( ti86_port_0006_r);
WRITE_HANDLER( ti85_port_0000_w);
WRITE_HANDLER( ti85_port_0001_w);
WRITE_HANDLER( ti85_port_0002_w);
WRITE_HANDLER( ti85_port_0003_w);
WRITE_HANDLER( ti85_port_0004_w);
WRITE_HANDLER( ti85_port_0005_w);
WRITE_HANDLER( ti85_port_0006_w);
WRITE_HANDLER( ti85_port_0007_w);
WRITE_HANDLER( ti86_port_0005_w);
WRITE_HANDLER( ti86_port_0006_w);

/* vidhrdw/ti85.c */
VIDEO_START( ti85 );
VIDEO_UPDATE( ti85 );
PALETTE_INIT( ti85 );
extern unsigned char ti85_palette[32*7][3];
extern unsigned short ti85_colortable[32][7];
												
