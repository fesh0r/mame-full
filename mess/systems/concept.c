/*
	Corvus Concept driver

	Relatively simple 68k-based system

	* 256 or 512 kbytes of DRAM
	* 4kbytes of SRAM
	* 8kbyte boot ROM
	* optional MacsBugs ROM
	* two serial ports, keyboard, bitmapped display, simple sound, omninet
	  LAN port (seems more or less similar to AppleTalk)
	* 4 expansion ports enable to add expansion cards, namely floppy disk
	and hard disk controllers (the expansion ports are partially compatible
	with Apple 2 expansion ports)

	Video: monochrome bitmapped display, 720*560 visible area (bitmaps are 768
	  pixels wide in memory).  One interesting feature is the fact that the
	  monitor can be rotated to give a 560*720 vertical display (you need to
	  throw a switch and reset the machine for the display rotation to be taken
	  into account, though).  One oddity is that the video hardware scans the
	  display from the lower-left corner to the upper-left corner (or from the
	  upper-right corner to the lower-left if the screen is flipped).
	Sound: simpler buzzer connected to the via via serial shift output
	Keyboard: intelligent controller, connected through an ACIA.  See CCOS
	  manual pp. 76 through 78.
	Clock: mm58174 RTC

	Raphael Nabet, 2003
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/concept.h"

static MEMORY_READ16_START (concept_readmem)

	{ 0x000000, 0x000007, MRA16_BANK1 },	/* boot ROM mirror */
	{ 0x000008, 0x000fff, MRA16_RAM },		/* static RAM */
	{ 0x010000, 0x011fff, MRA16_ROM },		/* boot ROM */
	{ 0x020000, 0x021fff, MRA16_ROM },		/* macsbugs ROM (optional) */
	{ 0x030000, 0x03ffff, concept_io_r },	/* I/O space */

	{ 0x080000, 0x0fffff, MRA16_BANK2 },	/* DRAM */

MEMORY_END

static MEMORY_WRITE16_START (concept_writemem)

	{ 0x000000, 0x000007, MWA16_ROM },		/* boot ROM mirror */
	{ 0x000008, 0x000fff, MWA16_RAM },		/* static RAM */
	{ 0x010000, 0x011fff, MWA16_ROM },		/* boot ROM */
	{ 0x020000, 0x021fff, MWA16_ROM },		/* macsbugs ROM (optional) */
	{ 0x030000, 0x03ffff, concept_io_w },	/* I/O space */

	{ 0x080000, 0x0fffff, MWA16_BANK2 },	/* DRAM */

MEMORY_END

/* init with simple, fixed, B/W palette */
static PALETTE_INIT( concept )
{
	palette_set_color(0, 0xff, 0xff, 0xff);
	palette_set_color(1, 0x00, 0x00, 0x00);
}

/* concept machine */
static MACHINE_DRIVER_START( concept )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8182000)        /* 16.364 Mhz / 2? */
	MDRV_CPU_MEMORY(concept_readmem,concept_writemem)
	//MDRV_CPU_VBLANK_INT(corvus_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)			/* 50 or 60, jumper-selectable */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)
	MDRV_MACHINE_INIT(concept)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(720, 560)
	MDRV_VISIBLE_AREA(0, 720-1, 0, 560-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT(concept)

	MDRV_VIDEO_START(concept)
	MDRV_VIDEO_UPDATE(concept)

	/* no sound? */
MACHINE_DRIVER_END


INPUT_PORTS_START( concept )
INPUT_PORTS_END


ROM_START( concept )
	ROM_REGION16_BE(0x100000,REGION_CPU1,0)	/* 68k rom and ram */
#if 0
	// variant for which I have the source code
	ROM_LOAD16_WORD( "cc.prm", 0x010000, 0x2000, CRC(b5a87dab))
#else
	ROM_LOAD16_BYTE( "u706", 0x010000, 0x1000, CRC(ee479f51))
	ROM_LOAD16_BYTE( "u711", 0x010001, 0x1000, CRC(acaefd07))

	ROM_LOAD16_BYTE( "u708", 0x020001, 0x1000, CRC(b4b59de9))
	ROM_LOAD16_BYTE( "u709", 0x020000, 0x1000, CRC(aa357112))
#endif
ROM_END


SYSTEM_CONFIG_START(concept)
	/* The concept should eventually support floppies, hard disks, etc. */
SYSTEM_CONFIG_END

/*	   YEAR   NAME	   PARENT	COMPAT	MACHINE   INPUT	   INIT	    CONFIG	 COMPANY   FULLNAME */
COMPX( 1982?, concept, 0,		0,		concept,  concept, /*concept*/0, concept, "Corvus", "Concept", GAME_NOT_WORKING )
