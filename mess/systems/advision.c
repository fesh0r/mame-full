/***************************************************************************

  /systems/advision.c

  Driver file to handle emulation of the Entex Adventurevision.

  by Daniel Boris (dboris@home.com)  1/20/2000

***************************************************************************/

/**********************************************
8048 Ports:
P1 	Bit 0..1  - RAM bank select
	Bit 3..7  - Keypad input:

P2 	Bit 0..3  - A8-A11
	Bit 4..7  - Sound control

T1	Mirror sync pulse

***********************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "vidhrdw/generic.h"
#include "includes/advision.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START(advision_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03FF) AM_READWRITE(MRA8_BANK1, MWA8_ROM)
	AM_RANGE(0x0400, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM	/* MAINRAM four banks */
ADDRESS_MAP_END


static ADDRESS_MAP_START(advision_ports, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00,     0xff)		AM_READWRITE(advision_MAINRAM_r, advision_MAINRAM_w)
	AM_RANGE(I8039_p1, I8039_p1)	AM_READWRITE(advision_getp1, advision_putp1)
	AM_RANGE(I8039_p2, I8039_p2)	AM_READWRITE(advision_getp2, advision_putp2)
	AM_RANGE(I8039_t0, I8039_t0)	AM_READ(advision_gett0)
	AM_RANGE(I8039_t1, I8039_t1)	AM_READ(advision_gett1)
ADDRESS_MAP_END


INPUT_PORTS_START( advision )
	PORT_START      /* IN0 */
    PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_PLAYER(1)
    PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_PLAYER(1)
    PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_PLAYER(1)
    PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
    PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
    PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1)
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
    PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
INPUT_PORTS_END

static MACHINE_DRIVER_START( advision )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", I8048, 14000000/15)
	MDRV_CPU_PROGRAM_MAP(advision_mem, 0)
	MDRV_CPU_IO_MAP(advision_ports, 0)

	MDRV_FRAMES_PER_SECOND(8*15)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( advision )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_VISIBLE_AREA(0,320-1,0,200-1)
	MDRV_PALETTE_LENGTH(8+2)
	MDRV_COLORTABLE_LENGTH(8*2)
	MDRV_PALETTE_INIT(advision)

	MDRV_VIDEO_START(advision)
	MDRV_VIDEO_UPDATE(advision)
MACHINE_DRIVER_END

ROM_START (advision)
	ROM_REGION(0x2800,REGION_CPU1, 0)
    ROM_LOAD ("avbios.rom", 0x1000, 0x400, CRC(279e33d1) SHA1(bf7b0663e9125c9bfb950232eab627d9dbda8460))
ROM_END

static void advision_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "bin\0";
	dev->must_be_loaded = 1;
	dev->load = device_load_advision_cart;
}

SYSTEM_CONFIG_START(advision)
	CONFIG_DEVICE(advision_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG		COMPANY   FULLNAME */
CONSX(1982, advision,	0,		0,		advision, advision,	0,		advision,	"Entex",  "Adventurevision", GAME_NO_SOUND )

