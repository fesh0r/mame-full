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
    AM_RANGE( 0x000000, 0x07ffff) AM_RAMBANK(3)			/* Chip Ram - 1Mb / 512k */
    AM_RANGE( 0xbfd000, 0xbfefff) AM_READWRITE( amiga_cia_r, amiga_cia_w )			/* 8510's CIA A and CIA B */
//  { 0xc00000, 0xd7ffff, MRA8_BANK1 },          /* Internal Expansion Ram - 1.5 Mb */
    AM_RANGE( 0xdbf000, 0xdfffff) AM_READWRITE( amiga_custom_r, amiga_custom_w )	/* Custom Chips */
    AM_RANGE( 0xf80000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)					/* System ROM - mirror */
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

static gfx_decode gfxdecodeinfo[] =
{
    { -1 } /* end of array */
};


static MACHINE_DRIVER_START( ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD( M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(amiga_mem, 0)
	MDRV_CPU_VBLANK_INT(amiga_irq, 262)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( amiga )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(456, 262)
	MDRV_VISIBLE_AREA(120, 456-1, 32, 262-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(4096)
	MDRV_COLORTABLE_LENGTH(4096)
	MDRV_PALETTE_INIT( amiga )

	MDRV_VIDEO_START( amiga )
	MDRV_VIDEO_UPDATE( generic_bitmapped )
MACHINE_DRIVER_END


/***************************************************************************

  Amiga specific stuff

***************************************************************************/

static int amiga_cia_0_portA_r( void )
{
	int ret = readinputport( 0 ) & 0xc0;
	ret |= amiga_fdc_status_r();
	return ret; /* Gameport 1 and 0 buttons */
}

static void amiga_cia_0_portA_w( int data ) 
{
	if ( (data & 1) == 1)
	{
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MRA16_BANK3 );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_ROM );
	}
	else if ( ((data & 1) == 0))
	{
		/* overlay disabled, map RAM on 0x000000 */
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MRA16_BANK4 );
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_BANK4 );
		memory_set_bankptr(4, memory_region(REGION_CPU1));
	}

	set_led_status( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led on Amiga*/
}

static UINT16 amiga_read_joy0dat(void)
{
	if ( readinputport( 0 ) & 0x20 ) {
		int input = ( readinputport( 1 ) >> 4 );
		int	top,bot,lft,rgt;

		top = ( input >> 3 ) & 1;
		bot = ( input >> 2 ) & 1;
		lft = ( input >> 1 ) & 1;
		rgt = input & 1;

		if ( lft ) top ^= 1;
		if ( rgt ) bot ^= 1;

		return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
	} else {
		int input = ( readinputport( 2 ) & 0xff );

		input |= ( readinputport( 3 ) & 0xff ) << 8;

		return input;
	}
}

static UINT16 amiga_read_joy1dat(void)
{
	if ( readinputport( 0 ) & 0x10 ) {
		int input = ( readinputport( 1 ) & 0x0f );
		int	top,bot,lft,rgt;

		top = ( input >> 3 ) & 1;
		bot = ( input >> 2 ) & 1;
		lft = ( input >> 1 ) & 1;
		rgt = input & 1;

		if ( lft ) top ^= 1;
		if ( rgt ) bot ^= 1;

		return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
	} else {
		int input = ( readinputport( 4 ) & 0xff );

		input |= ( readinputport( 5 ) & 0xff ) << 8;

		return input;
	}
}

static UINT16 amiga_read_dskbytr(void)
{
	return amiga_fdc_get_byte();
}

static void amiga_write_dsklen(UINT16 data)
{
	if ( data & 0x8000 ) {
		if ( CUSTOM_REG(REG_DSKLEN) & 0x8000 )
			amiga_fdc_setup_dma();
	}
}

static const struct amiga_machine_interface amiga_intf =
{
	amiga_cia_0_portA_r,	/* CIA0 port A read */
	NULL,					/* CIA0 port B read: parallel port? */
	amiga_cia_0_portA_w,	/* CIA0 port A write */
	NULL,					/* CIA0 port B write: parallel port? */
	NULL,					/* CIA1 port A read */
	NULL,					/* CIA1 port B read */
	NULL,					/* CIA1 port A write */
	amiga_fdc_control_w,	/* CIA1 port B write */
	amiga_read_joy0dat,		/* read_joy0dat */
	amiga_read_joy1dat,		/* read_joy1dat */
	amiga_read_dskbytr,		/* read_dskbytr */
	amiga_write_dsklen,		/* write_dsklen */
	NULL,					/* interrupt_callback */
	NULL					/* reset_callback */
};

static DRIVER_INIT( amiga )
{
	amiga_machine_config(&amiga_intf);

	/* set up memory */
	memory_set_bankptr(3, memory_region(REGION_USER1));
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( amiga )
    ROM_REGION(0x200000, REGION_CPU1, 0) /* for ram, etc */

	ROM_REGION(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP ( "kick13.rom",  0x00000, 0x40000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785))
	ROM_COPY( REGION_USER1, 0x00000, 0x40000, 0x40000 )
ROM_END

ROM_START( cdtv )
    ROM_REGION(0x200000, REGION_CPU1, 0) /* for ram, etc */

	ROM_REGION(0x80000, REGION_USER1, 0)
    ROM_LOAD16_WORD_SWAP ( "cdtv13.rom",  0x00000, 0x80000, CRC(42BAA124))
ROM_END

SYSTEM_CONFIG_START(amiga)
	CONFIG_DEVICE(amiga_floppy_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY	FULLNAME */
COMP( 1984, amiga,    0,		0,		ntsc,     amiga,    amiga,	amiga,	"Commodore Business Machines Co.",  "Amiga 500 (NTSC)", GAME_NOT_WORKING )
COMP( 1990, cdtv,     0,       0,		ntsc,     amiga,    amiga,	amiga,	"Commodore Business Machines Co.",  "Amiga CDTV (NTSC)", GAME_NOT_WORKING )
