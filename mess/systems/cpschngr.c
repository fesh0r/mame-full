/***************************************************************************

  Capcom System 1
  ===============

  Driver provided by:
  Paul Leaman (paul@vortexcomputing.demon.co.uk)

  M680000 for game, Z80, YM-2151 and OKIM6295 for sound.

  68000 clock speeds are unknown for all games (except where commented)

merged Street Fighter Zero for MESS

***************************************************************************/

#include "driver.h"

#ifdef GAME
#undef GAME
#endif

#ifdef GAMEX
#undef GAMEX
#endif

#define GAME(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME)
#define GAMEX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)

#include "drivers/cps1.c"

/* this is to keep GCC from whining about unused statics */
int dummy_function(void)
{
	return ((int) construct_forgottn) | ((int) construct_sf2) | ((int) construct_qsound);
}

void wof_decode(void)      { }
void dino_decode(void)     { }
void punisher_decode(void) { }
void slammast_decode(void) { }

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define CODE_SIZE 0x200000

INPUT_PORTS_START( sfzch )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Pause", KEYCODE_F1, IP_JOY_NONE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE  )	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

ROM_START( sfzch )
	ROM_REGION( CODE_SIZE, REGION_CPU1,0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzch23",        0x000000, 0x80000,CRC( 0x1140743f ))
	ROM_LOAD16_WORD_SWAP( "sfza22",         0x080000, 0x80000,CRC( 0x8d9b2480 ))
	ROM_LOAD16_WORD_SWAP( "sfzch21",        0x100000, 0x80000,CRC( 0x5435225d ))
	ROM_LOAD16_WORD_SWAP( "sfza20",         0x180000, 0x80000,CRC( 0x806e8f38 ))

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROMX_LOAD( "sfz01",         0x000000, 0x80000, CRC(0x0dd53e62), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz02",         0x000002, 0x80000, CRC(0x94c31e3f), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz03",         0x000004, 0x80000, CRC(0x9584ac85), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz04",         0x000006, 0x80000, CRC(0xb983624c), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz05",         0x200000, 0x80000, CRC(0x2b47b645), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz06",         0x200002, 0x80000, CRC(0x74fd9fb1), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz07",         0x200004, 0x80000, CRC(0xbb2c734d), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz08",         0x200006, 0x80000, CRC(0x454f7868), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz10",         0x400000, 0x80000, CRC(0x2a7d675e), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz11",         0x400002, 0x80000, CRC(0xe35546c8), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz12",         0x400004, 0x80000, CRC(0xf122693a), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz13",         0x400006, 0x80000, CRC(0x7cf942c8), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz14",         0x600000, 0x80000, CRC(0x09038c81), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz15",         0x600002, 0x80000, CRC(0x1aa17391), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz16",         0x600004, 0x80000, CRC(0x19a5abd6), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz17",         0x600006, 0x80000, CRC(0x248b3b73), ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x28000, REGION_CPU2,0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz09",         0x00000, 0x08000,CRC( 0xc772628b ))
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1,0 )	/* Samples */
	ROM_LOAD( "sfz18",         0x00000, 0x20000,CRC( 0x61022b2d ))
	ROM_LOAD( "sfz19",         0x20000, 0x20000,CRC( 0x3b5886d5 ))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT		INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1995, sfzch,    0,        0,		cps1,	  sfzch,    cps1,	NULL,	"Capcom", "CPS Changer (Street Fighter ZERO)" )

