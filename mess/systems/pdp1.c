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


#include "driver.h"

#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"

/*
 * PRECISION CRT DISPLAY (TYPE 30)
 * is the only display - hardware emulated, this is needed for SPACEWAR!
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
static MEMORY_READ_START18(pdp1_readmem)
	{ 0x0000, 0xffff, pdp1_read_mem },
MEMORY_END

static MEMORY_WRITE_START18(pdp1_writemem)
	{ 0x0000, 0xffff, pdp1_write_mem },
MEMORY_END


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
		Note that I can see 2 additional keys whose purpose is unknown to me (I cannot read the
		caps on the photograph).
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
	PORT_BITX(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE)

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


/* palette: grey levels follow an exponential law, so that decrementing the color index periodically
will simulate the remanence of a cathode ray tube */
static unsigned char palette[] =
{
	0x00,0x00,0x00, /* black */
	11,11,11,
	14,14,14,
	18,18,18,
	22,22,22,
	27,27,27,
	34,34,34,
	43,43,43,
	53,53,53,
	67,67,67,
	84,84,84,
	104,104,104,
	131,131,131,
	163,163,163,
	204,204,204,
	0xFF,0xFF,0xFF,  /* white */
	0x00,0xFF,0x00,  /* green */
	0x00,0x40,0x00,  /* dark green */
	0xFF,0x00,0x00   /* red */
};

static unsigned short colortable[] =
{
	pen_panel_bg, pen_panel_caption,
	pen_typewriter_bg, pen_black,
	pen_typewriter_bg, pen_red
};

/* Initialise the palette */
static void pdp1_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy(sys_palette, palette, sizeof(palette));
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
		NULL,		pdp1_iot,	pdp1_iot,	pdp1_iot,	pdp1_iot,	pdp1_iot,	pdp1_iot,	pdp1_iot,
	/*				spacewar																 */
		NULL,		pdp1_iot,	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*							lag												glf?/jsp?	gpl?/gpr?/gcf? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*	rrb			rcb?		rcc?		cks			mcs			mes			mel			 */
		pdp1_iot,	NULL,		NULL,		pdp1_iot,	NULL,		NULL,		NULL,		NULL,
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
	0,	/* hardware multiply support defined in input ports and pdp1_init_machine */
	0,	/* hardware divide support defined in input ports and pdp1_init_machine */
	0	/* type 20 sequence break system support defined in input ports and pdp1_init_machine */
};


/* note I don't know about the speed of the machine, I only know
 * how long each instruction takes in micro seconds
 * below speed should therefore also be read in something like
 * microseconds of instructions
 */
static struct MachineDriver machine_driver_pdp1 =
{
	/* basic machine hardware */
	{
		{
			CPU_PDP1,
			1000000,	/* should be 200000, but this makes little sense as there is no master clock */
			pdp1_readmem, pdp1_writemem,0,0,
			pdp1_interrupt, 1, /* fake interrupt */
			0, 0,
			& pdp1_reset_param
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,
	pdp1_init_machine,
	0,

	/* video hardware */
	virtual_width, virtual_height, { 0, virtual_width-1, 0, virtual_height-1 },

	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),

	pdp1_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	pdp1_vh_start,
	pdp1_vh_stop,
	pdp1_vh_update,

	/* sound hardware */
	0,0,0,0
};


static const struct IODevice io_pdp1[] =
{
	{	/* one perforated tape reader, and one puncher */
		IO_PUNCHTAPE,			/* type */
		2,						/* count */
		"tap\0rim\0",			/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		NULL,					/* id */
		pdp1_tape_init,			/* init */
		pdp1_tape_exit,			/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{	/* typewriter out */
		IO_PRINTER,					/* type */
		1,							/* count */
		"typ\0",					/* file extensions */
		IO_RESET_NONE,				/* reset depth */
		NULL,						/* id */
		pdp1_typewriter_init,		/* init */
		pdp1_typewriter_exit,		/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
		NULL,						/* status */
		NULL,						/* seek */
		NULL,						/* tell */
		NULL,						/* input */
		NULL,						/* output */
		NULL,						/* input chunk */
		NULL						/* output chunk */
	},
	{ IO_END }
};


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


/*COMPUTER_CONFIG_START(pdp1)
	CONFIG_RAM_DEFAULT(4 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM(64 * 1024)
COMPUTER_CONFIG_END*/


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMP( 1961, pdp1,	  0, 		pdp1,	  pdp1, 	pdp1,	  "Digital Equipment Corporation",  "PDP-1" )
