/***************************************************************************
Acorn Atom:

Memory map.

CPU: 65C02
		0000-00ff Zero page
		0100-01ff Stack
		0200-1fff RAM (expansion)
		0a00-0a04 FDC 8271
		2000-21ff RAM (dos catalogue buffer)
		2200-27ff RAM (dos seq file buffer)
		2800-28ff RAM (float buffer)
		2900-7fff RAM (text RAM)
		8000-97ff VDG 6847
		9800-9fff RAM (expansion)
		a000-afff ROM (extension)
		b000-b003 PPIA 8255
		b003-b7ff NOP
		b800-bbff VIA 6522
		bc00-bfdf NOP
		bfe0-bfe2 MOUSE	- extension??
		bfe3-bfff NOP
		c000-cfff ROM (basic)
		d000-dfff ROM (float)
		e000-efff ROM (dos)
		f000-ffff ROM (kernel)

Video:		MC6847

Sound:		Buzzer
Floppy:		FDC8271

Hardware:	PPIA 8255

	output	b000	0 - 3 keyboard row, 4 - 7 graphics mode
			b002	0 cas output, 1 enable 2.4Khz, 2 buzzer, 3 colour set

	input	b001	0 - 5 keyboard column, 6 CTRL key, 7 SHIFT key
			b002	4 2.4kHz input, 5 cas input, 6 REPT key, 7 60 Hz input

			VIA 6522


	DOS:

	The original location of the 8271 memory mapped registers is 0xa00-0x0a04.
	(This is the memory range assigned by Acorn in their design.)

	This is in the middle of the area for expansion RAM. Many Atom owners
	thought this was a bad design and have modified their Atom's and dos rom
	to use a different memory area.

	The atom driver in MESS uses the original memory area.



***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"
#include "includes/i8271.h"
#include "devices/basicdsk.h"
#include "devices/flopdrv.h"
#include "machine/6522via.h"
#include "devices/printer.h"
#include "includes/centroni.h"

/* functions */

/* port i/o functions */

/* memory w/r functions */

static MEMORY_READ_START (atom_readmem)
	{ 0x0000, 0x09ff, MRA_RAM },
    { 0x0a00, 0x0a04, atom_8271_r},
	{ 0x0a05, 0x07fff, MRA_RAM},
	{ 0x8000, 0x97ff, videoram_r },		// VDG 6847
	{ 0x9800, 0x9fff, MRA_RAM },
    { 0xb000, 0xb003, ppi8255_0_r },    // PPIA 8255
	{ 0xb800, 0xbbff, atom_via_r},		// VIA 6522
    { 0xc000, 0xcfff, MRA_ROM },
    { 0xd000, 0xdfff, MRA_ROM },
    { 0xe000, 0xefff, MRA_ROM },
    { 0xf000, 0xffff, MRA_ROM },

MEMORY_END

static MEMORY_WRITE_START (atom_writemem)
	{ 0x0000, 0x09ff, MWA_RAM },
    { 0x0a00, 0x0a04, atom_8271_w},
	{ 0x0a05, 0x07fff, MWA_RAM},
	{ 0x8000, 0x97ff, videoram_w, &videoram, &videoram_size}, // VDG 6847
	{ 0x9800, 0x9fff, MWA_RAM },
    { 0xb000, 0xb003, ppi8255_0_w },    // PIA 8255
	{ 0xb800, 0xbbff, atom_via_w},		// VIA 6522
	{ 0xc000, 0xffff, MWA_ROM },
    { 0xd000, 0xdfff, MWA_ROM },
    { 0xe000, 0xefff, MWA_ROM },
    { 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START (atomeb_readmem)
	{ 0x0000, 0x09ff, MRA_RAM },
    { 0x0a00, 0x0a04, atom_8271_r},
	{ 0x0a05, 0x07fff, MRA_RAM},
	{ 0x8000, 0x97ff, videoram_r },		// VDG 6847
	{ 0x9800, 0x9fff, MRA_RAM },
    { 0xa000, 0xafff, MRA_BANK1 },		// eprom data from eprom box
    { 0xb000, 0xb003, ppi8255_0_r },    // PPIA 8255
	{ 0xb800, 0xbbff, atom_via_r},		// VIA 6522
    { 0xbfff, 0xbfff, atom_eprom_box_r},
	{ 0xc000, 0xcfff, MRA_ROM },
    { 0xd000, 0xdfff, MRA_ROM },
    { 0xe000, 0xefff, MRA_ROM },
    { 0xf000, 0xffff, MRA_ROM },

MEMORY_END

static MEMORY_WRITE_START (atomeb_writemem)
	{ 0x0000, 0x09ff, MWA_RAM },
    { 0x0a00, 0x0a04, atom_8271_w},
	{ 0x0a05, 0x07fff, MWA_RAM},
	{ 0x8000, 0x97ff, videoram_w, &videoram, &videoram_size}, // VDG 6847
	{ 0x9800, 0x9fff, MWA_RAM },
 /*   { 0xa000, 0xafff, MWA_NOP }, */
    { 0xb000, 0xb003, ppi8255_0_w },    // PIA 8255
	{ 0xb800, 0xbbff, atom_via_w},		// VIA 6522
    { 0xbfff, 0xbfff, atom_eprom_box_w},
	{ 0xc000, 0xffff, MWA_ROM },
    { 0xd000, 0xdfff, MWA_ROM },
    { 0xe000, 0xefff, MWA_ROM },
    { 0xf000, 0xffff, MWA_ROM },
MEMORY_END
/* graphics output */

/* keyboard input */

/* not implemented: BREAK */

INPUT_PORTS_START (atom)
	PORT_START /* 0 VBLANK */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* 1 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "escape", KEYCODE_ESC, IP_JOY_NONE)

	PORT_START /* 2 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)

	PORT_START /* 3 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "up", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START /* 4 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "right", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)

	PORT_START /* 5 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "caps lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* 6 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "uparrow", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "copy", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "enter", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)

	PORT_START /* 8 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)

	PORT_START /* 10 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)

	PORT_START /* 11 CTRL & SHIFT */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LControl", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RControl", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "LShift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RShift", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START /* 12 REPT key */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LAlt", KEYCODE_LALT, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RAlt", KEYCODE_RALT, IP_JOY_NONE)

