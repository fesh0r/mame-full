/***************************************************************************

Ace Video Fuit Machine hardware
(c)1981 ACE Leisure

Driver by SMF & Guddler 04/02/2007

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/generic.h"

#include "sidewndr.lh"

static void acefruit_update_irq( int vpos )
{
	int col;
	int row = vpos / 8;

	for( col = 0; col < 32; col++ )
	{
		int tile_index = ( col * 32 ) + row;
		int color = colorram[ tile_index ];

		switch( color )
		{
		case 0x0c:
			cpunum_set_input_line( 0, 0, HOLD_LINE );
			break;
		}
	}
}

static mame_timer *acefruit_refresh_timer;

static void acefruit_refresh(int ref)
{
	int vpos = video_screen_get_vpos( 0 );

	video_screen_update_partial( 0, vpos );
	acefruit_update_irq( vpos );

	vpos = ( ( vpos / 8 ) + 1 ) * 8;

	mame_timer_adjust( acefruit_refresh_timer, video_screen_get_time_until_pos( 0, vpos, 0 ), 0, time_never );
}

VIDEO_START( acefruit )
{
	acefruit_refresh_timer = timer_alloc( acefruit_refresh );

	return 0;
}

INTERRUPT_GEN( acefruit_vblank )
{
	cpunum_set_input_line( 0, 0, HOLD_LINE );
	mame_timer_adjust( acefruit_refresh_timer, time_zero, 0, time_never );
}

VIDEO_UPDATE( acefruit )
{
	int startrow = cliprect->min_y / 8;
	int endrow = cliprect->max_y / 8;
	int row;
	int col;

	for( row = startrow; row <= endrow; row++ )
	{
		int spriterow = 0;
		int spriteindex = 0;
		int spriteparameter = 0;

		for( col = 0; col < 32; col++ )
		{
			int tile_index = ( col * 32 ) + row;
			int code = videoram[ tile_index ];
			int color = colorram[ tile_index ];

			if( color < 0x4 )
			{
				drawgfx( bitmap, Machine->gfx[ 1 ], code, color, 0, 0, col * 16, row * 8, cliprect, TRANSPARENCY_NONE, 0 );
			}
			else if( color >= 0x5 && color <= 0x7 )
			{
				int y;
				int x;
				int spriteskip[] = { 1, 2, 4 };
				int spritesize = spriteskip[ color - 5 ];
				const gfx_element *gfx = Machine->gfx[ 0 ];

				for( x = 0; x < 16; x++ )
				{
					for( y = 0; y < 8; y++ )
					{
						UINT16 *dst = BITMAP_ADDR16( bitmap, y + ( row * 8 ), x + ( col * 16 ) );
						int sprite = ( spriteram[ ( spriteindex / 64 ) % 6 ] & 0xf ) ^ 0xf;
						*( dst ) = *( gfx->gfxdata + ( sprite * gfx->char_modulo ) + ( ( spriterow + y ) * gfx->line_modulo ) + ( ( spriteindex % 64 ) >> 1 ) );
					}

					spriteindex += spritesize;
				}
			}
			else
			{
				int y;
				int x;

				for( x = 0; x < 16; x++ )
				{
					for( y = 0; y < 8; y++ )
					{
						UINT16 *dst = BITMAP_ADDR16( bitmap, y + ( row * 8 ), x + ( col * 16 ) );
						*( dst ) = 0;
					}
				}

				if( color == 0x8 )
				{
					if( spriteparameter == 0 )
					{
						spriteindex = code & 0xf;
					}
					else
					{
						spriterow = ( ( code >> 0 ) & 0x3 ) * 8;
						spriteindex += ( ( code >> 2 ) & 0x1 ) * 16;
					}

					spriteparameter = !spriteparameter;
				}
				else if( color == 0xc )
				{
					/* irq generated in acefruit_update_irq() */
				}
			}
		}
	}

	return 0;
}

static UINT32 sidewndr_payout_r(void *param)
{
	int bit_mask = (int)param;

	switch (bit_mask)
	{
		case 0x01:
			return ((readinputportbytag("FAKE") & bit_mask) >> 0);
		case 0x02:
			return ((readinputportbytag("FAKE") & bit_mask) >> 1);
		default:
			logerror("sidewndr_payout_r : invalid %02X bit_mask\n",bit_mask);
			return 0;
	}
}

