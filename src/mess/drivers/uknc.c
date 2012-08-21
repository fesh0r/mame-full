/***************************************************************************

        UKNC

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"


class uknc_state : public driver_device
{
public:
	uknc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};


static ADDRESS_MAP_START(uknc_mem, AS_PROGRAM, 16, uknc_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAM  // RAM
	AM_RANGE( 0x8000, 0xffff ) AM_ROM  // ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(uknc_sub_mem, AS_PROGRAM, 16, uknc_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAM  // RAM
	AM_RANGE( 0x8000, 0xffff ) AM_ROM  // ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( uknc )
INPUT_PORTS_END


static MACHINE_RESET(uknc)
{
}

static VIDEO_START( uknc )
{
}

static SCREEN_UPDATE_IND16( uknc )
{
	return 0;
}

static const struct t11_setup t11_data =
{
	0x36ff			/* initial mode word has DAL15,14,11,8 pulled low */
};


static MACHINE_CONFIG_START( uknc, uknc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", T11, 8000000)
	MCFG_CPU_CONFIG(t11_data)
	MCFG_CPU_PROGRAM_MAP(uknc_mem)

	MCFG_CPU_ADD("subcpu",  T11, 6000000)
	MCFG_CPU_CONFIG(t11_data)
	MCFG_CPU_PROGRAM_MAP(uknc_sub_mem)

	MCFG_MACHINE_RESET(uknc)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_VIDEO_START(uknc)
	MCFG_SCREEN_UPDATE_STATIC(uknc)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( uknc )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "uknc.rom", 0x8000, 0x8000, CRC(a1536994) SHA1(b3c7c678c41ffa9b37f654fbf20fef7d19e6407b))

	ROM_REGION( 0x10000, "subcpu", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1987, uknc,   0,      0,       uknc,      uknc, driver_device,    0,    "<unknown>",  "UKNC", GAME_NOT_WORKING | GAME_NO_SOUND)
