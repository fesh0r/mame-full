/*
	MESS Driver for TI-99/8 Computer.
	Raphael Nabet, 2003.
*/
/*
	TI-99/8 preliminary info:

Name: Texas Instruments Computer TI-99/8 (no "Home")

References:
	* machine room <http://...>
	* TI99/8 user manual
	* TI99/8 schematics
	* TI99/8 ROM source code
	* Message on TI99 yahoo group for CPU info

General:
	* a few dozen units were built in 1983, never released
	* CPU is a custom variant of tms9995 (part code MP9537): the 16-bit RAM and
	  (presumably) the on-chip decrementer are disabled
	* 220kb(?) of ROM, including monitor, GPL interpreter, TI-extended basic
	  II, and a P-code interpreter with a few utilities.  More specifically:
	  - 32kb system ROM with GPL interpreter, TI-extended basic II and a few
	    utilities (no dump, but 90% of source code is available and has been
	    compiled)
	  - 18kb system GROMs, with monitor and TI-extended basic II (no dump,
	    source code is available but has NOT been compiled, 99/4a GROMs can
	    sort of work instead)
	  - 4(???)kb DSR ROM for hexbus (no dump)
	  - 32(?)kb speech ROM: contents are slightly different from the 99/4(a)
	    speech ROMs, due to the use of a tms5220 speech synthesizer instead of
	    the older tms0285 (no dump, but 99/4(a) speech ROMs should work mostly
	    OK)
	  - 12(???)kb ROM with PCode interpreter (no dump)
	  - 2(3???)*48kb of GROMs with PCode data files (no dump)
	* 2kb SRAM (16 bytes of which are hidden), 64kb DRAM (expandable to almost
	  16MBytes), 16kb vdp RAM
	* tms9118 vdp (similar to tms9918a, slightly different bus interface and
	  timings)
	* I/O
	  - 50-key keyboard, plus 2 optional joysticks
	  - sound and speech (both ti99/4(a)-like)
	  - Hex-Bus
	  - Cassette
	* cartridge port on the top
	* 50-pin(?) expansion port on the back
	* Programs can enable/disable the ROM and memory mapped register areas.

Mapper:
	Mapper has 4kb page size (-> 16 pages per map file), 32 bits per page
	entry.  Address bits A0-A3 are the page index, whereas bits A4-A15 are the
	offset in the page.  Physical address space is 16Mbytes.  All pages are 4
	kBytes in lenght, and they can start anywhere in the 24-bit physical
	address space.  The mapper can load any of 4 map files from SRAM by DMA.
	Map file 0 is used by BIOS, file 1 by memory XOPs(?), file 2 by P-code
	interpreter(???).

	Format of map table entry:
	* bit 0: WTPROT: page is write protected if 1
	* bit 1: XPROT: page is execute protected if 1
	* bit 2: RDPROT: page is read protected if 1
	* bit 3: reserved, value is ignored
	* bits 4-7: reserved, always forced to 0
	* bits 8-23: page base address in 24-bit virtual address space

	Format of mapper control register:
	* bit 0-4: unused???
	* bit 5-6: map file to load/save (0 for file 0, 1 for file 1, etc.)
	* bit 7: 0 -> load map file from RAM, 1 -> save map file to RAM

	Format of mapper status register (cleared by read):
	* bit 0: WPE - Write-Protect Error
	* bit 1: XCE - eXeCute Error
	* bit 2: RPE - Read-Protect Error
	* bits 3-7: unused???

	Memory error interrupts are enabled by setting WTPROT/XPROT/RDPROT.  When
	an error occurs, the tms9901 INT1* pin is pulled low (active).  The pin
	remains low until the mapper status register is read.

24-bit address map:
	* >000000->00ffff: console RAM
	* >010000->feffff: expansion?
	* >ff0000->ff0fff: empty???
	* >ff1000->ff3fff: unused???
	* >ff4000->ff5fff: DSR space
	* >ff6000->ff7fff: cartridge space
	* >ff8000->ff9fff(???): >4000 ROM (normally enabled with a write to CRU >2700)
	* >ffa000->ffbfff(?): >2000 ROM
	* >ffc000->ffdfff(?): >6000 ROM


CRU map:
	Since the tms9995 supports full 15-bit CRU addresses, the >1000->17ff
	(>2000->2fff) range was assigned to support up to 16 extra expansion slot.
	The good thing with using >1000->17ff is the fact that older expansion
	cards that only decode 12 address bits will think that addresses
	>1000->17ff refer to internal TI99 peripherals (>000->7ff range), which
	suppresses any risk of bus contention.
	* >0000->001f (>0000->003e): tms9901
	  - P4: 1 -> MMD (Memory Mapped Devices?) at >8000, ROM enabled
	  - P5: 1 -> no P-CODE GROMs
	* >0800->17ff (>1000->2ffe): Peripheral CRU space
	* >1380->13ff (>2700->27fe): Internal DSR, with two output bits:
	  - >2700: Internal DSR select (parts of Basic and various utilities)
	  - >2702: SBO -> hardware reset


Memory map (TMS9901 P4 == 1):
	When TMS9901 P4 output is set, locations >8000->9fff are ignored by mapper.
	* >8000->83ff: SRAM (>8000->80ff is used by the mapper DMA controller
	  to hold four map files) (r/w)
	* >8400: sound port (w)
	* >8410->87ff: SRAM (r/w)
	* >8800: VDP data read port (r)
	* >8802: VDP status read port (r)
	* >8810: memory mapper status and control registers (r/w)
	* >8c00: VDP data write port (w)
	* >8c02: VDP address and register write port (w)
	* >9000: speech synthesizer read port (r)
	* >9400: speech synthesizer write port (w)
	* >9800 GPL data read port (r)
	* >9802 GPL address read port (r)
	* >9c00 GPL data write port -- unused (w)
	* >9c02 GPL address write port (w)


Memory map (TMS9901 P5 == 0):
	When TMS9901 P5 output is cleared, locations >f840->f8ff(?) are ignored by
	mapper.
	* >f840: data port for P-code grom library 0 (r?)
	* >f880: data port for P-code grom library 1 (r?)
	* >f8c0: data port for P-code grom library 2 (r?)
	* >f842: address port for P-code grom library 0 (r/w?)
	* >f882: address port for P-code grom library 1 (r/w?)
	* >f8c2: address port for P-code grom library 2 (r/w?)


Cassette interface:
	Identical to ti99/4(a), except that the CS2 unit is not implemented.


Keyboard interface:
	The keyboard interface uses the console tms9901 PSI, but the pin assignment
	and key matrix are different from both 99/4 and 99/4a.
	- P0-P3: column select
	- INT6*-INT11*: row inputs (int6* is only used for joystick fire)
*/

#include "driver.h"
#include "inputx.h"
#include "vidhrdw/tms9928a.h"

#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "sndhrdw/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "cpu/tms9900/tms9900.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"

/*
  Memory map - see description above
*/

static MEMORY_READ_START (ti99_8_readmem )

	{ 0x0000, 0xffff, ti99_8_r },

MEMORY_END

static MEMORY_WRITE_START ( ti99_8_writemem )

	{ 0x0000, 0xffff, ti99_8_w },

MEMORY_END


/*
  CRU map - see description above
*/

static PORT_WRITE_START ( ti99_8_writecru )

	{0x0000, 0x07ff, tms9901_0_CRU_write},
	{0x0800, 0x17ff, ti99_8_peb_CRU_w},

PORT_END

static PORT_READ_START ( ti99_8_readcru )

	{0x0000, 0x00ff, tms9901_0_CRU_read},
	{0x0100, 0x02ff, ti99_8_peb_CRU_r},

PORT_END


/* ti99/8 : 54-key keyboard */
INPUT_PORTS_START(ti99_8)

	/* 1 port for config */
	PORT_START	/* config */
		PORT_BITX( config_fdc_mask << config_fdc_bit, /*fdc_kind_hfdc << config_fdc_bit*/0, IPT_DIPSWITCH_NAME, "Floppy disk controller", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, "none" )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments SD" )
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
			PORT_DIPSETTING( fdc_kind_hfdc << config_fdc_bit, "Myarc's HFDC" )
		PORT_BITX( config_ide_mask << config_ide_bit, /*1 << config_ide_bit*/0, IPT_DIPSWITCH_NAME, "Nouspickel's IDE card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_BITX( config_rs232_mask << config_rs232_bit, /*1 << config_rs232_bit*/0, IPT_DIPSWITCH_NAME, "TI RS232 card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )
		PORT_BITX( config_hsgpl_mask << config_hsgpl_bit, 0/*1 << config_hsgpl_bit*/, IPT_DIPSWITCH_NAME, "SNUG HSGPL card", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_hsgpl_bit, DEF_STR( On ) )
		PORT_BITX( config_mecmouse_mask << config_mecmouse_bit, 0, IPT_DIPSWITCH_NAME, "Mechatronics Mouse", KEYCODE_NONE, IP_JOY_NONE )
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_mecmouse_bit, DEF_STR( On ) )


	/* 3 ports for mouse */
	PORT_START /* Mouse - X AXIS */
		PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - Y AXIS */
		PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* Mouse - buttons */
		PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
		PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)


	/* 16 ports for keyboard and joystick */
	PORT_START    /* col 0 */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "ALPHA LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, KEYCODE_RALT)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "LSHIFT", KEYCODE_LSHIFT, IP_JOY_NONE,	UCHAR_SHIFT_1)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 1 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "1 ! DEL", KEYCODE_1, IP_JOY_NONE,		'1',	'!')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "q Q", KEYCODE_Q, IP_JOY_NONE,			'q',	'Q')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "a A", KEYCODE_A, IP_JOY_NONE,			'a',	'A')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "z Z", KEYCODE_Z, IP_JOY_NONE,			'z',	'Z')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 2 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "2 @ INS", KEYCODE_2, IP_JOY_NONE,		'2',	'@')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "w W", KEYCODE_W, IP_JOY_NONE,			'w',	'W')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE,	's',	'S')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE,	'x',	'X')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 3 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "3 #", KEYCODE_3, IP_JOY_NONE,			'3',	'#')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "e E (UP)", KEYCODE_E, IP_JOY_NONE,		'e',	'E')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE,	'd',	'D')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "c C", KEYCODE_C, IP_JOY_NONE,			'c',	'C')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 4 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE,		'4',	'$')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "r R", KEYCODE_R, IP_JOY_NONE,			'r',	'R')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "f F", KEYCODE_F, IP_JOY_NONE,			'f',	'F')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "v V", KEYCODE_V, IP_JOY_NONE,			'v',	'V')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 5 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE,		'5',	'%')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "t T", KEYCODE_T, IP_JOY_NONE,			't',	'T')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "g G", KEYCODE_G, IP_JOY_NONE,			'g',	'G')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "b B", KEYCODE_B, IP_JOY_NONE,			'b',	'B')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 6 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE,	'6',	'^')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "y Y", KEYCODE_Y, IP_JOY_NONE,			'y',	'Y')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "h H", KEYCODE_H, IP_JOY_NONE,			'h',	'H')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "n N", KEYCODE_N, IP_JOY_NONE,			'n',	'N')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 7 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "7 & AID", KEYCODE_7, IP_JOY_NONE,		'7',	'&')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "u U", KEYCODE_U, IP_JOY_NONE,			'u',	'U')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "j J", KEYCODE_J, IP_JOY_NONE,			'j',	'J')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "m M", KEYCODE_M, IP_JOY_NONE,			'm',	'M')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 8 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "8 * REDO", KEYCODE_8, IP_JOY_NONE,		'8',	'*')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "i I", KEYCODE_I, IP_JOY_NONE,			'i',	'I')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "k K", KEYCODE_K, IP_JOY_NONE,			'k',	'K')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, ", <", KEYCODE_COMMA, IP_JOY_NONE,		',',	'<')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 9 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "9 ( BACK", KEYCODE_9, IP_JOY_NONE,		'9',	'(')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "o O", KEYCODE_O, IP_JOY_NONE,			'o',	'O')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "l L", KEYCODE_L, IP_JOY_NONE,			'l',	'L')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, ". >", KEYCODE_STOP, IP_JOY_NONE,		'.',	'>')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 10 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "0 )", KEYCODE_0, IP_JOY_NONE,			'0',	')')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "p P", KEYCODE_P, IP_JOY_NONE,			'p',	'P')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "; :", KEYCODE_COLON, IP_JOY_NONE,		';',	':')
		PORT_KEY2(0x02, IP_ACTIVE_LOW, "/ ?", KEYCODE_SLASH, IP_JOY_NONE,		'/',	'?')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 11 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE,	'=',	'+')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "[ {", KEYCODE_OPENBRACE, IP_JOY_NONE,	'[',	'{')
		PORT_KEY2(0x04, IP_ACTIVE_LOW, "' \"", KEYCODE_QUOTE, IP_JOY_NONE,		'\'',	'"')
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RSHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 12 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "- _", KEYCODE_MINUS, IP_JOY_NONE,		'-',	'_')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "] }", KEYCODE_CLOSEBRACE, IP_JOY_NONE,	']',	'}')
		PORT_KEY1(0x04, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, IP_JOY_NONE,		13)
		PORT_KEY1(0x02, IP_ACTIVE_LOW, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE,	' ')
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 13 */
		PORT_KEY2(0x10, IP_ACTIVE_LOW, "\\ |", KEYCODE_BACKSLASH, IP_JOY_NONE,	'\\',	'|')
		PORT_KEY2(0x08, IP_ACTIVE_LOW, "` ~", KEYCODE_BACKSLASH2, IP_JOY_NONE,	'`',	'~')
		PORT_BITX(0x07, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 14: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

	PORT_START    /* col 15: "wired handset 2" (= joystick 2) */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

INPUT_PORTS_END

/*
	TMS9919 (a.k.a. SN76489) sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,				/* one sound chip */
	{ 3579545 },	/* clock speed. connected to the TMS9918A CPUCLK pin... */
	{ 75 }			/* Volume.  I don't know the best value. */
};

