/*****************************************************************

GCE Vectrex

Mathis Rosenhauer
Christopher Salomon (technical advice)
Bruce Tomlin (hardware info)

*****************************************************************/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/vector.h"
#include "machine/6522via.h"
#include "includes/vectrex.h"
#include "devices/cartslot.h"
#include "sound/ay8910.h"

ADDRESS_MAP_START( vectrex_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x7fff) AM_READ( MRA8_ROM )
	AM_RANGE( 0xc800, 0xcbff) AM_READ( MRA8_RAM )
	AM_RANGE( 0xcc00, 0xcfff) AM_READ( vectrex_mirrorram_r )
	AM_RANGE( 0xd000, 0xd7ff) AM_READ( via_0_r )    /* VIA 6522 */
	AM_RANGE( 0xe000, 0xffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

ADDRESS_MAP_START( vectrex_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0xc800, 0xcbff) AM_WRITE( MWA8_RAM) AM_BASE( &vectrex_ram )
	AM_RANGE( 0xcc00, 0xcfff) AM_WRITE( vectrex_mirrorram_w )
	AM_RANGE( 0xd000, 0xd7ff) AM_WRITE( via_0_w )    /* VIA 6522 */
	AM_RANGE( 0xe000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

INPUT_PORTS_START( vectrex )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2)

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(30)

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_REVERSE

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_PLAYER(2)

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_REVERSE PORT_PLAYER(2)

	PORT_START /* IN5 */
	//PORT_DIPNAME( 0x01, 0x00, "3D Imager", CODE_NONE )
	PORT_DIPNAME( 0x01, 0x00, "3D Imager")
	PORT_DIPSETTING(0x00, DEF_STR ( Off ))
	PORT_DIPSETTING(0x01, DEF_STR ( On ))
	//PORT_DIPNAME( 0x02, 0x00, "Separate images", CODE_NONE )
	PORT_DIPNAME( 0x02, 0x00, "Separate images")
	PORT_DIPSETTING(0x00, DEF_STR ( No ))
	PORT_DIPSETTING(0x02, DEF_STR ( Yes ))
	//PORT_DIPNAME( 0x1c, 0x10, "Left eye", CODE_NONE )
	PORT_DIPNAME( 0x1c, 0x10, "Left eye")
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x04, "Red")
	PORT_DIPSETTING(0x08, "Green")
	PORT_DIPSETTING(0x0c, "Blue")
	PORT_DIPSETTING(0x10, "Color")
	//PORT_DIPNAME( 0xe0, 0x80, "Right eye", CODE_NONE )
	PORT_DIPNAME( 0xe0, 0x80, "Right eye")
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x20, "Red")
	PORT_DIPSETTING(0x40, "Green")
	PORT_DIPSETTING(0x60, "Blue")
	PORT_DIPSETTING(0x80, "Color")

	PORT_START /* IN6 */
	PORT_DIPNAME( 0x03, 0x00, "Lightpen")
	PORT_DIPSETTING(0x00, DEF_STR ( Off ))
	PORT_DIPSETTING(0x01, "left port")
	PORT_DIPSETTING(0x02, "right port")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_CODE(MOUSECODE_1_BUTTON1)

    PORT_START /* Lightpen - X AXIS */
    PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(35) PORT_KEYDELTA(1) PORT_PLAYER(1)

    PORT_START /* Lightpen - Y AXIS */
    PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(35) PORT_KEYDELTA(1) PORT_PLAYER(1)


INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	0,
	vectrex_psg_port_w,
	0
};


static MACHINE_DRIVER_START( vectrex )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, 1500000)        /* 1.5 Mhz */
	MDRV_CPU_PROGRAM_MAP(vectrex_readmem, vectrex_writemem)

	MDRV_FRAMES_PER_SECOND(60)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(300, 400)
	MDRV_VISIBLE_AREA(0, 500, 0, 600)
	MDRV_PALETTE_LENGTH(256 + 32768)

	MDRV_VIDEO_START( vectrex )	
	MDRV_VIDEO_UPDATE( vectrex )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)	
MACHINE_DRIVER_END

static void vectrex_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "bin\0gam\0vec\0";
	dev->load = device_load_vectrex_cart;
}

SYSTEM_CONFIG_START(vectrex)
	CONFIG_DEVICE(vectrex_cartslot_getinfo)
SYSTEM_CONFIG_END

ROM_START(vectrex)
    ROM_REGION(0x10000,REGION_CPU1, 0)
    ROM_LOAD("system.img", 0xe000, 0x2000, CRC(ba13fb57))
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

ADDRESS_MAP_START( raaspec_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x7fff) AM_READ( MRA8_ROM )
	AM_RANGE( 0x8000, 0x87ff) AM_READ( MRA8_RAM ) /* Battery backed RAM for the Spectrum I+ */
	AM_RANGE( 0xc800, 0xcbff) AM_READ( MRA8_RAM )
	AM_RANGE( 0xcc00, 0xcfff) AM_READ( vectrex_mirrorram_r )
	AM_RANGE( 0xd000, 0xd7ff) AM_READ( via_0_r )
	AM_RANGE( 0xe000, 0xffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

ADDRESS_MAP_START( raaspec_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0x8000, 0x87ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0xa000, 0xa000) AM_WRITE( raaspec_led_w )
	AM_RANGE( 0xc800, 0xcbff) AM_WRITE( MWA8_RAM) AM_BASE( &vectrex_ram )
	AM_RANGE( 0xcc00, 0xcfff) AM_WRITE( vectrex_mirrorram_w )
	AM_RANGE( 0xd000, 0xd7ff) AM_WRITE( via_0_w )
	AM_RANGE( 0xe000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

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
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SELECT )

INPUT_PORTS_END


static MACHINE_DRIVER_START( raaspec )
	MDRV_IMPORT_FROM( vectrex )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( raaspec_readmem, raaspec_writemem )

	MDRV_PALETTE_LENGTH(254)

	MDRV_VIDEO_START( raaspec )
MACHINE_DRIVER_END

ROM_START(raaspec)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("spectrum.bin", 0x0000, 0x8000, CRC(20af7f3f))
	ROM_LOAD("system.img", 0xe000, 0x2000, CRC(ba13fb57))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT		CONFIG		COMPANY	FULLNAME */
CONS( 1982, vectrex,  0, 		0,		vectrex,  vectrex,	vectrex,	vectrex,	"General Consumer Electronics",   "Vectrex" )
CONS( 1984, raaspec,  vectrex,	0,		raaspec,  raaspec,	0,			NULL,		"Roy Abel & Associates",   "Spectrum I+" )
