/***************************************************************************
    
	IBM AT Compatibles

***************************************************************************/

#include "driver.h"
#include "sound/3812intf.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "devices/printer.h"

#include "includes/uart8250.h"
#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/pc_vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_video.h"
#include "includes/pc.h"

#include "machine/pc_hdc.h"
#include "includes/pc_ide.h"
#include "includes/pc_fdc_h.h"
#include "includes/pckeybrd.h"
#include "includes/pclpt.h"
#include "includes/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/at.h"
#include "includes/ibmat.h"
#include "includes/ps2.h"

#include "includes/pcshare.h"

#include "devices/mflopimg.h"
#include "devices/harddriv.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"

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

static ADDRESS_MAP_START( at_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x09ffff) AM_MIRROR(0xff000000) AM_RAMBANK(10)
	AM_RANGE(0x0a0000, 0x0affff) AM_NOP
	AM_RANGE(0x0b0000, 0x0b7fff) AM_NOP
	AM_RANGE(0x0b8000, 0x0bffff) AM_READWRITE(MRA8_RAM, pc_video_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c0000, 0x0c7fff) AM_ROM
	AM_RANGE(0x0c8000, 0x0cffff) AM_ROM
	AM_RANGE(0x0d0000, 0x0effff) AM_ROM
	AM_RANGE(0x0f0000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x1fffff) AM_RAM
	AM_RANGE(0x200000, 0xfeffff) AM_NOP
	AM_RANGE(0xff0000, 0xffffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( at386_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(24) )
	AM_RANGE(0x000000, 0x09ffff) AM_MIRROR(0xff000000) AM_RAMBANK(10)
	AM_RANGE(0x0a0000, 0x0b7fff) AM_MIRROR(0xff000000) AM_NOP
	AM_RANGE(0x0b8000, 0x0bffff) AM_MIRROR(0xff000000) AM_READWRITE(MRA32_RAM, pc_video_videoram32_w) AM_BASE((data32_t **) &videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c0000, 0x0c7fff) AM_MIRROR(0xff000000) AM_ROM
	AM_RANGE(0x0c8000, 0x0cffff) AM_MIRROR(0xff000000) AM_ROM
	AM_RANGE(0x0d0000, 0x0effff) AM_MIRROR(0xff000000) AM_ROM
	AM_RANGE(0x0f0000, 0x0fffff) AM_MIRROR(0xff000000) AM_ROM
	AM_RANGE(0x100000, 0x19ffff) AM_MIRROR(0xff000000) AM_RAMBANK(20)
	AM_RANGE(0x200000, 0xfeffff) AM_MIRROR(0xff000000) AM_NOP
	AM_RANGE(0xff0000, 0xffffff) AM_MIRROR(0xff000000) AM_ROM
ADDRESS_MAP_END



static READ8_HANDLER(at_dma8237_1_r)
{
	return dma8237_1_r(offset / 2);
}

static WRITE8_HANDLER(at_dma8237_1_w)
{
	dma8237_1_w(offset / 2, data);
}

static READ32_HANDLER(at32_dma8237_1_r)
{
	return read32_with_read8_handler(at_dma8237_1_r, offset, mem_mask);
}

static WRITE32_HANDLER(at32_dma8237_1_w)
{
	write32_with_write8_handler(at_dma8237_1_w, offset, data, mem_mask);
}



static ADDRESS_MAP_START(at_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_0_r,				dma8237_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_0_r,				pic8259_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_0_r,				pit8253_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_8042_8_r,				at_8042_8_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port_r,			mc146818_port_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page8_r,				at_page8_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_1_r,				pic8259_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at_dma8237_1_r,			at_dma8237_1_w)
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(at_mfm_0_r,				at_mfm_0_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x0220, 0x022f) AM_READWRITE(soundblaster_r,			soundblaster_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc_parallelport2_r,		pc_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(pc_COM4_r,				pc_COM4_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(pc_COM2_r,				pc_COM2_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_parallelport1_r,		pc_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0388) AM_READWRITE(YM3812_status_port_0_r,	YM3812_control_port_0_w)
	AM_RANGE(0x0389, 0x0389) AM_WRITE(								YM3812_write_port_0_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_READWRITE(pc_COM3_r,				pc_COM3_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(pc_COM1_r,				pc_COM1_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(at386_io, ADDRESS_SPACE_IO, 32)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_32_0_r,			dma8237_32_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_32_0_r,			pic8259_32_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_32_0_r,			pit8253_32_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_8042_32_r,				at_8042_32_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port32_r,		mc146818_port32_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page32_r,				at_page32_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_32_1_r,			pic8259_32_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at32_dma8237_1_r,			at32_dma8237_1_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc32_parallelport2_r,		pc32_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(pc32_COM4_r,				pc32_COM4_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(pc32_COM2_r,				pc32_COM2_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc32_HDC1_r,				pc32_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc32_HDC2_r,				pc32_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc32_parallelport1_r,		pc32_parallelport1_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc32_fdc_r,				pc32_fdc_w)
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc32_parallelport0_r,		pc32_parallelport0_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(pc32_COM1_r,				pc32_COM1_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(ps2m30286_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(dma8237_0_r,				dma8237_0_w)
	AM_RANGE(0x0020, 0x003f) AM_READWRITE(pic8259_0_r,				pic8259_0_w)
	AM_RANGE(0x0040, 0x005f) AM_READWRITE(pit8253_0_r,				pit8253_0_w)
	AM_RANGE(0x0060, 0x006f) AM_READWRITE(at_8042_8_r,				at_8042_8_w)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE(mc146818_port_r,			mc146818_port_w)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE(at_page8_r,				at_page8_w)
	AM_RANGE(0x00a0, 0x00bf) AM_READWRITE(pic8259_1_r,				pic8259_1_w)
	AM_RANGE(0x00c0, 0x00df) AM_READWRITE(at_dma8237_1_r,			at_dma8237_1_w)
	AM_RANGE(0x0100, 0x0107) AM_READWRITE(ps2_pos_r,				ps2_pos_w)
	AM_RANGE(0x01f0, 0x01f7) AM_READWRITE(at_mfm_0_r,				at_mfm_0_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x0220, 0x022f) AM_READWRITE(soundblaster_r,			soundblaster_w)
	AM_RANGE(0x0278, 0x027f) AM_READWRITE(pc_parallelport2_r,		pc_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_READWRITE(pc_COM4_r,				pc_COM4_w)
	AM_RANGE(0x02f8, 0x02ff) AM_READWRITE(pc_COM2_r,				pc_COM2_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_parallelport1_r,		pc_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0388) AM_READWRITE(YM3812_status_port_0_r,	YM3812_control_port_0_w)
	AM_RANGE(0x0389, 0x0389) AM_WRITE(								YM3812_write_port_0_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_READWRITE(pc_COM3_r,				pc_COM3_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_READWRITE(pc_COM1_r,				pc_COM1_w)
ADDRESS_MAP_END


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
	MDRV_CPU_ADD_TAG("main", type, clock)					\
	MDRV_CPU_PROGRAM_MAP(mem##_map, mem##_map)				\
	MDRV_CPU_IO_MAP(port##_io, port##_io)					\
	MDRV_CPU_VBLANK_INT(vblankfunc, 4)						\
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


static MACHINE_DRIVER_START( at386 )
    /* basic machine hardware */
	/* original at 6 mhz, at03 8 megahertz */
	MDRV_CPU_ATPC(at386, at386, I386, 12000000, at_cga_frame_interrupt)

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
	MDRV_SOUND_ADD(DAC, dac_interface)

	MDRV_NVRAM_HANDLER( mc146818 )
MACHINE_DRIVER_END


#if 0
	// ibm at
	// most likely 2 32 kbyte chips for 16 bit access
    ROM_LOAD("atbios.bin", 0xf0000, 0x10000, CRC(674426be)) // BASIC C1.1, beeps
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmat.0", 0xf0000, 0x8000, CRC(4995be7a))
    ROM_LOAD_ODD("ibmat.1", 0xf0000, 0x8000, CRC(c32713e4))

	/* I know about a 1984 version in 2 32kb roms */

	/* at, ami bios and diagnostics */
    ROM_LOAD_EVEN("rom01.bin", 0xf0000, 0x8000, CRC(679296a7))
    ROM_LOAD_ODD("rom02.bin", 0xf0000, 0x8000, CRC(65ae1f97))

	/* */
    ROM_LOAD("neat286.bin", 0xf0000, 0x10000, CRC(07985d9b))
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("neat.0", 0xf0000, 0x8000, CRC(4c36e61d))
    ROM_LOAD_ODD("neat.1", 0xf0000, 0x8000, CRC(4e90f294))

	/* most likely 1 chip!, for lower costs */
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, CRC(3df9732a))

	/* at486 */
    ROM_LOAD("at486.bin", 0xf0000, 0x10000, CRC(31214616))

    ROM_LOAD("", 0x??000, 0x2000, CRC())
#endif

ROM_START( ibmat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at111585.0", 0xf0000, 0x8000, CRC(4995be7a) SHA1(8e8e5c863ae3b8c55fd394e345d8cca48b6e575c))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at111585.1", 0xf0001, 0x8000, CRC(c32713e4) SHA1(22ed4e2be9f948682891e2fd056a97dbea01203c))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( i8530286 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// saved from running machine
    ROM_LOAD16_BYTE("ps2m30.0", 0xe0000, 0x10000, CRC(9965a634))
	ROM_RELOAD(0xfe0000,0x10000)
    ROM_LOAD16_BYTE("ps2m30.1", 0xe0001, 0x10000, CRC(1448d3cb))
	ROM_RELOAD(0xfe0001,0x10000)
ROM_END

ROM_START( at )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( atvga )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at110387.1", 0xf0001, 0x8000, CRC(679296a7) SHA1(ae891314cac614dfece686d8e1d74f4763cf40e3))
	ROM_RELOAD(0xff0001,0x8000)
    ROM_LOAD16_BYTE("at110387.0", 0xf0000, 0x8000, CRC(65ae1f97) SHA1(91a29c7deecf7a9afbba330e64e0eee9aafee4d1))
	ROM_RELOAD(0xff0000,0x8000)
ROM_END

ROM_START( neat )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD16_BYTE("at030389.0", 0xf0000, 0x8000, CRC(4c36e61d))
	ROM_RELOAD(0xff0000,0x8000)
    ROM_LOAD16_BYTE("at030389.1", 0xf0001, 0x8000, CRC(4e90f294))
	ROM_RELOAD(0xff0001,0x8000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( at386 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at386.bin", 0xf0000, 0x10000, CRC(3df9732a) SHA1(def71567dee373dc67063f204ef44ffab9453ead))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

ROM_START( at486 )
    ROM_REGION(0x1000000,REGION_CPU1, 0)
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
    ROM_LOAD("at486.bin", 0xf0000, 0x10000, CRC(31214616))
	ROM_RELOAD(0xff0000,0x10000)
	ROM_REGION(0x08100, REGION_GFX1, 0)
    ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

SYSTEM_CONFIG_START(ibmat)
	CONFIG_RAM_DEFAULT( (640+1024) * 1024 )
	CONFIG_DEVICE_PRINTER(3)
	CONFIG_DEVICE_FLOPPY(2, pc)
	CONFIG_DEVICE_HARDDISK(4)
	CONFIG_QUEUE_CHARS( at_keyboard )
	CONFIG_ACCEPT_CHAR( at_keyboard )
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR		NAME		PARENT	COMPAT	MACHINE     INPUT	    INIT	    CONFIG   COMPANY	 FULLNAME */
COMPX ( 1985,	ibmat,		0,		0,		atcga,		atcga,		atcga,	    ibmat,   "International Business Machines",  "IBM PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1988,	i8530286,	ibmat,	0,		ps2m30286,	atvga,		ps2m30286,	ibmat,   "International Business Machines",  "IBM PS2 Model 30 286", GAME_NOT_WORKING )
COMPX ( 1987,	at,			ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "PC/AT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1989,	neat,		ibmat,	0,		atcga,      atcga,		atcga,	    ibmat,   "",  "NEAT (CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMPX ( 1988,	at386,		ibmat,	0,		at386,      atcga,		at386,	    ibmat,   "MITAC INC",  "PC/AT 386(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
//COMPX ( 1990,	at486,		ibmat,	0,		at386,      atcga,		at386,	    ibmat,   "",  "PC/AT 486(CGA, MF2 Keyboard)", GAME_NOT_WORKING )
COMP  ( 1987,	atvga,		0,		0,		atvga,      atvga,		at_vga,     ibmat,   "",  "PC/AT (VGA, MF2 Keyboard)" )
