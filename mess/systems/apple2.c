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
#include "inputx.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"

static ADDRESS_MAP_START( apple2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x01ff) AM_READWRITE(MRA8_A2BANK_0000,		MWA8_A2BANK_0000)
	AM_RANGE(0x0200, 0x03ff) AM_READWRITE(MRA8_A2BANK_0200,		MWA8_A2BANK_0200)
	AM_RANGE(0x0400, 0x07ff) AM_READWRITE(MRA8_A2BANK_0400,		MWA8_A2BANK_0400)
	AM_RANGE(0x0800, 0x1fff) AM_READWRITE(MRA8_A2BANK_0800,		MWA8_A2BANK_0800)
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(MRA8_A2BANK_2000,		MWA8_A2BANK_2000)
	AM_RANGE(0x4000, 0xbfff) AM_READWRITE(MRA8_A2BANK_4000,		MWA8_A2BANK_4000)
	AM_RANGE(0xc000, 0xc00f) AM_READWRITE(apple2_c00x_r,		apple2_c00x_w)
	AM_RANGE(0xc010, 0xc01f) AM_READWRITE(apple2_c01x_r,		apple2_c01x_w)
	AM_RANGE(0xc020, 0xc02f) AM_READWRITE(apple2_c02x_r,		apple2_c02x_w)
	AM_RANGE(0xc030, 0xc03f) AM_READWRITE(apple2_c03x_r,		apple2_c03x_w)
	AM_RANGE(0xc040, 0xc04f) AM_NOP
	AM_RANGE(0xc050, 0xc05f) AM_READWRITE(apple2_c05x_r,		apple2_c05x_w)
	AM_RANGE(0xc060, 0xc06f) AM_READWRITE(apple2_c06x_r,		MWA8_NOP)
	AM_RANGE(0xc070, 0xc07f) AM_READWRITE(apple2_c07x_r,		apple2_c07x_w)
	AM_RANGE(0xc080, 0xc08f) AM_READWRITE(apple2_c08x_r,		apple2_c08x_w)
	AM_RANGE(0xc090, 0xc09f) AM_READWRITE(apple2_c0xx_slot1_r,	apple2_c0xx_slot1_w)
	AM_RANGE(0xc0a0, 0xc0af) AM_READWRITE(apple2_c0xx_slot2_r,	apple2_c0xx_slot2_w)
	AM_RANGE(0xc0b0, 0xc0bf) AM_READWRITE(apple2_c0xx_slot3_r,	apple2_c0xx_slot3_w)
	AM_RANGE(0xc0c0, 0xc0cf) AM_READWRITE(apple2_c0xx_slot4_r,	apple2_c0xx_slot4_w)
	AM_RANGE(0xc0d0, 0xc0df) AM_READWRITE(apple2_c0xx_slot5_r,	apple2_c0xx_slot5_w)
	AM_RANGE(0xc0e0, 0xc0ef) AM_READWRITE(apple2_c0xx_slot6_r,	apple2_c0xx_slot6_w)
	AM_RANGE(0xc0f0, 0xc0ff) AM_READWRITE(apple2_c0xx_slot7_r,	apple2_c0xx_slot7_w)
	AM_RANGE(0xc100, 0xc2ff) AM_READWRITE(MRA8_A2BANK_C100,		MWA8_A2BANK_C100)
	AM_RANGE(0xc300, 0xc3ff) AM_READWRITE(MRA8_A2BANK_C300,		MWA8_A2BANK_C300)
	AM_RANGE(0xc400, 0xc7ff) AM_READWRITE(MRA8_A2BANK_C400,		MWA8_A2BANK_C400)
	AM_RANGE(0xc800, 0xcfff) AM_READWRITE(MRA8_A2BANK_C800,		MWA8_A2BANK_C800)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(MRA8_A2BANK_D000,		MWA8_A2BANK_D000)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(MRA8_A2BANK_E000,		MWA8_A2BANK_E000)
ADDRESS_MAP_END


/**************************************************************************
***************************************************************************/

#define JOYSTICK_DELTA			80
#define JOYSTICK_SENSITIVITY	100

