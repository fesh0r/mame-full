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
#include "devices/printer.h"

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

#include "devices/pc_hdc.h"
#include "includes/pc_ide.h"
#include "includes/pc_fdc_h.h"
#include "devices/pc_flopp.h"
#include "includes/pckeybrd.h"
#include "includes/pclpt.h"
#include "includes/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/europc.h"
#include "includes/tandy1t.h"
#include "includes/amstr_pc.h"
#include "includes/ibmpc.h"

#include "includes/pcshare.h"
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

static READ_HANDLER( return_0xff ) { return 0xff; }

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
	{ 0x0240, 0x257, pc_rtc_r },
//	{ 0x0240, 0x257, return_0xff }, // anonymous bios should not recognized realtimeclock
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

static PORT_READ_START( pc200_readport )
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0065, pc1640_port60_r },
{ 0x0078, 0x0078, pc1640_mouse_x_r }, //?
{ 0x007a, 0x007a, pc1640_mouse_y_r }, //?
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


static PORT_WRITE_START( pc200_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0065, pc1640_port60_w },
{ 0x0078, 0x0078, pc1640_mouse_x_w }, //?
{ 0x007a, 0x007a, pc1640_mouse_y_w }, //?
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
{ 0x0200, 0x0207, pc_JOY_r }, //?
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
{ 0x0200, 0x0207, pc_JOY_w }, //?
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
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
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
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
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
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

static unsigned i86_address_mask = 0x000fffff;

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
static void pc_irqhandler(int linestate) {}

static struct YM3812interface ym3812_interface = {
	1,
	ym3812_StdClock, /* I hope this is the clock used on the original Adlib Sound card */
	{255}, /* volume adjustment in relation to speaker and tandy1000 sound neccessary */
	{pc_irqhandler}
};
#endif

static struct SN76496interface t1t_sound_interface = {
	1,
	{2386360},
	{255,}
};

#define MDRV_CPU_PC(mem, port, type, clock, vblankfunc)	\
	MDRV_CPU_ADD_TAG("main", type, clock)				\
	MDRV_CPU_MEMORY(mem##_readmem, mem##_writemem)		\
	MDRV_CPU_PORTS(port##_readport, port##_writeport)	\
	MDRV_CPU_VBLANK_INT(vblankfunc, 4)					\
	MDRV_CPU_CONFIG(i86_address_mask)


static MACHINE_DRIVER_START( pcmda )
	/* basic machine hardware */
	MDRV_CPU_PC(pc, pc, V20, 4772720, pc_mda_frame_interrupt)	/* 4,77 Mhz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_mda)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*9, 25*14)
	MDRV_VISIBLE_AREA(0,80*9-1, 0,25*14-1)
	MDRV_GFXDECODE(pc_mda_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(mda_colortable) / sizeof(mda_colortable[0]))
	MDRV_PALETTE_INIT(pc_mda)

	MDRV_VIDEO_START(pc_mda)
	MDRV_VIDEO_UPDATE(pc_mda)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pccga )
	/* basic machine hardware */
	MDRV_CPU_PC(pc, pc, I88, 4772720, pc_cga_frame_interrupt)	/* 4,77 Mhz */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_cga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_VISIBLE_AREA(0,80*8-1, 0,25*8-1)
	MDRV_GFXDECODE(CGA_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(cga_colortable) / sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_cga)

	MDRV_VIDEO_START(pc_cga)
	MDRV_VIDEO_UPDATE(pc_cga)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( europc )
	/* basic machine hardware */
	MDRV_CPU_PC(europc, europc, I88, 4772720*2, pc_aga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_aga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*9, 25*14)
	MDRV_VISIBLE_AREA(0,80*9-1, 0,25*14-1)
	MDRV_GFXDECODE(europc_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH((sizeof(cga_colortable)+sizeof(mda_colortable) )/sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_aga)

	MDRV_VIDEO_START(pc_aga)
	MDRV_VIDEO_UPDATE(pc_aga)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif

	MDRV_NVRAM_HANDLER( europc_rtc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xtcga )
	/* basic machine hardware */
	MDRV_CPU_PC(pc, pc, I86, 12000000, pc_cga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_cga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)

	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_VISIBLE_AREA(0,80*8-1, 0,25*8-1)
	MDRV_GFXDECODE(CGA_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(cga_colortable) / sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_cga)

	MDRV_VIDEO_START(pc_cga)
	MDRV_VIDEO_UPDATE(pc_cga)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc200 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc200, I86, 8000000, pc_aga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_aga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*9, 25*14)
	MDRV_VISIBLE_AREA(0,80*9-1, 0,25*14-1)
	MDRV_GFXDECODE(aga_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH((sizeof(cga_colortable)+sizeof(mda_colortable) )/sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_aga)

	MDRV_VIDEO_START(pc_aga)
	MDRV_VIDEO_UPDATE(pc_aga)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc1512 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc1640, I86, 8000000, pc_cga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_aga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_VISIBLE_AREA(0,80*8-1, 0,25*8-1)
	MDRV_GFXDECODE(CGA_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(cga_colortable) / sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_cga)

	MDRV_VIDEO_START(pc1512)
	MDRV_VIDEO_UPDATE(pc1512)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc1640 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc1640, I86, 8000000, pc_vga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_vga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(720, 350)
	MDRV_VISIBLE_AREA(0,720-1, 0,350-1)
	MDRV_GFXDECODE(vga_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(ega_palette) / sizeof(ega_palette[0]))
	MDRV_COLORTABLE_LENGTH(0x100*2 /*sizeof(vga_colortable) / sizeof(vga_colortable[0])*/)
	MDRV_PALETTE_INIT(ega)

	MDRV_VIDEO_START(ega)
	MDRV_VIDEO_UPDATE(ega)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xtvga )
	/* basic machine hardware */
	MDRV_CPU_PC(pc, pc, I86, 12000000, pc_vga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_vga)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(720, 480)
	MDRV_VISIBLE_AREA(0,720-1, 0,480-1)
	MDRV_GFXDECODE(vga_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(vga_palette) / sizeof(vga_palette[0]))
	MDRV_COLORTABLE_LENGTH(0x100*2 /*sizeof(vga_colortable) / sizeof(vga_colortable[0])*/)
	MDRV_PALETTE_INIT(vga)

	MDRV_VIDEO_START(vga)
	MDRV_VIDEO_UPDATE(vga)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( t1000hx )
	/* basic machine hardware */
	MDRV_CPU_PC(t1t, t1t, I88, 8000000, tandy1000_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(pc_t1t)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*8, 25*9)
	MDRV_VISIBLE_AREA(0,80*8-1, 0,25*9-1)
	MDRV_GFXDECODE(t1000hx_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(pcjr_colortable) / sizeof(pcjr_colortable[0]))
	MDRV_PALETTE_INIT(pcjr)

	MDRV_VIDEO_START(pc_t1t)
	MDRV_VIDEO_UPDATE(pc_t1t)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
	MDRV_SOUND_ADD(SN76496, t1t_sound_interface)

	MDRV_NVRAM_HANDLER( tandy1000 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( t1000sx )
	MDRV_IMPORT_FROM(t1000hx)
	MDRV_GFXDECODE(t1000sx_gfxdecodeinfo)
MACHINE_DRIVER_END

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

	/* pc xt mfm controller
	   2 harddisks 17 sectors, 4 head, 613 tracks
	   serves 2 controllers? 0x320-3, 0x324-7, dma 3, irq5
	   movable, works at 0xee000 */
	/* western digital 06/28/89 */
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)

	/* lcs 6210d asic i2.1 09/01/1988 */
	/* problematic, currently showing menu and calls int21 (hangs)! */
    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, 0xa96317da)
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
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, 0x61dbf242)
	ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)
	ROM_REGION(0x02000,REGION_GFX1, 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BADCRC(0x1305dcf5)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000sx )
	ROM_REGION(0x100000,REGION_CPU1, 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, 0x61dbf242)
	ROM_LOAD("t1000sx.f0", 0xf0000, 0x10000, 0x0e016ecf)
	ROM_REGION(0x02000,REGION_GFX1, 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BADCRC(0x1305dcf5)) //taken from europc, 9th blank
ROM_END

ROM_START( ibmxt )
	ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
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
    ROM_LOAD("40109.bin",     0x00000, 0x02000, 0xecf9ebe8)
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
    ROM_LOAD("40109.bin",     0x00000, 0x02000, 0xecf9ebe8)
ROM_END

