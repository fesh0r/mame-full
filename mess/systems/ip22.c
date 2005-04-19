/*********************************************************************\
*
*   SGI IP22 Indy workstation
*
*  Skeleton Driver
*
*  Todo: Everything
*
*  Memory map:
*
*  18000000 - 1effffff      RESERVED - Unused
*  1f000000 - 1f3fffff      GIO - GFX
*  1f400000 - 1f5fffff      GIO - EXP0
*  1f600000 - 1f9fffff      GIO - EXP1 - Unused
*  1fa00000 - 1fa02047      Memory Controller
*  1fb00000 - 1fb1a7ff      HPC3 CHIP1
*  1fb80000 - 1fb9a7ff      HPC3 CHIP0
*  1fc00000 - 1fc7ffff      BIOS
*
*  References used:
*    MipsLinux: http://www.mips-linux.org/
*      linux-2.6.6/include/newport.h
*      linux-2.6.6/include/asm-mips/sgi/gio.h
*      linux-2.6.6/include/asm-mips/sgi/mc.h
*      linux-2.6.6/include/asm-mips/sgi/hpc3.h
*
\*********************************************************************/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "machine/sgi.h"

#define VERBOSE_LEVEL ( 2 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static VIDEO_START( ip225015 )
{
	return 0;
}

static VIDEO_UPDATE( ip225015 )
{
}

static INTERRUPT_GEN( ip22_update_chips )
{
	mc_update();
}

static ADDRESS_MAP_START( ip225015_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x0000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_SHARE(2) AM_REGION( REGION_USER1, 0 )
	AM_RANGE( 0x80000000, 0x8000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xa0000000, 0xa000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xbfa00000, 0xbfa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0xbfc00000, 0xbfc7ffff ) AM_ROM AM_SHARE(2) /* BIOS Mirror */
ADDRESS_MAP_END

static MACHINE_INIT( ip225015 )
{
	mc_init();
}

static DRIVER_INIT( ip225015 )
{
}

INPUT_PORTS_START( ip225015 )
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static struct mips3_config config =
{
	32768,	/* code cache size */
	32768	/* data cache size */
};

MACHINE_DRIVER_START( ip225015 )
	MDRV_CPU_ADD_TAG( "main", R5000BE, 50000000*3 )
	MDRV_CPU_CONFIG( config )
	MDRV_CPU_PROGRAM_MAP( ip225015_map, 0 )
	MDRV_CPU_VBLANK_INT( ip22_update_chips, 10000 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( ip225015 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(800, 600)
	MDRV_VISIBLE_AREA(0, 799, 0, 599)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START( ip225015 )
	MDRV_VIDEO_UPDATE( ip225015 )
MACHINE_DRIVER_END

ROM_START( ip225015 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip225015.bin", 0x000000, 0x080000, CRC(aee5502e) SHA1(9243fef0a3508790651e0d6d2705c887629b1280) )
ROM_END

SYSTEM_CONFIG_START( ip225015 )
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMPX( 1993, ip225015, 0,        0,        ip225015, ip225015, ip225015, ip225015, "Silicon Graphics, Inc", "Indy (R5000, 150MHz)", GAME_NOT_WORKING | GAME_NO_SOUND )
