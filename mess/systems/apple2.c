/***************************************************************************

Apple II

This family of computers bank-switches everything up the wazoo.

Remarkable features
-------------------

Apple II (original model)
-------------------------

RAM: 4/8/12/16/20/24/32/36/48 KB (according to the manual)

ROM: 8 KB mapped to $E000-$FFFF
Empty ROM sockets mapped at $D000-$D7FF (usually occupied by Programmer's
Aid #1 chip) and $D800-$DFFF (usually empty, but a couple of 3rd party
chips were produced)

HI-RES Palette has only 4 colors: 0 - black, 1 - green, 2 - purple,
3 - white

Due to an hardware bug, green/purple artifacts are present in text mode
too!

No 80 columns
No Open/Solid Apple keys
No Up/Down arrows key

Users often connected the SHIFT key to the paddle #2 button (mapped to
$C063) in order to inform properly written software that characters were
to be intended upper/lower case

*** TODO: Should MESS emulate this via a dipswitch?

Integer BASIC in ROM, AppleSoft must be loaded from disk or tape

No AutoStart ROM: once the machine was switched on, the user had to manually
perform the reset cycle pressing, guess what, RESET ;)

Apple II+
---------

RAM: 16/32/48 KB + extra 16 KB at $C000 if using Apple Language Card
ROM: 12 KB mapped to $D000-$DFFF

HI-RES Palette has four more entries: 4 - black (again), 5 - orange,
6 - blue, 7 - white (again)

No more artifact bug in text mode

No 80 columns
No Open/Solid Apple keys
NO Up/Down arrows keys

Users still did the SHIFT key mod

AppleSoft BASIC in ROM

AutoStart ROM - no more need to press RESET after switching the machine on

Apple IIe
---------

RAM: 64 KB + optional bank of 64 KB (see 80 columns card)
ROM: 16 KB

80 columns card: this card was available in two versions - one equipped
with 1 KB of memory to provide the extra RAM for the display, the other
equipped with full 64 KB of RAM - the 80 columns card is not included
in the standard configuration and is available as add-on.

Open/Solid Apple keys mapped to buttons 0 and 1 of the paddle #1
Up/Down arrows keys
Connector for an optional numeric keypad

Apple begins manifacturing its machines with the SHIFT key mod

Revision A motherboards cannot handle double-hires graphics, Revision B can

*** TODO: Should MESS emulate this via a dipswitch?

Apple IIe (enhanced)
--------------------

The enhancement consists in bugfix of the ROM code, a 65c02 instead of the
6502 and a change in the character generator ROM which now includes the
so called "MouseText" characters (thus, no flashing characters in 80
columns mode)

Double hi-res mode is supported

Apple IIe (Platinum)
--------------------

Identical to IIe enhanced except for:

The numerical keypad is integrated into the main keyboard (although the
internal connector is still present)

The CLEAR key on the keypad generates the same character of the ESC key,
but some users did an hardware modification so that it generates CTRL-X

*** TODO: Should MESS emulate this via a dipswitch?

The 64 KB 80 columns card is built in

Due to the SHIFT key mod, if the user press both SHIFT and the paddle
button where the shift key was connected, a short circuit is caused
and the power supply is shut down!

Apple IIc
---------

Same as IIe enhanced (Rev B) except for:

There are no slots in hardware. The machine however sees (for compatibility
reasons):

Two Super Serial Cards in slots 1-2
80 columns card (64 KB version) in slot 3
Mouse in slot 4
Easter Egg in slot 5 (!)
Disk II in slot 6
External 5.25 drive in slot 7

MouseText characters

No numerical keypad

Switchables keyboard layouts (the user, via an external switch, can choose
between two layouts, i.e. US and German, and in the USA QWERTY and Dvorak)

*** TODO: Should MESS emulate this?

Apple IIc (UniDisk 3.5)
-----------------------

Identical to IIc except for:

ROM: 32 KB

The disk firmware can handle up to four 3.5 disk drives or three 3.5 drives
and a 5.25 drive

Preliminary support (but not working and never completed) for AppleTalk
network in slot 7

Apple IIc (Original Memory Expansion)
-------------------------------------

ROMSET NOT DUMPED

Identical to IIc except for:

Support for Memory Expansion Board (mapped to slot 4)
This card can provide up to 1 MB of RAM in increments of 256 MB
The firmware in ROM sees the extra RAM as a RAMdisk

Since the expansion is mapped to slot 4, mouse is now mapped to slot 7

Apple IIc (Revised Memory Expansion)
------------------------------------

ROMSET NOT DUMPED

Identical to IIc (OME) except for bugfixes

Apple IIc Plus
--------------

Identical to IIc (RME) except for:

The 65c02 works at 4MHz

The machine has an internal "Apple 3.5" drive (which is DIFFERENT from the
UniDisk 3.5 drive!)

The external drive port supports not only 5.25 drives but also UniDisk and
Apple 3.5 drives, allowing via daisy-chaining any combination of UniDisk,
Apple 3.5 and Apple 5.25 drives - up to three devices

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"
#include "inputx.h"
#include "snprintf.h"

static MEMORY_READ_START( readmem_apple2 )
	{ 0x0000, 0x01ff, MRA_BANK4 },
	{ 0x0200, 0x03ff, MRA_BANK5 },
	{ 0x0400, 0x07ff, MRA_BANK7 },
	{ 0x0800, 0x1fff, MRA_BANK8 },
	{ 0x2000, 0x3fff, MRA_BANK9 },
	{ 0x4000, 0xbfff, MRA_BANK10 },
	{ 0xc000, 0xc00f, apple2_c00x_r },
	{ 0xc010, 0xc01f, apple2_c01x_r },
	{ 0xc020, 0xc02f, apple2_c02x_r },
	{ 0xc030, 0xc03f, apple2_c03x_r },
	{ 0xc040, 0xc04f, MRA_NOP },
	{ 0xc050, 0xc05f, apple2_c05x_r },
	{ 0xc060, 0xc06f, apple2_c06x_r },
	{ 0xc070, 0xc07f, apple2_c07x_r },
	{ 0xc080, 0xc08f, apple2_c08x_r },
	{ 0xc090, 0xc09f, apple2_c0xx_slot1_r },
	{ 0xc0a0, 0xc0af, apple2_c0xx_slot2_r },
	{ 0xc0b0, 0xc0bf, apple2_c0xx_slot3_r },
	{ 0xc0c0, 0xc0cf, apple2_c0xx_slot4_r },
	{ 0xc0d0, 0xc0df, apple2_c0xx_slot5_r },
	{ 0xc0e0, 0xc0ef, apple2_c0xx_slot6_r },
	{ 0xc0f0, 0xc0ff, apple2_c0xx_slot7_r },
	{ 0xc400, 0xc4ff, apple2_slot4_r },
	{ 0xc100, 0xc2ff, MRA_BANK3 },
	{ 0xc300, 0xc3ff, MRA_BANK11 },
	{ 0xc400, 0xc7ff, MRA_BANK12 },
	{ 0xc800, 0xcfff, MRA_BANK6 },
	{ 0xd000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_BANK2 },
MEMORY_END

static MEMORY_WRITE_START( writemem_apple2 )
	{ 0x0000, 0x01ff, MWA_BANK4 },
	{ 0x0200, 0x03ff, MWA_BANK5 },
	{ 0x0400, 0x07ff, MWA_BANK7 },
	{ 0x0800, 0x1fff, MWA_BANK8 },
	{ 0x2000, 0x3fff, MWA_BANK9 },
	{ 0x4000, 0xbfff, MWA_BANK10 },
	{ 0xc000, 0xc00f, apple2_c00x_w },
	{ 0xc010, 0xc01f, apple2_c01x_w },
	{ 0xc020, 0xc02f, apple2_c02x_w },
	{ 0xc030, 0xc03f, apple2_c03x_w },
	{ 0xc040, 0xc04f, MWA_NOP },
	{ 0xc050, 0xc05f, apple2_c05x_w },
	{ 0xc060, 0xc06f, MWA_NOP },
	{ 0xc070, 0xc07f, apple2_c07x_w },
	{ 0xc080, 0xc08f, apple2_c08x_w },
	{ 0xc090, 0xc09f, apple2_c0xx_slot1_w },
	{ 0xc0a0, 0xc0af, apple2_c0xx_slot2_w },
	{ 0xc0b0, 0xc0bf, apple2_c0xx_slot3_w },
	{ 0xc0c0, 0xc0cf, apple2_c0xx_slot4_w },
	{ 0xc0d0, 0xc0df, apple2_c0xx_slot5_w },
	{ 0xc0e0, 0xc0ef, apple2_c0xx_slot6_w },
	{ 0xc0f0, 0xc0ff, apple2_c0xx_slot7_w },
	{ 0xc100, 0xc2ff, MWA_BANK3 },
	{ 0xc300, 0xc3ff, MWA_BANK11 },
	{ 0xc400, 0xc7ff, MWA_BANK12 },
	{ 0xd000, 0xdfff, MWA_BANK1 },
	{ 0xe000, 0xffff, MWA_BANK2 },
MEMORY_END

/**************************************************************************
***************************************************************************/

#define JOYSTICK_DELTA			80
#define JOYSTICK_SENSITIVITY	100

INPUT_PORTS_START( apple2 )

    PORT_START /* [0] VBLANK */
    PORT_BIT ( 0xBF, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )

    PORT_START /* [1] KEYS #1 */
    PORT_KEY1( 0x01, IP_ACTIVE_HIGH, "Backsp ", 		KEYCODE_BACKSPACE,   IP_JOY_NONE, 8 )
    PORT_KEY0( 0x02, IP_ACTIVE_HIGH, "Left   ", 		KEYCODE_LEFT,        IP_JOY_NONE )
    PORT_KEY1( 0x04, IP_ACTIVE_HIGH, "Tab    ", 		KEYCODE_TAB,         IP_JOY_NONE, 9 )
    PORT_KEY1( 0x08, IP_ACTIVE_HIGH, "Down   ", 		KEYCODE_DOWN,        IP_JOY_NONE, 10 )
    PORT_KEY0( 0x10, IP_ACTIVE_HIGH, "Up     ", 		KEYCODE_UP,          IP_JOY_NONE )
    PORT_KEY1( 0x20, IP_ACTIVE_HIGH, "Enter  ", 		KEYCODE_ENTER,       IP_JOY_NONE, 13 )
    PORT_KEY0( 0x40, IP_ACTIVE_HIGH, "Right  ", 		KEYCODE_RIGHT,       IP_JOY_NONE )
    PORT_KEY1( 0x80, IP_ACTIVE_HIGH, "Escape ",			KEYCODE_ESC,         IP_JOY_NONE, 27 )

    PORT_START /* [2] KEYS #2 */
    PORT_KEY1( 0x01, IP_ACTIVE_HIGH, "Space  ", 		KEYCODE_SPACE,		IP_JOY_NONE, ' ' )
    PORT_KEY1( 0x02, IP_ACTIVE_HIGH, "'      ", 		KEYCODE_QUOTE,		IP_JOY_NONE, '\'' )
    PORT_KEY1( 0x04, IP_ACTIVE_HIGH, ",      ", 		KEYCODE_COMMA,		IP_JOY_NONE, ',' )
    PORT_KEY1( 0x08, IP_ACTIVE_HIGH, "-      ", 		KEYCODE_MINUS,		IP_JOY_NONE, '-' )
    PORT_KEY1( 0x10, IP_ACTIVE_HIGH, ".      ", 		KEYCODE_STOP,		IP_JOY_NONE, '.' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "/ ?    ", 		KEYCODE_SLASH,		IP_JOY_NONE, '/', '?' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "0      ", 		KEYCODE_0,			IP_JOY_NONE, '0', ')' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "1      ", 		KEYCODE_1,			IP_JOY_NONE, '1', '!' )

    PORT_START /* [3] KEYS #3 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "2      ", 		KEYCODE_2,			IP_JOY_NONE, '2', '\"' )
	PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "3      ", 		KEYCODE_3,			IP_JOY_NONE, '3', '#' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "4      ", 		KEYCODE_4,			IP_JOY_NONE, '4', '$' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "5      ", 		KEYCODE_5,			IP_JOY_NONE, '5', '%' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "6      ", 		KEYCODE_6,			IP_JOY_NONE, '6', '^' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "7      ", 		KEYCODE_7,			IP_JOY_NONE, '7', '&' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "8      ", 		KEYCODE_8,			IP_JOY_NONE, '8', '*' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "9      ",			KEYCODE_9,			IP_JOY_NONE, '9', '(' )

    PORT_START /* [4] KEYS #4 */
	PORT_KEY1( 0x01, IP_ACTIVE_HIGH, ":      ", 		KEYCODE_COLON,		IP_JOY_NONE, ':' )
    PORT_KEY1( 0x02, IP_ACTIVE_HIGH, "=      ", 		KEYCODE_EQUALS,		IP_JOY_NONE, '=' )
	PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "[ {    ", 		KEYCODE_OPENBRACE,	IP_JOY_NONE, '[', '{' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "\\ |    ", 		KEYCODE_BACKSLASH,	IP_JOY_NONE, '\\', '|' )
	PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "] }    ",			KEYCODE_CLOSEBRACE,	IP_JOY_NONE, ']', '}' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "^ ~    ",			KEYCODE_TILDE,		IP_JOY_NONE, '^', '~' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "a A    ", 		KEYCODE_A,			IP_JOY_NONE, 'a', 'A' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "b B    ", 		KEYCODE_B,			IP_JOY_NONE, 'b', 'B' )

    PORT_START /* [5] KEYS #5 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "c C    ", 		KEYCODE_C,			IP_JOY_NONE, 'c', 'C' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "d D    ", 		KEYCODE_D,			IP_JOY_NONE, 'd', 'D' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "e E    ",			KEYCODE_E,			IP_JOY_NONE, 'e', 'E' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "f F    ", 		KEYCODE_F,			IP_JOY_NONE, 'f', 'F' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "g G    ", 		KEYCODE_G,			IP_JOY_NONE, 'g', 'G' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "h H    ", 		KEYCODE_H,			IP_JOY_NONE, 'h', 'H' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "i I    ", 		KEYCODE_I,			IP_JOY_NONE, 'i', 'I' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "j J    ", 		KEYCODE_J,			IP_JOY_NONE, 'j', 'J' )

    PORT_START /* [6] KEYS #6 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "k K    ",			KEYCODE_K,			IP_JOY_NONE, 'k', 'K' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "l L    ",			KEYCODE_L,			IP_JOY_NONE, 'l', 'L' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "m M    ", 		KEYCODE_M,			IP_JOY_NONE, 'm', 'M' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "n N    ", 		KEYCODE_N,			IP_JOY_NONE, 'n', 'N' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "o O    ", 		KEYCODE_O,			IP_JOY_NONE, 'o', 'O' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "p P    ", 		KEYCODE_P,			IP_JOY_NONE, 'p', 'P' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "q Q    ", 		KEYCODE_Q,			IP_JOY_NONE, 'q', 'Q' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "r R    ", 		KEYCODE_R,			IP_JOY_NONE, 'r', 'R' )

    PORT_START /* [7] KEYS #7 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "s S    ", 		KEYCODE_S,			IP_JOY_NONE, 's', 'S' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "t T    ", 		KEYCODE_T,			IP_JOY_NONE, 't', 'T' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "u U    ", 		KEYCODE_U,			IP_JOY_NONE, 'u', 'U' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "v V    ", 		KEYCODE_V,			IP_JOY_NONE, 'v', 'V' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "w W    ", 		KEYCODE_W,			IP_JOY_NONE, 'w', 'W' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "x X    ", 		KEYCODE_X,			IP_JOY_NONE, 'x', 'X' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "y Y    ", 		KEYCODE_Y,			IP_JOY_NONE, 'y', 'Y' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "z Z    ",			KEYCODE_Z,			IP_JOY_NONE, 'z', 'Z' )

    PORT_START /* [8] Special keys */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_DEFAULT )
    PORT_KEY1( 0x02, IP_ACTIVE_HIGH, "Left Shift",		KEYCODE_LSHIFT,		IP_JOY_NONE, UCHAR_SHIFT_1 )
    PORT_KEY1( 0x04, IP_ACTIVE_HIGH, "Right Shift",		KEYCODE_RSHIFT,		IP_JOY_NONE, UCHAR_SHIFT_1 )
    PORT_KEY1( 0x08, IP_ACTIVE_HIGH, "Left Control",	KEYCODE_LCONTROL,	IP_JOY_NONE, UCHAR_SHIFT_2 )
	PORT_KEY0( 0x10, IP_ACTIVE_HIGH, "Open Apple",		KEYCODE_LALT,		IP_JOY_NONE)
	PORT_KEY0( 0x20, IP_ACTIVE_HIGH, "Closed Apple",	KEYCODE_RALT,		IP_JOY_NONE)
    PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1,	"Button 0",	IP_KEY_DEFAULT,	IP_JOY_DEFAULT )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2,	"Button 1",	IP_KEY_DEFAULT,	IP_JOY_DEFAULT )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3,	"Button 2",	IP_KEY_DEFAULT,	IP_JOY_DEFAULT )
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Reset",	KEYCODE_F3,		IP_JOY_NONE )

	PORT_START /* [9] Joystick 1 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_4_PAD, KEYCODE_6_PAD, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* [10] Joystick 1 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_8_PAD, KEYCODE_2_PAD, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* [11] Joystick 2 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_4_PAD, KEYCODE_6_PAD, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* [12] Joystick 2 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_8_PAD, KEYCODE_2_PAD, JOYCODE_1_UP, JOYCODE_1_DOWN)
INPUT_PORTS_END

/* according to Steve Nickolas (author of Dapple), our original palette would
 * have been more appropriate for an Apple IIgs.  So we've substituted in the
 * Robert Munafo palette instead, which is more accurate on 8-bit Apples
 */