INPUT_PORTS_START( apple2 )

    PORT_START /* [0] VBLANK */
    PORT_BIT ( 0xBF, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )

    PORT_START /* [1] KEYS #1 */
    PORT_KEY1( 0x01, IP_ACTIVE_HIGH, "Delete ", 		KEYCODE_BACKSPACE,   IP_JOY_NONE, 8 )
    PORT_KEY0( 0x02, IP_ACTIVE_HIGH, "\x1b      ", 		KEYCODE_LEFT,        IP_JOY_NONE )
    PORT_KEY1( 0x04, IP_ACTIVE_HIGH, "Tab    ", 		KEYCODE_TAB,         IP_JOY_NONE, 9 )
    PORT_KEY1( 0x08, IP_ACTIVE_HIGH, "\x19      ", 		KEYCODE_DOWN,        IP_JOY_NONE, 10 )
    PORT_KEY0( 0x10, IP_ACTIVE_HIGH, "\x18      ", 		KEYCODE_UP,          IP_JOY_NONE )
    PORT_KEY1( 0x20, IP_ACTIVE_HIGH, "Return ", 		KEYCODE_ENTER,       IP_JOY_NONE, 13 )
    PORT_KEY0( 0x40, IP_ACTIVE_HIGH, "\x1a      ", 		KEYCODE_RIGHT,       IP_JOY_NONE )
    PORT_KEY1( 0x80, IP_ACTIVE_HIGH, "Esc    ",			KEYCODE_ESC,         IP_JOY_NONE, 27 )

    PORT_START /* [2] KEYS #2 */
    PORT_KEY1( 0x01, IP_ACTIVE_HIGH, "Space  ", 		KEYCODE_SPACE,		IP_JOY_NONE, ' ' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "' \"    ", 		KEYCODE_QUOTE,		IP_JOY_NONE, '\'', '\"' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, ", <    ", 		KEYCODE_COMMA,		IP_JOY_NONE, ',', '<' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "- _    ", 		KEYCODE_MINUS,		IP_JOY_NONE, '-', '_' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, ". >    ", 		KEYCODE_STOP,		IP_JOY_NONE, '.', '>' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "/ ?    ", 		KEYCODE_SLASH,		IP_JOY_NONE, '/', '?' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "0 )    ", 		KEYCODE_0,			IP_JOY_NONE, '0', ')' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "1 !    ", 		KEYCODE_1,			IP_JOY_NONE, '1', '!' )

    PORT_START /* [3] KEYS #3 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "2 @    ", 		KEYCODE_2,			IP_JOY_NONE, '2', '@' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "3 #    ", 		KEYCODE_3,			IP_JOY_NONE, '3', '#' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "4 $    ", 		KEYCODE_4,			IP_JOY_NONE, '4', '$' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "5 %    ", 		KEYCODE_5,			IP_JOY_NONE, '5', '%' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "6 ^    ", 		KEYCODE_6,			IP_JOY_NONE, '6', '^' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "7 &    ", 		KEYCODE_7,			IP_JOY_NONE, '7', '&' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "8 *    ", 		KEYCODE_8,			IP_JOY_NONE, '8', '*' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "9 (    ",			KEYCODE_9,			IP_JOY_NONE, '9', '(' )

    PORT_START /* [4] KEYS #4 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "; :    ", 		KEYCODE_COLON,		IP_JOY_NONE, ';', ':' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "= +    ", 		KEYCODE_EQUALS,		IP_JOY_NONE, '=', '+' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "[ {    ", 		KEYCODE_OPENBRACE,	IP_JOY_NONE, '[', '{' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "\\ |    ", 		KEYCODE_BACKSLASH,	IP_JOY_NONE, '\\', '|' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "] }    ",			KEYCODE_CLOSEBRACE,	IP_JOY_NONE, ']', '}' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "` ~    ",			KEYCODE_TILDE,		IP_JOY_NONE, '`', '~' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "A a    ", 		KEYCODE_A,			IP_JOY_NONE, 'A', 'a' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "B b    ", 		KEYCODE_B,			IP_JOY_NONE, 'B', 'b' )

    PORT_START /* [5] KEYS #5 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "C c    ", 		KEYCODE_C,			IP_JOY_NONE, 'C', 'c' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "D d    ", 		KEYCODE_D,			IP_JOY_NONE, 'D', 'd' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "E e    ",			KEYCODE_E,			IP_JOY_NONE, 'E', 'e' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "F f    ", 		KEYCODE_F,			IP_JOY_NONE, 'F', 'f' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "G g    ", 		KEYCODE_G,			IP_JOY_NONE, 'G', 'g' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "H h    ", 		KEYCODE_H,			IP_JOY_NONE, 'H', 'h' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "I i    ", 		KEYCODE_I,			IP_JOY_NONE, 'I', 'i' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "J j    ", 		KEYCODE_J,			IP_JOY_NONE, 'J', 'j' )

    PORT_START /* [6] KEYS #6 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "K k    ",			KEYCODE_K,			IP_JOY_NONE, 'K', 'k' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "L l    ",			KEYCODE_L,			IP_JOY_NONE, 'L', 'l' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "M m    ", 		KEYCODE_M,			IP_JOY_NONE, 'M', 'm' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "N n    ", 		KEYCODE_N,			IP_JOY_NONE, 'N', 'n' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "O o    ", 		KEYCODE_O,			IP_JOY_NONE, 'O', 'o' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "P p    ", 		KEYCODE_P,			IP_JOY_NONE, 'P', 'p' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "Q q    ", 		KEYCODE_Q,			IP_JOY_NONE, 'Q', 'q' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "R r    ", 		KEYCODE_R,			IP_JOY_NONE, 'R', 'r' )

    PORT_START /* [7] KEYS #7 */
    PORT_KEY2( 0x01, IP_ACTIVE_HIGH, "S s    ", 		KEYCODE_S,			IP_JOY_NONE, 'S', 's' )
    PORT_KEY2( 0x02, IP_ACTIVE_HIGH, "T t    ", 		KEYCODE_T,			IP_JOY_NONE, 'T', 't' )
    PORT_KEY2( 0x04, IP_ACTIVE_HIGH, "U u    ", 		KEYCODE_U,			IP_JOY_NONE, 'U', 'u' )
    PORT_KEY2( 0x08, IP_ACTIVE_HIGH, "V v    ", 		KEYCODE_V,			IP_JOY_NONE, 'V', 'v' )
    PORT_KEY2( 0x10, IP_ACTIVE_HIGH, "W w    ", 		KEYCODE_W,			IP_JOY_NONE, 'W', 'w' )
    PORT_KEY2( 0x20, IP_ACTIVE_HIGH, "X x    ", 		KEYCODE_X,			IP_JOY_NONE, 'X', 'x' )
    PORT_KEY2( 0x40, IP_ACTIVE_HIGH, "Y y    ", 		KEYCODE_Y,			IP_JOY_NONE, 'Y', 'y' )
    PORT_KEY2( 0x80, IP_ACTIVE_HIGH, "Z z    ",			KEYCODE_Z,			IP_JOY_NONE, 'Z', 'z' )

    PORT_START /* [8] Special keys */
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_DEFAULT )
    PORT_KEY1( 0x02, IP_ACTIVE_HIGH, "Left Shift",		KEYCODE_LSHIFT,		IP_JOY_NONE, UCHAR_SHIFT_1 )
    PORT_KEY1( 0x04, IP_ACTIVE_HIGH, "Right Shift",		KEYCODE_RSHIFT,		IP_JOY_NONE, UCHAR_SHIFT_1 )
    PORT_KEY1( 0x08, IP_ACTIVE_HIGH, "Control",			KEYCODE_LCONTROL,	IP_JOY_NONE, UCHAR_SHIFT_2 )
    PORT_KEY0( 0x10, IP_ACTIVE_HIGH, "Open Apple",		KEYCODE_LALT,		IP_JOY_NONE)
    PORT_KEY0( 0x20, IP_ACTIVE_HIGH, "Solid Apple",		KEYCODE_RALT,		IP_JOY_NONE)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Reset",	KEYCODE_F3,		IP_JOY_NONE )

	PORT_START /* [9] Joystick 1 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* [10] Joystick 1 Y Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* [11] Joystick 2 X Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_1_PAD, KEYCODE_3_PAD, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* [12] Joystick 2 Y Axis */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_5_PAD, KEYCODE_2_PAD, JOYCODE_1_UP, JOYCODE_1_DOWN)
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

