/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amiga.h"

static ADDRESS_MAP_START(readmem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x000000, 0x07ffff) AM_READ( MRA16_RAM )            /* Chip Ram - 1Mb / 512k */
    AM_RANGE( 0xbfd000, 0xbfefff) AM_READ( amiga_cia_r )        /* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MRA8_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    AM_RANGE( 0xdbf000, 0xdfffff) AM_READ( amiga_custom_r )     /* Custom Chips */
    AM_RANGE( 0xf00000, 0xffffff) AM_READ( MRA16_BANK2 )          /* System ROM - mirror */
ADDRESS_MAP_END

static ADDRESS_MAP_START(writemem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x000000, 0x07ffff) AM_WRITE( MWA16_RAM )            /* Chip Ram - 1Mb / 512k */
    AM_RANGE( 0xbfd000, 0xbfefff) AM_WRITE( amiga_cia_w )        /* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MWA16_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    AM_RANGE( 0xdbf000, 0xdfffff) AM_WRITE( amiga_custom_w )     /* Custom Chips */
    AM_RANGE( 0xf00000, 0xffffff) AM_WRITE( MWA16_ROM )            /* System ROM */
ADDRESS_MAP_END

/**************************************************************************
***************************************************************************/

INPUT_PORTS_START( amiga )
    PORT_START /* joystick/mouse buttons */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_DIPNAME( 0x20, 0x00, "Input Port 0 Device")
    PORT_DIPSETTING( 0x00, "Mouse" )
    PORT_DIPSETTING( 0x20, "Joystick" )
    PORT_DIPNAME( 0x10, 0x10, "Input Port 1 Device")
    PORT_DIPSETTING( 0x00, "Mouse" )
    PORT_DIPSETTING( 0x10, "Joystick" )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )  /* Unused */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )

    PORT_START
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) /* Joystick - Port 1 */
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL ) /* Joystick - Port 2 */
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )

    PORT_START /* Mouse port 0 - X AXIS */
    PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

    PORT_START /* Mouse port 0 - Y AXIS */
    PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

    PORT_START /* Mouse port 1 - X AXIS */
    PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

    PORT_START /* Mouse port 1 - Y AXIS */
    PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )
INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { -1 } /* end of array */
};


static MACHINE_DRIVER_START( ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD( M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
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
    ROM_REGION(0x200000,REGION_CPU1,ROMREGION_16BIT) /* for ram, etc */
    ROM_LOAD ( "kick13.rom",  0x180000, 0x80000, CRC(f6290043))
ROM_END

ROM_START( cdtv )
    ROM_REGION(0x200000,REGION_CPU1,ROMREGION_16BIT) /* for ram, etc */
    ROM_LOAD ( "cdtv13.rom",  0x180000, 0x80000, CRC(42BAA124))
ROM_END

SYSTEM_CONFIG_START(amiga)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 4, "adf\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, device_init_amiga_fdc, NULL, device_load_amiga_fdc, NULL, NULL)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY	FULLNAME */
COMPX( 1984, amiga,    0,		0,		ntsc,     amiga,    0,		amiga,	"Commodore Business Machines Co.",  "Amiga 500 (NTSC)", GAME_NOT_WORKING )
COMPX( 1990, cdtv,     0,       0,		ntsc,     amiga,    0,		amiga,	"Commodore Business Machines Co.",  "Amiga CDTV (NTSC)", GAME_NOT_WORKING )
