/***************************************************************************

  Sony PSX
  ========
  Preliminary driver by smf

issues:

psxj22, psxa22, psxe22, psxj30, psxa30, psxj40
 garbage at the bottom of the screen since merging with mame code

psxe30
 may never have booted

***************************************************************************/

#include "driver.h"
#include "cpu/mips/mips.h"
#include "includes/psx.h"

static MEMORY_WRITE32_START( psx_writemem )
	{ 0x00000000, 0x001fffff, MWA32_RAM },	/* ram */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_w },
	{ 0x1f801070, 0x1f801077, psxirq_w },
	{ 0x1f801080, 0x1f8010ff, psxdma_w },
	{ 0x1f801800, 0x1f801803, psx_cd_w },
	{ 0x1f801810, 0x1f801817, psxgpu_w },
	{ 0x80000000, 0x801fffff, MWA32_BANK2 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MWA32_BANK3 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM },	/* bios */
MEMORY_END

static MEMORY_READ32_START( psx_readmem )
	{ 0x00000000, 0x001fffff, MRA32_RAM },		/* ram */
	{ 0x1f800000, 0x1f8003ff, MRA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_r },
	{ 0x1f801070, 0x1f801077, psxirq_r },
	{ 0x1f801080, 0x1f8010ff, psxdma_r },
	{ 0x1f801800, 0x1f801803, psx_cd_r },
	{ 0x1f801810, 0x1f801817, psxgpu_r },
	{ 0x80000000, 0x801fffff, MRA32_BANK2 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MRA32_BANK3 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA32_BANK4 },	/* bios */
MEMORY_END

static DRIVER_INIT( psx )
{
	cpu_setbank( 1, memory_region( REGION_CPU1 ) + 0x200000 );
	cpu_setbank( 2, memory_region( REGION_CPU1 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER1 ) );
}

INPUT_PORTS_START( psx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

static MACHINE_DRIVER_START( psx )
	/* basic machine hardware */
	MDRV_CPU_ADD(PSXCPU, 33868800) /* 33MHz ?? */
	MDRV_CPU_MEMORY(psx_readmem,psx_writemem)
	MDRV_CPU_VBLANK_INT(psx,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(psx)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psxgpu)
	MDRV_VIDEO_START(psxgpu1024x512)
	MDRV_VIDEO_UPDATE(psxgpu)
	MDRV_VIDEO_STOP(psxgpu)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

ROM_START( psx )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1000.bin",  0x0000000, 0x080000, CRC(3b601fc8) SHA1(343883a7b555646da8cee54aadd2795b6e7dd070) )
ROM_END

ROM_START( psxj22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5000.bin",  0x0000000, 0x080000, CRC(24fc7e17) SHA1(ffa7f9a7fb19d773a0c3985a541c8e5623d2c30d) ) /* bad 0x8c93a399 */
ROM_END

ROM_START( psxa22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1001.bin",  0x0000000, 0x080000, CRC(37157331) SHA1(10155d8d6e6e832d6ea66db9bc098321fb5e8ebf) )
ROM_END

ROM_START( psxe22 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "dtlh3002.bin",  0x0000000, 0x080000, CRC(1e26792f) SHA1(b6a11579caef3875504fcf3831b8e3922746df2c) )
ROM_END

ROM_START( psxj30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5500.bin",  0x0000000, 0x080000, CRC(ff3eeb8c) SHA1(b05def971d8ec59f346f2d9ac21fb742e3eb6917) )
ROM_END

ROM_START( psxa30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7003.bin",  0x0000000, 0x080000, CRC(8d8cb7e4) SHA1(0555c6fae8906f3f09baf5988f00e55f88e9f30b) )
ROM_END

ROM_START( psxe30 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5502.bin",  0x0000000, 0x080000, CRC(4d9e7c86) SHA1(f8de9325fc36fcfa4b29124d291c9251094f2e54) )
ROM_END

ROM_START( psxj40 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7000.bin",  0x0000000, 0x080000, CRC(ec541cd0) SHA1(77b10118d21ac7ffa9b35f9c4fd814da240eb3e9) )
ROM_END

ROM_START( psxa41 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7001.bin",  0x0000000, 0x080000, CRC(502224b6) SHA1(14df4f6c1e367ce097c11deae21566b4fe5647a9) )
ROM_END

ROM_START( psxe41 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7502.bin",  0x0000000, 0x080000, CRC(318178bf) SHA1(8d5de56a79954f29e9006929ba3fed9b6a418c1d) )
ROM_END

SYSTEM_CONFIG_START( psx )
SYSTEM_CONFIG_END

/*
The version number & release date is stored in ascii text at the end of every bios, except for scph1000.
There is also a BCD encoded date at offset 0x100, but this is set to 041211995 in every version apart
from scph1000 & scph7000 ( where it is 22091994 & 29051997 respectively ).
*/

/*		YEAR	NAME	PARENT	COMPAT	MACHINE INPUT	INIT	CONFIG  COMPANY 	FULLNAME */
CONSX( 1994,	psx,	0,		0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph1000)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxj22,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph5000 J v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxa22,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph1001/dtlh3000 A v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxe22,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph1002/dtlh3002 E v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1996,	psxj30,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph5500 J v3.0 09/09/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1996,	psxa30,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph7003 A v3.0 11/18/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxe30,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph5502 E v3.0 01/06/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxj40,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph7000 J v4.0 08/18/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxa41,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph7001 A v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxe41,	psx,	0,		psx,	psx,	psx,	psx,	"Sony",		"Sony PSX (scph7502 E v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
