/* Poke Champ */

/* This is a Korean hack of Data East's Pocket Gal

   It uses RAM for Palette instead of PROMs
   Samples are played by OKIM6295
   Different Banking
   More Tiles, 8bpp
   Sprites 4bpp? instead of 2bpp
   Many code changes

   Todo:

   Fix colours
   Fix sound banking
   Verify frequencies etc.

*/

/* README

-The ROMs are labeled as "Unico".
-The CPUs and some other chips are labeled as "SEA HUNTER".
-The chips with the "SEA HUNTER" label all have their
 surfaces scratched out, so I don't know what they are
 (all 40 -pin chips).

ROMs 1 to 4 = GFX?
ROMs 5 to 8 = Program?
ROM 9 = Sound CPU code?
ROM 10 = Sound samples?
ROM 11 = Main CPU code?

-There's a "copyright 1987 data east corp.all rights reserved"
 string inside ROM 11

-Sound = Yamaha YM2203C + Y3014B

-Also, there are some GALs on the board (not dumped) a
 8-dips bank and two oscilators (4 MHz and 24 MHz, both near
 the sound parts).

ClawGrip, Jul 2006

*/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/2203intf.h"
#include "sound/3812intf.h"
//#include "sound/msm5205.h"
#include "sound/okim6295.h"

extern WRITE8_HANDLER( pokechmp_videoram_w );
extern WRITE8_HANDLER( pokechmp_flipscreen_w );

extern VIDEO_START( pokechmp );
extern VIDEO_UPDATE( pokechmp );


static WRITE8_HANDLER( pokechmp_bank_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	if (data == 0x00)
	{
		memory_set_bankptr(1,&RAM[0x10000]);
		memory_set_bankptr(2,&RAM[0x12000]);
	}
	if (data == 0x01)
	{
		memory_set_bankptr(1,&RAM[0x14000]);
		memory_set_bankptr(2,&RAM[0x16000]);
	}
	if (data == 0x02)
	{
		memory_set_bankptr(1,&RAM[0x20000]);
		memory_set_bankptr(2,&RAM[0x22000]);
	}

	if (data == 0x03)
	{
		memory_set_bankptr(1,&RAM[0x04000]);
		memory_set_bankptr(2,&RAM[0x06000]);
	}
}

static WRITE8_HANDLER( pokechmp_sound_bank_w )
{
	memory_set_bank(3, (data >> 2) & 1);
}

static WRITE8_HANDLER( pokechmp_sound_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
}


INLINE void pokechmp_set_color(pen_t color, int rshift, int gshift, int bshift, UINT16 data)
{
	palette_set_color(Machine, color, pal5bit(data >> rshift), pal5bit(data >> gshift), pal5bit(data >> bshift));
}


WRITE8_HANDLER( pokechmp_paletteram_w )
{
	paletteram[offset] = data;
	pokechmp_set_color(offset &0x3ff, 0, 5, 10, (paletteram[offset&0x3ff]<<8) | ( paletteram[ (offset&0x3ff)+0x400 ] )  );
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x1800, 0x1800) AM_READ(input_port_0_r)
	AM_RANGE(0x1a00, 0x1a00) AM_READ(input_port_1_r)
	AM_RANGE(0x1c00, 0x1c00) AM_READ(input_port_2_r)

	/* Extra on Poke Champ (not on Pocket Gal) */
	AM_RANGE(0x2000, 0x27ff) AM_READ(MRA8_RAM)

	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0800, 0x0fff) AM_WRITE(pokechmp_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1000, 0x11ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x1801, 0x1801) AM_WRITE(pokechmp_flipscreen_w)
	AM_RANGE(0x1802, 0x181f) AM_WRITE(MWA8_NOP)
/* 1800 - 0x181f are unused BAC-06 registers, see video/dec0.c */
	AM_RANGE(0x1a00, 0x1a00) AM_WRITE(pokechmp_sound_w)
	AM_RANGE(0x1c00, 0x1c00) AM_WRITE(pokechmp_bank_w)

	/* Extra on Poke Champ (not on Pocket Gal) */
	AM_RANGE(0x2000, 0x27ff) AM_WRITE(pokechmp_paletteram_w) AM_BASE(&paletteram)

	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

/***************************************************************************/

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2800, 0x2800) AM_READ(OKIM6295_status_0_r ) // extra
	AM_RANGE(0x3000, 0x3000) AM_READ(soundlatch_r)
//  AM_RANGE(0x3400, 0x3400) AM_READ(pokechmp_adpcm_reset_r)    /* ? not sure */
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK3)
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0800, 0x0800) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x0801, 0x0801) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x1000, 0x1000) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x1001, 0x1001) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(MWA8_NOP)	/* MSM5205 chip on Pocket Gal, not connected here? */
//  AM_RANGE(0x2000, 0x2000) AM_WRITE(pokechmp_sound_bank_w)/ * might still be sound bank */
	AM_RANGE(0x2800, 0x2800) AM_WRITE(OKIM6295_data_0_w) // extra
	AM_RANGE(0x4000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


INPUT_PORTS_START( pokechmp )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START	/* Dip switch */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_3C ) )
 	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
 	PORT_DIPNAME( 0x08, 0x08, "Allow 2 Players Game" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
 	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
 	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(	0x00, "100" )
	PORT_DIPSETTING(	0x20, "120" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout pokechmp_charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(1,8), RGN_FRAC(0,8),RGN_FRAC(3,8),RGN_FRAC(2,8),RGN_FRAC(5,8),RGN_FRAC(4,8),RGN_FRAC(7,8),RGN_FRAC(6,8) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	 /* every char takes 8 consecutive bytes */
};


static const gfx_layout pokechmp_spritelayout =
{
	16,16,  /* 16*16 sprites */
	RGN_FRAC(1,8),   /* 1024 sprites */
	4,
	{ RGN_FRAC(1,8),RGN_FRAC(5,8), RGN_FRAC(3,8),RGN_FRAC(7,8) },
	{ 128+7, 128+6, 128+5, 128+4, 128+3, 128+2, 128+1, 128+0, 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every char takes 8 consecutive bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &pokechmp_charlayout,   0x100, 32 }, /* chars */
	{ REGION_GFX2, 0x00000, &pokechmp_spritelayout,   0,  32 }, /* sprites */
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( pokechmp )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 4000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(M6502, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(pokechmp)
	MDRV_VIDEO_UPDATE(pokechmp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)

	MDRV_SOUND_ADD(YM3812, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 4000000/4) // ?? unknown frequency
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)	/* sound fx */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static DRIVER_INIT( pokechmp )
{
	memory_configure_bank(3, 0, 2, memory_region(REGION_CPU2) + 0x10000, 0x4000);
}


ROM_START( pokechmp )
	ROM_REGION( 0x24000, REGION_CPU1, 0 )	 /* 64k for code + 16k for banks */
	ROM_LOAD( "pokechamp_11_27010.bin",	   0x10000, 0x14000, CRC(9afb6912) SHA1(e45da9524e3bb6f64a68200b70d0f83afe6e4379) )
	ROM_CONTINUE(			   0x04000, 0xc000)

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	 /* 96k for code + 96k for decrypted opcodes */
	ROM_LOAD( "pokechamp_09_27c512.bin",	   0x10000, 0x8000, CRC(c78f6483) SHA1(a0d063effd8d1850f674edccb6e7a285b2311d21) )
	ROM_CONTINUE(			   0x08000, 0x8000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE)
	/* Seems to be 8bpp */
	ROM_LOAD( "pokechamp_05_27c020.bin",	   0x00000, 0x40000, CRC(554cfa42) SHA1(862d0dd83697da7bd52dc640c34926c62691afea) )
	ROM_LOAD( "pokechamp_06_27c020.bin",	   0x40000, 0x40000, CRC(00bb9536) SHA1(1a5584297ebb425d6ce331955e0c6a4f467cd1e6) )
	ROM_LOAD( "pokechamp_07_27c020.bin",	   0x80000, 0x40000, CRC(4b15ab5e) SHA1(5523134853b9ea1c81fd5aeb58061376d94e9298) )
	ROM_LOAD( "pokechamp_08_27c020.bin",	   0xc0000, 0x40000, CRC(e9db54d6) SHA1(ac3b7c06d0f61847bf9bc6147f2f88d712f2b4b3) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	/* the first half of all these roms is identical... for rom 3 both halves match, is the first half unused? 4bpp gfx? */
	ROM_LOAD( "pokechamp_01_27c512.bin",	   0x00000, 0x10000, CRC(338fc412) SHA1(bb8ae99ee6a399a8c67bedb88d0837fd0a4a426c) )
	ROM_LOAD( "pokechamp_02_27c512.bin",	   0x10000, 0x10000, CRC(1ff44545) SHA1(2eee44484accce7b0ba21babf6e8344b234a4e87) )
	ROM_LOAD( "pokechamp_03_27c512.bin",	   0x20000, 0x10000, CRC(99f9884a) SHA1(096d6ce70dc51fb9142e80e1ec45d6d7225481f5) )
	ROM_LOAD( "pokechamp_04_27c512.bin",	   0x30000, 0x10000, CRC(ee6991af) SHA1(8eca3cdfd2eb74257253957a87b245b7f85bd038) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "pokechamp_10_27c040.bin",	   0x00000, 0x80000, CRC(b54806ed) SHA1(c6e1485c263ebd9102ff1e8c09b4c4ca5f63c3da) )
ROM_END

GAME( 1995, pokechmp, 0, pokechmp, pokechmp, pokechmp, ROT0, "DGRM", "Poke Champ", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )
