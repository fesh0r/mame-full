#include "snapquik.h"

/* machine/ti85.c */

extern UINT8 ti85_LCD_memory_base;
extern UINT8 ti85_LCD_contrast;
extern UINT8 ti85_LCD_status;
extern UINT8 ti85_timer_interrupt_mask;

extern MACHINE_INIT( ti81 );
extern MACHINE_INIT( ti85 );
extern MACHINE_STOP( ti85 );
extern MACHINE_INIT( ti86 );
extern MACHINE_STOP( ti86 );

extern NVRAM_HANDLER( ti81 );
extern NVRAM_HANDLER( ti85 );
extern NVRAM_HANDLER( ti86 );

extern SNAPSHOT_LOAD( ti8x );

extern int ti85_serial_init (int, void *fp, int open_mode);
extern void ti85_serial_exit (int);

extern WRITE_HANDLER( ti81_port_0007_w);
extern READ_HANDLER( ti85_port_0000_r);
extern READ_HANDLER( ti85_port_0001_r);
extern READ_HANDLER( ti85_port_0002_r);
extern READ_HANDLER( ti85_port_0003_r);
extern READ_HANDLER( ti85_port_0004_r);
extern READ_HANDLER( ti85_port_0005_r);
extern READ_HANDLER( ti85_port_0006_r);
extern READ_HANDLER( ti85_port_0007_r);
extern READ_HANDLER( ti86_port_0005_r);
extern READ_HANDLER( ti86_port_0006_r);
extern WRITE_HANDLER( ti85_port_0000_w);
extern WRITE_HANDLER( ti85_port_0001_w);
extern WRITE_HANDLER( ti85_port_0002_w);
extern WRITE_HANDLER( ti85_port_0003_w);
extern WRITE_HANDLER( ti85_port_0004_w);
extern WRITE_HANDLER( ti85_port_0005_w);
extern WRITE_HANDLER( ti85_port_0006_w);
extern WRITE_HANDLER( ti85_port_0007_w);
extern WRITE_HANDLER( ti86_port_0005_w);
extern WRITE_HANDLER( ti86_port_0006_w);

/* vidhrdw/ti85.c */
extern VIDEO_START( ti85 );
extern VIDEO_UPDATE( ti85 );
extern PALETTE_INIT( ti85 );
extern unsigned char ti85_palette[32*7][3];
extern unsigned short ti85_colortable[32][7];
												
