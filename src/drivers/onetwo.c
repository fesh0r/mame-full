/*

One Two

*/

#include "driver.h"

static struct tilemap *fg_tilemap;
static data8_t *fgram;

static void get_fg_tile_info(int tile_index)
{
	int code = (fgram[tile_index*2+1]<<8)|(fgram[tile_index*2]<<0);
	SET_TILE_INFO(
			0,
			code,
			0,
			0)
}

WRITE_HANDLER( onetwo_fgram_w )
{
	fgram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset/2);
}

/* wrong? */
WRITE_HANDLER ( onetwo_palette_w )
{
	int col,coldat,r,g,b;

	paletteram[offset] = data;

	col = offset & 0xff;

	coldat = (paletteram[col]) << 8 |  (paletteram[col+0x100]);

	r = (coldat >>  5) & 0x1f;
	g = (coldat >> 10) & 0x1f;
	b = (coldat >>  0) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(col,r,g,b);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc800, 0xc9ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc800, 0xc9ff) AM_WRITE(onetwo_palette_w) AM_BASE(&paletteram)
	AM_RANGE(0xd000, 0xdfff) AM_WRITE(onetwo_fgram_w) AM_BASE (&fgram )
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

WRITE_HANDLER ( onetwo_cpubank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1)+0x10000;

	cpu_setbank(1,&RAM[data*0x4000]);
}

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(onetwo_cpubank_w)
ADDRESS_MAP_END



INPUT_PORTS_START( onetwo )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) // freeze? vblank?


	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "4" ) // coins?
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout tiles8x8x6_layout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+4, RGN_FRAC(2,3)+0,RGN_FRAC(1,3)+4, RGN_FRAC(1,3)+0,RGN_FRAC(0,3)+4, RGN_FRAC(0,3)+0  },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8,4*8,5*8,6*8,7*8 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x6_layout, 0, 16 },
	{ -1 }
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* ? frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

VIDEO_START(onetwo)
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64, 32);
	return 0;
}


VIDEO_UPDATE(onetwo)
{
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

static MACHINE_DRIVER_START( onetwo )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(onetwo)
	MDRV_VIDEO_UPDATE(onetwo)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( onetwo )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* main z80 */
	ROM_LOAD( "am27c010.0", 0x00000, 0x20000, CRC(83431e6e) SHA1(61ab386a1d0af050f091f5df28c55ad5ad1a0d4b) )
	ROM_RELOAD ( 0x10000, 0x20000 )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )
	ROM_LOAD( "m27c4000.3", 0x000000, 0x80000, CRC(c72ff3a0) SHA1(17394d8a8b5ef4aee9522d87ba92ef1285f4d76a) )
	ROM_LOAD( "m27c4000.4", 0x080000, 0x80000, CRC(0ca40557) SHA1(ca2db57d64ece90f2066f15b276c8d5827dcb4fa) )
	ROM_LOAD( "zr040p.5",   0x100000, 0x80000, CRC(664b6679) SHA1(f9f78bd34fb58e24f890a540382392e1c9d01220) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound z80 */
	ROM_LOAD( "s27c512.2", 0x00000, 0x10000, CRC(90aba4f3) SHA1(914b1c8684993ddc7200a3d61e07f4f6d59e9d02) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "tm27c020.1", 0x00000, 0x40000, CRC(b10d3132) SHA1(42613e17b6a1300063b8355596a2dc7bcd903777) )
ROM_END

GAMEX( 1997, onetwo, 0, onetwo, onetwo, 0, ROT0, "Barko", "One + Two", GAME_NOT_WORKING )
