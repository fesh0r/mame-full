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
#include "printer.h"

#include "includes/uart8250.h"
#include "includes/pic8259.h"
#include "includes/dma8237.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_t1t.h"

#include "includes/pc_hdc.h"
#include "includes/pc_ide.h"
#include "includes/pc_fdc_h.h"
#include "includes/pc_flopp.h"
#include "includes/pckeybrd.h"
#include "includes/pclpt.h"
#include "includes/pc_mouse.h"

#include "includes/europc.h"
#include "includes/tandy1t.h"
#include "includes/amstr_pc.h"
#include "includes/at.h"

#include "includes/pc.h"

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
	{ 0x0080, 0x0087, dma8237_0_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x0213, 0x0213, pc_EXP_r },
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x0378, 0x037b, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
PORT_END

static PORT_WRITE_START( pc_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, ppi8255_0_w },
	{ 0x0080, 0x0087, dma8237_0_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
#ifdef GAMEBLASTER
	{ 0x220, 0x220, saa1099_write_port_0_w },
	{ 0x221, 0x221, saa1099_control_port_0_w },
	{ 0x222, 0x222, saa1099_write_port_1_w },
	{ 0x223, 0x223, saa1099_control_port_1_w },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
	{ 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
PORT_END

static PORT_READ_START( europc_readport)
	{ 0x0000, 0x000f, dma8237_0_r },
	{ 0x0020, 0x0021, pic8259_0_r },
	{ 0x0040, 0x0043, pit8253_0_r },
	{ 0x0060, 0x0063, ppi8255_0_r },
	{ 0x0070, 0x0071, mc146818_port_r },
	{ 0x0080, 0x0087, dma8237_0_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x0213, 0x0213, pc_EXP_r },
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x0378, 0x037b, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x03d0, 0x03df, pc_CGA_r },
	{ 0x0254, 0x0257, europc_r },
	{ 0x0354, 0x0357, europc_r },
PORT_END

static PORT_WRITE_START( europc_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, ppi8255_0_w },
	{ 0x0070, 0x0071, mc146818_port_w },
	{ 0x0080, 0x0087, dma8237_0_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x03d0, 0x03df, pc_CGA_w },
	{ 0x0254, 0x0257, europc_w },
	{ 0x0354, 0x0357, europc_w },
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
	{ 0x0080, 0x0087, dma8237_0_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x0213, 0x0213, pc_EXP_r },
	{ 0x0378, 0x037f, pc_t1t_p37x_r },
    { 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
	{ 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x03d0, 0x03df, pc_T1T_r },
PORT_END

static PORT_WRITE_START( t1t_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0063, tandy1000_pio_w },
	{ 0x0080, 0x0087, dma8237_0_page_w },
	{ 0x00c0, 0x00c0, SN76496_0_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0378, 0x037f, pc_t1t_p37x_w },
    { 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x03d0, 0x03df, pc_T1T_w },
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
	{ 0x0080, 0x0087, dma8237_0_page_r },
	{ 0x0200, 0x0207, pc_JOY_r },
    { 0x0213, 0x0213, pc_EXP_r },
	{ 0x0278, 0x027b, pc_parallelport2_r },
	{ 0x0378, 0x037b, pc1640_port378_r },
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
PORT_END


static PORT_WRITE_START( pc1640_writeport )
	{ 0x0000, 0x000f, dma8237_0_w },
	{ 0x0020, 0x0021, pic8259_0_w },
	{ 0x0040, 0x0043, pit8253_0_w },
	{ 0x0060, 0x0065, pc1640_port60_w },
	{ 0x0070, 0x0071, mc146818_port_w },
	{ 0x0078, 0x0078, pc1640_mouse_x_w },
	{ 0x007a, 0x007a, pc1640_mouse_y_w },
	{ 0x0080, 0x0087, dma8237_0_page_w },
	{ 0x0200, 0x0207, pc_JOY_w },
    { 0x0213, 0x0213, pc_EXP_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
	{ 0x03bc, 0x03bd, pc_parallelport0_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
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
	{ 0x200000, 0xffffff, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( at_writemem )
	{ 0x000000, 0x03ffff, MWA_RAM },
	{ 0x040000, 0x09ffff, MWA_RAM },
	{ 0x0a0000, 0x0affff, MWA_NOP },
	{ 0x0b0000, 0x0b7fff, MWA_NOP },
	{ 0x0b8000, 0x0bbfff, pc_cga_videoram_w, &videoram, &videoram_size },
	{ 0x0c0000, 0x0c7fff, MWA_ROM },
	{ 0x0c8000, 0x0cffff, MWA_ROM },
    { 0x0d0000, 0x0effff, MWA_ROM },
	{ 0x0f0000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x1fffff, MWA_RAM },
	{ 0x200000, 0xffffff, MWA_NOP },
MEMORY_END

static PORT_READ_START( at_readport )
	{ 0x0000, 0x001f, dma8237_0_r },
	{ 0x0020, 0x003f, pic8259_0_r },
	{ 0x0040, 0x005f, pit8253_0_r },
	{ 0x0060, 0x006f, at_8042_r },
	{ 0x0070, 0x007f, mc146818_port_r },
	{ 0x0080, 0x0087, dma8237_0_page_r },
	{ 0x0088, 0x008f, dma8237_1_page_r },
	{ 0x0090, 0x0097, dma8237_0_page_r }, //?
	{ 0x0098, 0x009f, dma8237_1_page_r },
	{ 0x00a0, 0x00bf, pic8259_1_r },
	{ 0x00c0, 0x00df, dma8237_at_1_r },
	{ 0x0200, 0x0207, pc_JOY_r },
	{ 0x0278, 0x027f, pc_parallelport2_r },
	{ 0x0378, 0x037f, pc_parallelport1_r },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_status_port_0_r },
#endif
	{ 0x03bc, 0x03be, pc_parallelport0_r },
	{ 0x03f8, 0x03ff, pc_COM1_r },
	{ 0x02f8, 0x02ff, pc_COM2_r },
	{ 0x03e8, 0x03ef, pc_COM3_r },
	{ 0x02e8, 0x02ef, pc_COM4_r },
	{ 0x03f0, 0x03f7, pc_fdc_r },
    { 0x0320, 0x0323, pc_HDC1_r },
	{ 0x0324, 0x0327, pc_HDC2_r },
	{ 0x01f0, 0x01f7, at_mfm_0_r },
PORT_END

static PORT_WRITE_START( at_writeport )
	{ 0x0000, 0x001f, dma8237_0_w },
	{ 0x0020, 0x003f, pic8259_0_w },
	{ 0x0040, 0x005f, pit8253_0_w },
	{ 0x0060, 0x006f, at_8042_w },
	{ 0x0070, 0x007f, mc146818_port_w },
	{ 0x0080, 0x0087, dma8237_0_page_w },
	{ 0x0088, 0x008f, dma8237_1_page_w },
	{ 0x0090, 0x0097, dma8237_0_page_w }, // ?
	{ 0x0098, 0x009f, dma8237_1_page_w },
	{ 0x00a0, 0x00bf, pic8259_1_w },
	{ 0x00c0, 0x00df, dma8237_at_1_w },
	{ 0x0200, 0x0207, pc_JOY_w },
	{ 0x0278, 0x027b, pc_parallelport2_w },
	{ 0x0378, 0x037b, pc_parallelport1_w },
	{ 0x03bc, 0x03be, pc_parallelport0_w },
	{ 0x03f8, 0x03ff, pc_COM1_w },
	{ 0x02f8, 0x02ff, pc_COM2_w },
#ifdef ADLIB
	{ 0x0388, 0x0388, YM3812_control_port_0_w },
	{ 0x0389, 0x0389, YM3812_write_port_0_w },
#endif
#ifdef GAMEBLASTER
	{ 0x220, 0x220, saa1099_write_port_0_w },
	{ 0x221, 0x221, saa1099_control_port_0_w },
	{ 0x222, 0x222, saa1099_write_port_1_w },
	{ 0x223, 0x223, saa1099_control_port_1_w },
#endif
	{ 0x03e8, 0x03ef, pc_COM3_w },
	{ 0x02e8, 0x02ef, pc_COM4_w },
	{ 0x03f0, 0x03f7, pc_fdc_w },
    { 0x0320, 0x0323, pc_HDC1_w },
	{ 0x0324, 0x0327, pc_HDC2_w },
	{ 0x01f0, 0x01f7, at_mfm_0_w },
PORT_END

static unsigned char palette[] = {
/*  normal colors */
    0x00,0x00,0x00, 0x00,0x00,0x7f, 0x00,0x7f,0x00, 0x00,0x7f,0x7f,
    0x7f,0x00,0x00, 0x7f,0x00,0x7f, 0x7f,0x7f,0x00, 0x7f,0x7f,0x7f,
/*  light colors */
    0x3f,0x3f,0x3f, 0x00,0x00,0xff, 0x00,0xff,0x00, 0x00,0xff,0xff,
    0xff,0x00,0x00, 0xff,0x00,0xff, 0xff,0xff,0x00, 0xff,0xff,0xff,
/*  black for 640x200 gfx */
    0x00,0x00,0x00
};

#define PC_JOYSTICK \
	PORT_START	/* IN15 */\
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED ) \
	PORT_BITX( 0x0010, 0x0000, IPT_BUTTON1,	"Joystick 1 Button 1",	CODE_DEFAULT,		CODE_NONE)\
	PORT_BITX( 0x0020, 0x0000, IPT_BUTTON2,	"Joystick 1 Button 2",	CODE_NONE,		CODE_DEFAULT)\
	PORT_BITX( 0x0040, 0x0000, IPT_BUTTON1|IPF_PLAYER2,	"Joystick 2 Button 1",	CODE_NONE,		JOYCODE_2_BUTTON1)\
	PORT_BITX( 0x0080, 0x0000, IPT_BUTTON2|IPF_PLAYER2,	"Joystick 2 Button 2",	CODE_NONE,		JOYCODE_2_BUTTON2)\
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

#define PC_KEYBOARD \
    PORT_START  /* IN4 */\
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */\
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */\
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */\
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */\
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */\
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */\
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */\
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */\
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */\
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */\
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */\
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */\
		\
	PORT_START	/* IN5 */\
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */\
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */\
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */\
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */\
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */\
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */\
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */\
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */\
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */\
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */\
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */\
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */\
		\
	PORT_START	/* IN6 */\
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */\
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */\
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */\
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */\
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */\
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"` ~",          KEYCODE_TILDE,      IP_JOY_NONE ) /* `                           29  A9 */\
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */\
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"\\ |",         KEYCODE_BACKSLASH,  IP_JOY_NONE ) /* \                           2B  AB */\
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */\
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */\
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */\
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */\
		\
	PORT_START	/* IN7 */\
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */\
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */\
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */\
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP * (PrtScr)",KEYCODE_ASTERISK,   IP_JOY_NONE ) /* Keypad *  (PrtSc)           37  B7 */\
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */\
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */\
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */\
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */\
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */\
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */\
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */\
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */\
		\
	PORT_START	/* IN8 */\
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */\
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */\
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"ScrLock",      KEYCODE_SCRLOCK,    IP_JOY_NONE ) /* Scroll Lock                 46  C6 */\
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP 7 (Home)",  KEYCODE_7_PAD,      KEYCODE_HOME )/* Keypad 7  (Home)            47  C7 */\
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"KP 8 (Up)",    KEYCODE_8_PAD,      KEYCODE_UP )  /* Keypad 8  (Up arrow)        48  C8 */\
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_9_PAD,      KEYCODE_PGUP) /* Keypad 9  (PgUp)            49  C9 */\
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"KP -",         KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* Keypad -                    4A  CA */\
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"KP 4 (Left)",  KEYCODE_4_PAD,      KEYCODE_LEFT )/* Keypad 4  (Left arrow)      4B  CB */\
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */\
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"KP 6 (Right)", KEYCODE_6_PAD,      KEYCODE_RIGHT )/* Keypad 6  (Right arrow)     4D  CD */\
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"KP +",         KEYCODE_PLUS_PAD,   IP_JOY_NONE ) /* Keypad +                    4E  CE */\
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_1_PAD,      KEYCODE_END ) /* Keypad 1  (End)             4F  CF */\
		\
	PORT_START	/* IN9 */\
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"KP 2 (Down)",  KEYCODE_2_PAD,      KEYCODE_DOWN ) /* Keypad 2  (Down arrow)      50  D0 */\
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_3_PAD,      KEYCODE_PGDN ) /* Keypad 3  (PgDn)            51  D1 */\
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"KP 0 (Ins)",   KEYCODE_0_PAD,      KEYCODE_INSERT ) /* Keypad 0  (Ins)             52  D2 */\
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"KP . (Del)",   KEYCODE_DEL_PAD,    KEYCODE_DEL ) /* Keypad .  (Del)             53  D3 */\
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )\
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"(84/102)\\",   KEYCODE_BACKSLASH2, IP_JOY_NONE ) /* Backslash 2                 56  D6 */\
		\
	PORT_BIT ( 0xff80, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN10 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )\
		\
	PORT_START	/* IN11 */\
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )



INPUT_PORTS_START( pcmda )
	PORT_START /* IN0 */
	PORT_BIT ( 0x80, 0x80,	 IPT_VBLANK )
	PORT_BIT ( 0x7f, 0x7f,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x30, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x02, DEF_STR(Yes) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Any floppy drive installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x01, DEF_STR(Yes) )

    PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x80, DEF_STR(Yes) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x40, DEF_STR(Yes) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
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
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PC_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( xtcga )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Turbo Switch", CODE_DEFAULT, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "Off(4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On(12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

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
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BIT ( 0x30, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x06, 0x00,	 IPT_UNUSED )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"Esc",          KEYCODE_ESC,        IP_JOY_NONE ) /* Esc                         01  81 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"1 !",          KEYCODE_1,          IP_JOY_NONE ) /* 1                           02  82 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"2 @",          KEYCODE_2,          IP_JOY_NONE ) /* 2                           03  83 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"3 #",          KEYCODE_3,          IP_JOY_NONE ) /* 3                           04  84 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"4 $",          KEYCODE_4,          IP_JOY_NONE ) /* 4                           05  85 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"5 %",          KEYCODE_5,          IP_JOY_NONE ) /* 5                           06  86 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"6 ^",          KEYCODE_6,          IP_JOY_NONE ) /* 6                           07  87 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"7 &",          KEYCODE_7,          IP_JOY_NONE ) /* 7                           08  88 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"8 *",          KEYCODE_8,          IP_JOY_NONE ) /* 8                           09  89 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"9 (",          KEYCODE_9,          IP_JOY_NONE ) /* 9                           0A  8A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"0 )",          KEYCODE_0,          IP_JOY_NONE ) /* 0                           0B  8B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"- _",          KEYCODE_MINUS,      IP_JOY_NONE ) /* -                           0C  8C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"= +",          KEYCODE_EQUALS,     IP_JOY_NONE ) /* =                           0D  8D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"<--",          KEYCODE_BACKSPACE,  IP_JOY_NONE ) /* Backspace                   0E  8E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"Tab",          KEYCODE_TAB,        IP_JOY_NONE ) /* Tab                         0F  8F */

	PORT_START	/* IN5 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"Q",            KEYCODE_Q,          IP_JOY_NONE ) /* Q                           10  90 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"W",            KEYCODE_W,          IP_JOY_NONE ) /* W                           11  91 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"E",            KEYCODE_E,          IP_JOY_NONE ) /* E                           12  92 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"R",            KEYCODE_R,          IP_JOY_NONE ) /* R                           13  93 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"T",            KEYCODE_T,          IP_JOY_NONE ) /* T                           14  94 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"Y",            KEYCODE_Y,          IP_JOY_NONE ) /* Y                           15  95 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"U",            KEYCODE_U,          IP_JOY_NONE ) /* U                           16  96 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"I",            KEYCODE_I,          IP_JOY_NONE ) /* I                           17  97 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"O",            KEYCODE_O,          IP_JOY_NONE ) /* O                           18  98 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"P",            KEYCODE_P,          IP_JOY_NONE ) /* P                           19  99 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"[ {",          KEYCODE_OPENBRACE,  IP_JOY_NONE ) /* [                           1A  9A */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"] }",          KEYCODE_CLOSEBRACE, IP_JOY_NONE ) /* ]                           1B  9B */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Enter",        KEYCODE_ENTER,      IP_JOY_NONE ) /* Enter                       1C  9C */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"L-Ctrl",       KEYCODE_LCONTROL,   IP_JOY_NONE ) /* Left Ctrl                   1D  9D */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"A",            KEYCODE_A,          IP_JOY_NONE ) /* A                           1E  9E */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"S",            KEYCODE_S,          IP_JOY_NONE ) /* S                           1F  9F */

	PORT_START	/* IN6 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"D",            KEYCODE_D,          IP_JOY_NONE ) /* D                           20  A0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F",            KEYCODE_F,          IP_JOY_NONE ) /* F                           21  A1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"G",            KEYCODE_G,          IP_JOY_NONE ) /* G                           22  A2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"H",            KEYCODE_H,          IP_JOY_NONE ) /* H                           23  A3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"J",            KEYCODE_J,          IP_JOY_NONE ) /* J                           24  A4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"K",            KEYCODE_K,          IP_JOY_NONE ) /* K                           25  A5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"L",            KEYCODE_L,          IP_JOY_NONE ) /* L                           26  A6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"; :",          KEYCODE_COLON,      IP_JOY_NONE ) /* ;                           27  A7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"' \"",         KEYCODE_QUOTE,      IP_JOY_NONE ) /* '                           28  A8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Cursor Up",    KEYCODE_UP,			IP_JOY_NONE ) /*                             29  A9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"L-Shift",      KEYCODE_LSHIFT,     IP_JOY_NONE ) /* Left Shift                  2A  AA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"Cursor Left",  KEYCODE_LEFT,		IP_JOY_NONE ) /*                             2B  AB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"Z",            KEYCODE_Z,          IP_JOY_NONE ) /* Z                           2C  AC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"X",            KEYCODE_X,          IP_JOY_NONE ) /* X                           2D  AD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"C",            KEYCODE_C,          IP_JOY_NONE ) /* C                           2E  AE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"V",            KEYCODE_V,          IP_JOY_NONE ) /* V                           2F  AF */

	PORT_START	/* IN7 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"B",            KEYCODE_B,          IP_JOY_NONE ) /* B                           30  B0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"N",            KEYCODE_N,          IP_JOY_NONE ) /* N                           31  B1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"M",            KEYCODE_M,          IP_JOY_NONE ) /* M                           32  B2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	", <",          KEYCODE_COMMA,      IP_JOY_NONE ) /* ,                           33  B3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	". >",          KEYCODE_STOP,       IP_JOY_NONE ) /* .                           34  B4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"/ ?",          KEYCODE_SLASH,      IP_JOY_NONE ) /* /                           35  B5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"R-Shift",      KEYCODE_RSHIFT,     IP_JOY_NONE ) /* Right Shift                 36  B6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"Print",		CODE_NONE,			IP_JOY_NONE ) /*                             37  B7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Alt",          KEYCODE_LALT,       IP_JOY_NONE ) /* Left Alt                    38  B8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"Space",        KEYCODE_SPACE,      IP_JOY_NONE ) /* Space                       39  B9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Caps",         KEYCODE_CAPSLOCK,   IP_JOY_NONE ) /* Caps Lock                   3A  BA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"F1",           KEYCODE_F1,         IP_JOY_NONE ) /* F1                          3B  BB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"F2",           KEYCODE_F2,         IP_JOY_NONE ) /* F2                          3C  BC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"F3",           KEYCODE_F3,         IP_JOY_NONE ) /* F3                          3D  BD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"F4",           KEYCODE_F4,         IP_JOY_NONE ) /* F4                          3E  BE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"F5",           KEYCODE_F5,         IP_JOY_NONE ) /* F5                          3F  BF */

	PORT_START	/* IN8 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"F6",           KEYCODE_F6,         IP_JOY_NONE ) /* F6                          40  C0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"F7",           KEYCODE_F7,         IP_JOY_NONE ) /* F7                          41  C1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"F8",           KEYCODE_F8,         IP_JOY_NONE ) /* F8                          42  C2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"F9",           KEYCODE_F9,         IP_JOY_NONE ) /* F9                          43  C3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"F10",          KEYCODE_F10,        IP_JOY_NONE ) /* F10                         44  C4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"NumLock",      KEYCODE_NUMLOCK,    IP_JOY_NONE ) /* Num Lock                    45  C5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	"Hold",			KEYCODE_SCRLOCK,    IP_JOY_NONE ) /*		                     46  C6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"KP 7 /",		KEYCODE_7_PAD,      IP_JOY_NONE ) /* Keypad 7                    47  C7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"KP 8 ~",		KEYCODE_8_PAD,      IP_JOY_NONE ) /* Keypad 8                    48  C8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"KP 9 (PgUp)",  KEYCODE_9_PAD,      IP_JOY_NONE ) /* Keypad 9  (PgUp)            49  C9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"Cursor Down",  KEYCODE_DOWN,		IP_JOY_NONE ) /*                             4A  CA */
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD,	"KP 4 |",		KEYCODE_4_PAD,		IP_JOY_NONE ) /* Keypad 4                    4B  CB */
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD,	"KP 5",         KEYCODE_5_PAD,      IP_JOY_NONE ) /* Keypad 5                    4C  CC */
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD,	"KP 6",			KEYCODE_6_PAD,		IP_JOY_NONE ) /* Keypad 6                    4D  CD */
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD,	"Cursor Right", KEYCODE_RIGHT,		IP_JOY_NONE ) /*                             4E  CE */
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD,	"KP 1 (End)",   KEYCODE_1_PAD,      IP_JOY_NONE ) /* Keypad 1  (End)             4F  CF */

	PORT_START	/* IN9 */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD,	"KP 2 `",		KEYCODE_2_PAD,		IP_JOY_NONE ) /* Keypad 2                    50  D0 */
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD,	"KP 3 (PgDn)",  KEYCODE_3_PAD,      IP_JOY_NONE ) /* Keypad 3  (PgDn)            51  D1 */
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD,	"KP 0",			KEYCODE_0_PAD,      IP_JOY_NONE ) /* Keypad 0                    52  D2 */
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD,	"KP - (Del)",   KEYCODE_MINUS_PAD,  IP_JOY_NONE ) /* - Delete                    53  D3 */
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD,	"Break",		KEYCODE_STOP,       IP_JOY_NONE ) /* Break                       54  D4 */
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD,	"+ Insert",		KEYCODE_PLUS_PAD,	IP_JOY_NONE ) /* + Insert                    55  D5 */
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD,	".",			KEYCODE_DEL_PAD,    IP_JOY_NONE ) /* .                           56  D6 */
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD,	"Enter",		KEYCODE_ENTER_PAD,  IP_JOY_NONE ) /* Enter                       57  D7 */
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD,	"Home",			KEYCODE_HOME,       IP_JOY_NONE ) /* HOME                        58  D8 */
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD,	"F11",			KEYCODE_F11,        IP_JOY_NONE ) /* F11                         59  D9 */
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD,	"F12",			KEYCODE_F12,        IP_JOY_NONE ) /* F12                         5a  Da */

	PORT_START	/* IN10 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

	PORT_START	/* IN11 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pc1512 )
	PORT_START /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

    PORT_START /* IN1 */
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Name/Language", KEYCODE_NONE, IP_JOY_NONE )
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
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AMSTRAD_KEYBOARD

