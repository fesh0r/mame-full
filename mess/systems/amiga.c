/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/amiga.h"

static struct MemoryReadAddress readmem[] =
{
    { 0x000000, 0x07ffff, MRA_RAM },            /* Chip Ram - 1Mb / 512k */
    { 0xbfd000, 0xbfefff, amiga_cia_r },        /* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MRA_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    { 0xdbf000, 0xdfffff, amiga_custom_r },     /* Custom Chips */
    { 0xf00000, 0xffffff, MRA_BANK2 },          /* System ROM - mirror */
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x000000, 0x07ffff, MWA_RAM },            /* Chip Ram - 1Mb / 512k */
    { 0xbfd000, 0xbfefff, amiga_cia_w },        /* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MWA_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    { 0xdbf000, 0xdfffff, amiga_custom_w },     /* Custom Chips */
    { 0xf00000, 0xffffff, MWA_ROM },            /* System ROM */
    { -1 }  /* end of table */
};

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

static struct MachineDriver machine_driver_ntsc =
{
    /* basic machine hardware */
    {
        {
            CPU_M68000,
            7159090,            /* 7.15909 Mhz (NTSC) */
            readmem,writemem,0,0,
            amiga_vblank_irq,1,
        }
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,      /* frames per second, vblank duration */
    1,
    amiga_init_machine,
    0,

    /* video hardware */
    456, 262, /* screen width, screen height ( 312 for PAL ) */
    { 120, 456 - 1, 32, 262 - 1 },          /* visible_area */

    gfxdecodeinfo,                  /* graphics decode info */
    4096, 4096,                     /* number of colors, colortable size */
    amiga_init_palette,             /* convert color prom */

    VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
    0,
    amiga_vh_start,
    amiga_vh_stop,
    amiga_vh_screenrefresh,

    /* sound hardware */
    0,0,0,0
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( amiga )
    ROM_REGION(0x200000,REGION_CPU1,0) /* for ram, etc */
    ROM_LOAD_WIDE( "kick13.rom",  0x180000, 0x80000, 0xf6290043)
ROM_END

static const struct IODevice io_amiga[] = {
    {
        IO_FLOPPY,          /* type */
        4,                  /* count */
        "adf\0",            /* file extensions */
        IO_RESET_NONE,      /* reset if file changed */
        NULL,               /* id */
        amiga_fdc_init,     /* init */
        NULL,               /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 1984, amiga,    0,        ntsc,     amiga,    0,        "Commodore Business Machines Co.",  "Amiga 500 (NTSC)", GAME_NOT_WORKING | GAME_REQUIRES_16BIT )

ROM_START( cdtv )
    ROM_REGION(0x200000,REGION_CPU1,0) /* for ram, etc */
    ROM_LOAD_WIDE( "cdtv13.rom",  0x180000, 0x80000, 0x42BAA124)
ROM_END

static const struct IODevice io_cdtv[] = {
    {
        IO_FLOPPY,          /* type */
        4,                  /* count */
        "adf\0",            /* file extensions */
        IO_RESET_NONE,      /* reset if file changed */
        NULL,               /* id */
        amiga_fdc_init,     /* init */
        NULL,               /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 1990, cdtv,     0,        ntsc,     amiga,    0,        "Commodore Business Machines Co.",  "Amiga CDTV (NTSC)", GAME_NOT_WORKING | GAME_REQUIRES_16BIT )
