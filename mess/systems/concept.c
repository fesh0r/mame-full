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
	Sound: simpler buzzer connected to the via shift register
	Keyboard: intelligent controller, connected through an ACIA.  See CCOS
	  manual pp. 76 through 78. and User Guide p. 2-1 through 2-9.
	Clock: mm58174 RTC

	Raphael Nabet, 2003
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/concept.h"
#include "devices/mflopimg.h"
#include "formats/basicdsk.h"

static ADDRESS_MAP_START(concept_memmap, ADDRESS_SPACE_PROGRAM, 16)

	AM_RANGE(0x000000, 0x000007) AM_READWRITE(MRA16_BANK1, MWA16_ROM)	/* boot ROM mirror */
	AM_RANGE(0x000008, 0x000fff) AM_READWRITE(MRA16_RAM, MWA16_RAM)		/* static RAM */
	AM_RANGE(0x010000, 0x011fff) AM_READWRITE(MRA16_ROM, MWA16_ROM)		/* boot ROM */
	AM_RANGE(0x020000, 0x021fff) AM_READWRITE(MRA16_ROM, MWA16_ROM)		/* macsbugs ROM (optional) */
	AM_RANGE(0x030000, 0x03ffff) AM_READWRITE(concept_io_r,concept_io_w)/* I/O space */

	AM_RANGE(0x080000, 0x0fffff) AM_READWRITE(/*MRA16_BANK2, MWA16_BANK2*/MRA16_RAM, MWA16_RAM)	/* DRAM */

ADDRESS_MAP_END

/* init with simple, fixed, B/W palette */
/* Is the palette black on white or white on black??? */
static PALETTE_INIT( concept )
{
	palette_set_color(0, 0xff, 0xff, 0xff);
	palette_set_color(1, 0x00, 0x00, 0x00);
}

/* concept machine */
static MACHINE_DRIVER_START( concept )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8182000)        /* 16.364 Mhz / 2 */
	MDRV_CPU_PROGRAM_MAP(concept_memmap, 0)
	MDRV_CPU_VBLANK_INT(concept_interrupt,1)

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

	PORT_START	/* port 0: keys 0x00 through 0x0f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(right)",  KEYCODE_RIGHT,     IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3",        KEYCODE_3_PAD,     IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9",        KEYCODE_9_PAD,     IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "HOME",     KEYCODE_HOME,      IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6",        KEYCODE_6_PAD,     IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",",        KEYCODE_PLUS_PAD,  IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-",        KEYCODE_MINUS_PAD, IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER",    KEYCODE_ENTER_PAD, IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(left)",   KEYCODE_LEFT,      IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1",        KEYCODE_1_PAD,     IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7",        KEYCODE_7_PAD,     IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(down)",   KEYCODE_DOWN,      IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4",        KEYCODE_4_PAD,     IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8",        KEYCODE_8_PAD,     IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5",        KEYCODE_5_PAD,     IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2",        KEYCODE_2_PAD,     IP_JOY_NONE)

	PORT_START	/* port 1: keys 0x10 through 0x1f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= +",      KEYCODE_EQUALS,    IP_JOY_NONE)

		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",      KEYCODE_OPENBRACE,IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "BACKSPACE",KEYCODE_BACKSPACE, IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER",    KEYCODE_ENTER,     IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",      KEYCODE_CLOSEBRACE,IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ |",     KEYCODE_BACKSLASH, IP_JOY_NONE)

		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )",      KEYCODE_0,         IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",      KEYCODE_SLASH,     IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P",        KEYCODE_P,         IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- _",      KEYCODE_MINUS,     IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; :",      KEYCODE_COLON,     IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "` ~",      KEYCODE_BACKSLASH2, IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "' \"",     KEYCODE_QUOTE,     IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT (r)",KEYCODE_RSHIFT,    IP_JOY_NONE)

	PORT_START	/* port 2: keys 0x20 through 0x2f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F1",       KEYCODE_F1,        IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2",       KEYCODE_F2,        IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F3",       KEYCODE_F3,        IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4",       KEYCODE_F4,        IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F5",       KEYCODE_F5,        IP_JOY_NONE)

		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",      KEYCODE_4,         IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",      KEYCODE_5,         IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R",        KEYCODE_R,         IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T",        KEYCODE_T,         IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F",        KEYCODE_F,         IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G",        KEYCODE_G,         IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V",        KEYCODE_V,         IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B",        KEYCODE_B,         IP_JOY_NONE)

	PORT_START	/* port 3: keys 0x30 through 0x3f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 @",     KEYCODE_2,         IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",      KEYCODE_3,         IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W",        KEYCODE_W,         IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E",        KEYCODE_E,         IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S",        KEYCODE_S,         IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D",        KEYCODE_D,         IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X",        KEYCODE_X,         IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C",        KEYCODE_C,         IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESC",      KEYCODE_ESC,       IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",      KEYCODE_1,         IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "TAB",      KEYCODE_TAB,       IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q",        KEYCODE_Q,         IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "CAPS LOCK",KEYCODE_CAPSLOCK,  IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A",        KEYCODE_A,         IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT (l)",KEYCODE_LSHIFT,    IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z",        KEYCODE_Z,         IP_JOY_NONE)

	PORT_START	/* port 4: keys 0x40 through 0x4f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 ^",      KEYCODE_6,         IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 &",      KEYCODE_7,         IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y",        KEYCODE_Y,         IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U",        KEYCODE_U,         IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H",        KEYCODE_H,         IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J",        KEYCODE_J,         IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N",        KEYCODE_N,         IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M",        KEYCODE_M,         IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CONTROL",  KEYCODE_LCONTROL,  IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "FAST",     KEYCODE_TILDE,     IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "COMMAND",  KEYCODE_LALT,      IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(space)",  KEYCODE_SPACE,     IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ALT",      KEYCODE_RALT,      IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0",        KEYCODE_0_PAD,     IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "00",       KEYCODE_ASTERISK,  IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".",        KEYCODE_DEL_PAD,   IP_JOY_NONE)

	PORT_START	/* port 5: keys 0x50 through 0x5f */
		PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *",      KEYCODE_8,         IP_JOY_NONE)
		PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (",      KEYCODE_9,         IP_JOY_NONE)
		PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I",        KEYCODE_I,         IP_JOY_NONE)
		PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O",        KEYCODE_O,         IP_JOY_NONE)
		PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K",        KEYCODE_K,         IP_JOY_NONE)
		PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L",        KEYCODE_L,         IP_JOY_NONE)
		PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",      KEYCODE_COMMA,     IP_JOY_NONE)
		PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",      KEYCODE_STOP,      IP_JOY_NONE)
		PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F6",       KEYCODE_F6,        IP_JOY_NONE)
		PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F7",       KEYCODE_F7,        IP_JOY_NONE)
		PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F8",       KEYCODE_F8,        IP_JOY_NONE)
		PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F9",       KEYCODE_F9,        IP_JOY_NONE)
		PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F10",      KEYCODE_F10,       IP_JOY_NONE)
		PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(up)",     KEYCODE_UP,        IP_JOY_NONE)
		PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "???",      KEYCODE_SLASH_PAD, IP_JOY_NONE)
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "BREAK",    KEYCODE_NUMLOCK,   IP_JOY_NONE)

