/*****************************************************************

GCE Vectrex

Mathis Rosenhauer
Christopher Salomon (technical advice)
Bruce Tomlin (hardware info)

*****************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "machine/6522via.h"
#include "includes/vectrex.h"
#include "devices/cartslot.h"

MEMORY_READ_START( vectrex_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc800, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, vectrex_mirrorram_r },
	{ 0xd000, 0xd7ff, via_0_r },    /* VIA 6522 */
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( vectrex_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc800, 0xcbff, MWA_RAM, &vectrex_ram },
	{ 0xcc00, 0xcfff, vectrex_mirrorram_w },
	{ 0xd000, 0xd7ff, via_0_w },    /* VIA 6522 */
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( vectrex )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH,  IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,  IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,  IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	//PORT_DIPNAME( 0x01, 0x00, "3D Imager", IP_KEY_NONE )
	PORT_DIPNAME( 0x01, 0x00, "3D Imager")
	PORT_DIPSETTING(0x00, DEF_STR ( Off ))
	PORT_DIPSETTING(0x01, DEF_STR ( On ))
	//PORT_DIPNAME( 0x02, 0x00, "Separate images", IP_KEY_NONE )
	PORT_DIPNAME( 0x02, 0x00, "Separate images")
	PORT_DIPSETTING(0x00, DEF_STR ( No ))
	PORT_DIPSETTING(0x02, DEF_STR ( Yes ))
	//PORT_DIPNAME( 0x1c, 0x10, "Left eye", IP_KEY_NONE )
	PORT_DIPNAME( 0x1c, 0x10, "Left eye")
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x04, "Red")
	PORT_DIPSETTING(0x08, "Green")
	PORT_DIPSETTING(0x0c, "Blue")
	PORT_DIPSETTING(0x10, "Color")
	//PORT_DIPNAME( 0xe0, 0x80, "Right eye", IP_KEY_NONE )
	PORT_DIPNAME( 0xe0, 0x80, "Right eye")
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x20, "Red")
	PORT_DIPSETTING(0x40, "Green")
	PORT_DIPSETTING(0x60, "Blue")
	PORT_DIPSETTING(0x80, "Color")
INPUT_PORTS_END

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz */
	{ 20 },
    /*AY8910_DEFAULT_GAIN,*/
	{ input_port_0_r },
	{ 0 },
	{ vectrex_psg_port_w },
	{ 0 }
};


static MACHINE_DRIVER_START( vectrex )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, 1500000)        /* 1.5 Mhz */
	MDRV_CPU_MEMORY(vectrex_readmem, vectrex_writemem)

	MDRV_FRAMES_PER_SECOND(60)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(300, 400)
	MDRV_VISIBLE_AREA(0, 500, 0, 600)
	MDRV_PALETTE_LENGTH(256 + 32768)

	MDRV_VIDEO_START( vectrex )	
	MDRV_VIDEO_UPDATE( vectrex )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

SYSTEM_CONFIG_START(vectrex)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "bin\0gam\0vec\0", NULL, NULL, device_load_vectrex_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

ROM_START(vectrex)
    ROM_REGION(0x10000,REGION_CPU1, 0)
    ROM_LOAD("system.img", 0xe000, 0x2000,CRC( 0xba13fb57))
ROM_END


/*****************************************************************

  RA+A Spectrum I+

  The Spectrum I+ was a modified Vectrex. It had a 32K ROM cart
  and 2K additional battery backed RAM (0x8000 - 0x87ff). PB6
  was used to signal inserted coins to the VIA. The unit was
  controlled by 8 buttons (2x4 buttons of controller 1 and 2).
  Each button had a LED which were mapped to 0xa000.
  The srvice mode can be accessed by pressing button
  8 during startup. As soon as all LEDs light up,
  press 2 and 3 without releasing 8. Then release 8 and
  after that 2 and 3. You can leave the screen where you enter
  ads by pressing 8 several times.

*****************************************************************/

MEMORY_READ_START( raaspec_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM }, /* Battery backed RAM for the Spectrum I+ */
	{ 0xc800, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, vectrex_mirrorram_r },
	{ 0xd000, 0xd7ff, via_0_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( raaspec_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, raaspec_led_w },
	{ 0xc800, 0xcbff, MWA_RAM, &vectrex_ram },
	{ 0xcc00, 0xcfff, vectrex_mirrorram_w },
	{ 0xd000, 0xd7ff, via_0_w },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( raaspec )
	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON8 )

	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SELECT1 )

INPUT_PORTS_END


static MACHINE_DRIVER_START( raaspec )
	MDRV_IMPORT_FROM( vectrex )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( raaspec_readmem, raaspec_writemem )

	MDRV_PALETTE_LENGTH(254)

	MDRV_VIDEO_START( raaspec )
MACHINE_DRIVER_END

ROM_START(raaspec)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("spectrum.bin", 0x0000, 0x8000,CRC( 0x20af7f3f))
	ROM_LOAD("system.img", 0xe000, 0x2000,CRC( 0xba13fb57))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT	CONFIG		COMPANY	FULLNAME */
CONS( 1982, vectrex,  0, 		0,		vectrex,  vectrex,	0,		vectrex,	"General Consumer Electronics",   "Vectrex" )
CONS( 1984, raaspec,  vectrex,	0,		raaspec,  raaspec,	0,		NULL,		"Roy Abel & Associates",   "Spectrum I+" )
