/***************************************************************************
	vtech2.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de> MESS driver, Jan 2000
	Davide Moretti <dave@rimini.com> ROM dump and hardware description

	LASER 350 (it has only 16K of RAM)
	FFFF|-------|
		| Empty |
		|	5	|
	C000|-------|
		|  RAM	|
		|	3	|
	8000|-------|-------|-------|
		|  ROM	|Display|  I/O	|
		|	1	|	3	|	2	|
	4000|-------|-------|-------|
		|  ROM	|
		|	0	|
	0000|-------|


	Laser 500/700 with 64K of RAM and
	Laser 350 with 64K RAM expansion module
	FFFF|-------|
		|  RAM	|
		|	5	|
	C000|-------|
		|  RAM	|
		|	4	|
	8000|-------|-------|-------|
		|  ROM	|Display|  I/O	|
		|	1	|	7	|	2	|
	4000|-------|-------|-------|
		|  ROM	|
		|	0	|
	0000|-------|


	Bank REGION_CPU1	   Contents
	0	 0x00000 - 0x03fff ROM 1st half
	1	 0x04000 - 0x07fff ROM 2nd half
	2			n/a 	   I/O 2KB area (mirrored 8 times?)
	3	 0x0c000 - 0x0ffff Display RAM (16KB) present in Laser 350 only!
	4	 0x10000 - 0x13fff RAM #4
	5	 0x14000 - 0x17fff RAM #5
	6	 0x18000 - 0x1bfff RAM #6
	7	 0x1c000 - 0x1ffff RAM #7 (Display RAM with 64KB)
	8	 0x20000 - 0x23fff RAM #8 (Laser 700 or 128KB extension)
	9	 0x24000 - 0x27fff RAM #9
	A	 0x28000 - 0x2bfff RAM #A
	B	 0x2c000 - 0x2ffff RAM #B
	C	 0x30000 - 0x33fff ROM expansion
	D	 0x34000 - 0x34fff ROM expansion
	E	 0x38000 - 0x38fff ROM expansion
	F	 0x3c000 - 0x3ffff ROM expansion

    TODO:
		Add ROMs and drivers for the Laser100, 110,
		210 and 310 machines and the Texet 8000.
		They should probably go to the vtech1.c files, though.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "devices/cartslot.h"
#include "includes/vtech2.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static MEMORY_READ_START( readmem )
	{ 0x00000, 0x03fff, MRA_BANK1 },
	{ 0x04000, 0x07fff, MRA_BANK2 },
	{ 0x08000, 0x0bfff, MRA_BANK3 },
	{ 0x0c000, 0x0ffff, MRA_BANK4 },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x00000, 0x03fff, MWA_BANK1 },
	{ 0x04000, 0x07fff, MWA_BANK2 },
	{ 0x08000, 0x0bfff, MWA_BANK3 },
	{ 0x0c000, 0x0ffff, MWA_BANK4 },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x10, 0x1f, laser_fdc_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x10, 0x1f, laser_fdc_w },
	{ 0x40, 0x43, laser_bank_select_w },
	{ 0x44, 0x44, laser_bg_mode_w },
	{ 0x45, 0x45, laser_two_color_w },
PORT_END

INPUT_PORTS_START( laser350 )
	PORT_START /* IN0 KEY ROW 0 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",       KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",           KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",           KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",           KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",           KEYCODE_V,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",           KEYCODE_B,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",           KEYCODE_N,          IP_JOY_NONE )

    PORT_START /* IN1 KEY ROW 1 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",        KEYCODE_LCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",           KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",           KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",           KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",           KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",           KEYCODE_G,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",           KEYCODE_H,          IP_JOY_NONE )

    PORT_START /* IN2 KEY ROW 2 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* TAB not on the Laser350 */
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",           KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",           KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",           KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",           KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",           KEYCODE_T,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",           KEYCODE_Y,          IP_JOY_NONE )

    PORT_START /* IN3 KEY ROW 3 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* ESC not on the Laser350 */
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !",         KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @",         KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #",         KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $",         KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %",         KEYCODE_5,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^",         KEYCODE_6,          IP_JOY_NONE )

	PORT_START /* IN4 KEY ROW 4 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "= +",         KEYCODE_EQUALS,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "- _",         KEYCODE_MINUS,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )",         KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (",         KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 *",         KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 &",         KEYCODE_7,          IP_JOY_NONE )

	PORT_START /* IN5 KEY ROW 5 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* BS not on the Laser350 */
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",           KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",           KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",           KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",           KEYCODE_U,          IP_JOY_NONE )

	PORT_START /* IN6 KEY ROW 6 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN",      KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "' \"",        KEYCODE_QUOTE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :",         KEYCODE_COLON,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",           KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",           KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",           KEYCODE_J,          IP_JOY_NONE )

	PORT_START /* IN7 KEY ROW 7 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* GRAPH not on the Laser350 */
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "` ~",         KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",       KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?",         KEYCODE_SLASH,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >",         KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <",         KEYCODE_COMMA,      IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",           KEYCODE_M,          IP_JOY_NONE )

    PORT_START /* IN8 KEY ROW A */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN9 KEY ROW B */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN10 KEY ROW C */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN11 KEY ROW D */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) /* not on the Laser350 */

	PORT_START /* IN12 Tape control */
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape start",         KEYCODE_SLASH_PAD, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape stop",          KEYCODE_ASTERISK,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape rewind",        KEYCODE_MINUS_PAD, IP_JOY_NONE )
	PORT_BIT (0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END

INPUT_PORTS_START( laser500 )
	PORT_START /* IN0 KEY ROW 0 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT",       KEYCODE_LSHIFT,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",           KEYCODE_Z,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",           KEYCODE_X,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",           KEYCODE_C,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",           KEYCODE_V,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",           KEYCODE_B,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",           KEYCODE_N,          IP_JOY_NONE )

    PORT_START /* IN1 KEY ROW 1 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",        KEYCODE_LCONTROL,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",           KEYCODE_A,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",           KEYCODE_S,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",           KEYCODE_D,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",           KEYCODE_F,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",           KEYCODE_G,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",           KEYCODE_H,          IP_JOY_NONE )

    PORT_START /* IN2 KEY ROW 2 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB",         KEYCODE_TAB,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",           KEYCODE_Q,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",           KEYCODE_W,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",           KEYCODE_E,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",           KEYCODE_R,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",           KEYCODE_T,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",           KEYCODE_Y,          IP_JOY_NONE )

    PORT_START /* IN3 KEY ROW 3 */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC",         KEYCODE_ESC,        IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !",         KEYCODE_1,          IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @",         KEYCODE_2,          IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #",         KEYCODE_3,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $",         KEYCODE_4,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %",         KEYCODE_5,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^",         KEYCODE_6,          IP_JOY_NONE )

	PORT_START /* IN4 KEY ROW 4 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "= +",         KEYCODE_EQUALS,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "- _",         KEYCODE_MINUS,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )",         KEYCODE_0,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (",         KEYCODE_9,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 *",         KEYCODE_8,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 &",         KEYCODE_7,          IP_JOY_NONE )

	PORT_START /* IN5 KEY ROW 5 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "BS",          KEYCODE_BACKSPACE,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",           KEYCODE_P,          IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",           KEYCODE_O,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",           KEYCODE_I,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",           KEYCODE_U,          IP_JOY_NONE )

	PORT_START /* IN6 KEY ROW 6 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN",      KEYCODE_ENTER,      IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "' \"",        KEYCODE_QUOTE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :",         KEYCODE_COLON,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",           KEYCODE_L,          IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",           KEYCODE_K,          IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",           KEYCODE_J,          IP_JOY_NONE )

	PORT_START /* IN7 KEY ROW 7 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRAPH",       KEYCODE_LALT,       IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "` ~",         KEYCODE_TILDE,      IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",       KEYCODE_SPACE,      IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ ?",         KEYCODE_SLASH,      IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >",         KEYCODE_STOP,       IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <",         KEYCODE_COMMA,      IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",           KEYCODE_M,          IP_JOY_NONE )

    PORT_START /* IN8 KEY ROW A */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",          KEYCODE_F1,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",          KEYCODE_F2,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",          KEYCODE_F3,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",          KEYCODE_F4,         IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN9 KEY ROW B */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F10",         KEYCODE_F10,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "F9",          KEYCODE_F9,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F8",          KEYCODE_F8,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "F7",          KEYCODE_F7,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6",          KEYCODE_F6,         IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",          KEYCODE_F5,         IP_JOY_NONE )

	PORT_START /* IN10 KEY ROW C */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS",        KEYCODE_CAPSLOCK,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "DLINE",       KEYCODE_PGUP,       IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOME",        KEYCODE_HOME,       IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP",          KEYCODE_UP,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT",        KEYCODE_LEFT,       IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT",       KEYCODE_RIGHT,      IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN",        KEYCODE_DOWN,       IP_JOY_NONE )

	PORT_START /* IN11 KEY ROW D */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\ |",        KEYCODE_BACKSLASH,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "] }",         KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "[ {",         KEYCODE_OPENBRACE,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "æ",           KEYCODE_ASTERISK,   IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL",         KEYCODE_DEL,        IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "INS",         KEYCODE_INSERT,     IP_JOY_NONE )

	PORT_START /* IN12 Tape control */
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape start",         KEYCODE_SLASH_PAD, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape stop",          KEYCODE_ASTERISK,  IP_JOY_NONE )
    PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tape rewind",        KEYCODE_MINUS_PAD, IP_JOY_NONE )
	PORT_BIT (0x1f, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END

static struct GfxLayout charlayout_80 =
{
	8,8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 					/* every char takes 8 bytes */
};

static struct GfxLayout charlayout_40 =
{
	8*2,8,					/* 8*2 x 8 characters */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 					/* every char takes 8 bytes */
};

static struct GfxLayout gfxlayout_1bpp =
{
	8,1,					/* 8x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,6,5,4,3,2,1,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static struct GfxLayout gfxlayout_1bpp_dw =
{
	8*2,1,					/* 8 times 2x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,7,6,6,5,5,4,4,3,3,2,2,1,1,0,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static struct GfxLayout gfxlayout_1bpp_qw =
{
	8*4,1,					/* 8 times 4x1 pixels */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0,0 },
	/* y offsets */
	{ 0 },
	8						/* one byte per code */
};

