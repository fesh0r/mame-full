/*

Ashita no Joe (Success Joe) [Wave]

The set is missing GFX ROMs
 Hitachi HN62414 Mask ROMs x 9

Tow sub-boards:

Upper:

 Program
  ROMS 1 to 8 (ST M27512)
  Standard Motorola MC68000P8
  8.0000 MHz osc.
  PALs W9011A (AMD PALCE16V8H) + W9011B (MMI PAL 16L88CN)

 Sound
  ROM 9 (ST M27256)
  Standard Zilog Z80 (Z0840004PSC)
  Yamaha YM2203C

 GFX?
  Mask ROMs 401, 402 & 403 (Hitachi HN62414 Mask ROMs)

Lower:

 GFX?
  Mask ROMs 404, 405, 406, 407, 408 & 409 (Hitachi HN62414 Mask ROMs)
  PAL W90120R2 (MMI PAL 16L88CN)
  EPL (Ricoh EPL16P8BP, not dumped)
  13.3330 MHz osc.

Dips:

 Two banks (* = default)
  A
                                    1   2   3   4   5   6   7   8
   Game Style      * Table          ON                      OFF OFF
                   Upright          OFF                     OFF OFF
   Secreen Reverse * Usual              OFF                 OFF OFF
                   Reverse              ON                  OFF OFF
   Test Mode       * Game mode              OFF             OFF OFF
                   Test Mode                ON              OFF OFF
   Demo Sound      * Yes                        OFF         OFF OFF
                   No                           ON          OFF OFF
   Play Fee - Coin * 1 Coin 1 Play                  OFF OFF OFF OFF
                   1 Coin 2 Play                    ON  OFF OFF OFF
                   2 Coin 1 Play                    OFF ON  OFF OFF
                   2 Coin 3 Play                    ON  ON  OFF OFF

  B
                                    1   2   3   4   5   6   7   8
   Difficulty      * Rank B         OFF OFF OFF OFF OFF OFF OFF OFF
                   Rank A           ON  OFF OFF OFF OFF OFF OFF OFF
                   Rank C           OFF ON  OFF OFF OFF OFF OFF OFF
                   Rank D           ON  ON  OFF OFF OFF OFF OFF OFF

   Easy (A) -> Difficult (D)

Game is controled with 4-direction lever and two buttons
Coin B is not used

*/

#include "driver.h"

static data16_t* ashnojoetileram16;
static struct tilemap *joetilemap;

static void get_joe_tile_info(int tile_index)
{
	int code = ashnojoetileram16 [tile_index];

	SET_TILE_INFO(
			0,
			code& 0xfff,
			((code >> 12) & 0xf)+0x10,
			0)
}

static WRITE16_HANDLER( ashnojoe_tileram_w )
{
	ashnojoetileram16[offset] = data;
	tilemap_mark_tile_dirty(joetilemap,offset);
}


INPUT_PORTS_START( ashnojoe )
	PORT_START	/* 16-bit */
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )


	PORT_START	/* 16-bit */
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )


	PORT_START	/* 16-bit */
	PORT_DIPNAME( 0x0001, 0x0001, "2" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static MEMORY_READ16_START( ashnojoe_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, MRA16_RAM },
	{ 0x044000, 0x048fff, MRA16_RAM },
	{ 0x049000, 0x049fff, MRA16_RAM },
	{ 0x04a000, 0x04a001, input_port_0_word_r },
	{ 0x04a004, 0x04a005, input_port_1_word_r },
	{ 0x04a00a, 0x04a00b, input_port_2_word_r },
	{ 0x04c000, 0x04ffff, MRA16_RAM },

	{ 0x080000, 0x0bffff, MRA16_ROM }, // missing (tested)
MEMORY_END

static MEMORY_WRITE16_START( ashnojoe_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, ashnojoe_tileram_w, &ashnojoetileram16 },
	{ 0x044000, 0x048fff, MWA16_RAM }, // more tilemaps? sprites?
	{ 0x049000, 0x049fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0x04a006, 0x04a007, MWA16_NOP },
	{ 0x04c000, 0x04ffff, MWA16_RAM },
MEMORY_END

static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo ashnojoe_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 0x20 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 0x20 },
	{ -1 }
};


VIDEO_START(ashnojoe)
{
	joetilemap =  tilemap_create(get_joe_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64, 64);
	return 0;
}

VIDEO_UPDATE(ashnojoe)
{
	tilemap_draw(bitmap,cliprect,joetilemap,0,0);
}

static MACHINE_DRIVER_START( ashnojoe )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz? */
	MDRV_CPU_MEMORY(ashnojoe_readmem,ashnojoe_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

//	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)
	MDRV_GFXDECODE(ashnojoe_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000/2)

	MDRV_VIDEO_START(ashnojoe)
	MDRV_VIDEO_UPDATE(ashnojoe)

	/* sound hardware */
MACHINE_DRIVER_END

ROM_START( ashnojoe )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "5.bin", 0x00000, 0x10000, CRC(c61e1569) SHA1(422c18f5810539b5a9e3a9bd4e3b4d70bde8d1d5) )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x10000, CRC(c0a16338) SHA1(fb127b9d38f2c9807b6e23ff71935fc8a22a2e8f) )

	/* missing data? (tested) */

	ROM_REGION( 0x14000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "9.bin", 0x0000, 0x8000, CRC(8767e212) SHA1(13bf927febedff9d7d164fbf0da7fb3a588c2a94) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.bin", 0x00000, 0x10000, CRC(9bcb160e) SHA1(1677048e5ce26562ff7ba36fcc2d0ed5a652b91e) )
	ROM_LOAD( "7.bin", 0x10000, 0x10000, CRC(7e1efc42) SHA1(e3c282072fdaa0b98c2a1bf25fd02c680d9ca4d7) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin", 0x00000, 0x10000, CRC(7e2d86b5) SHA1(8b8d1b9240a700e29afc109eddf6e58a0a7666a4) ) // taito logo
	ROM_LOAD( "4.bin", 0x10000, 0x10000, CRC(aa6336d3) SHA1(43f70cc3223f11d7929dd44b0edf0a31f5fe41c3) )
	ROM_LOAD( "1.bin", 0x20000, 0x10000, CRC(1bf585f0) SHA1(4003941636e7fded95e880109c3c9dd1d8f28b07) )
	ROM_LOAD( "2.bin", 0x30000, 0x10000, CRC(c3254938) SHA1(fd57163f740cd4fdecca94cced91314c289741ae) )

	ROM_LOAD( "maskroms", 0x00000, 0x10, NO_DUMP )
	/* missing more gfx */
ROM_END


GAMEX( 19??, ashnojoe, 0, ashnojoe, ashnojoe, 0,   ROT0, "WAVE / Taito Corporation", "Ashita no Joe (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND )
