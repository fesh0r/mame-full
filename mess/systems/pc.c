/***************************************************************************

	drivers/pc.c

	PC-XT memory map

	00000-9FFFF   RAM
	A0000-AFFFF   NOP		or videoram EGA/VGA
	B0000-B7FFF   videoram	MDA, page #0
	B8000-BFFFF   videoram	CGA and/or MDA page #1, T1T mapped RAM
	C0000-C7FFF   NOP		or ROM EGA/VGA
	C8000-C9FFF   ROM		XT HDC #1
	CA000-CBFFF   ROM		XT HDC #2
	D0000-EFFFF   NOP		or 'adapter RAM'
	F0000-FDFFF   NOP		or ROM Basic + other Extensions
	FE000-FFFFF   ROM

***************************************************************************/
#include "driver.h"
#include "sound/3812intf.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "printer.h"

#include "includes/uart8250.h"
#include "includes/pic8259.h"
#include "includes/dma8237.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_aga.h"
#include "includes/pc_t1t.h"

#include "includes/pc_hdc.h"
#include "includes/pc_ide.h"
#include "includes/pc_fdc_h.h"
#include "includes/pc_flopp.h"
#include "includes/pckeybrd.h"
#include "includes/pclpt.h"
#include "includes/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/europc.h"
#include "includes/tandy1t.h"
#include "includes/amstr_pc.h"
#include "includes/ibmpc.h"
#include "includes/at.h"

#include "includes/pc.h"

/* window resizing with dirtybuffering traping in xmess window */
//#define RESIZING_WORKING

#define ym3812_StdClock 3579545

/*
  adlib (YM3812/OPL2 chip), part of many many soundcards (soundblaster)
  soundblaster: YM3812 also accessible at 0x228/9 (address jumperable)
  soundblaster pro version 1: 2 YM3812 chips
   at 0x388 both accessed, 
   at 0x220/1 left?, 0x222/3 right? (jumperable)
  soundblaster pro version 2: 1 OPL3 chip

  pro audio spectrum +: 2 OPL2
  pro audio spectrum 16: 1 OPL3
 */
#define ADLIB	/* YM3812/OPL2 Chip */
/*
  creativ labs game blaster (CMS creativ music system)
  2 x saa1099 chips
  also on sound blaster 1.0
  option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define GAMEBLASTER

// IO Expansion, only a little bit for ibm bios self tests
//#define EXP_ON

READ_HANDLER( return_0xff ) { return 0xff; }

static MEMORY_READ_START( pc_readmem )
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xbffff, MRA_NOP },
	{ 0xc0000, 0xc7fff, MRA_NOP },
	{ 0xc8000, 0xcffff, MRA_ROM },
	{ 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc_writemem )
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xbffff, MWA_NOP },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xcffff, MWA_ROM },
	{ 0xd0000, 0xeffff, MWA_NOP },
	{ 0xf0000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( pc_readport )
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0063, ppi8255_0_r },
	{ 0x0080, 0x0087, pc_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x240, 0x257, pc_rtc_r },
//	{ 0x240, 0x257, return_0xff }, // anonymous bios should not recogniced realtimeclock
#ifdef EXP_ON
	{ 0x0210, 0x0217, pc_EXP_r },
#endif
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
//	{ 0x340, 0x357, pc_rtc_r },
	{ 0x340, 0x357, return_0xff }, // anonymous bios should not recogniced realtimeclock
	{ 0x0378, 0x037b, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
PORT_END

static PORT_WRITE_START( pc_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, ppi8255_0_w },
	{ 0x0080, 0x0087, pc_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
#ifdef EXP_ON
    { 0x0210, 0x0217, pc_EXP_w },
#endif
#ifdef GAMEBLASTER
	{ 0x220, 0x220, saa1099_write_port_0_w },
	{ 0x221, 0x221, saa1099_control_port_0_w },
	{ 0x222, 0x222, saa1099_write_port_1_w },
	{ 0x223, 0x223, saa1099_control_port_1_w },
#endif
	{ 0x240, 0x257, pc_rtc_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
//	{ 0x340, 0x357, pc_rtc_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
PORT_END

static MEMORY_READ_START( europc_readmem )
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xaffff, MRA_NOP },
	{ 0xb0000, 0xbffff, pc_aga_videoram_r },
	{ 0xc0000, 0xc7fff, MRA_NOP },
	{ 0xc8000, 0xcffff, MRA_ROM },
	{ 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( europc_writemem )
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xaffff, MWA_NOP },
	{ 0xb0000, 0xbffff, pc_aga_videoram_w, &videoram, &videoram_size },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xcffff, MWA_ROM },
	{ 0xd0000, 0xeffff, MWA_NOP },
	{ 0xf0000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( europc_readport)
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0063, europc_pio_r },
	{ 0x0080, 0x0087, pc_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x0250, 0x025f, europc_jim_r },
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x2e0, 0x2e0, europc_jim2_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
//	{ 0x0350, 0x035f, europc_r },
	{ 0x0378, 0x037b, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x3b0, 0x3bf, pc_aga_mda_r },
//	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x3d0, 0x3df, pc_aga_cga_r },
	{ 0x03d0, 0x03df, pc_CGA_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
PORT_END

static PORT_WRITE_START( europc_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, europc_pio_w },
	{ 0x0080, 0x0087, pc_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
	{ 0x0250, 0x025f, europc_jim_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
//	{ 0x0350, 0x035f, europc_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x3b0, 0x3bf, pc_aga_mda_w },
//	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x3d0, 0x3df, pc_aga_cga_w },
	{ 0x03d0, 0x03df, pc_CGA_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
PORT_END

static MEMORY_READ_START(t1t_readmem)
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xaffff, MRA_RAM },
	{ 0xb0000, 0xb7fff, MRA_NOP },
	{ 0xb8000, 0xbffff, pc_t1t_videoram_r },
	{ 0xc0000, 0xc7fff, MRA_NOP },
	{ 0xc8000, 0xc9fff, MRA_ROM },
	{ 0xca000, 0xcffff, MRA_NOP },
	{ 0xd0000, 0xeffff, MRA_NOP },
	{ 0xf0000, 0xfffff, MRA_ROM },
PORT_END

static MEMORY_WRITE_START( t1t_writemem )
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xaffff, MWA_RAM },
    { 0xb0000, 0xb7fff, MWA_NOP },
	{ 0xb8000, 0xbffff, pc_t1t_videoram_w },
	{ 0xc0000, 0xc7fff, MWA_NOP },
	{ 0xc8000, 0xc9fff, MWA_ROM },
	{ 0xca000, 0xcffff, MWA_NOP },
	{ 0xd0000, 0xeffff, MWA_NOP },
	{ 0xf0000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( t1t_readport )
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0063, tandy1000_pio_r },
	{ 0x0080, 0x0087, pc_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x0378, 0x037f, pc_t1t_p37x_r },
    { 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03d0, 0x03df, pc_T1T_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
PORT_END

static PORT_WRITE_START( t1t_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, tandy1000_pio_w },
	{ 0x0080, 0x0087, pc_page_w },
	{ 0x00c0, 0x00c0, SN76496_0_w },
	{ 0x0200, 0x0207, pc_JOY_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x0378, 0x037f, pc_t1t_p37x_w },
    { 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03d0, 0x03df, pc_T1T_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
PORT_END

static MEMORY_READ_START( pc1640_readmem )
	{ 0x00000, 0x7ffff, MRA_RAM },
	{ 0x80000, 0x9ffff, MRA_RAM },
	{ 0xa0000, 0xbffff, MRA_NOP },
	{ 0xc0000, 0xc7fff, MRA_ROM },
    { 0xc8000, 0xcffff, MRA_ROM },
    { 0xd0000, 0xfbfff, MRA_NOP },
	{ 0xfc000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pc1640_writemem )
	{ 0x00000, 0x7ffff, MWA_RAM },
	{ 0x80000, 0x9ffff, MWA_RAM },
	{ 0xa0000, 0xbffff, MWA_NOP },
	{ 0xc0000, 0xc7fff, MWA_ROM },
	{ 0xc8000, 0xcffff, MWA_ROM },
    { 0xd0000, 0xfbfff, MWA_NOP },
	{ 0xfc000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( pc1640_readport )
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0065, pc1640_port60_r },
	{ 0x0070, 0x0071, mc146818_port_r },
	{ 0x0078, 0x0078, pc1640_mouse_x_r },
	{ 0x007a, 0x007a, pc1640_mouse_y_r },
	{ 0x0080, 0x0087, pc_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x0378, 0x037b, pc1640_port378_r },
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
PORT_END


static PORT_WRITE_START( pc1640_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0065, pc1640_port60_w },
	{ 0x0070, 0x0071, mc146818_port_w },
	{ 0x0078, 0x0078, pc1640_mouse_x_w },
	{ 0x007a, 0x007a, pc1640_mouse_y_w },
	{ 0x0080, 0x0087, pc_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
	{ 0x03bc, 0x03bd, pc_parallelport0_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
PORT_END

static MEMORY_READ_START( at_readmem )
	{ 0x000000, 0x07ffff, MRA_RAM },
	{ 0x080000, 0x09ffff, MRA_RAM },
	{ 0x0a0000, 0x0affff, MRA_NOP },
	{ 0x0b0000, 0x0b7fff, MRA_NOP },
	{ 0x0b8000, 0x0bffff, MRA_RAM },
	{ 0x0c0000, 0x0c7fff, MRA_ROM },
    { 0x0c8000, 0x0cffff, MRA_ROM },
    { 0x0d0000, 0x0effff, MRA_ROM },
	{ 0x0f0000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x1fffff, MRA_RAM },
	{ 0x200000, 0xfeffff, MRA_NOP },
	{ 0xff0000, 0xffffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( at_writemem )
	{ 0x000000, 0x07ffff, MWA_RAM },
	{ 0x080000, 0x09ffff, MWA_RAM },
	{ 0x0a0000, 0x0affff, MWA_NOP },
	{ 0x0b0000, 0x0b7fff, MWA_NOP },
	{ 0x0b8000, 0x0bbfff, pc_cga_videoram_w, &videoram, &videoram_size },
	{ 0x0c0000, 0x0c7fff, MWA_ROM },
	{ 0x0c8000, 0x0cffff, MWA_ROM },
    { 0x0d0000, 0x0effff, MWA_ROM },
	{ 0x0f0000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x1fffff, MWA_RAM },
	{ 0x200000, 0xfeffff, MWA_NOP },
	{ 0xff0000, 0xffffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( at_readport )
	{ 0x0000, 0x001f, dma8237_0_r },
	{ 0x0020, 0x003f, pic8259_0_r },
	{ 0x0040, 0x005f, pit8253_0_r },
	{ 0x0060, 0x006f, at_8042_r },
	{ 0x0070, 0x007f, mc146818_port_r },
	{ 0x0080, 0x009f, at_page_r }, // 90-9f ?
	{ 0x00a0, 0x00bf, pic8259_1_r },
	{ 0x00c0, 0x00df, dma8237_at_1_r },
	{ 0x01f0, 0x01f7, at_mfm_0_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x220, 0x22f, soundblaster_r },
	{ 0x0278, 0x027f, pc_parallelport2_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x0378, 0x037f, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
PORT_END

static PORT_WRITE_START( at_writeport )
	{ 0x0000, 0x001f, dma8237_0_w },
	{ 0x0020, 0x003f, pic8259_0_w },
	{ 0x0040, 0x005f, pit8253_0_w },
	{ 0x0060, 0x006f, at_8042_w },
	{ 0x0070, 0x007f, mc146818_port_w },
	{ 0x0080, 0x009f, at_page_w },
	{ 0x00a0, 0x00bf, pic8259_1_w },
	{ 0x00c0, 0x00df, dma8237_at_1_w },
	{ 0x01f0, 0x01f7, at_mfm_0_w },
	{ 0x0200, 0x0207, pc_JOY_w },
#if 0 && defined(GAMEBLASTER)
	{ 0x220, 0x220, saa1099_write_port_0_w },
	{ 0x221, 0x221, saa1099_control_port_0_w },
	{ 0x222, 0x222, saa1099_write_port_1_w },
	{ 0x223, 0x223, saa1099_control_port_1_w },
#endif
    { 0x220, 0x22f, soundblaster_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
PORT_END

#define PC_JOYSTICK \
	PORT_START	/* IN15 */\
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED ) \
	PORT_BITX( 0x0010, 0x0000, IPT_BUTTON1,	"Joystick 1 Button 1", CODE_DEFAULT, CODE_NONE)\
	PORT_BITX( 0x0020, 0x0000, IPT_BUTTON2,	"Joystick 1 Button 2", CODE_DEFAULT, CODE_NONE)\
	PORT_BITX( 0x0040, 0x0000, IPT_BUTTON1|IPF_PLAYER2,	"Joystick 2 Button 1", CODE_NONE, JOYCODE_2_BUTTON1)\
	PORT_BITX( 0x0080, 0x0000, IPT_BUTTON2|IPF_PLAYER2,	"Joystick 2 Button 2", CODE_NONE, JOYCODE_2_BUTTON2)\
		\
	PORT_START	/* IN16 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)\
		\
	PORT_START /* IN17 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)\
		\
	PORT_START	/* IN18 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)\
		\
	PORT_START /* IN19 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_UP,JOYCODE_2_DOWN)

INPUT_PORTS_START( pcmda )
	PORT_START /* IN0 */
	PORT_BIT ( 0x80, 0x80,	 IPT_VBLANK )
	PORT_BIT ( 0x7f, 0x7f,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x30, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x02, DEF_STR(Yes) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Any floppy drive installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x01, DEF_STR(Yes) )

    PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x80, DEF_STR(Yes) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x40, DEF_STR(Yes) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PC_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pccga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PC_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( europc )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */

	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	EUROPC_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( bondwell )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Turbo Switch", CODE_DEFAULT, CODE_NONE )
	PORT_DIPSETTING(	0x00, "Off(4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On(12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( xtcga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Turbo Switch", CODE_DEFAULT, CODE_NONE )
	PORT_DIPSETTING(	0x00, "Off(4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On(12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PC_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( tandy1t )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BIT ( 0xff, 0xff,	 IPT_UNUSED )

    PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BIT ( 0x30, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x06, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	TANDY1000_KEYB

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pc200 )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Name/Language", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "English/less checks" )
	PORT_DIPSETTING(	0x01, "Italian/Italiano" ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. vänta" ) 
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, "Spanish/Español" ) //Por favor tilde n
	PORT_DIPSETTING(	0x05, "French/Francais" ) //patientez cedilla c
	PORT_DIPSETTING(	0x06, "German/Deutsch" ) // bitte warten
	PORT_DIPSETTING(	0x07, "English" ) // please wait
	PORT_BITX( 0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x40", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x08, "0x08" )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x80", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x10, "0x10" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Integrated Graphics Adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "CGA 1" )
	PORT_DIPSETTING(	0x10, "CGA 2" )
	PORT_DIPSETTING(	0x20, "external" )
	PORT_DIPSETTING(	0x30, "MDA" )
	PORT_BITX( 0xc0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Startup Mode", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "external Color 80 Columns" )
	PORT_DIPSETTING(	0x40, "Color 40 Columns" )
	PORT_DIPSETTING(	0x80, "Color 80 Columns" )
	PORT_DIPSETTING(	0xc0, "Mono" )
	PORT_START /* IN2 */
PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pc1512 )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Name/Language", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "English/512k only/less checks" )
	PORT_DIPSETTING(	0x01, "Italian/Italiano" ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. vänta" ) 
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, "Spanish/Español" ) //Por favor tilde n
	PORT_DIPSETTING(	0x05, "French/Francais" ) //patientez cedilla c
	PORT_DIPSETTING(	0x06, "German/Deutsch" ) // bitte warten
	PORT_DIPSETTING(	0x07, "English" ) // please wait
	PORT_BIT( 0x20, 0x20,	IPT_UNUSED ) // pc1512 integrated special cga
	PORT_BIT( 0xc0, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe00, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe000, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_START /* IN2 */
PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AMSTRAD_KEYBOARD

//	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pc1640 )
	PORT_START	/* IN0 */
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 5", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 6", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 7", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 8", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Name/Language", CODE_NONE, CODE_NONE )
//	PORT_DIPSETTING(	0x00, "PC 512k" ) // machine crashes with ega bios at 0xc0000
	PORT_DIPSETTING(	0x01, "Italian/Italiano" ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. vänta" ) 
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, "Spanish/Español" ) //Por favor tilde n
	PORT_DIPSETTING(	0x05, "French/Francais" ) //patientez cedilla c
	PORT_DIPSETTING(	0x06, "German/Deutsch" ) // bitte warten
	PORT_DIPSETTING(	0x07, "English" ) // please wait
	PORT_BIT( 0x20, 0x00,	IPT_UNUSED ) // not pc1512 integrated special cga
	PORT_BITX( 0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x40", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x40, "0x40" )
	PORT_BITX( 0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x80", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x80, "0x80" )
#if 0
	PORT_BITX( 0x200, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x20 after 27[8a] read (font?)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x200, "0x20" )
	PORT_BITX( 0x400, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x40 after 27[8a] read (font?)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x400, "0x40" )
	PORT_BITX( 0x800, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x80 after 27[8a] read (font?)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x800, "0x80" )
	PORT_BITX( 0x2000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x20 after 427a read", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x2000, "0x20" )
	PORT_BITX( 0x4000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, " 37a 0x40 after 427a read", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x4000, "0x40" )
	PORT_BITX( 0x8000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, " 37a 0x80 after 427a read", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x8000, "0x80" )
#else
	PORT_BITX( 0xe00, 0x600, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a after 427a read", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x000, "?0standard" ) // diaresis a, seldom c, acut u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x200, "?scandinavian" ) //diaresis a, o slashed, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x600, "?spain" ) // tilde a, seldom c, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0xa00, "?greeck" ) // E, circumflex ???,micro, I, Z, big kappa?
	PORT_DIPSETTING(	0xe00, "?standard" ) // diaresis a, seldom e, acute u, circumflex e, grave a, acute e
	PORT_BITX( 0xe000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a after 427a read", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x0000, "?Integrated Graphic Adapter IGA (EGA)" )
	PORT_DIPSETTING(	0x2000, "?External EGA/VGA" )
	PORT_DIPSETTING(	0x6000, "CGA 40 Columns" )
	PORT_DIPSETTING(	0xa000, "CGA 80 Columns" )
	PORT_DIPSETTING(	0xe000, "MDA/Hercules/Multiple Graphic Adapters" )
#endif
	PORT_START /* IN2 */
PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AMSTRAD_KEYBOARD

//	INPUT_MICROSOFT_MOUSE

INPUT_PORTS_END

INPUT_PORTS_START( xtvga )
	PORT_START /* IN0 */
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )		PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Turbo Switch", CODE_DEFAULT, CODE_NONE )
	PORT_DIPSETTING(	0x00, "Off(4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On(12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( atcga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( atvga )
	PORT_START /* IN0 */
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", CODE_NONE, CODE_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

static unsigned i86_address_mask = 0x000fffff;
static unsigned i286_address_mask = 0x00ffffff;


static struct CustomSound_interface pc_sound_interface = {
	pc_sh_custom_start,
	pc_sh_stop,
	pc_sh_custom_update
};

#if defined(GAMEBLASTER)
static struct SAA1099_interface cms_interface = {
	2, 
	{
		{ 50, 50 },
		{ 50, 50 }
	}
};
#endif

#if defined(ADLIB)
/* irq line not connected to pc on adlib cards (and compatibles) */
static void irqhandler(int linestate) {}

static struct YM3812interface ym3812_interface = {
	1,
	ym3812_StdClock, /* I hope this is the clock used on the original Adlib Sound card */
	{255}, /* volume adjustment in relation to speaker and tandy1000 sound neccessary */
	{irqhandler}
};
#endif

static struct SN76496interface t1t_sound_interface = {
	1,
	{2386360},
	{255,}
};

static struct DACinterface dac_interface= { 1, { 50 }};

static struct GfxLayout pc_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8,
	  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8 },
	8*8 					/* every char takes 8 bytes (upper half) */
};

static struct GfxLayout pc_mda_gfxlayout_1bpp =
{
	8,1,					/* 8 x 32 graphics */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bit planes */
    /* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets (we only use one byte to build the block) */
	{ 0 },
	8						/* every code takes 1 byte */
};

static struct GfxDecodeInfo pc_mda_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pc_mda_charlayout,		0, 256 },
	{ 1, 0x1000, &pc_mda_gfxlayout_1bpp,256*2,	 1 },	/* 640x400x1 gfx */
    { -1 } /* end of array */
};

/* to be done:
   only 2 digital color lines to mda/hercules monitor
   (maximal 4 colors) */
static unsigned short mda_colortable[] = {
     0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 2,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     0, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2, 0,10, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* the two colors for HGC graphics */
     0, 10
};


static struct GfxLayout CGA_charlayout =
{
	8,16,					/* 8 x 16 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8,
	  0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static struct GfxLayout CGA_gfxlayout_1bpp =
{
    8,1,                   /* 8 x 32 graphics */
    256,                    /* 256 codes */
    1,                      /* 1 bit per pixel */
    { 0 },                  /* no bit planes */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
    8                       /* every code takes 1 byte */
};

static struct GfxLayout CGA_gfxlayout_2bpp =
{
	4,1,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
    { 0,2,4,6  },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
    8                       /* every code takes 1 byte */
};

static struct GfxLayout europc_cga_charlayout =
{
	8,16,					/* 8 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8,
	  8*8,9*8,10*8,11*8,
	  12*8,13*8,14*8,15*8 },
    8*16                     /* every char takes 8 bytes */
};

static struct GfxLayout europc_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static struct GfxLayout vga_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{
		2*8, 6*8, 10*8, 14*8, 18*8, 22*8, 26*8, 30*8,
		34*8, 38*8, 42*8, 46*8, 50*8, 54*8, 58*8, 62*8,
		66*8, 70*8, 74*8, 78*8, 82*8, 86*8, 90*8, 94*8,
		98*8, 102*8, 106*8, 110*8, 114*8, 118*8, 122*8, 126*8
	},
	128*8
};


static unsigned short cga_colortable[] = {
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,
/* the color sets for 1bpp graphics mode */
	 0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	 0,8, 0,9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
/* the color sets for 2bpp graphics mode */
     /*0, 2, 4, 6,*/  0,10,12,14,
     /*0, 3, 5, 7,*/  0,11,13,15 // only 2 sets!?
};

static struct GfxDecodeInfo CGA_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,  16 },   /* 640x400x1 gfx */
	{ 1, 0x1000, &CGA_gfxlayout_2bpp, 256*2+16*2,   2 },   /* 320x200x4 gfx */
    { -1 } /* end of array */
};

static struct GfxDecodeInfo europc_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &europc_cga_charlayout,	   0, 256 },   /* single width */
	{ 1, 0x2000, &CGA_gfxlayout_1bpp,	   256*2,  16 },   /* 640x400x1 gfx */
	{ 1, 0x2000, &CGA_gfxlayout_2bpp, 256*2+16*2,   2 },   /* 320x200x4 gfx */
	{ 1, 0x1000, &europc_mda_charlayout,	   256*2+16*2+2*4, 256 },   /* single width */
	{ 1, 0x2000, &pc_mda_gfxlayout_1bpp,256*2+16*2+2*4+256*2,	 1 },	/* 640x400x1 gfx */
    { -1 } /* end of array */
};

static struct GfxDecodeInfo aga_gfxdecodeinfo[] =
{
	{ 1, 0x0800, &CGA_charlayout,			   0, 256 },   /* single width */
	{ 1, 0x2000, &CGA_gfxlayout_1bpp,	   256*2,  16 },   /* 640x400x1 gfx */
	{ 1, 0x2000, &CGA_gfxlayout_2bpp, 256*2+16*2,   2 },   /* 320x200x4 gfx */
	{ 1, 0x1000, &pc_mda_charlayout,			   256*2+16*2+2*4, 256 },   /* single width */
	{ 1, 0x2000, &pc_mda_gfxlayout_1bpp, 256*2+16*2+2*4+2*256,	 1 },	/* 640x400x1 gfx */
	{ 1, 0x0000, &CGA_charlayout,			   0, 256 },   /* thin cga charset */
    { -1 } /* end of array */
};