static struct GfxLayout gfxlayout_4bpp =
{
	2*4,1,					/* 2 times 4x1 pixels */
	256,					/* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* four bitplanes */
	/* x offsets */
	{ 4,4,4,4, 0,0,0,0 },
	/* y offsets */
	{ 0 },
	2*4 					/* one byte per code */
};

static struct GfxLayout gfxlayout_4bpp_dh =
{
	2*4,2,					/* 2 times 4x2 pixels */
	256,					/* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* four bitplanes */
	/* x offsets */
	{ 4,4,4,4, 0,0,0,0 },
	/* y offsets */
	{ 0,0 },
	2*4 					/* one byte per code */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout_80,		  0, 256 },
	{ REGION_GFX1, 0, &charlayout_40,		  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp,		  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp_dw,	  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_1bpp_qw,	  0, 256 },
	{ REGION_GFX2, 0, &gfxlayout_4bpp,	  2*256,   1 },
	{ REGION_GFX2, 0, &gfxlayout_4bpp_dh, 2*256,   1 },
	{ -1 } /* end of array */
};


static unsigned char vt_palette[] =
{
      0,  0,  0,    /* black */
      0,  0,127,    /* blue */
      0,127,  0,    /* green */
      0,127,127,    /* cyan */
    127,  0,  0,    /* red */
    127,  0,127,    /* magenta */
    127,127,  0,    /* yellow */
	160,160,160,	/* bright grey */
    127,127,127,    /* dark grey */
      0,  0,255,    /* bright blue */
      0,255,  0,    /* bright green */
      0,255,255,    /* bright cyan */
    255,  0,  0,    /* bright red */
    255,  0,255,    /* bright magenta */
    255,255,  0,    /* bright yellow */
    255,255,255,    /* bright white */
};