//	PC_JOYSTICK
INPUT_PORTS_END

INPUT_PORTS_START( pc1640 )
	PORT_START	/* IN0 */
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 5", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 6", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 7", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Paradise EGA 8", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Name/Language", KEYCODE_NONE, IP_JOY_NONE )
//	PORT_DIPSETTING(	0x00, "PC 512k" ) // machine crashes with ega bios at 0xc0000
	PORT_DIPSETTING(	0x01, "Italian/Italiano" ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. vänta" ) 
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, "Spanish/Español" ) //Por favor tilde n
	PORT_DIPSETTING(	0x05, "French/Francais" ) //patientez cedilla c
	PORT_DIPSETTING(	0x06, "German/Deutsch" ) // bitte warten
	PORT_DIPSETTING(	0x07, "English" ) // please wait
	PORT_BIT( 0x20, 0x00,	IPT_UNUSED ) // not pc1512 integrated special cga
	PORT_BITX( 0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x40", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x40, "0x40" )
	PORT_BITX( 0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x80", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x80, "0x80" )
#if 0
	PORT_BITX( 0x200, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x20 after 27[8a] read (font?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x200, "0x20" )
	PORT_BITX( 0x400, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x40 after 27[8a] read (font?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x400, "0x40" )
	PORT_BITX( 0x800, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x80 after 27[8a] read (font?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x800, "0x80" )
	PORT_BITX( 0x2000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a 0x20 after 427a read", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x2000, "0x20" )
	PORT_BITX( 0x4000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, " 37a 0x40 after 427a read", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x4000, "0x40" )
	PORT_BITX( 0x8000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, " 37a 0x80 after 427a read", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x8000, "0x80" )
#else
	PORT_BITX( 0xe00, 0x600, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a after 427a read", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x000, "?0standard" ) // diaresis a, seldom c, acut u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x200, "?scandinavian" ) //diaresis a, o slashed, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x600, "?spain" ) // tilde a, seldom c, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0xa00, "?greeck" ) // E, circumflex ???,micro, I, Z, big kappa?
	PORT_DIPSETTING(	0xe00, "?standard" ) // diaresis a, seldom e, acute u, circumflex e, grave a, acute e
	PORT_BITX( 0xe000, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "37a after 427a read", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x0000, "?Integrated Graphic Adapter IGA (EGA)" )
	PORT_DIPSETTING(	0x2000, "?External EGA/VGA" )
	PORT_DIPSETTING(	0x6000, "CGA 40 Columns" )
	PORT_DIPSETTING(	0xa000, "CGA 80 Columns" )
	PORT_DIPSETTING(	0xe000, "MDA/Hercules/Multiple Graphic Adapters" )
#endif
	PORT_START /* IN2 */
PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AMSTRAD_KEYBOARD

//	INPUT_MICROSOFT_MOUSE

INPUT_PORTS_END

INPUT_PORTS_START( xtvga )
	PORT_START /* IN0 */
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )		PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Turbo Switch", CODE_DEFAULT, IP_JOY_NONE )
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
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
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
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 1", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 2", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 3", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "VGA 4", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )	
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

    PORT_START /* IN1 */
	PORT_BITX( 0xc0, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Number of floppy drives", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_BITX( 0x30, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Graphics adapter", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_BITX( 0x0c, 0x0c, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM banks", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "80387 installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy installed", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START /* IN2 */
	PORT_BITX( 0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_BITX( 0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BITX( 0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_BITX( 0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "COM4: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT1: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT2: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BITX( 0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "LPT3: enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_BITX( 0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Game port enable", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
    PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

    PORT_START /* IN3 */
	PORT_BITX( 0xf0, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Serial mouse", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
    PORT_DIPSETTING(    0x00, "none" )
	PORT_BITX( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC1 (C800:0 port 320-323)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "HDC2 (CA00:0 port 324-327)", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	AT_KEYBOARD

	INPUT_MICROSOFT_MOUSE

	PC_JOYSTICK
INPUT_PORTS_END

static unsigned i86_address_mask = 0x000fffff;


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
	8,32,					/* 8 x 32 graphics */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bit planes */
    /* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets (we only use one byte to build the block) */
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	8						/* every code takes 1 byte */
};

static struct GfxDecodeInfo pc_mda_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pc_mda_charlayout,		0, 256 },
	{ 1, 0x1000, &pc_mda_gfxlayout_1bpp,256*2,	 1 },	/* 640x400x1 gfx */
    { -1 } /* end of array */
};

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
     0,10,
};


static struct GfxLayout CGA_charlayout =
{
	8,32,					/* 8 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static struct GfxLayout CGA_charlayout_dw =
{
	16,32,					/* 16 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	/* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
      4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static struct GfxLayout CGA_gfxlayout_1bpp =
{
    8,32,                   /* 8 x 32 graphics */
    256,                    /* 256 codes */
    1,                      /* 1 bit per pixel */
    { 0 },                  /* no bit planes */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static struct GfxLayout CGA_gfxlayout_2bpp =
{
	8,32,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
    { 0,0, 2,2, 4,4, 6,6  },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static struct GfxLayout europc_cga_charlayout =
{
	8,32,					/* 8 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
    8*16                     /* every char takes 8 bytes */
};

static struct GfxLayout europc_cga_charlayout_dw =
{
	16,32,					/* 16 x 32 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	/* y offsets */
	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
	  4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8,
	  0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,
      4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
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
	 0,16,
/* the color sets for 2bpp graphics mode */
     0, 2, 4, 6,  0,10,12,14,
     0, 3, 5, 7,  0,11,13,15,
};

static struct GfxDecodeInfo CGA_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x0000, &CGA_charlayout_dw,		  0, 256 },   /* double width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,   1 },   /* 640x400x1 gfx */
	{ 1, 0x1000, &CGA_gfxlayout_2bpp, 256*2+1*2,   4 },   /* 320x200x4 gfx */
    { -1 } /* end of array */
};

static struct GfxDecodeInfo europc_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &europc_cga_charlayout,	  0, 256 },   /* single width */
	{ 1, 0x0000, &europc_cga_charlayout_dw,	  0, 256 },   /* double width */
	{ 1, 0x2000, &CGA_gfxlayout_1bpp,	  256*2,   1 },   /* 640x400x1 gfx */
	{ 1, 0x2000, &CGA_gfxlayout_2bpp, 256*2+1*2,   4 },   /* 320x200x4 gfx */
	{ 1, 0x1000, &europc_mda_charlayout,	  0, 256 },   /* single width */
    { -1 } /* end of array */
};

#if 0
static struct GfxDecodeInfo aga_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x0000, &CGA_charlayout_dw,		  0, 256 },   /* double width */
	{ 1, 0x2000, &CGA_gfxlayout_1bpp,	  256*2,   1 },   /* 640x400x1 gfx */
	{ 1, 0x2000, &CGA_gfxlayout_2bpp, 256*2+1*2,   4 },   /* 320x200x4 gfx */
	{ 1, 0x0800, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x0800, &CGA_charlayout_dw,		  0, 256 },   /* double width */
	{ 1, 0x1000, &MDA_charlayout,			  0, 256 },   /* single width */
    { -1 } /* end of array */
};
#endif

static struct GfxLayout t1t_gfxlayout_4bpp_160 =
{
	8,32,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,0,0,0,4,4,4,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	1*8 					/* every code takes 1 byte */
};

static struct GfxLayout t1t_gfxlayout_4bpp_320 =
{
	4,32,					/* 4 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,0,4,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
	1*8 					/* every code takes 1 byte */
};

static struct GfxLayout t1t_gfxlayout_2bpp_640 =
{
	4,32,					/* 4 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
	{ 0,2,4,6 },
    /* y offsets (we only use one byte to build the block) */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    8                       /* every code takes 1 byte */
};

static unsigned short t1t_colortable[] = {
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
/* the color set for 1bpp graphics mode */
	 0,16,
/* the color sets for 2bpp graphics mode */
     0, 2, 4, 6,  0,10,12,14,
     0, 3, 5, 7,  0,11,13,15,
/* the color set for 4bpp graphics mode */
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
};

static struct GfxDecodeInfo t1t_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,		  0,			256 },	/* single width */
	{ 1, 0x0000, &CGA_charlayout_dw,	  0,			256 },	/* double width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,		  1 },	/* 640x400 1bpp gfx */
    { 1, 0x1000, &CGA_gfxlayout_2bpp,     256*2+1*2,      4 },  /* 320x200 2bpp gfx */
	{ 1, 0x1000, &t1t_gfxlayout_4bpp_160, 256*2+1*2+4*4, 16 },	/* 160x200 4bpp gfx */
	{ 1, 0x1000, &t1t_gfxlayout_4bpp_320, 256*2+1*2+4*4, 16 },	/* 320x200 4bpp gfx */
    { 1, 0x1000, &t1t_gfxlayout_2bpp_640, 256*2+1*2,      4 },  /* 640x200 2bpp gfx */
    { -1 } /* end of array */
};


/* Initialise the mda palette */
static void mda_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
    memcpy(sys_palette,palette,sizeof(palette));
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
    sizeof(palette) / sizeof(palette[0]) / 3,
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
	memcpy(sys_palette,palette,sizeof(palette));
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
    80*8,                                       /* screen width */
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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

static struct MachineDriver machine_driver_europc =
{
    /* basic machine hardware */
    {
        {
            CPU_I88,
			4772720,	/* 4,77 Mhz */
			pc_readmem,pc_writemem,
			europc_readport,europc_writeport,
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
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	europc_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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
	},
	mc146818_nvram_handler
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
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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
//	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
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
	0, //VGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(ega_palette) / sizeof(ega_palette[0]) / 3,
	0, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
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
//	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
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
	0, //VGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(vga_palette) / sizeof(vga_palette[0]) / 3,
	0, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
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
	}
};

/* Initialise the t1t palette */
static void t1t_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
    memcpy(sys_palette,palette,sizeof(palette));
    memcpy(sys_colortable,t1t_colortable,sizeof(t1t_colortable));
}

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
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	t1t_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(t1t_colortable) / sizeof(t1t_colortable[0]),
	t1t_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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
			&i86_address_mask
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	at_machine_init,
	0,

    /* video hardware */
    80*8,                                       /* screen width */
	25*8*2, 									/* screen height (pixels doubled) */
	{ 0,80*8-1, 0,25*8*2-1},					/* visible_area */
	CGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(cga_colortable) / sizeof(cga_colortable[0]),
	cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
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
	0, //VGA_gfxdecodeinfo,							/* graphics decode info */
	sizeof(vga_palette) / sizeof(vga_palette[0]) / 3,
	0, //sizeof(vga_colortable) / sizeof(vga_colortable[0]),
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
	},
	mc146818_nvram_handler
};

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
	/* after memory test 101 fe4df dma controller? */
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
    ROM_LOAD("biospc.bin", 0xfe000, 0x2000, 0xe88792b3) //beepcode

	/* tandy 1000 hx */
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)

	// ibm xt
    ROM_LOAD("xthdd.c8", 0xc8000, 0x2000, 0xa96317da)
    ROM_LOAD("biosxt.bin", 0xf0000, 0x10000, 0x36c32fde) // BASIC C1.1, hangs
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
	   serves 2 controllers? 0x320-3, 0x324-7, no dma, irq5
	   movable, works at 0xee000 */
	/* western digital 06/28/89 */
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)

	/* lcs 6210d asic i2.1 09/01/1988 */
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
	ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, 0x80d3cf5d)
    ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, 0x673a4acc)
    ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, 0xaac3fc37)
    ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, 0x3062b3fc)
    ROM_LOAD("pc102782.bin", 0xfe000, 0x2000, 0xe88792b3) //beepcode
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( ibmpca )
	ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, 0x80d3cf5d)
    ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, 0x673a4acc)
    ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, 0xaac3fc37)
    ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, 0x3062b3fc)
    ROM_LOAD("pc081682.bin", 0xfe000, 0x2000, 0x5c3f0256)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( bondwell )
	ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4) // taken from other machine
	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, 0xd435a405)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069) // taken from cga