static struct GfxDecodeInfo vga_gfxdecodeinfo[] =
{
	{ 0, 0x0000, &vga_charlayout,			  0, 256 },   /* single width */
    { -1 } /* end of array */
};

static struct GfxLayout t1t_gfxlayout_4bpp =
{
	2,1,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
	1*8 					/* every code takes 1 byte */
};

static struct GfxDecodeInfo t1t_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &europc_cga_charlayout,		  0,			 256 },	/* single width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,		  16 },	/* 640x400 1bpp gfx */
    { 1, 0x1000, &CGA_gfxlayout_2bpp,     256*2+16*2,      4 },  /* 320x200 2bpp gfx */
	{ 1, 0x1000, &t1t_gfxlayout_4bpp,	  256*2+16*2+2*4, 16 },	/* 160x200 4bpp gfx */
    { -1 } /* end of array */
};


/* Initialise the mda palette */
static void mda_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
//    memcpy(sys_palette,mda_palette,sizeof(mda_palette));
    memcpy(sys_palette,cga_palette,sizeof(cga_palette));
    memcpy(sys_colortable,mda_colortable,sizeof(mda_colortable));
}

static struct MachineDriver machine_driver_pcmda =
{
    /* basic machine hardware */
    {
        {
            CPU_V20,
            4772720,    /* 4,77 Mhz */
            pc_readmem,pc_writemem,
            pc_readport,pc_writeport,
            pc_mda_frame_interrupt,4,
            0,0,
            &i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
    0,
    pc_mda_init_machine,
	0,

    /* video hardware */
    80*9,                                       /* screen width */
    25*14,                                      /* screen height */
    { 0,80*9-1, 0,25*14-1 },                    /* visible_area */
    pc_mda_gfxdecodeinfo,                       /* graphics decode info */
//    sizeof(mda_palette) / sizeof(mda_palette[0]),
    sizeof(cga_palette) / sizeof(cga_palette[0]),
    sizeof(mda_colortable) / sizeof(mda_colortable[0]),
    mda_init_palette,                           /* init palette */

    VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
    0,
    pc_mda_vh_start,
    pc_mda_vh_stop,
    pc_mda_vh_screenrefresh,

    /* sound hardware */
    0,0,0,0,
    {
        { SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
        { SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
    }
};

/* Initialise the cga palette */
static void cga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,cga_palette,sizeof(cga_palette));
	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
}

static struct MachineDriver machine_driver_pccga =
{
    /* basic machine hardware */
    {
        {
            CPU_I88,
			4772720,	/* 4,77 Mhz */
			pc_readmem,pc_writemem,
			pc_readport,pc_writeport,
			pc_cga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_cga_init_machine,
	0,

    /* video hardware */
    80*8, 25*8, 									/* screen width, height  */
	{ 0,80*8-1, 0,25*8-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_cga_vh_start,
	pc_cga_vh_stop,
	pc_cga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
	}
};

/* Initialise the cga palette */
static void aga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,cga_palette,sizeof(cga_palette));
	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
	memcpy((char*)sys_colortable+sizeof(cga_colortable), mda_colortable, sizeof(mda_colortable));
}

static struct MachineDriver machine_driver_europc =
{
    /* basic machine hardware */
    {
        {
            CPU_I88,
			4772720*2,
			europc_readmem,europc_writemem,
			europc_readport,europc_writeport,
			pc_aga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_aga_init_machine,
	0,

    /* video hardware */
    80*9,                                       /* screen width */
	25*14, 									/* screen height (pixels doubled) */
	{ 0,80*9-1, 0,25*14-1},					/* visible_area */
	europc_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	(sizeof(cga_colortable)+sizeof(mda_colortable) )/sizeof(cga_colortable[0]),
	aga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_aga_vh_start,
	pc_aga_vh_stop,
	pc_aga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
	},
	europc_rtc_nvram_handler
};

static struct MachineDriver machine_driver_xtcga =
{
    /* basic machine hardware */
    {
        {
            CPU_I86,
			12000000,
			pc_readmem,pc_writemem,
			pc_readport,pc_writeport,
			pc_cga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_cga_init_machine,
	0,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_cga_vh_start,
	pc_cga_vh_stop,
	pc_cga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
	}
};

static struct MachineDriver machine_driver_pc200 =
{
    /* basic machine hardware */
    {
        {
            CPU_I86,
			8000000,
			pc1640_readmem,pc1640_writemem,
			pc1640_readport,pc1640_writeport,
			pc_aga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_aga_init_machine,
	0,

    80*9,                                       /* screen width */
	25*14, 									/* screen height (pixels doubled) */
	{ 0,80*9-1, 0,25*14-1},					/* visible_area */
	aga_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	(sizeof(cga_colortable)+sizeof(mda_colortable) )/sizeof(cga_colortable[0]),
	aga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_aga_vh_start,
	pc_aga_vh_stop,
	pc200_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
	},
	mc146818_nvram_handler
};

static struct MachineDriver machine_driver_pc1512 =
{
    /* basic machine hardware */
    {
        {
            CPU_I86,
			8000000,
			pc1640_readmem,pc1640_writemem,
			pc1640_readport,pc1640_writeport,
			pc_cga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_cga_init_machine,
	0,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc1512_vh_start,
	pc1512_vh_stop,
	pc1512_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
	},
	mc146818_nvram_handler
};

static void ega_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,ega_palette,sizeof(ega_palette));
	memcpy(sys_colortable,cga_colortable,0x200);
}

static struct MachineDriver machine_driver_pc1640 =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			8000000,
			pc1640_readmem,pc1640_writemem,
			pc1640_readport,pc1640_writeport,
			pc_vga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_vga_init_machine,
	0,

    /* video hardware */
    720,                                       /* screen width */
	350, 									/* screen height (pixels doubled) */
	{ 0,720-1, 0,350-1},					/* visible_area */
	vga_gfxdecodeinfo,							/* graphics decode info */
	sizeof(ega_palette) / sizeof(ega_palette[0]),
	0x100*2, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
	ega_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER,
	0,
	ega_vh_start,
	vga_vh_stop,
	ega_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
	},
	mc146818_nvram_handler
};

static void vga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,vga_palette,sizeof(vga_palette));
	memcpy(sys_colortable,cga_colortable,0x200);
}


