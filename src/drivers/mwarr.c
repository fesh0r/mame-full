/*

Mighty Warrior
Elettronica Video-Games S.R.L, 19??

PCB Layout
----------

|----------------------------------------------|      |---------------------------|
|     M6295  OKI0      PAL          CN1        |      | SF2-0    SF2-1     SF4-1  |
|     M6295  OKI1      PAL     |------------|  |      |                           |
|         PAL                  |------------|  |      | DW-0     SF3-1     SF4-0  |
|                         6264                 |      |                           |
|      2018               6264                 |      |          DW-1      SF3-0  |
|J     2018         |-------|    PAL           |      |                           |
|A            PAL   |ACTEL  |                  |      | OBM-12   OBM-17    OBM-15 |
|M     |-------|    |A1020A |45MHz             |      |                           |
|M PAL |ACTEL  |    |PL84C  |    PAL           |      | OBM-6    OBM-11    OBM-9  |
|A     |A1020A |2018|-------|PAL               |      |                           |
|62256 |PL84C  |2018                           |      | OBM-0    OBM-5     OBM-3  |
|62256 |-------|                               |      |                           |
|DSW1  62256   62256                           |      | OBM-13   OBM-14    OBM-16 |
|DSW2  PRG_OD  PRG_EV                          |      |                           |
|  |------------|PAL               CN2         |      | OBM-7    OBM-8     OBM-10 |
|  |MC68000P10  |          2018|------------|  |      |                           |
|  |------------|   2018   2018|------------|  |      | OBM-1    OBM-2     OBM-4  |
|12MHz              2018        PAL            |      |                           |
|----------------------------------------------|      |---------------------------|
Notes:
      68000 clock : 12.000MHz
      M6295 clocks: 0.9375MHz (45/48). Sample Rate = 937500 / 132
      VSync       : 54Hz
      CN1/2       : Connector for plug-in ROM daughterboard. The board
                    contains only 26x 27C4001 EPROMs and 2x 74LS139 logic IC's.


*/

#include "driver.h"
#include "machine/random.h"

READ16_HANDLER( mwarr_random )
{
	return mame_rand();
}