WRITE8_HANDLER( acefruit_colorram_w )
{
	colorram[ offset ] = data & 0xf;
}

WRITE8_HANDLER( acefruit_coin_w )
{
	/* TODO: ? */
}

WRITE8_HANDLER( acefruit_sound_w )
{
	/* TODO: ? */
}

WRITE8_HANDLER( acefruit_lamp_w )
{
	int i;

	for( i = 0; i < 8; i++ )
	{
		output_set_lamp_value( ( offset * 8 ) + i, ( data >> i ) & 1 );
	}
}

WRITE8_HANDLER( acefruit_solenoid_w )
{
	int i;

	for( i = 0; i < 8; i++ )
	{
		output_set_indexed_value( "solenoid", i, ( data >> i ) & 1 );
	}
}

PALETTE_INIT( acefruit )
{
	/* sprites */
	palette_set_color( Machine, 0, 0x00, 0x00, 0x00 );
	palette_set_color( Machine, 1, 0x00, 0x00, 0xff );
	palette_set_color( Machine, 2, 0x00, 0xff, 0x00 );
	palette_set_color( Machine, 3, 0xff, 0x7f, 0x00 );
	palette_set_color( Machine, 4, 0xff, 0x00, 0x00 );
	palette_set_color( Machine, 5, 0xff, 0xff, 0x00 );
	palette_set_color( Machine, 6, 0xff, 0xff, 0xff );
	palette_set_color( Machine, 7, 0x7f, 0x3f, 0x1f );

	colortable[ 0 ] = 0;
	colortable[ 1 ] = 1;
	colortable[ 2 ] = 2;
	colortable[ 3 ] = 3;
	colortable[ 4 ] = 4;
	colortable[ 5 ] = 5;
	colortable[ 6 ] = 6;
	colortable[ 7 ] = 7;

	/* tiles */
	palette_set_color( Machine, 8, 0xff, 0xff, 0xff );
	palette_set_color( Machine, 9, 0x00, 0x00, 0xff );
	palette_set_color( Machine, 10, 0x00, 0xff, 0x00 );
	palette_set_color( Machine, 11, 0xff, 0x00, 0x00 );

	colortable[ 8 ] = 0;
	colortable[ 9 ] = 8;
	colortable[ 10 ] = 0;
	colortable[ 11 ] = 9;
	colortable[ 12 ] = 0;
	colortable[ 13 ] = 10;
	colortable[ 14 ] = 0;
	colortable[ 15 ] = 11;
}

