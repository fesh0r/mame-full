/*********************************************************************

	systems/oric.c

	Systems supported by this driver:

	Oric 1,
	Oric Atmos,
	Oric Telestrat,
	Pravetz 8D

	Pravetz is a Bulgarian copy of the Oric Atmos and uses
	Apple 2 disc drives for storage.

	This driver originally by Paul Cook, rewritten by Kevin Thacker.

*********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/oric.h"
#include "includes/centroni.h"
#include "devices/printer.h"
#include "devices/mflopimg.h"
#include "devices/cassette.h"
#include "formats/ap2_dsk.h"
#include "formats/oric_tap.h"

#include "includes/apple2.h"

/*
	Explaination of memory regions:

	I have split the memory region &c000-&ffff in this way because:

	All roms (os, microdisc and jasmin) use the 6502 IRQ vectors at the end
	of memory &fff8-&ffff, but they are different sizes. The os is 16k, microdisc
	is 8k and jasmin is 2k.

	There is also 16k of ram at &c000-&ffff which is normally masked
	by the os rom, but when the microdisc or jasmin interfaces are used,
	this ram can be accessed. For the microdisc and jasmin, the ram not
	covered by the roms for these interfaces, can be accessed
	if it is enabled.

	MRA_BANK1,MRA_BANK2 and MRA_BANK3 are used for a 16k rom.
	MRA_BANK2 and MRA_BANK3 are used for a 8k rom.
	MRA_BANK3 is used for a 2k rom.

	0x0300-0x03ff is I/O access. It is not defined below because the
	memory is setup dynamically depending on hardware that has been selected (microdisc, jasmin, apple2) etc.

*/


static MEMORY_READ_START(oric_readmem)
    { 0x0000, 0x02FF, MRA_RAM },

	/* { 0x0300, 0x03ff, oric_IO_r }, */
    { 0x0400, 0xBFFF, MRA_RAM },

    { 0xc000, 0xdFFF, MRA_BANK1 },
	{ 0xe000, 0xf7ff, MRA_BANK2 },
	{ 0xf800, 0xffff, MRA_BANK3 },
MEMORY_END

static MEMORY_WRITE_START(oric_writemem)
    { 0x0000, 0x02FF, MWA_RAM },
    /* { 0x0300, 0x03ff, oric_IO_w }, */
    { 0x0400, 0xbFFF, MWA_RAM },
    { 0xc000, 0xdFFF, MWA_BANK5 },
    { 0xe000, 0xf7ff, MWA_BANK6 },
	{ 0xf800, 0xffff, MWA_BANK7 },
MEMORY_END

