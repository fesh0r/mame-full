
#include "osdepend.h"

// include in ROM
#define VGA_ROM    ROM_LOAD("oakvga.bin", 0xc0000, 0x8000, 0x318c5f43)

extern unsigned char vga_palette[0x100*3];
extern unsigned char ega_palette[0x40*3];

void vga_init(mem_read_handler read_dipswitch);

void vga_reset(void);

// include in port access list
READ_HANDLER( ega_port_03b0_r );
READ_HANDLER( ega_port_03c0_r );
READ_HANDLER( ega_port_03d0_r );

READ_HANDLER( vga_port_03b0_r );
READ_HANDLER( vga_port_03c0_r );
READ_HANDLER( vga_port_03d0_r );

WRITE_HANDLER( vga_port_03b0_w );
WRITE_HANDLER( vga_port_03c0_w );
WRITE_HANDLER( vga_port_03d0_w );

/* include in memory read list
   { 0xa0000, 0xaffff, MRA_BANK1 }
   { 0xb0000, 0xb7fff, MRA_BANK2 }
   { 0xb8000, 0xbffff, MRA_BANK3 }
   { 0xc0000, 0xc7fff, MRA_ROM }
   and in memory write list
   { 0xa0000, 0xaffff, MWA_BANK1 }
   { 0xb0000, 0xb7fff, MWA_BANK2 }
   { 0xb8000, 0xbffff, MWA_BANK3 }
   { 0xc0000, 0xc7fff, MWA_ROM } */

int vga_vh_start(void);
void vga_vh_stop(void);
void ega_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void vga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
