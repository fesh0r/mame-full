/*
	Experimental exelvision driver

	Raphael Nabet, 2004

	Exelvision was a French company that designed and sold two computers:
	* EXL 100 (1984)
	* EXELTEL (1986), which is mostly compatible with EXL 100, but has an
	  integrated V23b modem and 5 built-in programs.
	These computer were mostly sold in France and in Europe (Spain); there was
	an Arabic version, too.

	Exelvision was founded by former TI employees, which is why their designs
	use TI components and have architectural reminiscences of the primitive
	TI-99/4 design (both computers are built around a microcontroller, have
	little CPU RAM and must therefore store program data in VRAM, and feature
	I/R keyboard and joysticks)

Specs:
	* main CPU is a variant of tms7020 (exl100) or tms7040 (exeltel).  AFAIK,
	  the only difference compared to a stock tms7020/7040 is the SWAP R0
	  instruction is replaced by a custom microcoded LVDP instruction that
	  reads a byte from the VDP VRAM read port; according to some reports, the
	  first 6 bytes of internal ROM (0xF000-0xF005 on an exeltel) are missing,
	  too.
	* in addition to the internal 128-byte RAM and 2kb (exl100) or 4kb
	  (exeltel) ROM, there are 2kb of CPU RAM and 64(?)kb (exeltel only?) of
	  CPU ROM.
	* I/O is controlled by a tms7041 (exl100) or tms7042 (exeltel) or a variant
	  thereof.  Communication with the main CPU is done through some custom
	  interface (I think), details are still to be worked out.
	* video: tms3556 VDP with 32kb of VRAM (expandable to 64kb), attached to
	  the main CPU.
	* sound: tms5220 speech synthesizer with speech ROM, attached to the I/O
	  CPU
	* keyboard and joystick: an I/R interface controlled by the I/O CPU enables
	  to use a keyboard and two joysticks
	* mass storage: tape interface controlled by the I/O CPU

STATUS:
	* EXL 100 cannot be emulated because the ROMs are not dumped
	* EXELTEL stops early in the boot process and displays a red error screen,
	  presumably because the I/O processor is not emulated

TODO:
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
	Main CPU memory map summary:

	@>0000-@>007f: tms7020/tms7040 internal RAM
	@>0080-@>00ff: reserved
	@>0100-@>010b: tms7020/tms7040 internal I/O ports
	@>010c-@>01ff: external I/O ports?
		@>012d (P45): tms3556 control write port???
		@>012e (P46): tms3556 VRAM write port???
	@>0200-@>7fff: system ROM? (two pages?) + cartridge ROMs? (one or two pages?)
	@>8000-@>bfff: free for expansion?
	@>c000-@>c7ff: CPU RAM?
	@>c800-@>efff: free for expansion?
	@>f000-@>f7ff: tms7040 internal ROM
	@>f800-@>ffff: tms7020/tms7040 internal ROM
*/

static ADDRESS_MAP_START(exelv_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x007f) AM_READWRITE(tms7000_internal_r, tms7000_internal_w)/* tms7020 internal RAM */
	AM_RANGE(0x0080, 0x00ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/* reserved */
	AM_RANGE(0x0000, 0x010b) AM_READWRITE(tms70x0_pf_r, tms70x0_pf_w)/* tms7020 internal I/O ports */
	//AM_RANGE(0x010c, 0x01ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)		/* external I/O ports */
	AM_RANGE(0x012d, 0x0012d) AM_READWRITE(MRA8_NOP, tms3556_reg_w)
	AM_RANGE(0x012e, 0x0012e) AM_READWRITE(MRA8_NOP, tms3556_vram_w)
	AM_RANGE(0x0200, 0x7fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)		/* system ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(MRA8_RAM, MWA8_RAM)		/* CPU RAM */
	AM_RANGE(0xc800, /*0xf7ff*/0xefff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(/*0xf800*/0xf000, 0xffff) AM_READWRITE(MRA8_ROM, MWA8_ROM)/* tms7020 internal ROM */

ADDRESS_MAP_END


/* keyboard: ??? */
static INPUT_PORTS_START(exelv)

	PORT_START

INPUT_PORTS_END


static MACHINE_DRIVER_START(exelv)

	/* basic machine hardware */
	/* TMS7020 CPU @ 4.91(?) MHz */
	MDRV_CPU_ADD(TMS7000_EXL, 4910000)
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
	ROM_LOAD("exeltelin.bin", 0xf006, 0x0ffa, CRC(c12f24b5))      /* internal ROM */
ROM_END

SYSTEM_CONFIG_START(exelv)

	/* cartridge port is not emulated */

SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT		COMPAT	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
/*COMP(	1984,	exl100,	0,			0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exl 100" )*/
COMP(	1986,	exeltel,0/*exl100*/,0,		exelv,		exelv,	NULL,	exelv,		"Exelvision",	"exeltel" )
