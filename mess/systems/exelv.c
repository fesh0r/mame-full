/*
	Experimental exelvision driver

	Raphael Nabet, 2004

TODO :
	* everything
*/

#include "driver.h"
#include "cpu/tms7000/tms7000.h"
#include "vidhrdw/tms3556.h"
/*#include "devices/cartslot.h"
#include "devices/cassette.h"*/


/*
	video initialization
*/
static int video_start_exelv(void)
{
	return tms3556_init(/*0x8000*/0x10000);	/* tms3556 with 32 kb of video RAM */
}

static void machine_init_exelv(void)
{
	tms3556_reset();
}

static void exelv_hblank_interrupt(void)
{
	tms3556_interrupt();
}

/*static DEVICE_LOAD(exelv_cart)
{
	return INIT_PASS;
}

static DEVICE_UNLOAD(exelv_cart)
{
}*/

/*
	Memory map summary:

	@>0000-@>007f: tms7020 internal RAM
	@>0080-@>00ff: reserved
	@>0100-@>010b: tms7020 internal I/O ports
	@>010c-@>01ff: external I/O ports?
		@>012e (P46): tms3556 VRAM write port???
	@>0200-@>7fff: system ROM? (two pages?) + cartridge ROMs? (one or two pages?)
	@>8000-@>bfff: free for expansion?
	@>c000-@>c7ff: CPU RAM?
	@>c800-@>f7ff: free for expansion?
	@>f800-@>ffff: tms7020 internal ROM
*/

static ADDRESS_MAP_START(exelv_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x007f) AM_READWRITE(tms7000_internal_r, tms7000_internal_w)/* tms7020 internal RAM */
	AM_RANGE(0x0080, 0x00ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)	/* reserved */
	AM_RANGE(0x0000, 0x010b) AM_READWRITE(tms70x0_pf_r, tms70x0_pf_w)/* tms7020 internal I/O ports */
	AM_RANGE(0x010c, 0x01ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)	/* external I/O ports */
	AM_RANGE(0x0200, 0x7fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)	/* system ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(MRA8_RAM, MWA8_RAM)	/* CPU RAM */
	AM_RANGE(0xc800, 0xf7ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(MRA8_ROM, MWA8_ROM)	/* tms7020 internal ROM */

ADDRESS_MAP_END


/* keyboard: ??? */
static INPUT_PORTS_START(exelv)

INPUT_PORTS_END


static MACHINE_DRIVER_START(exelv)

	/* basic machine hardware */
	/* TMS7020 CPU @ 4.91(?) MHz */
	MDRV_CPU_ADD(TMS7000, 4910000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(0)*/
	MDRV_CPU_PROGRAM_MAP(exelv_memmap, 0)
	/*MDRV_CPU_IO_MAP(0, 0)*/
	MDRV_CPU_VBLANK_INT(exelv_hblank_interrupt, 363)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( exelv )
	/*MDRV_MACHINE_STOP( exelv )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_VIDEO_START(exelv)
	MDRV_TMS3556

	/* sound */
	MDRV_SOUND_ATTRIBUTES(0)
	/*MDRV_SOUND_ADD(TMS5220, tms5220interface)*/

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(exeltel)
	/*CPU memory space*/
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("exeltel14.bin", 0x0000, 0x8000, CRC(52a80dd4))      /* system ROM */
ROM_END

SYSTEM_CONFIG_START(exelv)

	/* cartridge port is not emulated */

SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT		COMPAT	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
/*COMP(	1984,	exl100,	0,			0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exl 100" )*/
COMP(	1986,	exeltel,0/*exl100*/,0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exeltel" )
