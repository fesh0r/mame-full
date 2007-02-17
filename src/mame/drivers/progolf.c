/***************************************************************

Pro Golf
Data East 1981
PCB version (not cassette)
--------------

All eproms 2732

g0-m thru g6-m on top pcb.
g7-m thru g9-m on bottom pcb.

Three Proms are 82S123 or equivalents.

gam.k11
gbm.k4
gcm.a14

Top pcb contains:
-----------------
One 6502 CPU

Two AY-3-8910

One 6116 ram.

Two 8 dip banks
------------------

Bottom pcb contains:
--------------------
Epoxy CPU module.

MC6845P CRT controller.

Two 6116 rams.

Twenty four 8116 rams.

****************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/ay8910.h"

extern unsigned char *deco_charram;

PALETTE_INIT( btime );
VIDEO_START( btime );
VIDEO_UPDATE( progolf );

READ8_HANDLER( btime_mirrorvideoram_r );
WRITE8_HANDLER( btime_mirrorvideoram_w );

WRITE8_HANDLER( deco_charram_w );


static ADDRESS_MAP_START( main_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_RAM //AM_WRITE(deco_charram_w) AM_BASE(&deco_charram)
	AM_RANGE(0x2000, 0x7fff) AM_RAM //AM_WRITE(deco_charram_w) AM_BASE(&deco_charram)
	AM_RANGE(0x9000, 0x9000) AM_WRITENOP
	AM_RANGE(0x9000, 0x9000) AM_READNOP
	AM_RANGE(0x9200, 0x9200) AM_WRITENOP
	AM_RANGE(0x9400, 0x9400) AM_WRITENOP
	AM_RANGE(0x9600, 0x9600) AM_WRITENOP
	AM_RANGE(0x9600, 0x9600) AM_READ(input_port_0_r)     /* VBLANK */
	AM_RANGE(0x9800, 0x9800) AM_READNOP
	AM_RANGE(0x9800, 0x9801) AM_WRITENOP
	AM_RANGE(0x9a00, 0x9a00) AM_WRITENOP
	AM_RANGE(0x9a00, 0x9a00) AM_READNOP
	AM_RANGE(0x9e00, 0x9e00) AM_WRITENOP
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8400, 0x87ff) AM_RAM AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8800, 0x881f) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x8820, 0x8fff) AM_RAM
	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xafff) AM_READ(soundlatch_r)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0xc000, 0xcfff) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

INPUT_PORTS_START( progolf )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	/* */

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_COIN2 ) PORT_IMPULSE(2)
INPUT_PORTS_END

static gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },    /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static gfx_layout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,      /* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },  /* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
	  0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static gfx_layout progolf_charlayout =
{
	8,8,			/* 8*8 characters */
	RGN_FRAC(1,3),  /* 512 characters */
	3,				/* 3 bits per pixel */
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },  /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static gfx_layout progolf_charlayout2 =
{
	8,8,			/* 8*8 characters */
	RGN_FRAC(1,1),  /* 512 characters */
	1,				/* 3 bits per pixel */
	{ 0 },  /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

#if 1
static gfx_layout progolf_spritelayout2 =
{
	16,16,  /* 16*16 sprites */
	RGN_FRAC(1,1),    /* 256 sprites */
	1,//3,      /* 3 bits per pixel */
	{ 0},//RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },  /* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
	  0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};
#endif
static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &progolf_charlayout, 0, 8 }, /* sprites */
	{ 0,           0x2000, &charlayout,           0, 4 }, /* char set #1 */
	{ 0,           0x2000, &spritelayout,         0, 4 }, /* sprites */

	{ REGION_GFX2, 0x0000, &progolf_charlayout2, 0, 8 }, /* sprites */
	{ REGION_GFX2, 0x0000, &progolf_spritelayout2, 0, 8 }, /* sprites */
	{ -1 } /* end of array */
};


static INTERRUPT_GEN( progolf_interrupt )
{
	//if (readinputport(2) & 0xc0)
		cpunum_set_input_line(0, /*0*/INPUT_LINE_NMI, /*HOLD_LINE*/PULSE_LINE);
}

static MACHINE_DRIVER_START( progolf )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 2000000)		 /* ? */
	MDRV_CPU_PROGRAM_MAP(main_cpu,0)
//  MDRV_CPU_VBLANK_INT(progolf_interrupt,1)

//  MDRV_CPU_ADD(M6502, 500000)
//  MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
//  MDRV_CPU_VBLANK_INT(nmi_line_pulse,16)

	MDRV_SCREEN_REFRESH_RATE(57)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(3072))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32*3)

	MDRV_PALETTE_INIT(btime)

	MDRV_VIDEO_START(btime)
	MDRV_VIDEO_UPDATE(progolf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.23)
MACHINE_DRIVER_END


