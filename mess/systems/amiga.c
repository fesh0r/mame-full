/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amiga.h"
#include "machine/amigafdc.h"
#include "inputx.h"

static ADDRESS_MAP_START(amiga_mem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x000000, 0x07ffff) AM_RAMBANK(3)										/* Chip Ram - 1Mb / 512k */
    AM_RANGE( 0xbfd000, 0xbfefff) AM_READWRITE( amiga_cia_r, amiga_cia_w )			/* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MRA8_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    AM_RANGE( 0xdbf000, 0xdfffff) AM_READWRITE( amiga_custom_r, amiga_custom_w )	/* Custom Chips */
    AM_RANGE( 0xf80000, 0xffffff) AM_ROMBANK(1)										/* System ROM - mirror */
ADDRESS_MAP_END

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( amiga )
    PORT_START /* joystick/mouse buttons */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_COCKTAIL
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_CONFNAME( 0x20, 0x00, "Input Port 0 Device")
    PORT_CONFSETTING( 0x00, "Mouse" )
    PORT_CONFSETTING( 0x20, DEF_STR( Joystick ) )
    PORT_CONFNAME( 0x10, 0x10, "Input Port 1 Device")
    PORT_CONFSETTING( 0x00, "Mouse" )
    PORT_CONFSETTING( 0x10, DEF_STR( Joystick ) )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_COCKTAIL
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )

    PORT_START
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) /* Joystick - Port 1 */
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_COCKTAIL /* Joystick - Port 2 */
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_COCKTAIL
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_COCKTAIL
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_COCKTAIL

    PORT_START /* Mouse port 0 - X AXIS */
    PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

    PORT_START /* Mouse port 0 - Y AXIS */
    PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(1)

    PORT_START /* Mouse port 1 - X AXIS */
    PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(2)

    PORT_START /* Mouse port 1 - Y AXIS */
    PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_MINMAX(0,0) PORT_PLAYER(2)
INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { -1 } /* end of array */
};


static MACHINE_DRIVER_START( ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD( M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(amiga_mem, 0)
	MDRV_CPU_VBLANK_INT(amiga_vblank_irq,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( amiga )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(456, 262)
	MDRV_VISIBLE_AREA(120, 456-1, 32, 262-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(4096)
	MDRV_PALETTE_INIT( amiga )

	MDRV_VIDEO_START( amiga )
	MDRV_VIDEO_UPDATE( amiga )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( amiga )
    ROM_REGION(0x200000, REGION_CPU1, 0) /* for ram, etc */

	ROM_REGION(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP ( "kick13.rom",  0x00000, 0x80000, CRC(f6290043) SHA1(90933936cce43ca9bc6bf375662c076b27e3c458))
ROM_END

ROM_START( cdtv )
    ROM_REGION(0x200000, REGION_CPU1, 0) /* for ram, etc */

	ROM_REGION(0x80000, REGION_USER1, 0)
    ROM_LOAD16_WORD_SWAP ( "cdtv13.rom",  0x00000, 0x80000, CRC(42BAA124))
ROM_END

SYSTEM_CONFIG_START(amiga)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END

static DRIVER_INIT( amiga )
{
	/* set up memory */
	cpu_setbank(1, memory_region(REGION_USER1));
	cpu_setbank(3, memory_region(REGION_USER1));
}

/*     YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY	FULLNAME */
COMPX( 1984, amiga,    0,		0,		ntsc,     amiga,    amiga,	amiga,	"Commodore Business Machines Co.",  "Amiga 500 (NTSC)", GAME_NOT_WORKING )
COMPX( 1990, cdtv,     0,       0,		ntsc,     amiga,    amiga,	amiga,	"Commodore Business Machines Co.",  "Amiga CDTV (NTSC)", GAME_NOT_WORKING )
