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
#include "devices/snapquik.h"
#include "includes/psx.h"

struct 
{
	UINT8 id[ 8 ];
	UINT32 text;	/* SCE only */
	UINT32 data;	/* SCE only */
	UINT32 pc0;
	UINT32 gp0;		/* SCE only */
	UINT32 t_addr;
	UINT32 t_size;
	UINT32 d_addr;	/* SCE only */
	UINT32 d_size;	/* SCE only */
	UINT32 b_addr;	/* SCE only */
	UINT32 b_size;	/* SCE only */
	UINT32 s_addr;
	UINT32 s_size;
	UINT32 SavedSP;
	UINT32 SavedFP;
	UINT32 SavedGP;
	UINT32 SavedRA;
	UINT32 SavedS0;
	UINT8 dummy[ 0x800 - 76 ];
} m_psxexe_header;

static UINT8 *m_p_psxexe;

static OPBASE_HANDLER( psx_setopbase )
{
	if( address == 0x80030000 )
	{
		UINT8 *p_ram;
		UINT8 *p_psxexe;
		UINT32 n_stack;
		UINT32 n_ram;
		UINT32 n_left;
		UINT32 n_address;

		logerror( "psxexe_load: pc  %08x\n", m_psxexe_header.pc0 );
		logerror( "psxexe_load: org %08x\n", m_psxexe_header.t_addr );
		logerror( "psxexe_load: len %08x\n", m_psxexe_header.t_size );
		logerror( "psxexe_load: sp  %08x\n", m_psxexe_header.s_addr );
		logerror( "psxexe_load: len %08x\n", m_psxexe_header.s_size );

		p_ram = memory_region( REGION_CPU1 );
		n_ram = memory_region_length( REGION_CPU1 );

		p_psxexe = m_p_psxexe;

		n_address = m_psxexe_header.t_addr;
		n_left = m_psxexe_header.t_size;
		while( n_left != 0 )
		{
			p_ram[ BYTE4_XOR_LE( n_address ) % n_ram ] = *( p_psxexe );
			n_address++;
			p_psxexe++;
			n_left--;
		}

		free( m_p_psxexe );

		activecpu_set_reg( MIPS_PC, m_psxexe_header.pc0 );
		activecpu_set_reg( MIPS_R28, m_psxexe_header.gp0 );
		n_stack = m_psxexe_header.s_addr + m_psxexe_header.s_size;
		if( n_stack != 0 )
		{
			activecpu_set_reg( MIPS_R29, n_stack );
			activecpu_set_reg( MIPS_R30, n_stack );
		}

		memory_set_opbase_handler( 0, NULL );
		mips_stop();
	}
	return address;
}

static void psxexe_conv32( UINT32 *p_uint32 )
{
	UINT8 *p_uint8;

	p_uint8 = (UINT8 *)p_uint32;

	*( p_uint32 ) = p_uint8[ 0 ] |
		( p_uint8[ 1 ] << 8 ) |
		( p_uint8[ 2 ] << 16 ) |
		( p_uint8[ 3 ] << 24 );
}

static QUICKLOAD_LOAD( psxexe_load )
{
	if( mame_fread( fp, &m_psxexe_header, sizeof( m_psxexe_header ) ) != sizeof( m_psxexe_header ) )
	{
		logerror( "psxexe_load: invalid exe\n" );
		return INIT_FAIL;
	}
	if( memcmp( m_psxexe_header.id, "PS-X EXE", 8 ) != 0 )
	{
		logerror( "psxexe_load: invalid header id\n" );
		return INIT_FAIL;
	}

	psxexe_conv32( &m_psxexe_header.text );
	psxexe_conv32( &m_psxexe_header.data );
	psxexe_conv32( &m_psxexe_header.pc0 );
	psxexe_conv32( &m_psxexe_header.gp0 );
	psxexe_conv32( &m_psxexe_header.t_addr );
	psxexe_conv32( &m_psxexe_header.t_size );
	psxexe_conv32( &m_psxexe_header.d_addr );
	psxexe_conv32( &m_psxexe_header.d_size );
	psxexe_conv32( &m_psxexe_header.b_addr );
	psxexe_conv32( &m_psxexe_header.b_size );
	psxexe_conv32( &m_psxexe_header.s_addr );
	psxexe_conv32( &m_psxexe_header.s_size );
	psxexe_conv32( &m_psxexe_header.SavedSP );
	psxexe_conv32( &m_psxexe_header.SavedFP );
	psxexe_conv32( &m_psxexe_header.SavedGP );
	psxexe_conv32( &m_psxexe_header.SavedRA );
	psxexe_conv32( &m_psxexe_header.SavedS0 );

	m_p_psxexe = malloc( m_psxexe_header.t_size );
	if( m_p_psxexe == NULL )
	{
		logerror( "psxexe_load: out of memory\n" );
		return INIT_FAIL;
	}
	if( mame_fread( fp, m_p_psxexe, m_psxexe_header.t_size ) != m_psxexe_header.t_size )
	{
		logerror( "psxexe_load: invalid size\n" );
		return INIT_FAIL;
	}
	memory_set_opbase_handler( 0, psx_setopbase );
	return INIT_PASS;
}

static MACHINE_INIT( psx )
{
	psx_machine_init();
}

static DRIVER_INIT( psx )
{
	psx_driver_init();
}

static MEMORY_WRITE32_START( psx_writemem )
	{ 0x00000000, 0x001fffff, MWA32_RAM },	/* ram */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_w },
	{ 0x1f801070, 0x1f801077, psx_irq_w },
	{ 0x1f801080, 0x1f8010ff, psx_dma_w },
	{ 0x1f801800, 0x1f801803, psx_cd_w },
	{ 0x1f801810, 0x1f801817, psx_gpu_w },
	{ 0x80000000, 0x801fffff, MWA32_BANK2 },	/* ram mirror */
	{ 0x80600000, 0x807fffff, MWA32_BANK3 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MWA32_BANK4 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM },	/* bios */
MEMORY_END

static MEMORY_READ32_START( psx_readmem )
	{ 0x00000000, 0x001fffff, MRA32_RAM },		/* ram */
	{ 0x1f800000, 0x1f8003ff, MRA32_BANK1 },	/* scratchpad */
	{ 0x1f801040, 0x1f80104f, psx_serial_r },
	{ 0x1f801070, 0x1f801077, psx_irq_r },
	{ 0x1f801080, 0x1f8010ff, psx_dma_r },
	{ 0x1f801800, 0x1f801803, psx_cd_r },
	{ 0x1f801810, 0x1f801817, psx_gpu_r },
	{ 0x80000000, 0x801fffff, MRA32_BANK2 },	/* ram mirror */
	{ 0x80600000, 0x807fffff, MRA32_BANK3 },	/* ram mirror */
	{ 0xa0000000, 0xa01fffff, MRA32_BANK4 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA32_BANK5 },	/* bios */
MEMORY_END

static DRIVER_INIT( psx_mess )
{
	init_psx();
	cpu_setbank( 1, memory_region( REGION_USER1 ) );
	cpu_setbank( 2, memory_region( REGION_CPU1 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_CPU1 ) );
	cpu_setbank( 5, memory_region( REGION_USER2 ) );
}

INPUT_PORTS_START( psx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START   | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START   | IPF_PLAYER2 )
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
	MDRV_CPU_VBLANK_INT(psx_vblank, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT(psx)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_PALETTE_INIT(psx)
	MDRV_VIDEO_START(psx_type2_1024x512)
	MDRV_VIDEO_UPDATE(psx)
	MDRV_VIDEO_STOP(psx)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

ROM_START( psx )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph1000.bin",  0x0000000, 0x080000, CRC(3b601fc8) SHA1(343883a7b555646da8cee54aadd2795b6e7dd070) )
ROM_END

ROM_START( psxj22 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph5000.bin",  0x0000000, 0x080000, CRC(24fc7e17) SHA1(ffa7f9a7fb19d773a0c3985a541c8e5623d2c30d) ) /* bad 0x8c93a399 */
ROM_END

ROM_START( psxa22 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph1001.bin",  0x0000000, 0x080000, CRC(37157331) SHA1(10155d8d6e6e832d6ea66db9bc098321fb5e8ebf) )
ROM_END

ROM_START( psxe22 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "dtlh3002.bin",  0x0000000, 0x080000, CRC(1e26792f) SHA1(b6a11579caef3875504fcf3831b8e3922746df2c) )
ROM_END

ROM_START( psxj30 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph5500.bin",  0x0000000, 0x080000, CRC(ff3eeb8c) SHA1(b05def971d8ec59f346f2d9ac21fb742e3eb6917) )
ROM_END

ROM_START( psxa30 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph7003.bin",  0x0000000, 0x080000, CRC(8d8cb7e4) SHA1(0555c6fae8906f3f09baf5988f00e55f88e9f30b) )
ROM_END

ROM_START( psxe30 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph5502.bin",  0x0000000, 0x080000, CRC(4d9e7c86) SHA1(f8de9325fc36fcfa4b29124d291c9251094f2e54) )
ROM_END

ROM_START( psxj40 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph7000.bin",  0x0000000, 0x080000, CRC(ec541cd0) SHA1(77b10118d21ac7ffa9b35f9c4fd814da240eb3e9) )
ROM_END

ROM_START( psxa41 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph7001.bin",  0x0000000, 0x080000, CRC(502224b6) SHA1(14df4f6c1e367ce097c11deae21566b4fe5647a9) )
ROM_END

ROM_START( psxe41 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_REGION( 0x000400, REGION_USER1, 0 )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 )
	ROM_LOAD( "scph7502.bin",  0x0000000, 0x080000, CRC(318178bf) SHA1(8d5de56a79954f29e9006929ba3fed9b6a418c1d) )
ROM_END

SYSTEM_CONFIG_START( psx )
	CONFIG_DEVICE_QUICKLOAD( "exe\0", psxexe_load )
SYSTEM_CONFIG_END

/*
The version number & release date is stored in ascii text at the end of every bios, except for scph1000.
There is also a BCD encoded date at offset 0x100, but this is set to 041211995 in every version apart
from scph1000 & scph7000 ( where it is 22091994 & 29051997 respectively ).
*/

/*		YEAR	NAME	PARENT	COMPAT	MACHINE INPUT	INIT		CONFIG  COMPANY 	FULLNAME */
CONSX( 1994,	psx,	0,		0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph1000)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxj22,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph5000 J v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxa22,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph1001/dtlh3000 A v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1995,	psxe22,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph1002/dtlh3002 E v2.2 12/04/95)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1996,	psxj30,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph5500 J v3.0 09/09/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1996,	psxa30,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph7003 A v3.0 11/18/96)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxe30,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph5502 E v3.0 01/06/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxj40,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph7000 J v4.0 08/18/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxa41,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph7001 A v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
CONSX( 1997,	psxe41,	psx,	0,		psx,	psx,	psx_mess,	psx,	"Sony",		"Sony PSX (scph7502 E v4.1 12/16/97)", GAME_NO_SOUND | GAME_NOT_WORKING )