static struct MachineDriver machine_driver_xtvga =
{
	/* basic machine hardware */
	{
		{
			CPU_I86,
			12000000,	/* 4,77 Mhz */
			pc_readmem,pc_writemem,
			pc_readport,pc_writeport,
			pc_vga_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_vga_init_machine,
	0,

    /* video hardware */
    720,                                       /* screen width */
	480, 									/* screen height (pixels doubled) */
	{ 0,720-1, 0,480-1},					/* visible_area */
	vga_gfxdecodeinfo,							/* graphics decode info */
	sizeof(vga_palette) / sizeof(vga_palette[0]),
	0x100*2, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
	vga_init_palette,							/* init palette */
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,
	vga_vh_start,
	vga_vh_stop,
	vga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
	}
};

static struct MachineDriver machine_driver_t1000hx =
{
    /* basic machine hardware */
    {
        {
			CPU_I88,
			8000000, // 7.16 ?
			t1t_readmem,t1t_writemem,
			t1t_readport,t1t_writeport,
			tandy1000_frame_interrupt,4,
			0,0,
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_t1t_init_machine,
	0,

    /* video hardware */
	80*8,										/* screen width */
	25*9, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*9-1},					/* visible_area */
	t1t_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING_WORKING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
#else
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
#endif
	0,
	pc_t1t_vh_start,
	pc_t1t_vh_stop,
	pc_t1t_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface }, /* is this available on a Tandy ? */
		{ SOUND_SN76496, &t1t_sound_interface },
	},
	tandy1000_nvram_handler
};