static ADDRESS_MAP_START( mwarr_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x11ffff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( mwarr )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	// "Rotate" - also IPT_START1
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	// "Help"
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	// "Rotate" - also IPT_START2
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	// "Help"
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Helps" )			// "Power Count" in test mode
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0400, "1" )
	PORT_DIPNAME( 0x0800, 0x0800, "Bonus Bar Level" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( High ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Picture View" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END

static struct GfxLayout mwarr_tile_layout =
{
	16,8,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
	0,
	RGN_FRAC(1,2)+0,
	8,
	RGN_FRAC(1,2)+8,
	16,
	RGN_FRAC(1,2)+16,
	24,
	RGN_FRAC(1,2)+24
	},

	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static struct GfxLayout mwarr_tile_6bpp_16x16layout =
{
	16,16,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(0,6), RGN_FRAC(1,6), RGN_FRAC(2,6), RGN_FRAC(3,6), RGN_FRAC(4,6), RGN_FRAC(5,6)},
	{ 0,1,2,3,4,5,6,7,128,129,130,131,132,133,134,135 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &mwarr_tile_6bpp_16x16layout,   0x0, 2  },

	{ REGION_GFX2, 0, &mwarr_tile_layout,   0x0, 2  },
	{ REGION_GFX3, 0, &mwarr_tile_layout,   0x0, 2  },
	{ REGION_GFX4, 0, &mwarr_tile_layout,   0x0, 2  },
	{ REGION_GFX5, 0, &mwarr_tile_layout,   0x0, 2  },

	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};


VIDEO_START(mwarr)
{
	return 0;
}

VIDEO_UPDATE(mwarr)
{

}

static MACHINE_DRIVER_START( mwarr )
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(mwarr_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(mwarr)
	MDRV_VIDEO_UPDATE(mwarr)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


ROM_START( mwarr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "prg_ev", 0x00000, 0x80000, CRC(d1d5e0a6) SHA1(f47955459d41c904b96de000b32cae156ee3bcba) )
	ROM_LOAD16_BYTE( "prg_od", 0x00001, 0x80000, CRC(e5217d91) SHA1(6a5d282e8e5b98628f98530e3c47b9b398e9334e) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "oki0", 0x00000, 0x40000, CRC(005811ce) SHA1(9149bc8e9cc16ce3db4e22f8cb7ea8a57a66980e) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "oki1", 0x00000, 0x80000, CRC(bcde2330) SHA1(452d871360fa907d2e4ebad93c3fba9a3fa32fa7) )

	ROM_REGION( 0x900000, REGION_GFX1, 0 )
	ROM_LOAD( "obm-0",  0x000000, 0x80000, CRC(b4707ba1) SHA1(35330a31e9837e5f848a21fa6f589412b35a04a0) )
	ROM_LOAD( "obm-6",  0x080000, 0x80000, CRC(f9675acc) SHA1(06e0c0c0928ace331ebd08cfeeaa2c8b5603457f) )
	ROM_LOAD( "obm-12", 0x100000, 0x80000, CRC(6239c4dd) SHA1(128040e9517151faf15c75dc1f2d79c5a66b9e1c) )
	ROM_LOAD( "obm-1",  0x180000, 0x80000, CRC(817dcead) SHA1(697b4b3e18e022e0635a3b02cbce1e4d2959a732) )
	ROM_LOAD( "obm-7",  0x200000, 0x80000, CRC(3a93c499) SHA1(9ecd72c5ef4f0edbdc19946bd33aa4e74690756d) )
	ROM_LOAD( "obm-13", 0x280000, 0x80000, CRC(bac42f06) SHA1(6998e605db732e6be9d8213e96bfb04a258eae8f) )
	ROM_LOAD( "obm-2",  0x300000, 0x80000, CRC(68cd29b0) SHA1(02f7bf463cd15eaf4713d33494f19c4fcd199e87) )
	ROM_LOAD( "obm-8",  0x380000, 0x80000, CRC(f9482638) SHA1(ea6256136362a12a40d6b168157c28a14236fcc1) )
	ROM_LOAD( "obm-14", 0x400000, 0x80000, CRC(79ed46b8) SHA1(93b503b58a316be312a74f2da7df3dbcd275884b) )
	ROM_LOAD( "obm-3",  0x480000, 0x80000, CRC(6e924cb8) SHA1(3c56dfcd042108b1cd16395bcdda0fd92a6ab0f7) )
	ROM_LOAD( "obm-9",  0x500000, 0x80000, CRC(be1fb64e) SHA1(4141b6b78fa9830cf5dc4f4f0b29e87e57f70ccb) )
	ROM_LOAD( "obm-15", 0x580000, 0x80000, CRC(5e0efb71) SHA1(d556ed9307a9a9f59f5235981b4b091a88399c98) )
	ROM_LOAD( "obm-4",  0x600000, 0x80000, CRC(f34b67bd) SHA1(91d6553144e45ea1b96bf59403b3e26224b79a7d) )
	ROM_LOAD( "obm-10", 0x680000, 0x80000, CRC(00c68a23) SHA1(a4984932ac3fae368700f77ad16b35b0138dbc21) )
	ROM_LOAD( "obm-16", 0x700000, 0x80000, CRC(e9516379) SHA1(8d9aaa2ee1331dd3eb8951a1008811703dd9f41d) )
	ROM_LOAD( "obm-5",  0x780000, 0x80000, CRC(b2b976f3) SHA1(77dee06ac1187c8a1c4188951bd7b0a62ec84350) )
	ROM_LOAD( "obm-11", 0x800000, 0x80000, CRC(7bf1e4da) SHA1(4aeef3b7c23303580a851dc793e9671a2a0f421f) )
	ROM_LOAD( "obm-17", 0x880000, 0x80000, CRC(47bd56e8) SHA1(e10569e89083165a7efe29f84167a1c15171ccaf) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD( "sf2-0",  0x000000, 0x80000, CRC(622a1816) SHA1(b7b88a90ff69e8f2e291e1f9299708ec97ef9b77) )
	ROM_LOAD( "sf2-1",  0x080000, 0x80000, CRC(545f89e9) SHA1(e7d52dc2da3770d7310698af47da9ff7ec32388c) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD( "sf3-0",  0x000000, 0x80000, CRC(86cd162c) SHA1(95d5f300e3671ebe29b2331325f4d80b96988619) )
	ROM_LOAD( "sf3-1",  0x080000, 0x80000, CRC(2e755e54) SHA1(74b1e099358a07848f7c22c71fbe2661e1ebb417) )

	ROM_REGION( 0x100000, REGION_GFX4, 0 )
	ROM_LOAD( "sf4-0",  0x000000, 0x80000, CRC(25938b2d) SHA1(6336e41eee58cab9a524b9bca08965786cc133d3) )
	ROM_LOAD( "sf4-1",  0x080000, 0x80000, CRC(2269ce5c) SHA1(4c6169acf17bba94dc5684f5db60d5bcf73ad068) )

	ROM_REGION( 0x100000, REGION_GFX5, 0 )
	ROM_LOAD( "dw-0",  0x000000, 0x80000, CRC(b9b18d00) SHA1(4f38502c75eae88916bc58bfd5d255bac59d0813) )
	ROM_LOAD( "dw-1",  0x080000, 0x80000, CRC(7aea0b12) SHA1(07cbcd6ddcd9ead068b0f5763829e8474b699085) )
ROM_END

GAMEX( 199?, mwarr, 0, mwarr, mwarr, 0, ROT0,  "Elettronica Video-Games S.R.L,", "Mighty Warriors",GAME_NOT_WORKING|GAME_NO_SOUND )
