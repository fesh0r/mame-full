/***************************************************************************

    Robotron A7150

    04/10/2009 Skeleton driver.

    http://www.robotrontechnik.de/index.htm?/html/computer/a7150.htm

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"


class a7150_state : public driver_device
{
public:
	a7150_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};


static ADDRESS_MAP_START(a7150_mem, AS_PROGRAM, 16, a7150_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xeffff) AM_RAM
	AM_RANGE(0xf8000,0xfffff) AM_ROM AM_REGION("user1", 0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a7150 )
INPUT_PORTS_END


static MACHINE_RESET(a7150)
{
}

static VIDEO_START( a7150 )
{
}

static SCREEN_UPDATE_IND16( a7150 )
{
	return 0;
}

static MACHINE_CONFIG_START( a7150, a7150_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 4915000)
	MCFG_CPU_PROGRAM_MAP(a7150_mem)

	MCFG_MACHINE_RESET(a7150)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE_STATIC(a7150)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(a7150)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( a7150 )
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "a7150.rom", 0x0000, 0x8000, CRC(57855abd) SHA1(b58f1363623d2c3ff1221e449529ecaa22573bff))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, a7150,  0,      0,       a7150,     a7150, driver_device,    0,     "VEB Robotron",   "A7150", GAME_NOT_WORKING | GAME_NO_SOUND)
