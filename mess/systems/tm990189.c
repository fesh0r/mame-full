/*
	Experimental tm990/189 ("University Module") driver.

	The tm990/189 is a simple board built around a tms9980 at 2.0 MHz.
	The board features:
	* a calculator-like alphanumeric keyboard, a 10-digit 7 segment display,
	  a sound buzzer and 4 status LEDs (appearently interfaced through a
	  tms9901)
	* a 4kb ROM socket (0x3000-0x3fff), and a 2kb ROM socket (0x0800-0x0fff)
	* 1kb of RAM expandable to 2kb (0x0000-0x03ff and 0x0400-0x07ff)
	* a tms9901 controlling a custom parallel I/O port (available for
	  expansion)
	* an optional on-board serial interface (either TTY or RS232): TI ROMs
	  support a terminal connected to this port
	* an optional tape interface
	* an optional bus extension port for adding additionnal custom devices (TI
	  sold a video controller expansion built around a tms9918, which was
	  supported by University Basic)

	At least one tms9901 is set up so that it can trigger interrupts on the
	tms9980.

	TI sold two ROM sets for this machine: a monitor and assembler ("UNIBUG",
	packaged as one 4kb EPROM) and a Basic interpreter ("University BASIC",
	packaged as a 4kb and a 2kb EPROM).  Users could burn and install custom
	ROM sets, too.

	The board was sold to university to learn microprocessor system design.
	A few hobbyists may have bought one of these, too.  Such a board would
	probably have been much more insteresting if TI had managed to design a
	working 9900-based microcontroller design.

	Work on this system is paused because only the 4kb University BASIC ROM has
	been dumped.

	Raphael Nabet 2003
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/ti990.h"


static void machine_init_tm990_189(void)
{
	ti990_hold_load();

	ti990_reset_int();
}


/*
	tm990_189 video emulation - not implemented yet.

	Has an integrated 10 digit 7-segment display.
	Supports EIA and TTY termnals.
*/

static int video_start_tm990_189(void)
{
	return 0;
}

static VIDEO_UPDATE( tm990_189 )
{
	/* ... */
}


/*
	Memory map:

	0x0000-0x03ff: 1kb RAM (or ROM???)
	0x0400-0x07ff: 1kb onboard expansion RAM or ROM
	0x0800-0x0bff: 1kb onboard expansion RAM or ROM
	0x0c00-0x0fff: 1kb onboard expansion RAM or ROM
	0x1000-0x2fff: for offboard expansion
	0x3000-0x3fff: 4kb onboard ROM

*/

static MEMORY_READ_START (tm990_189_readmem)

	{ 0x0000, 0x07ff, MRA_RAM },		/* RAM */
	{ 0x0800, 0x0fff, MRA_ROM },		/* extra ROM - not dumped */
	{ 0x1000, 0x2fff, MRA_NOP },		/* reserved for expansion (RAM and/or tms9918 video controller) */
	{ 0x3000, 0x3fff, MRA_ROM },		/* main ROM - unibug or university basic */

MEMORY_END

static MEMORY_WRITE_START (tm990_189_writemem)

	{ 0x0000, 0x07ff, MWA_RAM },		/* RAM */
	{ 0x0800, 0x0fff, MWA_ROM },		/* extra ROM - not dumped */
	{ 0x1000, 0x2fff, MWA_NOP },		/* reserved for expansion */
	{ 0x3000, 0x3fff, MWA_ROM },		/* main ROM - unibug or university basic */

MEMORY_END


/*
	CRU map

	currently unknown

	The board seems to feature one tms9901(?) for keyboard and sound I/O,
	another tms9901 for expansion, and one optional tms9902 for serial I/O.

	CRU bits 17, 18 and 19: LEDs numbered 2, 3 and 4 (probably )
*/

static PORT_WRITE_START ( tm990_189_writeport )


PORT_END

static PORT_READ_START ( tm990_189_readport )


PORT_END

/*static struct beep_interface tm990_189_beep_interface =
{
	1,
	{ 50 }
};*/

/*static tms9900reset_param reset_params =
{
	idle_callback
};*/

static MACHINE_DRIVER_START(tm990_189)

	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MDRV_CPU_ADD(TMS9980, 2000000)
	/*MDRV_CPU_FLAGS(0)*/
	/*MDRV_CPU_CONFIG(reset_params)*/
	MDRV_CPU_MEMORY(tm990_189_readmem, tm990_189_writemem)
	MDRV_CPU_PORTS(tm990_189_readport, tm990_189_writeport)
	/*MDRV_CPU_VBLANK_INT(NULL, 0)*/
	/*MDRV_CPU_PERIODIC_INT(ti990_4_line_interrupt, 120//or 100 in Europe)*/

	/* video hardware - we emulate a single 911 vdt display */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( tm990_189 )
	/*MDRV_MACHINE_STOP( tm990_189 )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(560, 280)
	MDRV_VISIBLE_AREA(0, 560-1, 0, /*250*/280-1)

	/*MDRV_GFXDECODE(vdt911_gfxdecodeinfo)*/
	MDRV_PALETTE_LENGTH(/*vdt911_palette_size*/0)
	MDRV_COLORTABLE_LENGTH(/*vdt911_colortable_size*/0)

	/*MDRV_PALETTE_INIT(vdt911)*/
	MDRV_VIDEO_START(tm990_189)
	/*MDRV_VIDEO_STOP(tm990_189)*/
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(tm990_189)

	MDRV_SOUND_ATTRIBUTES(0)
	/* one beep tone generator */
	/*MDRV_SOUND_ADD(BEEP, tm990_189_beep_interface)*/

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(tm990189)
	/*CPU memory space*/
	ROM_REGION(0x4000, REGION_CPU1,0)

	/* extra ROM - not dumped */
	ROM_LOAD("unibasi2.bin", 0x0800, 0x0800, NO_DUMP)

	/* boot ROM */
	ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744))

ROM_END

static void init_tm990_189(void)
{

}

INPUT_PORTS_START(tm990_189)

	/* 45-key calculator-like alphanumeric keyboard... */

INPUT_PORTS_END

SYSTEM_CONFIG_START(tm990_189)
	/* a tape interface and a rs232 interface... */
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1980,	tm990189,	0,		0,		tm990_189,	tm990_189,	tm990_189,	tm990_189,	"Texas Instruments",	"TM990/189 (and 189-1) University Board microcomputer" )