/*
The telestrat has the memory regions split into 16k blocks.
Memory region &c000-&ffff can be ram or rom. */
static MEMORY_READ_START(telestrat_readmem)
    { 0x0000, 0x02FF, MRA_RAM },
    { 0x0300, 0x03ff, telestrat_IO_r },
    { 0x0400, 0xBFFF, MRA_RAM },
    { 0xc000, 0xfFFF, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START(telestrat_writemem)
    { 0x0000, 0x02FF, MWA_RAM },
    { 0x0300, 0x03ff, telestrat_IO_w },
    { 0x0400, 0xbFFF, MWA_RAM },
    { 0xc000, 0xffff, MWA_BANK2 },
MEMORY_END

#define INPUT_PORT_ORIC \
	PORT_START /* IN0 */ \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",         IP_KEY_NONE,        IP_JOY_NONE) \
 \
    PORT_START /* KEY ROW 0 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",      KEYCODE_3,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x X",      KEYCODE_X,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",      KEYCODE_1,          IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "v V",      KEYCODE_V,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",      KEYCODE_5,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "n N",      KEYCODE_N,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 ^",      KEYCODE_7,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 1 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "d D",      KEYCODE_D,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "q Q",      KEYCODE_Q,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESC",      KEYCODE_ESC,        IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "f F",      KEYCODE_F,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "r R",      KEYCODE_R,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "t T",      KEYCODE_T,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "j J",      KEYCODE_J,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 2 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "c C",      KEYCODE_C,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 @",      KEYCODE_2,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "z Z",      KEYCODE_Z,          IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CTRL",     KEYCODE_LCONTROL,   IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",      KEYCODE_4,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "b B",      KEYCODE_B,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &",      KEYCODE_6,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "m M",      KEYCODE_M,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 3 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "' \"",     KEYCODE_QUOTE,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ |",     KEYCODE_BACKSLASH,  IP_JOY_NONE) \
	PORT_BIT (0x20, 0x00, IPT_UNUSED) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- £",       KEYCODE_MINUS,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; :",      KEYCODE_COLON,      IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (",      KEYCODE_9,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "k K",      KEYCODE_K,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 4 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT",    KEYCODE_RIGHT,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN",     KEYCODE_DOWN,       IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT",     KEYCODE_LEFT,       IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LSHIFT",   KEYCODE_LSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP",       KEYCODE_UP,         IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",      KEYCODE_COMMA,      IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",      KEYCODE_STOP,       IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE",    KEYCODE_SPACE,      IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 5 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",      KEYCODE_OPENBRACE,  IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",      KEYCODE_CLOSEBRACE, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL",      KEYCODE_BACKSPACE,  IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "p P",      KEYCODE_P,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "o O",      KEYCODE_O,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "i I",      KEYCODE_I,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "u U",      KEYCODE_U,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 6 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "w W",      KEYCODE_W,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "s S",      KEYCODE_S,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "a A",      KEYCODE_A,          IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "e E",      KEYCODE_E,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "g G",      KEYCODE_G,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "h H",      KEYCODE_H,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "y Y",      KEYCODE_Y,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 7 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= +",      KEYCODE_EQUALS,     IP_JOY_NONE) \
	PORT_BIT (0x40, 0x00, IPT_UNUSED) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN",   KEYCODE_ENTER,      IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT",   KEYCODE_RSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",      KEYCODE_SLASH,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )",      KEYCODE_0,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "l L",      KEYCODE_L,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *",      KEYCODE_8,          IP_JOY_NONE)

#define INPUT_PORT_ORICA \
	PORT_START /* IN0 */ \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",         IP_KEY_NONE,        IP_JOY_NONE) \
 \
    PORT_START /* KEY ROW 0 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",      KEYCODE_3,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x X",      KEYCODE_X,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",      KEYCODE_1,          IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "v V",      KEYCODE_V,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",      KEYCODE_5,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "n N",      KEYCODE_N,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 ^",      KEYCODE_7,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 1 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "d D",      KEYCODE_D,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "q Q",      KEYCODE_Q,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ESC",      KEYCODE_ESC,        IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "f F",      KEYCODE_F,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "r R",      KEYCODE_R,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "t T",      KEYCODE_T,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "j J",      KEYCODE_J,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 2 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "c C",      KEYCODE_C,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 @",      KEYCODE_2,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "z Z",      KEYCODE_Z,          IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CTRL",     KEYCODE_LCONTROL,   IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",      KEYCODE_4,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "b B",      KEYCODE_B,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &",      KEYCODE_6,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "m M",      KEYCODE_M,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 3 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "' \"",     KEYCODE_QUOTE,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ |",     KEYCODE_BACKSLASH,  IP_JOY_NONE) \
	PORT_BIT (0x20, 0x00, IPT_UNUSED) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- £",       KEYCODE_MINUS,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; :",      KEYCODE_COLON,      IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (",      KEYCODE_9,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "k K",      KEYCODE_K,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 4 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT",    KEYCODE_RIGHT,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN",     KEYCODE_DOWN,       IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT",     KEYCODE_LEFT,       IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LSHIFT",   KEYCODE_LSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP",       KEYCODE_UP,         IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",      KEYCODE_COMMA,      IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",      KEYCODE_STOP,       IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE",    KEYCODE_SPACE,      IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 5 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ {",      KEYCODE_OPENBRACE,  IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] }",      KEYCODE_CLOSEBRACE, IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL",      KEYCODE_BACKSPACE,  IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "FUNCT",    KEYCODE_INSERT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "p P",      KEYCODE_P,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "o O",      KEYCODE_O,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "i I",      KEYCODE_I,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "u U",      KEYCODE_U,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 6 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "w W",      KEYCODE_W,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "s S",      KEYCODE_S,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "a A",      KEYCODE_A,          IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "e E",      KEYCODE_E,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "g G",      KEYCODE_G,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "h H",      KEYCODE_H,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "y Y",      KEYCODE_Y,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 7 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "= +",      KEYCODE_EQUALS,     IP_JOY_NONE) \
	PORT_BIT (0x40, 0x00, IPT_UNUSED) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN",   KEYCODE_ENTER,      IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT",   KEYCODE_RSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",      KEYCODE_SLASH,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 )",      KEYCODE_0,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "l L",      KEYCODE_L,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 *",      KEYCODE_8,          IP_JOY_NONE)

#define INPUT_PORT_PRAV8D \
	PORT_START /* IN0 */ \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "",                        IP_KEY_NONE,        IP_JOY_NONE) \
 \
    PORT_START /* KEY ROW 0 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 #",                     KEYCODE_3,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X (MYAGKEEY ZNAHK)",      KEYCODE_X,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 !",                     KEYCODE_1,          IP_JOY_NONE) \
        PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V (ZHEH)",                KEYCODE_V,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 %",                     KEYCODE_5,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N (EHN)",                 KEYCODE_N,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 '",                     KEYCODE_7,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 1 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D (DEH)",                 KEYCODE_D,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q (YAH)",                 KEYCODE_Q,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "OCB",                     KEYCODE_ESC,        IP_JOY_NONE) \
        PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F (EHF)",                 KEYCODE_F,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R (ERH)",                 KEYCODE_R,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T (TEH)",                 KEYCODE_T,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J (EE KRATKOYEH)",        KEYCODE_J,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 2 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C (TSEH)",                KEYCODE_C,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 \"",                    KEYCODE_2,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z (ZEH)",                 KEYCODE_Z,          IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MK",                      KEYCODE_LCONTROL,   IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 $",                     KEYCODE_4,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B (BEH)",                 KEYCODE_B,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 &",                     KEYCODE_6,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M (EHM)",                 KEYCODE_M,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 3 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "] (SCHYAH)",              KEYCODE_CLOSEBRACE, IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "; +",                     KEYCODE_EQUALS,     IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C/L",                     KEYCODE_INSERT,     IP_JOY_NONE) \
        PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, ": *",                     KEYCODE_COLON,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[ (SHAH)",                KEYCODE_OPENBRACE,  IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 )",                     KEYCODE_9,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K (KAH)",                 KEYCODE_K,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 4 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT",                   KEYCODE_RIGHT,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN",                    KEYCODE_DOWN,       IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT",                    KEYCODE_LEFT,       IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LSHIFT",                  KEYCODE_LSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP",                      KEYCODE_UP,         IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", <",                     KEYCODE_COMMA,      IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". >",                     KEYCODE_STOP,       IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE",                   KEYCODE_SPACE,      IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 5 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "@ (YOO)",                 KEYCODE_TILDE,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\ (EH)",                 KEYCODE_BACKSLASH,  IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL",                     KEYCODE_BACKSPACE,  IP_JOY_NONE) \
        PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P (PEH)",                 KEYCODE_P,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O (OH)",                  KEYCODE_O,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I (EE)",                  KEYCODE_I,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U (OO)",                  KEYCODE_U,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 6 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W (VEH)",                 KEYCODE_W,          IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S (EHS)",                 KEYCODE_S,          IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A (AH)",                  KEYCODE_A,          IP_JOY_NONE) \
	PORT_BIT (0x10, 0x00, IPT_UNUSED) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E (YEH)",                 KEYCODE_E,          IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G (GEH)",                 KEYCODE_G,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H (KHAH)",                KEYCODE_H,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y (TVYORDIY ZNAHK)",      KEYCODE_Y,          IP_JOY_NONE) \
 \
	PORT_START /* KEY ROW 7 */ \
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- =",                     KEYCODE_MINUS,      IP_JOY_NONE) \
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^ (CHEH)",                KEYCODE_HOME,       IP_JOY_NONE) \
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RETURN",                  KEYCODE_ENTER,      IP_JOY_NONE) \
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RSHIFT",                  KEYCODE_RSHIFT,     IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?",                     KEYCODE_SLASH,      IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0" ,                      KEYCODE_0,          IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L (EHL)",                 KEYCODE_L,          IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 (",                     KEYCODE_8,          IP_JOY_NONE)