static struct GfxLayout apple2_text_layout =
{
	14,8,		/* 14*8 characters */
	256,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7 },   /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxLayout apple2_dbltext_layout =
{
	7,8,		/* 7*8 characters */
	256,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 1, 2, 3, 4, 5, 6, 7 },    /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxDecodeInfo apple2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &apple2_text_layout, 0, 2 },
	{ REGION_GFX1, 0x0000, &apple2_dbltext_layout, 0, 2 },
	{ -1 } /* end of array */
};

static struct GfxLayout apple2e_text_layout =
{
	14,8,		/* 14*8 characters */
	1024,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },   /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxLayout apple2e_dbltext_layout =
{
	7,8,		/* 7*8 characters */
	1024,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },    /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static struct GfxDecodeInfo apple2e_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &apple2e_text_layout, 0, 2 },
	{ REGION_GFX1, 0x0000, &apple2e_dbltext_layout, 0, 2 },
	{ -1 } /* end of array */
};

static unsigned short apple2_colortable[] =
{
	15, 0,	/* normal */
	0, 15	/* inverse */
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
	switch(image_devtype(img)) {
	case IO_FLOPPY:
		snprintf(buf, bufsize, "Slot 6 Disk #%d", image_index_in_devtype(img) + 1);
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

static MACHINE_DRIVER_START( apple2_common )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
	MDRV_CPU_PROGRAM_MAP(apple2_map, 0)
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

	MDRV_VIDEO_START(apple2)
	MDRV_VIDEO_UPDATE(apple2)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, apple2_DAC_interface)
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2 )
	MDRV_IMPORT_FROM( apple2_common )
	MDRV_GFXDECODE(apple2_gfxdecodeinfo)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2p )
	MDRV_IMPORT_FROM( apple2_common )
	MDRV_VIDEO_START(apple2p)
	MDRV_GFXDECODE(apple2_gfxdecodeinfo)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2e )
	MDRV_IMPORT_FROM( apple2_common )
	MDRV_VIDEO_START(apple2e)
	MDRV_GFXDECODE(apple2e_gfxdecodeinfo)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apple2ee )
	MDRV_IMPORT_FROM( apple2e )
	MDRV_CPU_REPLACE("main", M65C02, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apple2c )
	MDRV_IMPORT_FROM( apple2ee )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2)
	ROM_REGION(0x0800,REGION_GFX1,0)
	ROM_LOAD ( "a2.chr", 0x0000, 0x0800, CRC(64f415c6 ))

	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD_OPTIONAL ( "progaid1.rom", 0x1000, 0x0800, CRC(4234e88a ))