static ADDRESS_MAP_START( acefruit_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x20ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x4400, 0x47ff) AM_READWRITE(MRA8_RAM, acefruit_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8000, 0x8000) AM_READ(input_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(input_port_1_r)
	AM_RANGE(0x8002, 0x8002) AM_READ(input_port_2_r)
	AM_RANGE(0x8003, 0x8003) AM_READ(input_port_3_r)
	AM_RANGE(0x8004, 0x8004) AM_READ(input_port_4_r)
	AM_RANGE(0x8005, 0x8005) AM_READ(input_port_5_r)
	AM_RANGE(0x8006, 0x8006) AM_READ(input_port_6_r)
	AM_RANGE(0x8007, 0x8007) AM_READ(input_port_7_r)
	AM_RANGE(0x6000, 0x6005) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0xa000, 0xa001) AM_WRITE(acefruit_lamp_w)
	AM_RANGE(0xa002, 0xa003) AM_WRITE(acefruit_coin_w)
	AM_RANGE(0xa004, 0xa004) AM_WRITE(acefruit_solenoid_w)
	AM_RANGE(0xa005, 0xa006) AM_WRITE(acefruit_sound_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( acefruit_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x00) AM_NOP /* ? */
ADDRESS_MAP_END

INPUT_PORTS_START( sidewndr )
	PORT_START_TAG("IN0")	// 0
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME( "Stop Nudge Random" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME( "Gamble" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )              /* "Cash in" */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK ) /* active low or high?? */
	PORT_BIT( 0xd8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")	// 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME( "Sidewind" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME( "Collect" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )              /* "Cash in" */
	PORT_DIPNAME( 0x08, 0x00, "Accountacy System Texts" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME( "Cancel/Clear" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME( "Refill" ) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )              /* "Token in" - also "Refill" when "Refill" mode ON */
	PORT_BIT( 0x08, 0x00, IPT_SPECIAL) PORT_CUSTOM(sidewndr_payout_r, 0x01)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")	// 3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME( "Hold/Nudge 1" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME( "Accountancy System" ) PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 )              /* "50P in" */
	PORT_BIT( 0x08, 0x00, IPT_SPECIAL) PORT_CUSTOM(sidewndr_payout_r, 0x02)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN4")	// 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME( "Hold/Nudge 2" )
	PORT_DIPNAME( 0x02, 0x00, "Allow Clear Data" )          /* in "Accountancy System" mode */
	PORT_DIPSETTING(    0x02, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "Set Lamp 0xa001-3" )         /* code at 0x173a - write lamp status at 0x01ed */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Sounds" )                    /* data in 0x206b and 0x206c - out sound at 0x193e */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN5")	// 5
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME( "Hold/Nudge 3" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME( "Test Program" ) PORT_TOGGLE
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN6")	// 6
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME( "Hold/Nudge 4" )
	/* I don't know exactly what this bit is supposed to do :(
       I only found that when bit is LOW, no data is updated
       (check "Accountancy System" mode). And when you switch
       it from LOW to HIGH, previous saved values are back
       (check for example the number of credits). */
	PORT_DIPNAME( 0x02, 0x02, "Save Data" )                 /* code at 0x1773 */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN7")	// 7
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )             /* next in "Accountancy System" mode */
	PORT_DIPNAME( 0x02, 0x00, "Clear Credits on Reset" )    /* also affects rolls */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("FAKE")	// fake port to handle settings via multiple input ports
	PORT_DIPNAME( 0x03, 0x00, "Payout %" )
	PORT_DIPSETTING(    0x00, "74%" )
	PORT_DIPSETTING(    0x02, "78%" )
	PORT_DIPSETTING(    0x01, "82%" )
	PORT_DIPSETTING(    0x03, "86%" )
INPUT_PORTS_END

INPUT_PORTS_START( sidewnda )
	PORT_INCLUDE(sidewndr)

	PORT_MODIFY("IN0")
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )          /* before COIN4 test - code at 0x0994 */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xd0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("IN1")
	PORT_DIPNAME( 0x08, 0x08, "Accountacy System Texts" )   /* bit test is inverted compared to 'sidewndr' */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

	PORT_MODIFY("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME( "Cancel" )          /* see IN4 bit 0 in "Accountancy System" mode */

	PORT_MODIFY("IN4")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME( "Clear Data" )     /* in "Accountancy System" mode */
    /* Similar to 'sidewndr' but different addresses */
	PORT_DIPNAME( 0x04, 0x00, "Set Lamp 0xa001-3" )         /* code at 0x072a - write lamp status at 0x00ff */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
    /* Similar to 'sidewndr' but different addresses */
	PORT_DIPNAME( 0x08, 0x00, "Sounds" )                    /* data in 0x2088 and 0x2089 - out sound at 0x012d */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("IN6")
	/* I don't know exactly what this bit is supposed to do :(
       I only found that when bit is LOW, no data is updated
       (check "Accountancy System" mode). */
	PORT_DIPNAME( 0x02, 0x02, "Save Data" )                 /* code at 0x0763 (similar to 'sidewndr') and 0x18db */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )

	PORT_MODIFY("IN7")
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )          /* code at 0x04a8 */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf4, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	16,8, /* 8*8 characters doubled horizontally */
	256, /* 256 characters */
	1, /* 1 bit per pixel */
	{ 0 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	32,32, /* 32*32 sprites */
	16, /* 16 sprites */
	3, /* 3 bits per pixel */
	/* Offset to the start of each bit */
	{ 0, 256*8*8, 256*8*8*2 },
	/* Offset to the start of each byte */
	{
		0,  1,   2,  3,  4,  5,  6,  7,
		8,  9,  10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31
	},
	/* Offset to the start of each line */
	{
		0*32,   1*32,  2*32,  3*32,  4*32,  5*32,  6*32,  7*32,
		8*32,   9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
		16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
		24*32, 25*32, 26*32, 27*32, 28*32, 29*32, 30*32, 31*32
	},
	/* Offset to next sprite (also happens to be number of bits per sprite) */
	32*32 /* every sprite takes 128 bytes */
};