INPUT_PORTS_START(oric)
	INPUT_PORT_ORIC
	PORT_START
	/* floppy interface  */
	PORT_DIPNAME( 0x07, 0x00, "Floppy disc interface" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "Microdisc" )
	PORT_DIPSETTING(    0x02, "Jasmin" )
/*	PORT_DIPSETTING(    0x03, "Low 8D DOS" ) */
/*	PORT_DIPSETTING(    0x04, "High 8D DOS" ) */

	/* vsync cable hardware. This is a simple cable connected to the video output
	to the monitor/television. The sync signal is connected to the cassette input
	allowing interrupts to be generated from the vsync signal. */
    PORT_BITX(0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Vsync cable hardware", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x8, DEF_STR( On) )
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_VBLANK)
INPUT_PORTS_END

INPUT_PORTS_START(orica)
	INPUT_PORT_ORICA
	PORT_START
	/* floppy interface  */
	PORT_DIPNAME( 0x07, 0x00, "Floppy disc interface" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "Microdisc" )
	PORT_DIPSETTING(    0x02, "Jasmin" )
/*	PORT_DIPSETTING(    0x03, "Low 8D DOS" ) */
/*	PORT_DIPSETTING(    0x04, "High 8D DOS" ) */

	/* vsync cable hardware. This is a simple cable connected to the video output
	to the monitor/television. The sync signal is connected to the cassette input
	allowing interrupts to be generated from the vsync signal. */
    PORT_BITX(0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Vsync cable hardware", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x8, DEF_STR( On) )
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_VBLANK)
INPUT_PORTS_END