ROM_START( progolf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "g4-m.2a",      0xb000, 0x1000, CRC(8f06ebc0) SHA1(c012dcaf06cbd9e49f3ae819d9cbed4df8751cec) )
	ROM_LOAD( "g3-m.4a",      0xc000, 0x1000, CRC(8101b231) SHA1(d933992c93b3cd9a052ac40ec1fa92a181b28691) )
	ROM_LOAD( "g2-m.6a",      0xd000, 0x1000, CRC(a4a0d8dc) SHA1(04db60d5cfca4834ac2cc7661f772704489cb329) )
	ROM_LOAD( "g1-m.8a",      0xe000, 0x1000, CRC(749032eb) SHA1(daa356b2c70bcd8cdd0c4df4268b6158bc8aae8e) )
	ROM_LOAD( "g0-m.9a",      0xf000, 0x1000, CRC(8f8b1e8e) SHA1(fc877a8f2b26ea48c5ba2324678d6077f3432a79) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "g6-m.1b",      0xf000, 0x1000, CRC(0c6fadf5) SHA1(9af2c2152b339cadab7aff0b0164d4431d2558bd) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g7-m.7a",      0x0000, 0x1000, CRC(16b42975) SHA1(29268a8a660781ff0de77b3b1bfc16edff7be134) )
	ROM_LOAD( "g8-m.9a",      0x1000, 0x1000, CRC(cf3f35da) SHA1(06acc29a5e282b5a9960eabebdb1a529910286b6) )
	ROM_LOAD( "g9-m.10a",     0x2000, 0x1000, CRC(7712e248) SHA1(4e7dd12d323cf8378adb1e32a763a1799e2b4bdc) )

	ROM_REGION( 0x60, REGION_PROMS, 0 )
	ROM_LOAD( "gcm.a14",      0x0000, 0x0020, CRC(8259e7db) SHA1(f98db5ebf8182eb0359fa372fa664cb6d3b09437) )
	ROM_LOAD( "gbm.k4",       0x0020, 0x0020, CRC(1ea3319f) SHA1(809af38e73fa1f30410e7d6b4504fe360ee9b091) )
	ROM_LOAD( "gam.k11",      0x0040, 0x0020, CRC(b9665de3) SHA1(4c5aba5f6589f4bce4692c0d5bb2811ab8e14aed) )
ROM_END

ROM_START( progolfa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "g4-m.a3",      0xb000, 0x1000, CRC(015a08d9) SHA1(671d5cd708e098dbda3e495a8b4ce3393c6971da) )
	ROM_LOAD( "g3-m.a4",      0xc000, 0x1000, CRC(c1339da5) SHA1(e9728dcc5f67fbe79eea818ba48421c46d9e63e9) )
	ROM_LOAD( "g2-m.a6",      0xd000, 0x1000, CRC(fafec36e) SHA1(70880d6f9b11505d466f36c12a43361ee2639fed) )
	ROM_LOAD( "g1-m.a8",      0xe000, 0x1000, CRC(749032eb) SHA1(daa356b2c70bcd8cdd0c4df4268b6158bc8aae8e) )
	ROM_LOAD( "g0-m.a9",      0xf000, 0x1000, CRC(a03c533f) SHA1(2e0006be40e32b64b1490bd339d9fc9302eee7c4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "g5-m.b1",      0xf000, 0x1000, CRC(0c6fadf5) SHA1(9af2c2152b339cadab7aff0b0164d4431d2558bd) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g7-m.a8",      0x0000, 0x1000, CRC(16b42975) SHA1(29268a8a660781ff0de77b3b1bfc16edff7be134) )
	ROM_LOAD( "g8-m.a9",      0x1000, 0x1000, CRC(cf3f35da) SHA1(06acc29a5e282b5a9960eabebdb1a529910286b6) )
	ROM_LOAD( "g9-m.a10",     0x2000, 0x1000, CRC(7712e248) SHA1(4e7dd12d323cf8378adb1e32a763a1799e2b4bdc) )

	ROM_REGION( 0x60, REGION_PROMS, 0 )
	ROM_LOAD( "gcm.a14",      0x0000, 0x0020, CRC(8259e7db) SHA1(f98db5ebf8182eb0359fa372fa664cb6d3b09437) )
	ROM_LOAD( "gbm.k4",       0x0020, 0x0020, CRC(1ea3319f) SHA1(809af38e73fa1f30410e7d6b4504fe360ee9b091) )
	ROM_LOAD( "gam.k11",      0x0040, 0x0020, CRC(b9665de3) SHA1(4c5aba5f6589f4bce4692c0d5bb2811ab8e14aed) )
ROM_END


DRIVER_INIT( progolf )
{
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	UINT8* decrypted = auto_malloc(0x10000);

	memory_set_decrypted_region(0,0x0000,0xffff, decrypted);

	/* Swap bits 5 & 6 for opcodes */
	for (A = 0;A < 0x10000;A++)
		decrypted[A] = BITSWAP8(rom[A],7,5,6,4,3,2,1,0);
}

GAME( 1981, progolf,  0,       progolf, progolf, progolf, ROT270, "Data East Corporation", "Pro Golf (set 1)", GAME_NOT_WORKING )
GAME( 1981, progolfa, progolf, progolf, progolf, progolf, ROT270, "Data East Corporation", "Pro Golf (set 2)", GAME_NOT_WORKING ) // doesn't display anything