static unsigned char apple2_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xE3, 0x1E, 0x60,	/* Dark Red */
	0x60, 0x4E, 0xBD,	/* Dark Blue */
	0xFF, 0x44, 0xFD,	/* Purple */
	0x00, 0xA3, 0x60,	/* Dark Green */
	0x9C, 0x9C, 0x9C,	/* Dark Gray */
	0x14, 0xCF, 0xFD,	/* Medium Blue */
	0xD0, 0xC3, 0xFF,	/* Light Blue */
	0x60, 0x72, 0x03,	/* Brown */
	0xFF, 0x6A, 0x3C,	/* Orange */
	0x9C, 0x9C, 0x9C,	/* Light Grey */
	0xFF, 0xA0, 0xD0,	/* Pink */
	0x14, 0xF5, 0x3C,	/* Light Green */
	0xD0, 0xDD, 0x8D,	/* Yellow */
	0x72, 0xFF, 0xD0,	/* Aquamarine */
	0xFF, 0xFF, 0xFF	/* White */
};

#if 0
static unsigned char apple2gs_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xD0, 0x00, 0x30,	/* Dark Red */
	0x00, 0x00, 0x90,	/* Dark Blue */
	0xD0, 0x20, 0xD0,	/* Purple */
	0x00, 0x70, 0x20,	/* Dark Green */
	0x50, 0x50, 0x50,	/* Dark Grey */
	0x20, 0x20, 0xF0,	/* Medium Blue */
	0x60, 0xA0, 0xF0,	/* Light Blue */
	0x80, 0x50, 0x00,	/* Brown */
	0xF0, 0x60, 0x00,	/* Orange */
	0xA0, 0xA0, 0xA0,	/* Light Grey */
	0xF0, 0x90, 0x80,	/* Pink */
	0x10, 0xD0, 0x00,	/* Light Green */
	0xF0, 0xF0, 0x00,	/* Yellow */
	0x40, 0xF0, 0x90,	/* Aquamarine */
	0xF0, 0xF0, 0xF0	/* White */
};
#endif

static unsigned short apple2_colortable[] =
{
    0,0,
    1,0,
    2,0,
    3,0,
    4,0,
    5,0,
    6,0,
    7,0,
    8,0,
    9,0,
    10,0,
    11,0,
    12,0,
    13,0,
    14,0,
    15,0,
};


/* Initialise the palette */
static PALETTE_INIT( apple2 )
{
	palette_set_colors(0, apple2_palette, sizeof(apple2_palette) / 3);
    memcpy(colortable,apple2_colortable,sizeof(apple2_colortable));
}

static GET_CUSTOM_DEVICENAME( apple2 )
{
	const char *name = NULL;
	switch(image_type(img)) {
	case IO_FLOPPY:
		snprintf(buf, bufsize, "Slot 6 Disk #%d", image_index(img) + 1);
		name = buf;
		break;
	}
	return name;
}

static struct DACinterface apple2_DAC_interface =
{
    1,          /* number of DACs */
    { 100 }     /* volume */
};

static struct AY8910interface ay8910_interface =
{
    2,  /* 2 chips */
    1022727,    /* 1.023 MHz */
    { 100, 100 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};

static MACHINE_DRIVER_START( apple2e )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
	MDRV_CPU_MEMORY(readmem_apple2, writemem_apple2)
	MDRV_CPU_VBLANK_INT(apple2_interrupt, 192/8)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( apple2 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(280*2, 192)
	MDRV_VISIBLE_AREA(0, (280*2)-1,0,192-1)
	MDRV_PALETTE_LENGTH(sizeof(apple2_palette)/3)
	MDRV_COLORTABLE_LENGTH(sizeof(apple2_colortable)/sizeof(unsigned short))
	MDRV_PALETTE_INIT(apple2)

	MDRV_VIDEO_START(apple2e)
	MDRV_VIDEO_UPDATE(apple2)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, apple2_DAC_interface)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apple2ee )
	MDRV_IMPORT_FROM( apple2e )
	MDRV_CPU_REPLACE("main", M65C02, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apple2c )
	MDRV_IMPORT_FROM( apple2ee )
	MDRV_VIDEO_START( apple2c )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2)
	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD_OPTIONAL ( "progaid1.rom", 0x1000, 0x0800, 0x4234e88a )
	
/* The area $D800-$DFFF in Apple II is reserved for 3rd party add-ons:
   Maybe MESS should map this space to a CARTSLOT device?              */

	ROM_LOAD ( "a2.e0", 0x2000, 0x0800, 0xc0a4ad3b )
	ROM_LOAD ( "a2.e8", 0x2800, 0x0800, 0xa99c2cf6 )
	ROM_LOAD ( "a2.f0", 0x3000, 0x0800, 0x62230d38 )
	ROM_LOAD ( "a2.f8", 0x3800, 0x0800, 0x020a86d0 )
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2p)
	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ( "a2p.d0", 0x1000, 0x0800, 0x6f05f949 )
	ROM_LOAD ( "a2p.d8", 0x1800, 0x0800, 0x1f08087c )
	ROM_LOAD ( "a2p.e0", 0x2000, 0x0800, 0x2b8d9a89 )
	ROM_LOAD ( "a2p.e8", 0x2800, 0x0800, 0x5719871a )
	ROM_LOAD ( "a2p.f0", 0x3000, 0x0800, 0x9a04eecf )
	ROM_LOAD ( "a2p.f8", 0x3800, 0x0800, 0xecffd453 )
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2e)
	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ( "a2e.cd", 0x0000, 0x2000, 0xe248835e )
	ROM_LOAD ( "a2e.ef", 0x2000, 0x2000, 0xfc3d59d8 )
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ee)
    ROM_REGION(0x4700,REGION_CPU1,0)
    ROM_LOAD ( "a2ee.cd", 0x0000, 0x2000, 0x443aa7c4 )
    ROM_LOAD ( "a2ee.ef", 0x2000, 0x2000, 0x95e10034 )
    ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, 0xce7144f6 ) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ep)
    ROM_REGION(0x4700,REGION_CPU1,0)
    ROM_LOAD ("a2ept.cf", 0x0000, 0x4000, 0x02b648c8)
    ROM_LOAD ("disk2_33.rom", 0x4500, 0x0100, 0xce7144f6) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2c)
    ROM_REGION(0x4000,REGION_CPU1,0)
    ROM_LOAD ( "a2c.128", 0x0000, 0x4000, 0xf0edaa1b )
ROM_END

ROM_START(apple2c0)
    ROM_REGION(0x8700,REGION_CPU1,0)
    ROM_LOAD("a2c.256", 0x0000, 0x8000, 0xc8b979b3)
ROM_END

ROM_START(apple2cp)
    ROM_REGION(0x8700,REGION_CPU1,0)
    ROM_LOAD("a2cplus.mon", 0x0000, 0x8000, 0x0b996420)
ROM_END

SYSTEM_CONFIG_START(apple2)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2, "dsk\0bin\0do\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, apple2_floppy_init, NULL, apple2_floppy_load, NULL, NULL)
	CONFIG_GET_CUSTOM_DEVICENAME( apple2 )
	CONFIG_RAM_DEFAULT			(128 * 1024)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT      CONFIG	COMPANY            FULLNAME */
COMP ( 1977, apple2,   0,        apple2e,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //" )
COMP ( 1979, apple2p,  apple2,   apple2e,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //+" )
COMP ( 1983, apple2e,  0,        apple2e,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //e" )
COMP ( 1985, apple2ee, apple2e,  apple2ee, apple2,   apple2,   apple2,	"Apple Computer", "Apple //e (enhanced)" )
COMP ( 1987, apple2ep, apple2e,  apple2ee, apple2,   apple2,   apple2,	"Apple Computer", "Apple //e (Platinum)" )
COMP ( 1984, apple2c,  0,        apple2c,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //c" )
COMPX( 1985, apple2c0, apple2c,  apple2c,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //c (UniDisk 3.5)",	GAME_NOT_WORKING )
COMPX( 1988, apple2cp, apple2c,  apple2c,  apple2,   apple2,   apple2,	"Apple Computer", "Apple //c Plus",			GAME_NOT_WORKING )