INPUT_PORTS_END


ROM_START( concept )
	ROM_REGION16_BE(0x100000,REGION_CPU1,0)	/* 68k rom and ram */

	// concept boot ROM
#if 0
	// version 0 level 6 release
	ROM_LOAD16_BYTE("bootl06h", 0x010000, 0x1000, CRC(66b6b259))
	ROM_LOAD16_BYTE("bootl06l", 0x010001, 0x1000, CRC(600940d3))
#elif 0
	// version 1 lvl 7 release
	ROM_LOAD16_BYTE("bootl17h", 0x010000, 0x1000, CRC(6dd9718f))
	ROM_LOAD16_BYTE("bootl17l", 0x010001, 0x1000, CRC(107a3830))
#elif 0
	// version 0 lvl 8 release
	ROM_LOAD16_BYTE("bootl08h", 0x010000, 0x1000, CRC(ee479f51))
	ROM_LOAD16_BYTE("bootl08l", 0x010001, 0x1000, CRC(acaefd07))
#else
	// version $F lvl 8 (development version found on a floppy disk along with
	// the source code)
	ROM_LOAD16_WORD("cc.prm", 0x010000, 0x2000, CRC(b5a87dab) SHA1(0da59af6cfeeb38672f71731527beac323d9c3d6))
#endif

#if 0
	// only known MACSbug release for the concept, with reset vector and
	// entry point (the reset vector seems to be bogus: is the ROM dump bad,
	// or were the ROMs originally loaded with buggy code?)
	ROM_LOAD16_BYTE("macsbugh", 0x020000, 0x1000, CRC(aa357112))
	ROM_LOAD16_BYTE("macsbugl", 0x020001, 0x1000, CRC(b4b59de9))
#endif

ROM_END


static FLOPPY_OPTIONS_START(concept)
#if 1
	/* SSSD 8" */
	FLOPPY_OPTION(concept, "img\0", "Corvus Concept 8\" SSSD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* SSDD 8" (according to ROMs) */
	FLOPPY_OPTION(concept, "img\0", "Corvus Concept 8\" SSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* Apple II DSDD 5"1/4 (according to ROMs) */
	FLOPPY_OPTION(concept, "img\0", "Corvus Concept Apple II 5\"1/4 DSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([35])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#elif 0
	/* actual formats found */
	FLOPPY_OPTION(concept, "img\0", "Corvus Concept 5\"1/4 DSDD disk image (256-byte sectors)", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
#else
	FLOPPY_OPTION(concept, "img\0", "Corvus Concept 5\"1/4 DSDD disk image (512-byte sectors)", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
#endif
FLOPPY_OPTIONS_END

SYSTEM_CONFIG_START(concept)
	/* The concept should eventually support floppies, hard disks, etc. */
	CONFIG_DEVICE_FLOPPY(4,	concept)
SYSTEM_CONFIG_END


/*	  YEAR  NAME	  PARENT	COMPAT	MACHINE   INPUT	   INIT	 CONFIG   COMPANY           FULLNAME */
COMP( 1982, concept,  0,		0,		concept,  concept, 0,    concept, "Corvus Systems", "Concept" )
