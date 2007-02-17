/*************************************************************************************************************

    Cow Race

    preliminary driver by Luca Elia

*************************************************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"
#include "sound/2203intf.h"

static tilemap *tmap;

static WRITE8_HANDLER( cowrace_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(tmap, offset);
}

static WRITE8_HANDLER( cowrace_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(tmap, offset);
}

static void get_tile_info(int tile_index)
{
	UINT16 code = videoram[ tile_index ] + (colorram[ tile_index ] << 8) ;
	SET_TILE_INFO(1, code & 0x1ff, 0, TILE_FLIPYX( 0 ));
}

VIDEO_START( cowrace )
{
	tmap = tilemap_create(	get_tile_info, tilemap_scan_rows,
							TILEMAP_TRANSPARENT, 8,8, 0x20,0x20	);

	tilemap_set_transparent_pen(tmap, 0);
	return 0;
}

VIDEO_UPDATE( cowrace )
{
	fillbitmap(bitmap,Machine->pens[0],cliprect);
	tilemap_draw(bitmap,cliprect, tmap, 0, 0);
	return 0;
}

static WRITE8_HANDLER( cowrace_soundlatch_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static READ8_HANDLER( ret_ff )
{
	return 0xff;
}

static READ8_HANDLER( ret_00 )
{
	return 0x00;
}

static UINT8 cowrace_38c2;

static WRITE8_HANDLER( cowrace_38c2_w )
{
	cowrace_38c2 = data;
}

static READ8_HANDLER( cowrace_30c3_r )
{
	switch( cowrace_38c2 )
	{
		case 0x02:	return 0x03;	break;
		case 0x04:	return 0x00;	break;
	}

	return 0xff;
}

static ADDRESS_MAP_START( mem_map_cowrace, ADDRESS_SPACE_PROGRAM, 8 )
AM_RANGE(0x302f, 0x302f) AM_READ( ret_00 )
AM_RANGE(0x30c3, 0x30c3) AM_READ( cowrace_30c3_r )
AM_RANGE(0x38c2, 0x38c2) AM_READWRITE( ret_ff, cowrace_38c2_w )

	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x33ff) AM_RAM
	AM_RANGE(0x4000, 0x43ff) AM_READWRITE( MRA8_RAM, cowrace_videoram_w ) AM_BASE( &videoram )
	AM_RANGE(0x5000, 0x53ff) AM_READWRITE( MRA8_RAM, cowrace_colorram_w ) AM_BASE( &colorram )
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map_cowrace, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_NOP
ADDRESS_MAP_END


static ADDRESS_MAP_START( mem_map_sound_cowrace, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map_sound_cowrace, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x40) AM_READWRITE(YM2203_read_port_0_r,YM2203_write_port_0_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(YM2203_control_port_0_w)
ADDRESS_MAP_END


static const gfx_layout layout8x8x2 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{
		RGN_FRAC(0,2),
		RGN_FRAC(1,2)
	},
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout layout8x8x4 =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{
		RGN_FRAC(0,4),
		RGN_FRAC(1,4),
		RGN_FRAC(2,4),
		RGN_FRAC(3,4)
	},
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_decode gfxdecodeinfo_cowrace[] =
{
	{ REGION_GFX1, 0x000000, &layout8x8x4, 0, 0x1 },
	{ REGION_GFX2, 0x000000, &layout8x8x2, 0, 0x1 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( cowrace )
	PORT_START	// IN0
INPUT_PORTS_END

static struct YM2203interface ym2203_interface_1 =
{
	soundlatch_r,	OKIM6295_status_0_r,	// read  A,B
	0,				OKIM6295_data_0_w,		// write A,B
	0
};


static MACHINE_DRIVER_START( cowrace )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(mem_map_cowrace,0)
	MDRV_CPU_IO_MAP(io_map_cowrace,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(mem_map_sound_cowrace,0)
	MDRV_CPU_IO_MAP(io_map_sound_cowrace,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	// NMI by main CPU

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo_cowrace)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(cowrace)
	MDRV_VIDEO_UPDATE(cowrace)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(OKIM6295, 1056000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.80)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.80)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_CONFIG(ym2203_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.80)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.80)
MACHINE_DRIVER_END


ROM_START( cowrace )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "u3.bin", 0x0000, 0x8000, CRC(c05c3bd3) SHA1(b7199a069ab45edd25e021589b79105cdfa5511a) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "u164.bin", 0x0000, 0x2000, CRC(9affa1c8) SHA1(bfc07693e8f749cbf20ab8cda33975b66f567962) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD( "u94.bin", 0x0000, 0x8000, CRC(945dc115) SHA1(bdd145234e6361c42ed20e8ca4cac64f07332748) )
	ROM_LOAD( "u95.bin", 0x8000, 0x8000, CRC(fc1fc006) SHA1(326a67c1ea0f487ecc8b7aef2d90124a01e6dee3) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )
	ROM_LOAD( "u139.bin", 0x0000, 0x2000, CRC(b746bb2f) SHA1(5f5f48752689079ed65fe7bb4a69512ada5db05d) )
	ROM_LOAD( "u140.bin", 0x2000, 0x2000, CRC(7e24b674) SHA1(c774efeb8e4e833e73c29007d5294c93df1abef4) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "u4.bin", 0x00000, 0x20000, CRC(f92a3ab5) SHA1(fc164492793597eadb8a50154410936edb74fa23) )
ROM_END

GAME( 19??, cowrace, 0, cowrace, cowrace, 0,	ROT0, "<unknown>", "Cow Race", GAME_NOT_WORKING )