/*
	TMS5220 speech synthesizer
*/
static struct TMS5220interface tms5220interface =
{
	680000L,					/* 640kHz -> 8kHz output */
	50,							/* Volume.  I don't know the best value. */
	NULL,						/* no IRQ callback */
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
};

/*
	1 tape unit
*/
static struct Wave_interface tape_input_intf =
{
	1,
	{
		20			/* Volume for CS1 */
	}
};


static const TMS9928a_interface tms9118_interface =
{
	TMS99x8A,
	0x4000,
	tms9901_set_int2
};

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	tms9901_set_int2
};

static struct tms9995reset_param ti99_8_processor_config =
{
	1,				/* enable automatic wait state generation */
					/* (in January 83 99/8 schematics sheet 9: the delay logic
					seems to keep READY low for one cycle when RESET* is
					asserted, but the timings are completely wrong this way) */
	0,				/* no IDLE callback */
	1				/* MP9537 mask */
};

static MACHINE_DRIVER_START(ti99_8_60hz)

	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10738635)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(ti99_8_processor_config)
	MDRV_CPU_MEMORY(ti99_8_readmem, ti99_8_writemem)
	MDRV_CPU_PORTS(ti99_8_readcru, ti99_8_writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9118_interface )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(ti99_8_50hz)

	/* basic machine hardware */
	/* TMS9995-MP9537 CPU @ 10.7 MHz */
	MDRV_CPU_ADD(TMS9995, 10738635)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(ti99_8_processor_config)
	MDRV_CPU_MEMORY(ti99_8_readmem, ti99_8_writemem)
	MDRV_CPU_PORTS(ti99_8_readcru, ti99_8_writecru)
	MDRV_CPU_VBLANK_INT(ti99_vblank_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti99 )
	MDRV_MACHINE_STOP( ti99 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_TMS9928A( &tms9129_interface )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(0)
	MDRV_SOUND_ADD(SN76496, tms9919interface)
	MDRV_SOUND_ADD(TMS5220, tms5220interface)
	MDRV_SOUND_ADD(WAVE, tape_input_intf)

MACHINE_DRIVER_END

/*
  ROM loading
*/
ROM_START(ti99_8)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_8,REGION_CPU1,0)
	ROM_LOAD("998rom.bin", 0x0000, 0x8000, NO_DUMP)		/* system ROMs */

	/*GROM memory space*/
	ROM_REGION(0x10000, region_grom, 0)
	ROM_LOAD("998grom.bin", 0x0000, 0x6000, NO_DUMP)	/* system GROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f)) /* TI disk DSR ROM */
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb)) /* TI rs232 DSR ROM */

	/* HSGPL memory space */
	ROM_REGION(region_hsgpl_len, region_hsgpl, 0)

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, BAD_DUMP CRC(58b155f7)) /* system speech ROM */