INPUT_PORTS_END

/* sound output */

static	struct	Speaker_interface atom_sh_interface =
{
	1,
	{ 100 },
	{ 0 },
	{ NULL }
};

/* machine definition */
static MACHINE_DRIVER_START( atom )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M65C02, 1000000)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(atom_readmem, atom_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(128)

	MDRV_MACHINE_INIT( atom )

	/* video hardware */
	MDRV_M6847_PAL( atom )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, atom_sh_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atomeb )
	MDRV_IMPORT_FROM( atom )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( atomeb_readmem, atomeb_writemem )

	MDRV_MACHINE_INIT( atomeb )
MACHINE_DRIVER_END


ROM_START (atom)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("akernel.rom", 0xf000, 0x1000, 0xc604db3d)
	ROM_LOAD ("dosrom.rom", 0xe000, 0x1000, 0xc431a9b7)
	ROM_LOAD ("afloat.rom", 0xd000, 0x1000, 0x81d86af7)
	ROM_LOAD ("abasic.rom", 0xc000, 0x1000, 0x43798b9b)
ROM_END

ROM_START (atomeb)
	ROM_REGION (0x10000+0x09000, REGION_CPU1,0)
	ROM_LOAD ("akernel.rom", 0xf000, 0x1000, 0xc604db3d)
	ROM_LOAD ("dosrom.rom", 0xe000, 0x1000, 0xc431a9b7)
	ROM_LOAD ("afloat.rom", 0xd000, 0x1000, 0x81d86af7)
	ROM_LOAD ("abasic.rom", 0xc000, 0x1000, 0x43798b9b)
	/* roms from another oric emulator */
	ROM_LOAD ("axr1.rom",0x010000,0x1000, 0x868fda8b)
	ROM_LOAD ("pcharme.rom",0x011000,0x1000,0x9e8bd79f)
	ROM_LOAD ("gags.rom",0x012000,0x1000,0x35e1d713)
	ROM_LOAD ("werom.rom",0x013000,0x1000,0xdfcb3bf8)
	ROM_LOAD ("unknown.rom",0x014000,0x1000,0x013b8f93)
	ROM_LOAD ("combox.rom",0x015000,0x1000,0x9c8210ab)
	ROM_LOAD ("salfaa.rom",0x016000,0x1000,0x0ef857b25)
	ROM_LOAD ("mousebox.rom",0x017000,0x01000,0x0dff30e4)
	ROM_LOAD ("atomicw.rom",0x018000,0x1000,0xa3fd737d)
ROM_END

SYSTEM_CONFIG_START(atom)
	CONFIG_DEVICE_CASSETTE			(1, "",			device_load_atom_cassette)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(2, "ssd\0",	device_load_atom_floppy)
	CONFIG_DEVICE_PRINTER			(1)
	CONFIG_DEVICE_QUICKLOAD			(	"atm\0",	atom)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT      CONFIG   COMPANY   FULLNAME */
COMP( 1979, atom,     0,        0,		atom,     atom,     0,        atom,    "Acorn",  "Atom" )
COMP( 1979, atomeb,   atom,     0,		atomeb,   atom,     0,        atom,    "Acorn",  "Atom with Eprom Box" )