ROM_END


ROM_START( pcmda )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("mda.chr",     0x00000, 0x01000, 0xac1686f3)
ROM_END

ROM_START( pc )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( europc )
    ROM_REGION(0x100000,REGION_CPU1)
	// hdd bios integrated!
    ROM_LOAD("50145", 0xf8000, 0x8000, 0x1775a11d) // V2.07
	ROM_REGION(0x02100,REGION_GFX1)
    ROM_LOAD("50146", 0x00000, 0x02000, 0x1305dcf5) //D1.0
ROM_END


ROM_START( ibmpcjr )
    ROM_REGION(0x100000,REGION_CPU1)
#ifndef MESS_DEBUG
	ROM_LOAD("bios.rom", 0xf0000, 0x10000, 0)
#else
    ROM_LOAD("basic.rom", 0xf6000, 0x8000, 0x0c19c1a8)
    ROM_LOAD("bios.rom", 0xfe000, 0x2000, 0x98463f95)
#endif
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( t1000hx )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, 0xd37a1d5f)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( ibmxt )
    ROM_REGION(0x100000,REGION_CPU1)
//    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, 0xa96317da) //this was inside
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_ODD("xt050986.0", 0xf0000, 0x8000, 0x83727c42) 
    ROM_LOAD_EVEN("xt050986.1", 0xf0000, 0x8000, 0x2a629953) // BASIC C1.1, hangs
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( xtcga )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( xtvga )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, 0x031aafad)
ROM_END