/* Initialise the palette */
static PALETTE_INIT( vtech2 )
{
	int i;

	palette_set_colors(0, vt_palette, sizeof(vt_palette) / 3);

	for (i = 0; i < 256; i++)
	{
		colortable[2*i] = i%16;
		colortable[2*i+1] = i/16;
	}
	for (i = 0; i < 16; i++)
		colortable[2*256+i] = i;
}

static struct Speaker_interface speaker_interface =
{
	1,		/* number of speakers */
	{ 75 }	/* volume */
};

static struct Wave_interface wave_interface = {
	1,
	{ 50 }
};

static mess_image *cassette_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

static INTERRUPT_GEN( vtech2_interrupt )
{
	int tape_control = readinputport(12);
	if( tape_control & 0x80 )
		device_status(cassette_image(), 1);
	if( tape_control & 0x40 )
		device_status(cassette_image(), 0);
	if( tape_control & 0x20 )
		device_seek(cassette_image(), 0, SEEK_SET);

	cpu_set_irq_line(0, 0, PULSE_LINE);
}

static MACHINE_DRIVER_START( laser350 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3694700)        /* 3.694700 Mhz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(vtech2_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( laser350 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(88*8, 24*8+32)
	MDRV_VISIBLE_AREA(0*8, 88*8-1, 0*8, 24*8+32-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(vt_palette)/sizeof(vt_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(256*2+16)
	MDRV_PALETTE_INIT(vtech2)

	MDRV_VIDEO_START(laser)
	MDRV_VIDEO_UPDATE(laser)

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser500 )
	MDRV_IMPORT_FROM( laser350 )
	MDRV_MACHINE_INIT( laser500 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser700 )
	MDRV_IMPORT_FROM( laser350 )
	MDRV_MACHINE_INIT( laser700 )
MACHINE_DRIVER_END


ROM_START(laser350)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000,CRC( 0x9bed01f7))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800,CRC( 0xed6bfb2a))
	ROM_REGION(0x00100,REGION_GFX2,0)
    /* initialized in init_laser */
ROM_END


ROM_START(laser500)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000,CRC( 0x9bed01f7))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800,CRC( 0xed6bfb2a))
	ROM_REGION(0x00100,REGION_GFX2,0)
	/* initialized in init_laser */
ROM_END

ROM_START(laser700)
	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("laserv3.rom", 0x00000, 0x08000,CRC( 0x9bed01f7))
	ROM_REGION(0x00800,REGION_GFX1,0)
	ROM_LOAD("laser.fnt",   0x00000, 0x00800,CRC( 0xed6bfb2a))
	ROM_REGION(0x00100,REGION_GFX2,0)
	/* initialized in init_laser */
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

SYSTEM_CONFIG_START(laser)
	CONFIG_DEVICE_CASSETTE(1, "cas\0", device_load_laser_cassette)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "rom\0", NULL, NULL, device_load_laser_cart, device_unload_laser_cart, NULL, NULL)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2, "dsk\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_laser_floppy, NULL, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	 NAME	   PARENT	 COMPAT	MACHINE   INPUT	 INIT	   CONFIG	COMPANY	 FULLNAME */
COMP( 1984?, laser350, 0,		 0,		laser350, laser350, laser,    laser,	"Video Technology",  "Laser 350" )
COMP( 1984?, laser500, laser350, 0,		laser500, laser500, laser,    laser,	"Video Technology",  "Laser 500" )
COMP( 1984?, laser700, laser350, 0,		laser700, laser500, laser,    laser,	"Video Technology",  "Laser 700" )