ROM_START( pc1512 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("40044.v1", 0xfc001, 0x2000, 0x668fcc94) // v1
    ROM_LOAD16_BYTE("40043.v1", 0xfc000, 0x2000, 0xf72f1582) // v1
	ROM_REGION(0x02100,REGION_GFX1, 0)
    ROM_LOAD("40045.bin",     0x00000, 0x02000, 0xdd5e030f)
ROM_END

ROM_START( pc1640 )
//    ROM_REGION(0x100000,REGION_CPU1, 0)
    ROM_REGION16_LE(0x100000,REGION_CPU1, 0)
    ROM_LOAD("40100", 0xc0000, 0x8000, 0xd2d1f1ae) // this bios seams to be made for the amstrad pc
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD16_BYTE("40043.v3", 0xfc001, 0x2000, 0xe40a1513) // v3
    ROM_LOAD16_BYTE("40044.v3", 0xfc000, 0x2000, 0xf1c074f3)
	ROM_REGION(0x02100,REGION_GFX1, 0)
    ROM_LOAD("40045.bin",     0x00000, 0x02000, 0xdd5e030f)
ROM_END

SYSTEM_CONFIG_START(ibmpc)
	CONFIG_DEVICE_PRINTER(3)
	CONFIG_DEVICE_PC_FLOPPY(2)
	CONFIG_DEVICE_PC_HARDDISK(4)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR		NAME		PARENT	MACHINE     INPUT	    INIT	    CONFIG   COMPANY	 FULLNAME */
COMP(  1982,	ibmpc,		0,		pccga,      pccga,	    pccga,	    ibmpc,   "International Business Machines",  "IBM PC 10/27/82" )
COMP(  1982,	ibmpca,		ibmpc,	pccga,      pccga,	    pccga,	    ibmpc,   "International Business Machines",  "IBM PC 08/16/82" )
COMP(  1987,	pc,			ibmpc,	pccga,      pccga,		pccga,	    ibmpc,   "",  "PC (CGA)" )
COMPX( 1985,	bondwell,	ibmpc,	pccga,		bondwell,   bondwell,	ibmpc,   "Bondwell Holding",  "BW230 (PRO28 Series)", GAME_NOT_WORKING )
COMP(  1988,	europc,		ibmpc,	europc,     europc,		europc,     ibmpc,   "Schneider Rdf. AG",  "EURO PC")

// pcjr (better graphics, better sound)
COMPX( 1983,	ibmpcjr,	ibmpc,	t1000hx,    tandy1t,	t1000hx,    ibmpc,   "International Business Machines",  "IBM PC Jr", GAME_NOT_WORKING|GAME_IMPERFECT_COLORS )
COMP(  1987,	t1000hx,	ibmpc,	t1000hx,    tandy1t,	t1000hx,	ibmpc,   "Tandy Radio Shack",  "Tandy 1000HX")
COMP(  1987,	t1000sx,	ibmpc,	t1000sx,    tandy1t,	t1000hx,	ibmpc,   "Tandy Radio Shack",  "Tandy 1000SX")

// xt class (pc but 8086)
COMP(  1986,	ibmxt,		ibmpc,	xtcga,      xtcga,		pccga,		ibmpc,   "International Business Machines",  "IBM PC/XT (CGA)" )
COMP(  1988,	pc200,		ibmpc,	pc200,		pc200,		pc200,		ibmpc,   "Sinclair Research",  "PC200 Professional Series")
COMPX( 1988,	pc20,		ibmpc,	pc200,		pc200,		pc200,		ibmpc,   "Amstrad plc",  "Amstrad PC20", GAME_ALIAS)
COMP(  1986,	pc1512,		ibmpc,	pc1512,     pc1512,		pc1512,		ibmpc,   "Amstrad plc",  "Amstrad PC1512")
COMPX( 1987,	pc1640,		ibmpc,	pc1640,     pc1640,		pc1640,		ibmpc,   "Amstrad plc",  "Amstrad PC1640 / PC6400 (US)", GAME_NOT_WORKING )
// ppc640 portable pc1512?, nec processor?
// pc2086 pc1512 with vga??

// these drivers will be discarded soon
COMP ( 1987,	pcmda,		ibmpc,	pcmda,      pcmda,		pcmda,	    ibmpc,   "",  "PC (MDA)" )
COMPX ( 1987,	xtvga,		ibmpc,	xtvga,      xtvga,		pc_vga,     ibmpc,   "",  "PC/XT (VGA, MF2 Keyboard)", GAME_NOT_WORKING )