ROM_START( pc1512 )
    ROM_REGION(0x100000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_EVEN("40044.v1", 0xfc000, 0x2000, 0x668fcc94) // v1
    ROM_LOAD_ODD("40043.v1", 0xfc000, 0x2000, 0xf72f1582) // v1
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069) // taken from cga
ROM_END

ROM_START( pc1640 )
    ROM_REGION(0x100000,REGION_CPU1)
	// this bios seams to be made for the amstrad pc
    ROM_LOAD("40100", 0xc0000, 0x8000, 0xd2d1f1ae)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_EVEN("40043.v3", 0xfc000, 0x2000, 0xe40a1513) // v3
    ROM_LOAD_ODD("40044.v3", 0xfc000, 0x2000, 0xf1c074f3)
ROM_END

ROM_START( ibmat )
    ROM_REGION(0x1000000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_ODD("at111585.0", 0xf0000, 0x8000, 0x4995be7a)
    ROM_LOAD_EVEN("at111585.1", 0xf0000, 0x8000, 0xc32713e4) // BASIC C1.1, beeps
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( at )
    ROM_REGION(0x1000000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_EVEN("at110387.1", 0xf0000, 0x8000, 0x679296a7)
    ROM_LOAD_ODD("at110387.0", 0xf0000, 0x8000, 0x65ae1f97)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,REGION_CPU1)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_EVEN("at110387.1", 0xf0000, 0x8000, 0x679296a7)
    ROM_LOAD_ODD("at110387.0", 0xf0000, 0x8000, 0x65ae1f97)
ROM_END

ROM_START( neat )
    ROM_REGION(0x1000000,REGION_CPU1)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, 0x8e9e2bd4)
    ROM_LOAD_ODD("at030389.0", 0xf0000, 0x8000, 0x4c36e61d)
    ROM_LOAD_EVEN("at030389.1", 0xf0000, 0x8000, 0x4e90f294)
	ROM_REGION(0x01100,REGION_GFX1)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
ROM_END


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
	IO_PRINTER_PORT(3,"\0"),
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
#define io_xtcga io_ibmpc
#define io_xtvga io_ibmpc
#define io_pc1512 io_ibmpc
#define io_pc1640 io_ibmpc

#define io_ibmat io_ibmpc
#define io_at io_ibmpc
#define io_atvga io_ibmpc
#define io_neat io_ibmpc

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR		NAME		PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
COMP ( 1982,	ibmpc,		0,		 pccga,    pccga,	 pccga,	   "International Business Machines",  "IBM PC 10/27/82" )
COMP ( 1982,	ibmpca,		ibmpc,	 pccga,    pccga,	 pccga,	   "International Business Machines",  "IBM PC 08/16/82" )
COMP ( 1987,	pc,			ibmpc,	 pccga,    pccga,	 pccga,	   "",  "PC (CGA)" )
COMPX ( 1985,	bondwell,	ibmpc,	 pccga,	   pccga,	 bondwell, "Bondwell Holding",  "BW230 (PRO28 Series)", GAME_NOT_WORKING )
COMPX ( 1988,	europc,		ibmpc,	 europc,   pccga,	 europc,   "Schneider Rdf. AG",  "EURO PC", GAME_NOT_WORKING )

// pcjr (better graphics, better sound)
COMPX( 1983,	ibmpcjr,	ibmpc,	 t1000hx,  tandy1t,  t1000hx,  "International Business Machines",  "IBM PC Jr", GAME_NOT_WORKING|GAME_IMPERFECT_COLORS )
COMPX( 1987,	t1000hx,	ibmpc,	 t1000hx,  tandy1t,  t1000hx,  "Tandy Radio Shack",  "Tandy 1000HX", GAME_IMPERFECT_COLORS )

// xt class (pc but 8086)
COMPX ( 1986,	ibmxt,    ibmpc,	 xtcga,    xtcga,	 pccga,	   "International Business Machines",  "IBM PC/XT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1986,	pc1512,   ibmpc,	 pc1512,   pc1512,	 pc1512,   "Amstrad plc",  "Amstrad PC1512", GAME_NOT_WORKING )
COMPX ( 1987,	pc1640,   ibmpc,	 pc1640,   pc1640,	 pc1640,   "Amstrad plc",  "Amstrad PC1640 / PC6400 (US)", GAME_NOT_WORKING )
// ppc640 portable pc1512?, nec processor?
// pc2086 pc1512 with vga??

// at class (many differences to xt)
COMPX ( 1985,	ibmat,    0,		 atcga,    atcga,	 atcga,	   "International Business Machines",  "IBM PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1987,	at,			ibmat,	 atcga,    atcga,	 atcga,	   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1989,	neat,		ibmat,	 atcga,    atcga,	 atcga,	   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )

// these drivers will be discarded soon
COMP ( 1987,	pcmda,		ibmpc,	 pcmda,    pcmda,	 pcmda,	   "",  "PC (MDA)" )
COMPX ( 1987,	xtcga,		ibmpc,	 xtcga,    xtcga,	 pccga,	   "",  "PC/XT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1987,	xtvga,		ibmpc,	 xtvga,    xtvga,	 pc_vga,   "",  "PC/XT (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1987,	atvga,		ibmat,	 atvga,    atvga,	 at_vga,   "",  "PC/AT (VGA, MF2 Keyboard)", GAME_NOT_WORKING )