static struct MachineDriver machine_driver_atcga =
{
    /* basic machine hardware */
    {
        {
            CPU_I286,
			12000000, /* original at 6 mhz, at03 8 megahertz */
			at_readmem,at_writemem,
			at_readport,at_writeport,
			at_cga_frame_interrupt,4,
			0,0,
			&i286_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	at_machine_init,
	0,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_cga_vh_start,
	pc_cga_vh_stop,
	pc_cga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
		{ SOUND_DAC, &dac_interface },
	},
	mc146818_nvram_handler
};

static struct MachineDriver machine_driver_atvga =
{
	/* basic machine hardware */
	{
		{
			CPU_I286,
			12000000,
			at_readmem,at_writemem,
			at_readport,at_writeport,
			at_vga_frame_interrupt,4,
			0,0,
			&i286_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	pc_vga_init_machine,
	0,

    /* video hardware */
    720,                                       /* screen width */
	480, 									/* screen height (pixels doubled) */
	{ 0,720-1, 0,480-1},					/* visible_area */
	vga_gfxdecodeinfo,							/* graphics decode info */
	sizeof(vga_palette) / sizeof(vga_palette[0]),
	0x100*2, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
	vga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	vga_vh_start,
	vga_vh_stop,
	vga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
		{ SOUND_DAC, &dac_interface },
	},
	mc146818_nvram_handler
};

#ifdef HAS_I386
static struct MachineDriver machine_driver_at386 =
{
    /* basic machine hardware */
    {
        {
            CPU_I386,
			12000000, /* original at 6 mhz, at03 8 megahertz */
			at_readmem,at_writemem,
			at_readport,at_writeport,
			at_cga_frame_interrupt,4,
			0,0,
			&i286_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	at_machine_init,
	0,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(cga_palette) / sizeof(cga_palette[0]),
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

#ifdef RESIZING
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_TYPE_RASTER,
#endif
	0,
	pc_cga_vh_start,
	pc_cga_vh_stop,
	pc_cga_vh_screenrefresh,

    /* sound hardware */
	0,0,0,0,
	{
		{ SOUND_CUSTOM, &pc_sound_interface },
#if defined(ADLIB)
		{ SOUND_YM3812, &ym3812_interface },
#endif
#if defined(GAMEBLASTER)
		{ SOUND_SAA1099, &cms_interface },
#endif
		{ SOUND_DAC, &dac_interface },
	},
	mc146818_nvram_handler
};
#endif

#if 0
	//pcjr roms? (incomplete dump, most likely 64 kbyte)
	// basic c1.20 
    ROM_LOAD("basic.rom", 0xf6000, 0x8000, 0x0c19c1a8)
	// ???
    ROM_LOAD("bios.rom", 0x??000, 0x2000, 0x98463f95)

	/* turbo xt */
	/* basic c1.10 */
    ROM_LOAD("rom05.bin", 0xf6000, 0x2000, 0x80d3cf5d)
    ROM_LOAD("rom04.bin", 0xf8000, 0x2000, 0x673a4acc)
    ROM_LOAD("rom03.bin", 0xfa000, 0x2000, 0xaac3fc37)
    ROM_LOAD("rom02.bin", 0xfc000, 0x2000, 0x3062b3fc)
	/* sw1 0x60 readback fails write 301 to screen fe3b7 */
	/* disk problems no disk gives 601 */
	/* 5000-026 08/16/82 */
    ROM_LOAD("rom01.bin", 0xfe000, 0x2000, 0x5c3f0256)

	/* anonymous works nice */
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)

	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, 0xd435a405)

	/* europc */
    ROM_LOAD("50145", 0xf8000, 0x8000, 0x1775a11d) // V2.07
//    ROM_LOAD("eurobios.bin", 0xf8000, 0x8000, 0x52185223) scrap
	/* cga, hercules character set */
    ROM_LOAD("50146", 0x00000, 0x02000, 0x1305dcf5) //D1.0

	// ibm pc
	// most likely 8 kbyte chips
    ROM_LOAD("basicpc.bin", 0xf6000, 0x8000, 0xebacb791) // IBM C1.1
	// split into 8 kbyte parts
	// the same as in the basic c1.10 as in the turboxt
	// 1501-476 10/27/82
    ROM_LOAD("biospc.bin", 0xfe000, 0x2000, 0xe88792b3)

	/* tandy 1000 hx */
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)

	// ibm xt
    ROM_LOAD("xthdd.c8", 0xc8000, 0x2000, 0xa96317da)
    ROM_LOAD("biosxt.bin", 0xf0000, 0x10000, 0x36c32fde) // BASIC C1.1
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmxt.0", 0xf0000, 0x8000, 0x83727c42)
    ROM_LOAD_ODD("ibmxt.1", 0xf0000, 0x8000, 0x2a629953)

	// ibm at
	// most likely 2 32 kbyte chips for 16 bit access
    ROM_LOAD("atbios.bin", 0xf0000, 0x10000, 0x674426be) // BASIC C1.1, beeps
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmat.0", 0xf0000, 0x8000, 0x4995be7a)
    ROM_LOAD_ODD("ibmat.1", 0xf0000, 0x8000, 0xc32713e4)

	/* I know about a 1984 version in 2 32kb roms */

	/* at, ami bios and diagnostics */
    ROM_LOAD_EVEN("rom01.bin", 0xf0000, 0x8000, 0x679296a7)
    ROM_LOAD_ODD("rom02.bin", 0xf0000, 0x8000, 0x65ae1f97)

	/* */
    ROM_LOAD("neat286.bin", 0xf0000, 0x10000, 0x07985d9b)
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("neat.0", 0xf0000, 0x8000, 0x4c36e61d)
    ROM_LOAD_ODD("neat.1", 0xf0000, 0x8000, 0x4e90f294)