static gfx_decode acefruit_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &spritelayout, 0, 1 },
	{ REGION_GFX1, 0x1800, &charlayout, 8, 4 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( acefruit )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2500000) /* 2.5MHz */
	MDRV_CPU_PROGRAM_MAP(acefruit_map,0)
	MDRV_CPU_IO_MAP(acefruit_io,0)
	MDRV_GFXDECODE(acefruit_gfxdecodeinfo)
	MDRV_CPU_VBLANK_INT(acefruit_vblank,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 511, 0, 255)
	MDRV_PALETTE_LENGTH(12)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_PALETTE_INIT(acefruit)
	MDRV_VIDEO_START(acefruit)
	MDRV_VIDEO_UPDATE(acefruit)

	/* sound hardware */
MACHINE_DRIVER_END

static DRIVER_INIT( sidewndr )
{
	UINT8 *ROM = memory_region( REGION_CPU1 );
	/* replace "ret nc" ( 0xd0 ) with "di" */
	ROM[ 0 ] = 0xf3;
	/* this is either a bad dump or the cpu core should set the carry flag on reset */
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sidewndr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2_h09.bin",    0x000000, 0x000800, BAD_DUMP CRC(141f3b0c) SHA1(1704feba950fe7aa939b9ed54c37264d10527d11) )
	ROM_LOAD( "2_h10.bin",    0x000800, 0x000800, CRC(36a2d4af) SHA1(2388e22245497240e5721895d94d2ccd1f579eff) )
	ROM_LOAD( "2_h11.bin",    0x001000, 0x000800, CRC(e2932643) SHA1(e1c0cd5d0cd332519432cbefa8718362a6cd1ccc) )
	ROM_LOAD( "2_h12.bin",    0x001800, 0x000800, CRC(26af0b1f) SHA1(36f0e54982688b9d5a24a6986a847ac69ee0a355) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8k for graphics */
	ROM_LOAD( "2_h05.bin",    0x000000, 0x000800, CRC(64b64cff) SHA1(c11f2bd2af68ae7f104b711deb7f6509fdbaeb8f) )
	ROM_LOAD( "2_h06.bin",    0x000800, 0x000800, CRC(6b96a586) SHA1(6d5ab8fefe37ca4dbc5057ebf31f12b33dbdf5c0) )
	ROM_LOAD( "2_h07.bin",    0x001000, 0x000800, CRC(3a8e68a2) SHA1(2ffe07360f57f0f11ecf326f00905747d9b66811) )
	ROM_LOAD( "2_h08.bin",    0x001800, 0x000800, CRC(bd19a758) SHA1(3fa812742f34643f66c67cb9bdb1d4d732c4f44d) )
ROM_END

ROM_START( sidewnda )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "h9.bin",       0x000000, 0x000800, CRC(9919fcfa) SHA1(04167b12ee9e60ef891893a305a35d3f2eccb0bb) )
	ROM_LOAD( "h10.bin",      0x000800, 0x000800, CRC(90502d00) SHA1(3bdd859d9146df2eb97b4517c446182569a55a46) )
	ROM_LOAD( "h11.bin",      0x001000, 0x000800, CRC(7375166c) SHA1(f05b01941423fd36e0a5d3aa913a594e4e7aa5d4) )
	ROM_LOAD( "h12.bin",      0x001800, 0x000800, CRC(4546c68c) SHA1(92104e2005fc772ea9f70451d9d674f95d3f0ba9) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8k for graphics */
	ROM_LOAD( "h5.bin",       0x000000, 0x000800, CRC(198da32c) SHA1(bf6c4ddcda0503095d310e08057dd88154952ef4) )
	ROM_LOAD( "h6.bin",       0x000800, 0x000800, CRC(e777130f) SHA1(3421c6f399e5ec749f1908f6b4ebff7761c6c5d9) )
	ROM_LOAD( "h7.bin",       0x001000, 0x000800, CRC(bfed5b8f) SHA1(f95074e8809297eec67da9d7e33ae1dd1c5eabc0) )
	ROM_LOAD( "h8.bin",       0x001800, 0x000800, CRC(05da2b71) SHA1(3a263f605ecc9e4dca9ce0ba815af16e28bf9bc8) )
ROM_END

GAMEL( 1981?, sidewndr, 0,        acefruit, sidewndr, sidewndr, ROT270, "ACE", "Sidewinder (set 1)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND, layout_sidewndr )
GAMEL( 1981?, sidewnda, sidewndr, acefruit, sidewnda, 0,        ROT270, "ACE", "Sidewinder (set 2)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND, layout_sidewndr )