INPUT_PORTS_START(prav8d)
	INPUT_PORT_PRAV8D
	/* force apple2 disc interface for pravetz */
	PORT_START
	PORT_DIPNAME( 0x07, 0x00, "Floppy disc interface" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x03, "Low 8D DOS" )
	PORT_DIPSETTING(    0x04, "High 8D DOS" )
	PORT_BITX(0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Vsync cable hardware", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x8, DEF_STR( On) )
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_VBLANK)
INPUT_PORTS_END

INPUT_PORTS_START(telstrat)
	INPUT_PORT_ORICA
	PORT_START
	/* vsync cable hardware. This is a simple cable connected to the video output
	to the monitor/television. The sync signal is connected to the cassette input
	allowing interrupts to be generated from the vsync signal. */
	PORT_BIT (0x07, 0x00, IPT_UNUSED)
	PORT_BITX(0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Vsync cable hardware", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x8, DEF_STR( On) )
	PORT_BIT( 0x010, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* left joystick port */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 UP", IP_KEY_NONE, JOYCODE_1_RIGHT)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 DOWN", IP_KEY_NONE, JOYCODE_1_LEFT)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 LEFT", IP_KEY_NONE, JOYCODE_1_BUTTON1)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 RIGHT", IP_KEY_NONE, JOYCODE_1_DOWN)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 0 FIRE 1", IP_KEY_NONE, JOYCODE_1_UP)
	/* right joystick port */
	PORT_START
	PORT_BITX(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 1 UP", IP_KEY_NONE, JOYCODE_2_RIGHT)
	PORT_BITX(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 1 DOWN", IP_KEY_NONE, JOYCODE_2_LEFT)
	PORT_BITX(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 1 LEFT", IP_KEY_NONE, JOYCODE_2_BUTTON1)
	PORT_BITX(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 1 RIGHT", IP_KEY_NONE, JOYCODE_2_DOWN)
	PORT_BITX(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD, "JOYSTICK 1 FIRE 1", IP_KEY_NONE, JOYCODE_2_UP)

INPUT_PORTS_END

static unsigned char oric_palette[8*3] = {
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
	0x00, 0xff, 0x00, 0xff, 0xff, 0x00,
	0x00, 0x00, 0xff, 0xff, 0x00, 0xff,
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static unsigned short oric_colortable[8] = {
	 0,1,2,3,4,5,6,7
};

/* Initialise the palette */
static PALETTE_INIT( oric )
{
	palette_set_colors(0, oric_palette, sizeof(oric_palette) / 3);
	memcpy(colortable, oric_colortable,sizeof(oric_colortable));
}

static struct Wave_interface wave_interface = {
	1,		/* 1 cassette recorder */
	{ 50 }	/* mixing levels in percent */
};

static struct AY8910interface oric_ay_interface =
{
	1,	/* 1 chips */
	1000000,	/* 1.0 MHz  */
	{ 25,25},
	{ 0 },
	{ 0 },
	{ oric_psg_porta_write },
	{ 0 }
};


static MACHINE_DRIVER_START( oric )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1000000)
	MDRV_CPU_MEMORY(oric_readmem,oric_writemem)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( oric )
	MDRV_MACHINE_STOP( oric )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*6, 28*8)
	MDRV_VISIBLE_AREA(0, 40*6-1, 0, 28*8-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(8)
	MDRV_PALETTE_INIT( oric )

	MDRV_VIDEO_START( oric )
	MDRV_VIDEO_UPDATE( oric )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, oric_ay_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( telstrat)
	MDRV_IMPORT_FROM( oric )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( telestrat_readmem,telestrat_writemem )

	MDRV_MACHINE_INIT( telestrat )
	MDRV_MACHINE_STOP( telestrat )
MACHINE_DRIVER_END


ROM_START(oric1)
	ROM_REGION(0x10000+0x04000+0x02000+0x0800,REGION_CPU1,0)
	ROM_LOAD ("basic10.rom", 0x10000, 0x4000, CRC(f18710b4))
	ROM_LOAD_OPTIONAL ("microdis.rom",0x014000, 0x02000, CRC(a9664a9c))
	ROM_LOAD_OPTIONAL ("jasmin.rom", 0x016000, 0x800, CRC(37220e89))
ROM_END

ROM_START(orica)
	ROM_REGION(0x10000+0x04000+0x02000+0x0800,REGION_CPU1,0)
	ROM_LOAD ("basic11b.rom", 0x10000, 0x4000, CRC(c3a92bef))
	ROM_LOAD_OPTIONAL ("microdis.rom",0x014000, 0x02000, CRC(a9664a9c))
	ROM_LOAD_OPTIONAL ("jasmin.rom", 0x016000, 0x800, CRC(37220e89))
ROM_END

ROM_START(telstrat)
	ROM_REGION(0x010000+(0x04000*4), REGION_CPU1,0)
	ROM_LOAD ("telmatic.rom", 0x010000, 0x02000, CRC(94358dc6))
	ROM_LOAD ("teleass.rom", 0x014000, 0x04000, CRC(68b0fde6))
	ROM_LOAD ("hyperbas.rom", 0x018000, 0x04000, CRC(1d96ab50))
	ROM_LOAD ("telmon24.rom", 0x01c000, 0x04000, CRC(aa727c5d))
ROM_END

ROM_START(prav8d)
    ROM_REGION (0x10000+0x4000+0x0100+0x200,REGION_CPU1,0)
    ROM_LOAD ("pravetzt.rom", 0x10000, 0x4000, CRC(58079502))
	ROM_LOAD_OPTIONAL ("8ddoslo.rom", 0x014000, 0x0100, CRC(0c82f636))
    ROM_LOAD_OPTIONAL ("8ddoshi.rom",0x014100, 0x0200, CRC(66309641))
ROM_END

ROM_START(prav8dd)
    ROM_REGION (0x10000+0x4000+0x0100+0x0200,REGION_CPU1,0)
    ROM_LOAD ("8d.rom", 0x10000, 0x4000, CRC(b48973ef))
    ROM_LOAD_OPTIONAL ("8ddoslo.rom", 0x014000, 0x0100, CRC(0c82f636))
    ROM_LOAD_OPTIONAL ("8ddoshi.rom",0x014100, 0x0200, CRC(66309641))
ROM_END

ROM_START(prav8dda)
    ROM_REGION (0x10000+0x4000+0x0100+0x200,REGION_CPU1,0)
    ROM_LOAD ("pravetzd.rom", 0x10000, 0x4000, CRC(f8d23821))
	ROM_LOAD_OPTIONAL ("8ddoslo.rom", 0x014000, 0x0100, CRC(0c82f636))
    ROM_LOAD_OPTIONAL ("8ddoshi.rom",0x014100, 0x0200, CRC(66309641))
ROM_END

SYSTEM_CONFIG_START(oric_common)
	CONFIG_DEVICE_CASSETTE(1, oric_cassette_formats)
	CONFIG_DEVICE_PRINTER(1)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(oric1)
	CONFIG_IMPORT_FROM(oric_common)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 4, "dsk\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE_OR_READ, device_init_oric_floppy, NULL, device_load_oric_floppy, NULL, floppy_status)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(prav8)
	CONFIG_IMPORT_FROM(oric_common)
	CONFIG_DEVICE_FLOPPY( 1, apple2 )
SYSTEM_CONFIG_END


/*    YEAR   NAME       PARENT	COMPAT	MACHINE     INPUT       INIT    CONFIG    COMPANY         FULLNAME */
COMP( 1983,  oric1,     0,      0,		oric,       oric,	    0,	    oric1,    "Tangerine",    "Oric 1" )
COMP( 1984,  orica,     oric1,	0,		oric,	    orica,	    0,	    oric1,    "Tangerine",    "Oric Atmos" )
COMP( 1985,  prav8d,    oric1,  0,		oric,       prav8d,     0,      prav8,    "Pravetz",      "Pravetz 8D")
COMPX( 1989, prav8dd,   oric1,  0,		oric,       prav8d,     0,      prav8,    "Pravetz",      "Pravetz 8D (Disk ROM)", GAME_COMPUTER_MODIFIED)
COMPX( 1992, prav8dda,  oric1,  0,		oric,       prav8d,     0,      prav8,    "Pravetz",      "Pravetz 8D (Disk ROM, RadoSoft)", GAME_COMPUTER_MODIFIED)
COMPX( 1986, telstrat,  oric1,  0,		telstrat,   telstrat,   0,      oric1,    "Tangerine",    "Oric Telestrat", GAME_NOT_WORKING )