	/* most likely 1 chip!, for lower costs */
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, 0x3df9732a)

	/* at486 */
    ROM_LOAD("at486.bin", 0xf0000, 0x10000, 0x31214616)

    ROM_LOAD("", 0x??000, 0x2000, 0x)

	/* pc xt mfm controller
	   2 harddisks 17 sectors, 4 head, 613 tracks
	   serves 2 controllers? 0x320-3, 0x324-7, dma 3, irq5
	   movable, works at 0xee000 */
	/* western digital 06/28/89 */
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)

	/* lcs 6210d asic i2.1 09/01/1988 */
	/* problematic, currently showing menu and calls int21 (hangs)! */
    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, 0xa96317da)

	// cutted from some aga char rom
	// 256 8x8 thick chars
	// 256 8x8 thin chars
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
	// first font of above
    ROM_LOAD("cga2.chr", 0x00000, 0x800, 0xa362ffe6)

	// cutted from some aga char rom
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("mda.chr",     0x00000, 0x01000, 0xac1686f3)

	// aga
	// 256 8x8 thick chars
	// 256 8x8 thin chars
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("aga.chr",     0x00000, 0x02000, 0xaca81498)
	// hercules font of above
    ROM_LOAD("hercules.chr", 0x00000, 0x1000, 0x7e8c9d76)
	
	/* oti 037 chip */
    ROM_LOAD("oakvga.bin", 0xc0000, 0x8000, 0x318c5f43)
	/* tseng labs famous et4000 isa vga card (oem) */
    ROM_LOAD("et4000b.bin", 0xc0000, 0x8000, 0xa903540d)	
	/* tseng labs famous et4000 isa vga card */
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)
#endif

ROM_START( ibmpc )
	ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, 0x80d3cf5d)
    ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, 0x673a4acc)
    ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, 0xaac3fc37)
    ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, 0x3062b3fc)
    ROM_LOAD("pc102782.bin", 0xfe000, 0x2000, 0xe88792b3)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( ibmpca )
	ROM_REGION(0x100000,REGION_CPU1,0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, 0x80d3cf5d)
    ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, 0x673a4acc)
    ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, 0xaac3fc37)
    ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, 0x3062b3fc)
    ROM_LOAD("pc081682.bin", 0xfe000, 0x2000, 0x5c3f0256)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( bondwell )
	ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4) // taken from other machine
	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, 0xd435a405)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069) // taken from cga
ROM_END

ROM_START( pcmda )
    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("mda.chr",     0x00000, 0x01000, 0xac1686f3)
ROM_END

ROM_START( pc )
    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
//    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, 0xa96317da)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( europc )
    ROM_REGION(0x100000,REGION_CPU1, 0)
	// hdd bios integrated!
    ROM_LOAD("50145", 0xf8000, 0x8000, 0x1775a11d) // V2.07
	ROM_REGION(0x02100,REGION_GFX1, 0)
    ROM_LOAD("50146", 0x00000, 0x02000, 0x1305dcf5) //D1.0
ROM_END


ROM_START( ibmpcjr )
    ROM_REGION(0x100000,REGION_CPU1, 0)
#ifndef MESS_DEBUG
	ROM_LOAD("bios.rom", 0xf0000, 0x10000, 0)
#else
    ROM_LOAD("basic.rom", 0xf6000, 0x8000, 0x0c19c1a8)
    ROM_LOAD("bios.rom", 0xfe000, 0x2000, 0x98463f95)
#endif
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( t1000hx )
    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)
//	ROM_REGION(0x01100,REGION_GFX1, 0)
	ROM_REGION(0x02000,REGION_GFX1, 0)
    // expects 8x9 charset!
//    ROM_LOAD("", 0x00000, 0x01000, 0x0 )
    ROM_LOAD("50146", 0x00000, 0x02000, BADCRC(0x1305dcf5)) //taken from europc, 9th blank
ROM_END

ROM_START( ibmxt )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
//    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, 0xa96317da) //this was inside
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("xt050986.0", 0xf0000, 0x8000, 0x83727c42) 
    ROM_LOAD16_BYTE("xt050986.1", 0xf0001, 0x8000, 0x2a629953)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( xtvga )
    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
ROM_END

ROM_START( pc200 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
	// special bios at 0xe0000 !?
    ROM_LOAD16_BYTE("pc20v2.0", 0xfc001, 0x2000, 0x41302eb8) // v2
    ROM_LOAD16_BYTE("pc20v2.1", 0xfc000, 0x2000, 0x71b84616) // v2
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x02100,REGION_GFX1, 0)
    ROM_LOAD("aga.chr",     0x00000, 0x02000, 0xaca81498) //taken from aga
ROM_END

ROM_START( pc20 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
	// special bios at 0xe0000 !?
    ROM_LOAD16_BYTE("pc20v2.0", 0xfc001, 0x2000, 0x41302eb8) // v2
    ROM_LOAD16_BYTE("pc20v2.1", 0xfc000, 0x2000, 0x71b84616) // v2
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x02100,REGION_GFX1, 0)
    ROM_LOAD("aga.chr",     0x00000, 0x02000, 0xaca81498) //taken from aga
ROM_END

ROM_START( pc1512 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("40044.v1", 0xfc001, 0x2000, 0x668fcc94) // v1
    ROM_LOAD16_BYTE("40043.v1", 0xfc000, 0x2000, 0xf72f1582) // v1
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069) // taken from cga
ROM_END

ROM_START( pc1640 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("40100", 0xc0000, 0x8000, 0xd2d1f1ae) // this bios seams to be made for the amstrad pc
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("40043.v3", 0xfc001, 0x2000, 0xe40a1513) // v3
    ROM_LOAD16_BYTE("40044.v3", 0xfc000, 0x2000, 0xf1c074f3)
ROM_END

ROM_START( ibmat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("at111585.0", 0xf0000, 0x8000, 0x4995be7a)
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at111585.1", 0xf0001, 0x8000, 0xc32713e4) // BASIC C1.1, beeps
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( at )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, 0x679296a7)
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, 0x65ae1f97)
	ROM_RELOAD(0xff0000,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, 0x679296a7)
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, 0x65ae1f97)
	ROM_RELOAD(0xff0000,0x8000)
ROM_END

ROM_START( neat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000, 0x4c36e61d)
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000, 0x4e90f294)
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

#ifdef HAS_I386
ROM_START( at386 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
#if 1
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, 0x3df9732a)
	ROM_RELOAD(0xff0000,0x10000)
#else
	// currently testing of 80286 features working in this core
	// no 80386 opcodes emulated yet
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, 0x679296a7)
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, 0x65ae1f97)
	ROM_RELOAD(0xff0000,0x8000)
#endif
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

#endif