/* The area $D800-$DFFF in Apple II is reserved for 3rd party add-ons:
   Maybe MESS should map this space to a CARTSLOT device?              */

	ROM_LOAD ( "a2.e0", 0x2000, 0x0800, CRC(c0a4ad3b ))
	ROM_LOAD ( "a2.e8", 0x2800, 0x0800, CRC(a99c2cf6 ))
	ROM_LOAD ( "a2.f0", 0x3000, 0x0800, CRC(62230d38 ))
	ROM_LOAD ( "a2.f8", 0x3800, 0x0800, CRC(020a86d0 ))
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, CRC(ce7144f6 )) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2p)
	ROM_REGION(0x0800,REGION_GFX1,0)
	ROM_LOAD ( "a2.chr", 0x0000, 0x0800, CRC(64f415c6 ))

	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ( "a2p.d0", 0x1000, 0x0800, CRC(6f05f949 ))
	ROM_LOAD ( "a2p.d8", 0x1800, 0x0800, CRC(1f08087c ))
	ROM_LOAD ( "a2p.e0", 0x2000, 0x0800, CRC(2b8d9a89 ))
	ROM_LOAD ( "a2p.e8", 0x2800, 0x0800, CRC(5719871a ))
	ROM_LOAD ( "a2p.f0", 0x3000, 0x0800, CRC(9a04eecf ))
	ROM_LOAD ( "a2p.f8", 0x3800, 0x0800, CRC(ecffd453 ))
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, CRC(ce7144f6 )) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2e)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC(b081df66 ))
	ROM_LOAD ( "a2ealt.chr", 0x1000, 0x1000,CRC(816a86f1 ))

	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ( "a2e.cd", 0x0000, 0x2000, CRC(e248835e ))
	ROM_LOAD ( "a2e.ef", 0x2000, 0x2000, CRC(fc3d59d8 ))
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, CRC(ce7144f6 )) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ee)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC( b081df66 ))
	ROM_LOAD ( "a2eealt.chr", 0x1000, 0x1000,CRC( 2651014d ))

	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ( "a2ee.cd", 0x0000, 0x2000, CRC(443aa7c4 ))
	ROM_LOAD ( "a2ee.ef", 0x2000, 0x2000, CRC(95e10034 ))
	ROM_LOAD ( "disk2_33.rom", 0x4500, 0x0100, CRC(ce7144f6 )) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ep)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC( b081df66 ))
	ROM_LOAD ( "a2eealt.chr", 0x1000, 0x1000,CRC( 2651014d ))

	ROM_REGION(0x4700,REGION_CPU1,0)
	ROM_LOAD ("a2ept.cf", 0x0000, 0x4000, CRC(02b648c8))
	ROM_LOAD ("disk2_33.rom", 0x4500, 0x0100, CRC(ce7144f6)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2c)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC( b081df66 ))
	ROM_LOAD ( "a2eealt.chr", 0x1000, 0x1000,CRC( 2651014d ))

	ROM_REGION(0x4000,REGION_CPU1,0)
	ROM_LOAD ( "a2c.128", 0x0000, 0x4000, CRC(f0edaa1b ))
ROM_END

ROM_START(apple2c0)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC( b081df66 ))
	ROM_LOAD ( "a2eealt.chr", 0x1000, 0x1000,CRC( 2651014d ))

	ROM_REGION(0x8700,REGION_CPU1,0)
	ROM_LOAD("a2c.256", 0x0000, 0x8000, CRC(c8b979b3))
ROM_END

ROM_START(apple2cp)
	ROM_REGION(0x2000,REGION_GFX1,0)
	ROM_LOAD ( "a2e.chr", 0x0000, 0x1000,CRC( b081df66 ))
	ROM_LOAD ( "a2eealt.chr", 0x1000, 0x1000,CRC( 2651014d ))

	ROM_REGION(0x8700,REGION_CPU1,0)
	ROM_LOAD("a2cplus.mon", 0x0000, 0x8000, CRC(0b996420))
ROM_END

SYSTEM_CONFIG_START(apple2_common)
	CONFIG_DEVICE_FLOPPY( 2, apple2 )
	CONFIG_GET_CUSTOM_DEVICENAME( apple2 )
	CONFIG_QUEUE_CHARS			( AY3600 )
	CONFIG_ACCEPT_CHAR			( AY3600 )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(apple2)
//	CONFIG_IMPORT_FROM( apple2_common )
//	CONFIG_RAM				(4 * 1024)	/* Still unsupported? */
//	CONFIG_RAM				(8 * 1024)	/* Still unsupported? */
//	CONFIG_RAM				(12 * 1024)	/* Still unsupported? */
//	CONFIG_RAM				(16 * 1024)
//	CONFIG_RAM				(20 * 1024)
//	CONFIG_RAM				(24 * 1024)
//	CONFIG_RAM				(32 * 1024)
//	CONFIG_RAM				(36 * 1024)
//	CONFIG_RAM				(48 * 1024)
//	CONFIG_RAM_DEFAULT			(64 * 1024)	/* At the moment the RAM bank $C000-$FFFF is available only if you choose   */
								/* default configuration: on real machine is present also in configurations */
								/* with less memory, provided that the language card is installed           */
	CONFIG_RAM_DEFAULT			(128 * 1024)	/* ONLY TEMPORARY - ACTUALLY NOT SUPPORTED!!! */
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(apple2p)
	CONFIG_IMPORT_FROM( apple2_common )
//	CONFIG_RAM				(16 * 1024)
//	CONFIG_RAM				(32 * 1024)
//	CONFIG_RAM				(48 * 1024)
//	CONFIG_RAM_DEFAULT			(64 * 1024)	/* At the moment the RAM bank $C000-$FFFF is available only if you choose   */
								/* default configuration: on real machine is present also in configurations */
								/* with less memory, provided that the language card is installed           */
	CONFIG_RAM_DEFAULT                      (128 * 1024)    /* ONLY TEMPORARY - ACTUALLY NOT SUPPORTED!!! */
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(apple2e)
	CONFIG_IMPORT_FROM( apple2_common )
	CONFIG_RAM_DEFAULT			(128 * 1024)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    COMPAT		MACHINE   INPUT     INIT      CONFIG	COMPANY            FULLNAME */
COMPX( 1977, apple2,   0,        0,			apple2,   apple2,   apple2,   apple2,	"Apple Computer", "Apple ][", GAME_IMPERFECT_COLORS )
COMPX( 1979, apple2p,  apple2,   0,			apple2p,  apple2,   apple2,   apple2p,	"Apple Computer", "Apple ][+", GAME_IMPERFECT_COLORS )
COMP ( 1983, apple2e,  0,        apple2,	apple2e,  apple2,   apple2,   apple2e,	"Apple Computer", "Apple //e" )
COMP ( 1985, apple2ee, apple2e,  0,			apple2ee, apple2,   apple2,   apple2e,	"Apple Computer", "Apple //e (enhanced)" )
COMP ( 1987, apple2ep, apple2e,  0,			apple2ee, apple2,   apple2,   apple2e,	"Apple Computer", "Apple //e (Platinum)" )
COMP ( 1984, apple2c,  0,        apple2,	apple2c,  apple2,   apple2,   apple2e,	"Apple Computer", "Apple //c" )
COMPX( 1985, apple2c0, apple2c,  0,			apple2c,  apple2,   apple2,   apple2e,	"Apple Computer", "Apple //c (UniDisk 3.5)",	GAME_NOT_WORKING )
COMPX( 1988, apple2cp, apple2c,  0,			apple2c,  apple2,   apple2,   apple2e,	"Apple Computer", "Apple //c Plus",			GAME_NOT_WORKING )
