/***************************************************************************
    ibm at compatibles																			
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
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_video.h"
#include "includes/pc.h"

#include "devices/pc_hdc.h"
#include "includes/pc_ide.h"
#include "includes/pc_fdc_h.h"
#include "devices/pc_flopp.h"
#include "includes/pckeybrd.h"
#include "includes/pclpt.h"
#include "includes/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/at.h"
#include "includes/ibmat.h"
#include "includes/ps2.h"

#include "includes/pcshare.h"

/* window resizing with dirtybuffering traping in xmess window */

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

/* ibm ps2m30
port accesses
0x0094 
0x0190 postcode
0x0103
0x03bc
0x3bd/3dd

 */
static PORT_READ_START( ps2m30286_readport )
	{ 0x0000, 0x001f, dma8237_0_r },
	{ 0x0020, 0x003f, pic8259_0_r },
	{ 0x0040, 0x005f, pit8253_0_r },
	{ 0x0060, 0x006f, at_8042_r },
	{ 0x0070, 0x007f, mc146818_port_r },
	{ 0x0080, 0x009f, at_page_r }, // 90-9f ?
	{ 0x00a0, 0x00bf, pic8259_1_r },
	{ 0x00c0, 0x00df, dma8237_at_1_r },
{ 0x0100, 0x107, ps2_pos_r },
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

static PORT_WRITE_START( ps2m30286_writeport )
	{ 0x0000, 0x001f, dma8237_0_w },
	{ 0x0020, 0x003f, pic8259_0_w },
	{ 0x0040, 0x005f, pit8253_0_w },
	{ 0x0060, 0x006f, at_8042_w },
	{ 0x0070, 0x007f, mc146818_port_w },
	{ 0x0080, 0x009f, at_page_w },
	{ 0x00a0, 0x00bf, pic8259_1_w },
	{ 0x00c0, 0x00df, dma8237_at_1_w },
{ 0x0100, 0x107, ps2_pos_w },
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

static unsigned i286_address_mask = 0x00ffffff;

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

static struct DACinterface dac_interface= { 1, { 50 }};


#define MDRV_CPU_ATPC(mem, port, type, clock, vblankfunc)	\
	MDRV_CPU_ADD_TAG("main", type, clock)				\
	MDRV_CPU_MEMORY(mem##_readmem, mem##_writemem)		\
	MDRV_CPU_PORTS(port##_readport, port##_writeport)	\
	MDRV_CPU_VBLANK_INT(vblankfunc, 4)					\
	MDRV_CPU_CONFIG(i286_address_mask)

static MACHINE_DRIVER_START( atcga )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I286, 12000000, at_cga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( at )

	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ps2m30286 )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I286, 12000000, at_cga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( at_vga )

	MDRV_IMPORT_FROM( pcvideo_vga )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atvga )
	/* basic machine hardware */
	MDRV_CPU_ATPC(at, at, I286, 12000000, at_vga_frame_interrupt)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( at_vga )

	MDRV_IMPORT_FROM( pcvideo_vga )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, pc_sound_interface)
#ifdef ADLIB
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD(SAA1099, cms_interface)
#endif
	MDRV_SOUND_ADD(DAC, dac_interface)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


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
	pc_cga_init_palette,							/* init palette */

	VIDEO_TYPE_RASTER,
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
	// ibm at
	// most likely 2 32 kbyte chips for 16 bit access
    ROM_LOAD("atbios.bin", 0xf0000, 0x10000,CRC( 0x674426be)) // BASIC C1.1, beeps
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmat.0", 0xf0000, 0x8000,CRC( 0x4995be7a))
    ROM_LOAD_ODD("ibmat.1", 0xf0000, 0x8000,CRC( 0xc32713e4))

	/* I know about a 1984 version in 2 32kb roms */

	/* at, ami bios and diagnostics */
    ROM_LOAD_EVEN("rom01.bin", 0xf0000, 0x8000,CRC( 0x679296a7))
    ROM_LOAD_ODD("rom02.bin", 0xf0000, 0x8000,CRC( 0x65ae1f97))

	/* */
    ROM_LOAD("neat286.bin", 0xf0000, 0x10000,CRC( 0x07985d9b))
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("neat.0", 0xf0000, 0x8000,CRC( 0x4c36e61d))
    ROM_LOAD_ODD("neat.1", 0xf0000, 0x8000,CRC( 0x4e90f294))

	/* most likely 1 chip!, for lower costs */
    ROM_LOAD("at386.bin", 0xf0000, 0x10000,CRC( 0x3df9732a))

	/* at486 */
    ROM_LOAD("at486.bin", 0xf0000, 0x10000,CRC( 0x31214616))

    ROM_LOAD("", 0x??000, 0x2000,CRC( 0x))