static const struct IODevice io_ibmpc[] = {
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"dsk\0",            /* file extensions */
		IO_RESET_NONE,		/* reset if file changed */
        NULL,               /* id */
		pc_floppy_init, 	/* init */
		pc_floppy_exit, 	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        floppy_status,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
	{
		IO_HARDDISK,		/* type */
		4,					/* count */
		"img\0",            /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
        NULL,               /* id */
		pc_harddisk_init,	/* init */
		pc_harddisk_exit,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
	IO_PRINTER_PORT(3,"prn\0"),
    { IO_END }
};

#define io_ibmpca io_ibmpc
#define io_pcmda io_ibmpc
#define io_pc io_ibmpc
#define io_bondwell io_ibmpc
#define io_europc io_ibmpc

#define io_ibmpcjr io_ibmpc
#define io_t1000hx io_ibmpc

#define io_ibmxt io_ibmpc
#define io_xtvga io_ibmpc
#define io_pc200 io_ibmpc
#define io_pc20 io_ibmpc
#define io_pc1512 io_ibmpc
#define io_pc1640 io_ibmpc

#define io_ibmat io_ibmpc
#define io_at io_ibmpc
#define io_atvga io_ibmpc
#define io_neat io_ibmpc
#define io_at386 io_ibmpc

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR		NAME		PARENT	MACHINE     INPUT	    INIT	    COMPANY	 FULLNAME */
COMP ( 1982,	ibmpc,		0,		pccga,      pccga,	    pccga,	    "International Business Machines",  "IBM PC 10/27/82" )
COMP ( 1982,	ibmpca,		ibmpc,	pccga,      pccga,	    pccga,	    "International Business Machines",  "IBM PC 08/16/82" )
COMP ( 1987,	pc,			ibmpc,	pccga,      pccga,		pccga,	    "",  "PC (CGA)" )
COMPX ( 1985,	bondwell,	ibmpc,	pccga,		bondwell,   bondwell,	"Bondwell Holding",  "BW230 (PRO28 Series)", GAME_NOT_WORKING )
COMP( 1988,		europc,		ibmpc,	europc,     europc,		europc,     "Schneider Rdf. AG",  "EURO PC")

// pcjr (better graphics, better sound)
COMPX( 1983,	ibmpcjr,	ibmpc,	t1000hx,    tandy1t,	t1000hx,    "International Business Machines",  "IBM PC Jr", GAME_NOT_WORKING|GAME_IMPERFECT_COLORS )
COMP( 1987,		t1000hx,	ibmpc,	t1000hx,    tandy1t,	t1000hx,	"Tandy Radio Shack",  "Tandy 1000HX")

// xt class (pc but 8086)
COMP( 1986,		ibmxt,		ibmpc,	xtcga,      xtcga,		pccga,		"International Business Machines",  "IBM PC/XT (CGA)" )
COMP ( 1988,	pc200,		ibmpc,	pc200,		pc200,		pc200,		"Sinclair Research",  "PC200 Professional Series")
COMPX ( 1988,	pc20,		ibmpc,	pc200,		pc200,		pc200,		"Amstrad plc",  "Amstrad PC20", GAME_ALIAS)
COMP ( 1986,	pc1512,		ibmpc,	pc1512,     pc1512,		pc1512,		"Amstrad plc",  "Amstrad PC1512")
COMPX ( 1987,	pc1640,		ibmpc,	pc1640,     pc1640,		pc1640,		"Amstrad plc",  "Amstrad PC1640 / PC6400 (US)", GAME_NOT_WORKING )
// ppc640 portable pc1512?, nec processor?
// pc2086 pc1512 with vga??

// at class (many differences to xt)
COMPX ( 1985,	ibmat,		0,		atcga,		atcga,		atcga,	   "International Business Machines",  "IBM PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1987,	at,			ibmat,	atcga,      atcga,		atcga,	   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1989,	neat,		ibmat,	atcga,      atcga,		atcga,	   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#ifdef HAS_I386
COMPX ( 1988,	at386,		ibmat,	at386,      atcga,		at386,	   "MITAC INC",  "PC/AT 386(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#endif

// these drivers will be discarded soon
COMP ( 1987,	pcmda,		ibmpc,	pcmda,      pcmda,		pcmda,	   "",  "PC (MDA)" )
COMPX ( 1987,	xtvga,		ibmpc,	xtvga,      xtvga,		pc_vga,    "",  "PC/XT (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1987,	atvga,		ibmat,	atvga,      atvga,		at_vga,    "",  "PC/AT (VGA, MF2 Keyboard)", GAME_NOT_WORKING )

#ifdef RUNTIME_LOADER
extern void pc_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"ibmpc")==0) drivers[i]=&driver_ibmpc;
		if ( strcmp(drivers[i]->name,"ibmpca")==0) drivers[i]=&driver_ibmpca;
		if ( strcmp(drivers[i]->name,"pc")==0) drivers[i]=&driver_pc;
		if ( strcmp(drivers[i]->name,"pcmda")==0) drivers[i]=&driver_pcmda;
		if ( strcmp(drivers[i]->name,"europc")==0) drivers[i]=&driver_europc;
		if ( strcmp(drivers[i]->name,"bondwell")==0) drivers[i]=&driver_bondwell;
		if ( strcmp(drivers[i]->name,"ibmpcjr")==0) drivers[i]=&driver_ibmpcjr;
		if ( strcmp(drivers[i]->name,"t1000hx")==0) drivers[i]=&driver_t1000hx;
		if ( strcmp(drivers[i]->name,"ibmxt")==0) drivers[i]=&driver_ibmxt;
		if ( strcmp(drivers[i]->name,"pc200")==0) drivers[i]=&driver_pc200;
		if ( strcmp(drivers[i]->name,"pc1512")==0) drivers[i]=&driver_pc1512;
		if ( strcmp(drivers[i]->name,"pc1640")==0) drivers[i]=&driver_pc1640;
		if ( strcmp(drivers[i]->name,"xtvga")==0) drivers[i]=&driver_xtvga;
		if ( strcmp(drivers[i]->name,"ibmat")==0) drivers[i]=&driver_ibmat;
		if ( strcmp(drivers[i]->name,"at")==0) drivers[i]=&driver_at;

		if ( strcmp(drivers[i]->name,"atvga")==0) drivers[i]=&driver_atvga;
		if ( strcmp(drivers[i]->name,"neat")==0) drivers[i]=&driver_neat;
		if ( strcmp(drivers[i]->name,"at386")==0) drivers[i]=&driver_at386;
	}
}
#endif