ROM_END

#define rom_ti99_8e rom_ti99_8

SYSTEM_CONFIG_START(ti99_8)
	/* one cartridge port */
	/* one cassette unit port */
	/* Hex-bus disk controller: supports up to 4 floppy disk drives */
	/* expansion port (similar to 99/4(a) - yet slightly different) */

	CONFIG_DEVICE_CASSETTE			(1, NULL)
	CONFIG_DEVICE_LEGACY			(IO_CARTSLOT,	3,	"bin\0c\0d\0g\0m\0crom\0drom\0grom\0mrom\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_READ,	NULL,	NULL,	device_load_ti99_cart,	device_unload_ti99_cart,	NULL)
#if 1
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",										device_load_ti99_floppy)
	CONFIG_DEVICE_LEGACY			(IO_HARDDISK, 	4, "hd\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_OR_READ,device_init_ti99_hd, NULL, device_load_ti99_hd, device_unload_ti99_hd, NULL)
	CONFIG_DEVICE_LEGACY			(IO_PARALLEL,	1, "\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_4_pio,	device_unload_ti99_4_pio,		NULL)
	CONFIG_DEVICE_LEGACY			(IO_SERIAL,		1, "\0",	DEVICE_LOAD_RESETS_NONE,	OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_4_rs232,	device_unload_ti99_4_rs232,	NULL)
	/*CONFIG_DEVICE_LEGACY			(IO_QUICKLOAD,	1, "\0",	DEVICE_LOAD_RESETS_CPU,		OSD_FOPEN_RW_CREATE_OR_READ,	NULL,	NULL,	device_load_ti99_hsgpl,		device_unload_ti99_hsgpl,	NULL)*/
#endif

SYSTEM_CONFIG_END

/*		YEAR	NAME		PARENT		COMPAT	MACHINE		INPUT	INIT		CONFIG		COMPANY					FULLNAME */
COMP(	1983,	ti99_8,		0,			0,		ti99_8_60hz,ti99_8,	ti99_8,		ti99_8,		"Texas Instruments",	"TI-99/8 Computer (US)" )
COMP(	1983,	ti99_8e,	ti99_8,		0,		ti99_8_50hz,ti99_8,	ti99_8,		ti99_8,		"Texas Instruments",	"TI-99/8 Computer (Europe)" )