#endif

ROM_START( ibmat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD16_BYTE("at111585.0", 0xf0000, 0x8000,CRC( 0x4995be7a))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at111585.1", 0xf0001, 0x8000,CRC( 0xc32713e4))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000,CRC( 0x42009069))
ROM_END

ROM_START( i8530286 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
	// saved from running machine
    ROM_LOAD16_BYTE("ps2m30.0", 0xe0000, 0x10000,CRC( 0x9965a634))
	ROM_RELOAD(0xfe0000,0x10000)
    ROM_LOAD16_BYTE("ps2m30.1", 0xe0001, 0x10000,CRC( 0x1448d3cb))
	ROM_RELOAD(0xfe0001,0x10000)
ROM_END

ROM_START( at )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000,CRC( 0x679296a7))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000,CRC( 0x65ae1f97))
	ROM_RELOAD(0xff0000,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000,CRC( 0x42009069))
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000,CRC( 0xf01e4be0))
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000,CRC( 0x679296a7))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000,CRC( 0x65ae1f97))
	ROM_RELOAD(0xff0000,0x8000)
ROM_END

ROM_START( neat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000,CRC( 0x4c36e61d))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000,CRC( 0x4e90f294))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000,CRC( 0x42009069))
ROM_END

#ifdef HAS_I386
ROM_START( at386 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD("at386.bin", 0xf0000, 0x10000,CRC( 0x3df9732a))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000,CRC( 0x42009069))
ROM_END

ROM_START( at486 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000,CRC( 0x8e9e2bd4))
    ROM_LOAD("at486.bin", 0xf0000, 0x10000,CRC( 0x31214616))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x01100,REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000,CRC( 0x42009069))
ROM_END
#endif

SYSTEM_CONFIG_START(ibmat)
	CONFIG_DEVICE_PRINTER(3)
	CONFIG_DEVICE_PC_FLOPPY(2)
	CONFIG_DEVICE_PC_HARDDISK(4)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR		NAME		PARENT	COMPAT	MACHINE     INPUT	    INIT	    CONFIG   COMPANY	 FULLNAME */
COMPX ( 1985,	ibmat,		0,		0,		atcga,		atcga,		atcga,	    ibmat,   "International Business Machines",  "IBM PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1988,	i8530286,	ibmat,	0,		ps2m30286,	atvga,		ps2m30286,	ibmat,   "International Business Machines",  "IBM PS2 Model 30 286", GAME_NOT_WORKING )
#ifdef MESS_DEBUG
COMPX ( 1987,	at,			ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#else
COMPX ( 1987,	at,			0,	    0,		atcga,      atcga,		atcga,	    ibmat,   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#endif
COMPX ( 1989,	neat,		ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#ifdef HAS_I386
COMPX ( 1988,	at386,		ibmat,	0,		at386,      atcga,		at386,	    ibmat,   "MITAC INC",  "PC/AT 386(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1990,	at486,		ibmat,	0,		at386,      atcga,		at386,	    ibmat,   "",  "PC/AT 486(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
#endif

COMP  ( 1987,	atvga,		ibmat,	0,		atvga,      atvga,		at_vga,     ibmat,   "",  "PC/AT (VGA, MF2 Keyboard)" )
