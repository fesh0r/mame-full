/*

Driver for a PDP1 emulator.

	Digital Equipment Corporation
	Brian Silverman (original Java Source)
	Vadim Gerasimov (original Java Source)
	Chris Salomon (MESS driver)
	Raphael Nabet (MESS driver)

Initially, this was a conversion of a JAVA emulator
(although code has been edited extensively ever since).
I have tried contacting the author, but heard as yet nothing of him,
so I don't know if it all right with him, but after all -> he did
release the source, so hopefully everything will be fine (no his
name is not Marat).

Note: naturally I have no PDP1, I have never seen one, nor have I any
programs for it.

The first supported program was:

SPACEWAR!

The first Videogame EVER!

When I saw the java emulator, running that game I was quite intrigued to
include a driver for MESS.
I think the historical value of SPACEWAR! is enormous.

Two other programs are supported: Munching squares and LISP.

Added Debugging and Disassembler...


Also:
ftp://minnie.cs.adfa.oz.au/pub/PDP-11/Sims/Supnik_2.3/software/lispswre.tar.gz
Is a packet which includes the original LISP as source and
binary form plus a makro assembler for PDP1 programs.

For more documentation look at the source for the driver,
and the cpu/pdp1/pdp1.c file (information about the whereabouts of information
and the java source).

*/

#include <math.h>

#include "driver.h"

#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"

/*
 *
 * The loading storing OS... is not emulated (I haven't a clue where to
 * get programs for the machine)
 *
 */





/* every memory handler is the same for now */

/* note: MEMORY HANDLERS used everywhere, since we don't need bytes, we
 * need 18 bit words, the handler functions return integers, so it should
 * be all right to use them.
 * This gives sometimes IO warnings!
 */
#ifdef SUPPORT_ODD_WORD_SIZES
#define pdp1_read_mem MRA32_RAM
#define pdp1_write_mem MWA32_RAM
#endif
static ADDRESS_MAP_START(pdp1_map, ADDRESS_SPACE_PROGRAM, 32)
#if 0
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(pdp1_read_mem, pdp1_write_mem)
#else
	AM_RANGE(0x00000, 0x3ffff) AM_READWRITE(pdp1_read_mem, pdp1_write_mem)
#endif
ADDRESS_MAP_END


INPUT_PORTS_START( pdp1 )

    PORT_START		/* 0: spacewar controllers */
	PORT_BITX( ROTATE_LEFT_PLAYER1, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT, "Spin Left Player 1", KEYCODE_A, JOYCODE_1_LEFT )
	PORT_BITX( ROTATE_RIGHT_PLAYER1, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT, "Spin Right Player 1", KEYCODE_S, JOYCODE_1_RIGHT )
	PORT_BITX( THRUST_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON1, "Thrust Player 1", KEYCODE_D, JOYCODE_1_BUTTON1 )
	PORT_BITX( FIRE_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON2, "Fire Player 1", KEYCODE_F, JOYCODE_1_BUTTON2 )
	PORT_BITX( ROTATE_LEFT_PLAYER2, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT|IPF_PLAYER2, "Spin Left Player 2", KEYCODE_LEFT, JOYCODE_2_LEFT )
	PORT_BITX( ROTATE_RIGHT_PLAYER2, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT|IPF_PLAYER2, "Spin Right Player 2", KEYCODE_RIGHT, JOYCODE_2_RIGHT )
	PORT_BITX( THRUST_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON1|IPF_PLAYER2, "Thrust Player 2", KEYCODE_UP, JOYCODE_2_BUTTON1 )
	PORT_BITX( FIRE_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON2|IPF_PLAYER2, "Fire Player 2", KEYCODE_DOWN, JOYCODE_2_BUTTON2 )
	PORT_BITX( HSPACE_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON3, "Hyperspace Player 1", KEYCODE_Z, JOYCODE_1_BUTTON3 )
	PORT_BITX( HSPACE_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON3|IPF_PLAYER2, "Hyperspace Player 2", KEYCODE_SLASH, JOYCODE_2_BUTTON3 )

	PORT_START		/* 1: various pdp1 operator control panel switches */
	PORT_BITX(pdp1_control, IP_ACTIVE_HIGH, IPT_KEYBOARD, "control panel key", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(pdp1_extend, IP_ACTIVE_HIGH, IPT_KEYBOARD, "extend", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(pdp1_start_nobrk, IP_ACTIVE_HIGH, IPT_KEYBOARD, "start (sequence break disabled)", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(pdp1_start_brk, IP_ACTIVE_HIGH, IPT_KEYBOARD, "start (sequence break enabled)", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(pdp1_stop, IP_ACTIVE_HIGH, IPT_KEYBOARD, "stop", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(pdp1_continue, IP_ACTIVE_HIGH, IPT_KEYBOARD, "continue", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(pdp1_examine, IP_ACTIVE_HIGH, IPT_KEYBOARD, "examine", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(pdp1_deposit, IP_ACTIVE_HIGH, IPT_KEYBOARD, "deposit", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(pdp1_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD, "read in", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(pdp1_reader, IP_ACTIVE_HIGH, IPT_KEYBOARD, "reader", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(pdp1_tape_feed, IP_ACTIVE_HIGH, IPT_KEYBOARD, "tape feed", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX(pdp1_single_step, IP_ACTIVE_HIGH, IPT_KEYBOARD, "single step", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(pdp1_single_inst, IP_ACTIVE_HIGH, IPT_KEYBOARD, "single inst", KEYCODE_SLASH, IP_JOY_NONE)

    PORT_START		/* 2: operator control panel sense switches */
	PORT_BITX(	  040, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 1", KEYCODE_1_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    040, DEF_STR( On ) )
	PORT_BITX(	  020, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 2", KEYCODE_2_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    020, DEF_STR( On ) )
	PORT_BITX(	  010, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 3", KEYCODE_3_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    010, DEF_STR( On ) )
	PORT_BITX(	  004, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 4", KEYCODE_4_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    004, DEF_STR( On ) )
	PORT_BITX(	  002, 002, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 5", KEYCODE_5_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    002, DEF_STR( On ) )
	PORT_BITX(	  001, 000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 6", KEYCODE_6_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(    000, DEF_STR( Off ) )
    PORT_DIPSETTING(    001, DEF_STR( On ) )

    PORT_START		/* 3: operator control panel test address switches */
	PORT_BITX( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 3", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 4", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 5", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Extension Test Address Switch 6", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 7", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 8", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 9", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 10", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 11", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 12", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 13", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 14", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 15", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 16", KEYCODE_W, IP_JOY_NONE )
   	PORT_BITX( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 17", KEYCODE_E, IP_JOY_NONE )
   	PORT_BITX( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Address Switch 18", KEYCODE_R, IP_JOY_NONE )

    PORT_START		/* 4: operator control panel test word switches MSB */
	PORT_BITX(    0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 1", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(    0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 2", KEYCODE_S, IP_JOY_NONE )

    PORT_START		/* 5: operator control panel test word switches LSB */
	PORT_BITX( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 3", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 4", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 5", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 6", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 7", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 8", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 9", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 10", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 11", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 12", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 13", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 14", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 15", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 16", KEYCODE_V, IP_JOY_NONE )
   	PORT_BITX( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 17", KEYCODE_B, IP_JOY_NONE )
   	PORT_BITX( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Test Word Switch 18", KEYCODE_N, IP_JOY_NONE )

	/*
		Note that I can see 2 additional keys whose purpose is unknown to me.
		The caps look like "MAR REL" for the leftmost one and "MAR SET" for
		rightmost one: maybe they were used to set the margin (I don't have the
		manual for the typewriter).
	*/
    PORT_START		/* 6: typewriter codes 00-17 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(Space)", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1 \"", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2 '", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3 ~", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4 (implies)", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5 (or)", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6 (and)", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7 <", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8 >", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9 (up arrow)", KEYCODE_9, IP_JOY_NONE)

    PORT_START		/* 7: typewriter codes 20-37 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0 (right arrow)", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/ ?", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, ", =", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab Key", KEYCODE_TAB, IP_JOY_NONE)

    PORT_START		/* 8: typewriter codes 40-57 */
	PORT_BITX(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(non-spacing middle dot) _", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "- +", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ") ]", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "(non-spacing overstrike) |", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "( [", KEYCODE_MINUS, IP_JOY_NONE)

    PORT_START		/* 9: typewriter codes 60-77 */
	PORT_BITX(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Lower Case", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, ". (multiply)", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Upper case", KEYCODE_RSHIFT, IP_JOY_NONE)
	/* hack to support my macintosh which does not differentiate the  Right Shift key */
	PORT_BITX(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Upper case", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE)

	PORT_START		/* 10: pseudo-input port with config */
	PORT_BITX( 0x0003, 0x0002, IPT_DIPSWITCH_NAME, "RAM size", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(   0x0000, "4kw" )
    PORT_DIPSETTING(   0x0001, "32kw")
    PORT_DIPSETTING(   0x0002, "64kw")
	PORT_BITX( 0x0004, 0x0000, IPT_DIPSWITCH_NAME, "Hardware multiply", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
    PORT_DIPSETTING(   0x0004, DEF_STR( On ) )
	PORT_BITX( 0x0008, 0x0000, IPT_DIPSWITCH_NAME, "Hardware divide", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
    PORT_DIPSETTING(   0x0008, DEF_STR( On ) )
	PORT_BITX( 0x0010, 0x0000, IPT_DIPSWITCH_NAME, "Type 20 sequence break system", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
    PORT_DIPSETTING(   0x0010, DEF_STR( On ) )
	PORT_BITX( 0x0020, 0x0000, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Type 32 light pen", KEYCODE_ENTER_PAD, IP_JOY_NONE )
    PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
    PORT_DIPSETTING(   0x0020, DEF_STR( On ) )

	PORT_START	/* 11: pseudo-input port with lightpen status */
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "select larger light pen tip", KEYCODE_PLUS_PAD, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "select smaller light pen tip", KEYCODE_MINUS_PAD, IP_JOY_DEFAULT)
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "light pen down", KEYCODE_NONE, IP_JOY_DEFAULT)

	PORT_START /* 12: lightpen - X AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_CENTER | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* 13: lightpen - Y AXIS */
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )


INPUT_PORTS_END


static struct GfxLayout fontlayout =
{
	6, 8,			/* 6*8 characters */
	pdp1_charnum,	/* 96+4 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fontlayout, 0, 3 },
	{ -1 }	/* end of array */
};


/*
	The static palette only includes the pens for the control panel and
	the typewriter, as the CRT palette is generated dynamically.

	The CRT palette defines various levels of intensity between white and
	black.  Grey levels follow an exponential law, so that decrementing the
	color index periodically will simulate the remanence of a cathode ray tube.
*/
static unsigned char palette[] =
{
	0xFF,0xFF,0xFF,	/* white */
	0x00,0xFF,0x00,	/* green */
	0x00,0x40,0x00,	/* dark green */
	0xFF,0x00,0x00,	/* red */
	0x80,0x80,0x80	/* light gray */
};

static unsigned short colortable[] =
{
	pen_panel_bg, pen_panel_caption,
	pen_typewriter_bg, pen_black,
	pen_typewriter_bg, pen_red
};

/* Initialise the palette */
static void palette_init_pdp1(unsigned short *sys_colortable, const unsigned char *dummy)
{
	/* rgb components for the two color emissions */
	const double r1 = .1, g1 = .1, b1 = .924, r2 = .7, g2 = .7, b2 = .076;
	/* half period in seconds for the two color emissions */
	const double half_period_1 = .05, half_period_2 = .20;
	/* refresh period in seconds */
	const double update_period = 1./refresh_rate;
	double decay_1, decay_2;
	double cur_level_1, cur_level_2;
#ifdef MAME_DEBUG
	/* level at which we stop emulating the decay and say the pixel is black */
	double cut_level = .02;
#endif
	int i;
	int r, g, b;


	/* initialize CRT palette */

	/* compute the decay factor per refresh frame */
	decay_1 = pow(.5, update_period / half_period_1);
	decay_2 = pow(.5, update_period / half_period_2);

	cur_level_1 = cur_level_2 = 255.;	/* start with maximum level */

	for (i=pen_crt_max_intensity; i>0; i--)
	{
		/* compute the current color */
		r = (int) ((r1*cur_level_1 + r2*cur_level_2) + .5);
		g = (int) ((g1*cur_level_1 + g2*cur_level_2) + .5);
		b = (int) ((b1*cur_level_1 + b2*cur_level_2) + .5);
		/* write color in palette */
		palette_set_color(i, r, g, b);
		/* apply decay for next iteration */
		cur_level_1 *= decay_1;
		cur_level_2 *= decay_2;
	}

#ifdef MAME_DEBUG
	{
		int recommended_pen_crt_num_levels;
		if (decay_1 > decay_2)
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_1))+1;
		else
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_2))+1;
		if (recommended_pen_crt_num_levels != pen_crt_num_levels)
			printf("File %s line %d: recommended value for pen_crt_num_levels is %d\n", __FILE__, __LINE__, recommended_pen_crt_num_levels);
	}
	/*if ((cur_level_1 > 255.*cut_level) || (cur_level_2 > 255.*cut_level))
		printf("File %s line %d: Please take higher value for pen_crt_num_levels or smaller value for decay\n", __FILE__, __LINE__);*/
#endif

	palette_set_color(0, 0, 0, 0);

	/* load static palette */
	palette_set_colors(pen_crt_num_levels, palette, sizeof(palette) / sizeof(palette[0]) / 3);

	memcpy(sys_colortable, colortable, sizeof(colortable));
}


pdp1_reset_param_t pdp1_reset_param =
{
	{	/* external iot handlers.  NULL means that the iot is unimplemented, unless there are
		parentheses around the iot name, in which case the iot is internal to the cpu core. */
		/* I put a ? when the source is the handbook, since a) I have used the maintainance manual
		as the primary source (as it goes more into details) b) the handbook and the maintainance
		manual occasionnally contradict each other. */
	/*	(iot)		rpa			rpb			tyo			tyi			ppa			ppb			dpy */
		NULL,		iot_rpa,	iot_rpb,	iot_tyo,	iot_tyi,	iot_ppa,	iot_ppb,	iot_dpy,
	/*				spacewar																 */
		NULL,		iot_011,	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*							lag												glf?/jsp?	gpl?/gpr?/gcf? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*	rrb			rcb?		rcc?		cks			mcs			mes			mel			 */
		iot_rrb,	NULL,		NULL,		iot_cks,	NULL,		NULL,		NULL,		NULL,
	/*	cad?		rac?		rbc?		pac						lpr/lfb/lsp swc/sci/sdf?/shr?	scv? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*	(dsc)		(asc)		(isb)		(cac)		(lsm)		(esm)		(cbs)		 */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*	icv?																	mri|rlc?	mrf/inr?/ccr? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*	mcb|dur?	mwc|mtf?	mrc|sfc?...	msm|cgo?	(eem/lem)	mic			muf			 */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	},
	pdp1_tape_read_binary,
	pdp1_io_sc_callback,
	0,	/* extend mode support defined in input ports and pdp1_init_machine */
	0,	/* hardware multiply/divide support defined in input ports and pdp1_init_machine */
	0	/* type 20 sequence break system support defined in input ports and pdp1_init_machine */
};


static MACHINE_DRIVER_START(pdp1)

	/* basic machine hardware */
	/* PDP1 CPU @ 200 kHz (no master clock, but the instruction and memory rate is 200 kHz) */
	MDRV_CPU_ADD(PDP1, 1000000/*the CPU core uses microsecond counts*/)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(pdp1_reset_param)
	MDRV_CPU_PROGRAM_MAP(pdp1_map, 0)
	/*MDRV_CPU_PORTS(readport, writeport)*/
	/* dummy interrupt: handles input */
	MDRV_CPU_VBLANK_INT(pdp1_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_FRAMES_PER_SECOND(refresh_rate)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( pdp1 )
	MDRV_MACHINE_STOP( pdp1 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware (includes the control panel and typewriter output) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(virtual_width, virtual_height)
	MDRV_VISIBLE_AREA(0, virtual_width-1, 0, virtual_height-1)
	/*MDRV_VISIBLE_AREA(0, crt_window_width-1, 0, crt_window_height-1)*/

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(pen_crt_num_levels + (sizeof(palette) / sizeof(palette[0]) / 3))
	MDRV_COLORTABLE_LENGTH(sizeof(colortable) / sizeof(colortable[0]))

	MDRV_PALETTE_INIT(pdp1)
	MDRV_VIDEO_START(pdp1)
	MDRV_VIDEO_EOF(pdp1)
	MDRV_VIDEO_UPDATE(pdp1)

	/* no sound */

MACHINE_DRIVER_END

/*
	pdp1 can address up to 65336 18 bit words when extended (4096 otherwise).
*/
ROM_START(pdp1)
	/*CPU memory space*/
#ifdef SUPPORT_ODD_WORD_SIZES
	ROM_REGION(0x10000 * sizeof(data32_t),REGION_CPU1,0)
		/* Note this computer has no ROM... */
#else
	ROM_REGION(0x10000 * sizeof(int),REGION_CPU1,0)
		/* Note this computer has no ROM... */
#endif

	ROM_REGION(pdp1_fontdata_size, REGION_GFX1, 0)
		/* space filled with our font */
ROM_END

SYSTEM_CONFIG_START(pdp1)
	/*CONFIG_RAM_DEFAULT(4 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM(64 * 1024)*/
	CONFIG_DEVICE_LEGACY(IO_PUNCHTAPE, 2, "tap\0rim\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_NONE, device_init_pdp1_tape, NULL, device_load_pdp1_tape, device_unload_pdp1_tape, NULL)
	CONFIG_DEVICE_LEGACY(IO_PRINTER, 1, "typ\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_WRITE, NULL, NULL, device_load_pdp1_typewriter, device_unload_pdp1_typewriter, NULL)
SYSTEM_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT	CONFIG	COMPANY	FULLNAME */
COMP( 1961, pdp1,	  0, 		0,		pdp1,	  pdp1, 	pdp1,	pdp1,	"Digital Equipment Corporation",  "PDP-1" )
