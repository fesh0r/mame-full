
#include "osdepend.h"


/* 
   include in memory read list
   { 0xa0000, 0xaffff, MRA_BANK1 }
   { 0xb0000, 0xb7fff, MRA_BANK2 }
   { 0xb8000, 0xbffff, MRA_BANK3 }
   { 0xc0000, 0xc7fff, MRA_ROM }

   and in memory write list
   { 0xa0000, 0xaffff, MWA_BANK1 }
   { 0xb0000, 0xb7fff, MWA_BANK2 }
   { 0xb8000, 0xbffff, MWA_BANK3 }
   { 0xc0000, 0xc7fff, MWA_ROM }
*/

extern unsigned char vga_palette[0x100][3];
extern unsigned char ega_palette[0x40][3];

void vga_init(mem_read_handler read_dipswitch);

void vga_reset(void);

// include in port access list
READ_HANDLER( ega_port_03b0_r );
READ_HANDLER( ega_port_03c0_r );
READ_HANDLER( ega_port_03d0_r );

READ_HANDLER( paradise_ega_03c0_r );

READ_HANDLER( vga_port_03b0_r );
READ_HANDLER( vga_port_03c0_r );
READ_HANDLER( vga_port_03d0_r );

WRITE_HANDLER( vga_port_03b0_w );
WRITE_HANDLER( vga_port_03c0_w );
WRITE_HANDLER( vga_port_03d0_w );

int ega_vh_start(void);
int vga_vh_start(void);

void vga_vh_stop(void);

void ega_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void vga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

/*
  pega notes (paradise)
  build in amstrad pc1640

  ROM_LOAD("40100", 0xc0000, 0x8000, 0xd2d1f1ae)

  4 additional dipswitches
  seems to have emulation modes at register level
  (mda/hgc lines bit 8 not identical to ega/vga)

  standard ega/vga dipswitches
  00000000	320x200
  00000001	640x200 hanging
  00000010	640x200 hanging
  00000011	640x200 hanging

  00000100	640x350 hanging
  00000101	640x350 hanging EGA mono
  00000110	320x200
  00000111	640x200

  00001000	640x200
  00001001	640x200
  00001010	720x350 partial visible
  00001011	720x350 partial visible

  00001100	320x200
  00001101	320x200
  00001110	320x200
  00001111	320x200

*/

/*
  oak vga (oti 037 chip)
  (below bios patch needed for running)

  ROM_LOAD("oakvga.bin", 0xc0000, 0x8000, 0x318c5f43)
*/
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif
