/***************************************************************************

  $Id: pc8801.c,v 1.29 2004/06/12 21:04:47 npwoods Exp $

***************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/pc8801.h"
#include "includes/nec765.h"
#include "machine/8255ppi.h"
#include "includes/d88.h"
#include "devices/basicdsk.h"

static struct GfxLayout char_layout_40L_h =
{
	8, 4,						/* 16 x 4 graphics */
	1024,						/* 256 codes */
	1,							/* 1 bit per pixel */
	{ 0x1000*8 },				/* no bitplanes */
	{ 0, 0, 1, 1, 2, 2, 3, 3 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2						/* code takes 8 times 8 bits */
};

static struct GfxLayout char_layout_40R_h =
{
	8, 4,						/* 16 x 4 graphics */
	1024,						/* 256 codes */
	1,							/* 1 bit per pixel */
	{ 0x1000*8 },				/* no bitplanes */
	{ 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2						/* code takes 8 times 8 bits */
};

static struct GfxLayout char_layout_80_h =
{
	8, 4,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static struct GfxLayout char_layout_40L_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 0, 1, 1, 2, 2, 3, 3 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static struct GfxLayout char_layout_40R_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static struct GfxLayout char_layout_80_l =
{
	8, 2,           /* 16 x 4 graphics */
	1024,            /* 256 codes */
	1,                      /* 1 bit per pixel */
	{ 0x1000*8 },          /* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8 },
	8 * 2           /* code takes 8 times 8 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout_80_l, 0, 16 },
	{ REGION_GFX1, 0, &char_layout_40L_l, 0, 16 },
	{ REGION_GFX1, 0, &char_layout_40R_l, 0, 16 },
	{ REGION_GFX1, 0, &char_layout_80_h, 0, 16 },
	{ REGION_GFX1, 0, &char_layout_40L_h, 0, 16 },
	{ REGION_GFX1, 0, &char_layout_40R_h, 0, 16 },
	{-1}
};       /* end of array */

/* Macro for DIPSW-1 */
#define DIPSW_1_1 \
	PORT_DIPNAME( 0x01, 0x01, "Terminal mode" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_2 \
	PORT_DIPNAME( 0x02, 0x00, "Text width" ) \
	PORT_DIPSETTING(    0x02, "40 chars/line" ) \
	PORT_DIPSETTING(    0x00, "80 chars/line" )
#define DIPSW_1_3 \
	PORT_DIPNAME( 0x04, 0x00, "Text height" ) \
	PORT_DIPSETTING(    0x04, "20 lines/screen" ) \
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
#define DIPSW_1_4 \
	PORT_DIPNAME( 0x08, 0x08, "Enable S parameter" ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_5 \
	PORT_DIPNAME( 0x10, 0x00, "Enable DEL code" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_6 \
	PORT_DIPNAME( 0x20, 0x20, "Memory wait" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_1_7 \
	PORT_DIPNAME( 0x40, 0x40, "Disable CMD SING" ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/* Macro for DIPSW-2 */
#define DIPSW_2_1 \
	PORT_DIPNAME( 0x01, 0x01, "Parity generate" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_2 \
	PORT_DIPNAME( 0x02, 0x00, "Parity type" ) \
	PORT_DIPSETTING(    0x00, "Even" ) \
	PORT_DIPSETTING(    0x02, "Odd" )
#define DIPSW_2_3 \
	PORT_DIPNAME( 0x04, 0x00, "Serial character length" ) \
	PORT_DIPSETTING(    0x04, "7 bits/char" ) \
	PORT_DIPSETTING(    0x00, "8 bits/char" )
#define DIPSW_2_4 \
	PORT_DIPNAME( 0x08, 0x08, "Stop bit length" ) \
	PORT_DIPSETTING(    0x08, "1" ) \
	PORT_DIPSETTING(    0x00, "2" )
#define DIPSW_2_5 \
	PORT_DIPNAME( 0x10, 0x10, "Enable X parameter" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_6 \
	PORT_DIPNAME( 0x20, 0x20, "Duplex mode" ) \
	PORT_DIPSETTING(    0x20, "half duplex" ) \
	PORT_DIPSETTING(    0x00, "full duplex" )
#define DIPSW_2_7 \
	PORT_DIPNAME( 0x40, 0x00, "Boot from internal FD" ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#define DIPSW_2_8 \
	PORT_DIPNAME( 0x80, 0x80, "Disable internal FD" ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/* Macro for other switch */
#define SW_V1V2N \
	PORT_DIPNAME( 0x03, 0x01, "Basic mode" ) \
	PORT_DIPSETTING(    0x02, "N-BASIC" ) \
	PORT_DIPSETTING(    0x03, "N88-BASIC (V1)" ) \
	PORT_DIPSETTING(    0x01, "N88-BASIC (V2)" )

#define SW_HS \
	PORT_DIPNAME( 0x04, 0x04, "Speed mode" ) \
	PORT_DIPSETTING(    0x00, "slow" ) \
	PORT_DIPSETTING(    0x04, "high" )

#define SW_8MHZ \
	PORT_DIPNAME( 0x08, 0x00, "Main CPU clock" ) \
	PORT_DIPSETTING(    0x00, "4MHz" ) \
	PORT_DIPSETTING(    0x08, "8MHz" )

#define SW_4MHZ_ONLY \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )

#define SW_SERIAL \
	PORT_DIPNAME( 0xf0, 0x80, "Serial speed" ) \
	PORT_DIPSETTING(    0x10, "75bps" ) \
	PORT_DIPSETTING(    0x20, "150bps" ) \
	PORT_DIPSETTING(    0x30, "300bps" ) \
	PORT_DIPSETTING(    0x40, "600bps" ) \
	PORT_DIPSETTING(    0x50, "1200bps" ) \
	PORT_DIPSETTING(    0x60, "2400bps" ) \
	PORT_DIPSETTING(    0x70, "4800bps" ) \
	PORT_DIPSETTING(    0x80, "9600bps" ) \
	PORT_DIPSETTING(    0x90, "19200bps" )

#define SW_EXTMEM \
	PORT_DIPNAME( 0x1f, 0x00, "Extension memory" ) \
	PORT_DIPSETTING(    0x00, "none" ) \
	PORT_DIPSETTING(    0x01, "32KB (PC-8012-02 x 1)" ) \
	PORT_DIPSETTING(    0x02, "64KB (PC-8012-02 x 2)" ) \
	PORT_DIPSETTING(    0x03, "128KB (PC-8012-02 x 4)" ) \
	PORT_DIPSETTING(    0x04, "128KB (PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x05, "256KB (PC-8801-02N x 2)" ) \
	PORT_DIPSETTING(    0x06, "512KB (PC-8801-02N x 4)" ) \
	PORT_DIPSETTING(    0x07, "1M (PIO-8234H-1M x 1)" ) \
	PORT_DIPSETTING(    0x08, "2M (PIO-8234H-2M x 1)" ) \
	PORT_DIPSETTING(    0x09, "4M (PIO-8234H-2M x 2)" ) \
	PORT_DIPSETTING(    0x0a, "8M (PIO-8234H-2M x 4)" ) \
	PORT_DIPSETTING(    0x0b, "1.1M (PIO-8234H-1M x 1 + PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x0c, "2.1M (PIO-8234H-2M x 1 + PC-8801-02N x 1)" ) \
	PORT_DIPSETTING(    0x0d, "4.1M (PIO-8234H-2M x 2 + PC-8801-02N x 1)" )

#define DUMMY_ROW \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )



INPUT_PORTS_START( pc88sr )
	/* [0] */
	PORT_START
	PORT_KEY1 (0x01, IP_ACTIVE_LOW, "pad 0", KEYCODE_0_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(0_PAD))
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "pad 1", KEYCODE_1_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(1_PAD))
	PORT_KEY1 (0x04, IP_ACTIVE_LOW, "pad 2", KEYCODE_2_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(2_PAD))
	PORT_KEY1 (0x08, IP_ACTIVE_LOW, "pad 3", KEYCODE_3_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(3_PAD))
	PORT_KEY1 (0x10, IP_ACTIVE_LOW, "pad 4", KEYCODE_4_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(4_PAD))
	PORT_KEY1 (0x20, IP_ACTIVE_LOW, "pad 5", KEYCODE_5_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(5_PAD))
	PORT_KEY1 (0x40, IP_ACTIVE_LOW, "pad 6", KEYCODE_6_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(6_PAD))
	PORT_KEY1 (0x80, IP_ACTIVE_LOW, "pad 7", KEYCODE_7_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(7_PAD))

	/* [1] */
	PORT_START
	PORT_KEY1 (0x01, IP_ACTIVE_LOW, "pad 8", KEYCODE_8_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(8_PAD))
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "pad 9", KEYCODE_9_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(9_PAD))
	PORT_KEY1 (0x04, IP_ACTIVE_LOW, "pad *", KEYCODE_ASTERISK, IP_JOY_NONE, UCHAR_MAMEKEY(ASTERISK))
	PORT_KEY1 (0x08, IP_ACTIVE_LOW, "pad +", KEYCODE_PLUS_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(PLUS_PAD))
	PORT_KEY1 (0x10, IP_ACTIVE_LOW, "pad =", KEYCODE_END, IP_JOY_NONE, UCHAR_MAMEKEY(END)) /* BAD */
	PORT_KEY1 (0x20, IP_ACTIVE_LOW, "pad ,", KEYCODE_NUMLOCK, IP_JOY_NONE, UCHAR_MAMEKEY(NUMLOCK)) /* BAD */
	PORT_KEY1 (0x40, IP_ACTIVE_LOW, "pad .", KEYCODE_DEL_PAD, IP_JOY_NONE, UCHAR_MAMEKEY(DEL_PAD))
	PORT_KEY1 (0x80, IP_ACTIVE_LOW, "pad return", KEYCODE_ENTER, KEYCODE_ENTER_PAD, 13)

	/* [2] */
	PORT_START
	PORT_KEY2 (0x01, IP_ACTIVE_LOW, "@ ~", KEYCODE_OPENBRACE, IP_JOY_NONE, '@', '~')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "a A", KEYCODE_A, IP_JOY_NONE, 'a', 'A')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, "b B", KEYCODE_B, IP_JOY_NONE, 'b', 'B')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "c C", KEYCODE_C, IP_JOY_NONE, 'c', 'C')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, "d D", KEYCODE_D, IP_JOY_NONE, 'd', 'D')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, "e E", KEYCODE_E, IP_JOY_NONE, 'e', 'E')
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "f F", KEYCODE_F, IP_JOY_NONE, 'f', 'F')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "g G", KEYCODE_G, IP_JOY_NONE, 'g', 'G')

	/* [3] */
	PORT_START
	PORT_KEY2 (0x01, IP_ACTIVE_LOW, "h H", KEYCODE_H, IP_JOY_NONE, 'h', 'H')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "i I", KEYCODE_I, IP_JOY_NONE, 'i', 'I')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, "j J", KEYCODE_J, IP_JOY_NONE, 'j', 'J')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "k K", KEYCODE_K, IP_JOY_NONE, 'k', 'K')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, "l L", KEYCODE_L, IP_JOY_NONE, 'l', 'L')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, "m M", KEYCODE_M, IP_JOY_NONE, 'm', 'M')
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "n N", KEYCODE_N, IP_JOY_NONE, 'n', 'N')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "o O", KEYCODE_O, IP_JOY_NONE, 'o', 'O')

	/* [4] */
	PORT_START
	PORT_KEY2 (0x01, IP_ACTIVE_LOW, "p P", KEYCODE_P, IP_JOY_NONE, 'p', 'P')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "q Q", KEYCODE_Q, IP_JOY_NONE, 'q', 'Q')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, "r R", KEYCODE_R, IP_JOY_NONE, 'r', 'R')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "s S", KEYCODE_S, IP_JOY_NONE, 's', 'S')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, "t T", KEYCODE_T, IP_JOY_NONE, 't', 'T')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, "u U", KEYCODE_U, IP_JOY_NONE, 'u', 'U')
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "v V", KEYCODE_V, IP_JOY_NONE, 'v', 'V')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "w W", KEYCODE_W, IP_JOY_NONE, 'w', 'W')

	/* [5] */
	PORT_START
	PORT_KEY2 (0x01, IP_ACTIVE_LOW, "x X", KEYCODE_X, IP_JOY_NONE, 'x', 'X')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "y Y", KEYCODE_Y, IP_JOY_NONE, 'y', 'Y')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, "z Z", KEYCODE_Z, IP_JOY_NONE, 'z', 'Z')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "[ {", KEYCODE_CLOSEBRACE, IP_JOY_NONE, '[', '{')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, "\\ |", KEYCODE_BACKSLASH2, IP_JOY_NONE, '\\', '|')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, "] }", KEYCODE_BACKSLASH, IP_JOY_NONE, ']', '}')
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "^ ~", KEYCODE_EQUALS, IP_JOY_NONE, '^', '~')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "- =", KEYCODE_MINUS, IP_JOY_NONE, '-', '=')

	/* [6] */
	PORT_START
	PORT_KEY1 (0x01, IP_ACTIVE_LOW, "0  ", KEYCODE_0, IP_JOY_NONE, '0')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "1 !", KEYCODE_1, IP_JOY_NONE, '1', '!')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, "2 \"", KEYCODE_2, IP_JOY_NONE, '2', '\"')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "3 #", KEYCODE_3, IP_JOY_NONE, '3', '#')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, "4 $", KEYCODE_4, IP_JOY_NONE, '4', '$')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, "5 %", KEYCODE_5, IP_JOY_NONE, '5', '%')
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "6 &", KEYCODE_6, IP_JOY_NONE, '6', '&')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "7 '", KEYCODE_7, IP_JOY_NONE, '7', '\'')

	/* [7] */
	PORT_START
	PORT_KEY2 (0x01, IP_ACTIVE_LOW, "8 (", KEYCODE_8, IP_JOY_NONE, '8', '(')
	PORT_KEY2 (0x02, IP_ACTIVE_LOW, "9 )", KEYCODE_9, IP_JOY_NONE, '9', ')')
	PORT_KEY2 (0x04, IP_ACTIVE_LOW, ": *", KEYCODE_QUOTE, IP_JOY_NONE, ':', '*')
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "; +", KEYCODE_COLON, IP_JOY_NONE, ';', '+')
	PORT_KEY2 (0x10, IP_ACTIVE_LOW, ", <", KEYCODE_COMMA, IP_JOY_NONE, ',', '<')
	PORT_KEY2 (0x20, IP_ACTIVE_LOW, ". >", KEYCODE_STOP, IP_JOY_NONE, '.', '>') 
	PORT_KEY2 (0x40, IP_ACTIVE_LOW, "/ ?", KEYCODE_SLASH, IP_JOY_NONE, '/', '?')
	PORT_KEY2 (0x80, IP_ACTIVE_LOW, "  _", KEYCODE_END, IP_JOY_NONE, 0, '_')

	/* [8] */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOME/CLR", KEYCODE_HOME, IP_JOY_NONE)
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "UP", KEYCODE_UP, IP_JOY_NONE, UCHAR_MAMEKEY(UP))
	PORT_KEY1 (0x04, IP_ACTIVE_LOW, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE, UCHAR_MAMEKEY(RIGHT))
	PORT_KEY2 (0x08, IP_ACTIVE_LOW, "INS/DEL", KEYCODE_DEL, KEYCODE_INSERT, 8, UCHAR_MAMEKEY(INSERT))
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRPH", KEYCODE_LALT, KEYCODE_RALT)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "KANA", KEYCODE_SCRLOCK, IP_JOY_NONE /* BAD */)
	PORT_KEY1 (0x40, IP_ACTIVE_LOW, "SHIFT", KEYCODE_LSHIFT, KEYCODE_RSHIFT, UCHAR_SHIFT_1)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, KEYCODE_RCONTROL)

	/* [9] */
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "STOP", KEYCODE_PAUSE, IP_JOY_NONE)
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "F1", KEYCODE_F1, IP_JOY_NONE, UCHAR_MAMEKEY(F1))
	PORT_KEY1 (0x04, IP_ACTIVE_LOW, "F2", KEYCODE_F2, IP_JOY_NONE, UCHAR_MAMEKEY(F2))
	PORT_KEY1 (0x08, IP_ACTIVE_LOW, "F3", KEYCODE_F3, IP_JOY_NONE /* BAD */, UCHAR_MAMEKEY(F3))
	PORT_KEY1 (0x10, IP_ACTIVE_LOW, "F4", KEYCODE_F4, IP_JOY_NONE /* BAD */, UCHAR_MAMEKEY(F4))
	PORT_KEY1 (0x20, IP_ACTIVE_LOW, "F5", KEYCODE_F5, IP_JOY_NONE, UCHAR_MAMEKEY(F5))
	PORT_KEY1 (0x40, IP_ACTIVE_LOW, "SPACE", KEYCODE_SPACE, IP_JOY_NONE, ' ')
	PORT_KEY1 (0x80, IP_ACTIVE_LOW, "ESC", KEYCODE_ESC, IP_JOY_NONE, 27 /* BAD */)

	/* [10] */
	PORT_START
	PORT_KEY1 (0x01, IP_ACTIVE_LOW, "TAB", KEYCODE_TAB, IP_JOY_NONE, 9 /* BAD */)
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "DOWN", KEYCODE_DOWN, IP_JOY_NONE, UCHAR_MAMEKEY(DOWN))
	PORT_KEY1 (0x04, IP_ACTIVE_LOW, "LEFT", KEYCODE_LEFT, IP_JOY_NONE, UCHAR_MAMEKEY(LEFT))
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "HELP", KEYCODE_END, IP_JOY_NONE /* BAD? */)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "COPY", KEYCODE_PRTSCR, IP_JOY_NONE /* BAD? */)
	PORT_KEY1 (0x20, IP_ACTIVE_LOW, "-", KEYCODE_MINUS_PAD, IP_JOY_NONE, '-')
	PORT_KEY1 (0x40, IP_ACTIVE_LOW, "/", KEYCODE_SLASH_PAD, IP_JOY_NONE, '/')
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE /* BAD? */)

	/* [11] */
	PORT_START
	PORT_KEY1 (0x01, IP_ACTIVE_LOW, "ROLL UP", KEYCODE_PGUP, IP_JOY_NONE /* BAD? */, UCHAR_MAMEKEY(PGUP))
	PORT_KEY1 (0x02, IP_ACTIVE_LOW, "ROLL DOWN", KEYCODE_PGDN, IP_JOY_NONE /* BAD? */, UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	DUMMY_ROW	/* port 0x0c */ \
	DUMMY_ROW	/* port 0x0d */ \
	DUMMY_ROW	/* port 0x0e */ \
	DUMMY_ROW	/* port 0x0f */

	PORT_START		/* EXSWITCH */
	SW_V1V2N
	SW_HS
	SW_4MHZ_ONLY
	SW_SERIAL

	PORT_START		/* DIP-SW1 */
	DIPSW_1_1
	DIPSW_1_2
	DIPSW_1_3
	DIPSW_1_4
	DIPSW_1_5
	DIPSW_1_6
	DIPSW_1_7

	PORT_START		/* DIP-SW2 */
	DIPSW_2_1
	DIPSW_2_2
	DIPSW_2_3
	DIPSW_2_4
	DIPSW_2_5
	DIPSW_2_6
	DIPSW_2_7
	DIPSW_2_8

	PORT_START		/* extension memory setting */
	SW_EXTMEM

INPUT_PORTS_END


ADDRESS_MAP_START( pc8801_readmem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x5fff) AM_READ( MRA8_BANK1 )
    AM_RANGE( 0x6000, 0x7fff) AM_READ( MRA8_BANK2 )
    AM_RANGE( 0x8000, 0x83ff) AM_READ( MRA8_BANK3 )
    AM_RANGE( 0x8400, 0xbfff) AM_READ( MRA8_BANK4 )
    AM_RANGE( 0xc000, 0xefff) AM_READ( MRA8_BANK5 )
    AM_RANGE( 0xf000, 0xffff) AM_READ( MRA8_BANK6 )
ADDRESS_MAP_END

ADDRESS_MAP_START( pc8801_writemem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x5fff) AM_WRITE( MWA8_BANK1 )
    AM_RANGE( 0x6000, 0x7fff) AM_WRITE( MWA8_BANK2 )
    AM_RANGE( 0x8000, 0x83ff) AM_WRITE( MWA8_BANK3 )
    AM_RANGE( 0x8400, 0xbfff) AM_WRITE( MWA8_BANK4 )
    AM_RANGE( 0xc000, 0xefff) AM_WRITE( MWA8_BANK5 )
    AM_RANGE( 0xf000, 0xffff) AM_WRITE( MWA8_BANK6 )
ADDRESS_MAP_END

static  READ8_HANDLER ( pc8801_unknown_in )
{
  logerror ("pc8801 : read from unknown port 0x%.2x.\n",offset);
  return 0xff;
}

ADDRESS_MAP_START( pc88sr_readport , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x00, 0x00) AM_READ( input_port_0_r )  /* keyboard */
    AM_RANGE( 0x01, 0x01) AM_READ( input_port_1_r )  /* keyboard */
    AM_RANGE( 0x02, 0x02) AM_READ( input_port_2_r )  /* keyboard */
    AM_RANGE( 0x03, 0x03) AM_READ( input_port_3_r )  /* keyboard */
    AM_RANGE( 0x04, 0x04) AM_READ( input_port_4_r )  /* keyboard */
    AM_RANGE( 0x05, 0x05) AM_READ( input_port_5_r )  /* keyboard */
    AM_RANGE( 0x06, 0x06) AM_READ( input_port_6_r )  /* keyboard */
    AM_RANGE( 0x07, 0x07) AM_READ( input_port_7_r )  /* keyboard */
    AM_RANGE( 0x08, 0x08) AM_READ( input_port_8_r )  /* keyboard */
    AM_RANGE( 0x09, 0x09) AM_READ( input_port_9_r )  /* keyboard */
    AM_RANGE( 0x0a, 0x0a) AM_READ( input_port_10_r ) /* keyboard */
    AM_RANGE( 0x0b, 0x0b) AM_READ( input_port_11_r ) /* keyboard */
    AM_RANGE( 0x0c, 0x0c) AM_READ( input_port_12_r ) /* keyboard */
    AM_RANGE( 0x0d, 0x0d) AM_READ( input_port_13_r ) /* keyboard */
    AM_RANGE( 0x0e, 0x0e) AM_READ( input_port_14_r ) /* keyboard */
    AM_RANGE( 0x0f, 0x0f) AM_READ( input_port_15_r ) /* keyboard */
     /* { 0x20, 0x21, }, RS-232C and cassette (not yet) */
    AM_RANGE( 0x30, 0x30) AM_READ( pc88sr_inport_30 ) /* DIP-SW1 */
    AM_RANGE( 0x31, 0x31) AM_READ( pc88sr_inport_31 ) /* DIP-SW2 */
    AM_RANGE( 0x32, 0x32) AM_READ( pc88sr_inport_32 )
    AM_RANGE( 0x40, 0x40) AM_READ( pc88sr_inport_40 )
    AM_RANGE( 0x44, 0x44) AM_READ( YM2203_status_port_0_r )
    AM_RANGE( 0x45, 0x45) AM_READ( YM2203_read_port_0_r )
    /* { 0x46, 0x47, }, OPNA extra port (not yet) */
    AM_RANGE( 0x50, 0x51) AM_READ( pc8801_crtc_read )
    AM_RANGE( 0x5c, 0x5c) AM_READ( pc8801_vramtest )
    AM_RANGE( 0x60, 0x68) AM_READ( pc8801_dmac_read )
    /* { 0x6e, 0x6e, }, CPU clock info (not yet) */
    /* { 0x6f, 0x6f, }, RS-232C speed ctrl (not yet) */
    AM_RANGE( 0x70, 0x70) AM_READ( pc8801_inport_70 )
    AM_RANGE( 0x71, 0x71) AM_READ( pc88sr_inport_71 )
    /* { 0x90, 0x9f, }, CD-ROM (unknown -- not yet) */
    /* { 0xa0, 0xa3, }, music & network (unknown -- not yet) */
    /* { 0xa8, 0xad, }, second sound board (not yet) */
    /* { 0xb4, 0xb5, }, Video art board (unknown -- not yet) */
    /* { 0xc1, 0xc1, }, (unknown -- not yet) */
    /* { 0xc2, 0xcf, }, music (unknown -- not yet) */
    /* { 0xd0, 0xd7, }, music & GP-IB (unknown -- not yet) */
    /* { 0xd8, 0xd8, }, GP-IB (unknown -- not yet) */
    /* { 0xdc, 0xdf, }, MODEM (unknown -- not yet) */
    AM_RANGE( 0xe2, 0xe3) AM_READ( pc8801_read_extmem ) /* expand RAM select */
    AM_RANGE( 0xe8, 0xeb) AM_READ( pc8801_read_kanji1 )
    AM_RANGE( 0xec, 0xed) AM_READ( pc8801_read_kanji2 ) /* JIS level2 Kanji ROM */
    /* { 0xf3, 0xf3, }, DMA floppy (unknown -- not yet) */
    /* { 0xf4, 0xf7, }, DMA 5'floppy (may be not released) */
    /* { 0xf8, 0xfb, }, DMA 8'floppy (unknown -- not yet) */
    AM_RANGE( 0xfc, 0xff) AM_READ( ppi8255_0_r )

    AM_RANGE( 0x00, 0xff) AM_READ( pc8801_unknown_in )
ADDRESS_MAP_END

static WRITE8_HANDLER ( pc8801_unknown_out )
{
  logerror ("pc8801 : write 0x%.2x to unknown port 0x%.2x.\n",data,offset);
}

ADDRESS_MAP_START( pc88sr_writeport , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x10, 0x10) AM_WRITE( pc8801_calender ) /* printer and clock and UOP3 */
     /* { 0x20, 0x21, }, RS-232C and cassette (not yet) */
    AM_RANGE( 0x30, 0x30) AM_WRITE( pc88sr_outport_30 )
    AM_RANGE( 0x31, 0x31) AM_WRITE( pc88sr_outport_31 )
    AM_RANGE( 0x32, 0x32) AM_WRITE( pc88sr_outport_32 )
    AM_RANGE( 0x34, 0x35) AM_WRITE( pc88sr_ALU )
    AM_RANGE( 0x40, 0x40) AM_WRITE( pc88sr_outport_40 )
    AM_RANGE( 0x44, 0x44) AM_WRITE( YM2203_control_port_0_w )
    AM_RANGE( 0x45, 0x45) AM_WRITE( YM2203_write_port_0_w )
    /* { 0x46, 0x47, }, OPNA extra port (not yet) */
    AM_RANGE( 0x50, 0x51) AM_WRITE( pc8801_crtc_write )
    AM_RANGE( 0x52, 0x5b) AM_WRITE( pc8801_palette_out )
    AM_RANGE( 0x5c, 0x5f) AM_WRITE( pc8801_vramsel )
    AM_RANGE( 0x60, 0x68) AM_WRITE( pc8801_dmac_write )
    /* { 0x6f, 0x6f, }, RS-232C speed ctrl (not yet) */
    AM_RANGE( 0x70, 0x70) AM_WRITE( pc8801_outport_70 )
    AM_RANGE( 0x71, 0x71) AM_WRITE( pc88sr_outport_71 )
    AM_RANGE( 0x78, 0x78) AM_WRITE( pc8801_outport_78 ) /* text window increment */
    /* { 0x90, 0x9f, }, CD-ROM (unknown -- not yet) */
    /* { 0xa0, 0xa3, }, music & network (unknown -- not yet) */
    /* { 0xa8, 0xad, }, second sound board (not yet) */
    /* { 0xb4, 0xb5, }, Video art board (unknown -- not yet) */
    /* { 0xc1, 0xc1, }, (unknown -- not yet) */
    /* { 0xc2, 0xcf, }, music (unknown -- not yet) */
    /* { 0xd0, 0xd7, }, music & GP-IB (unknown -- not yet) */
    /* { 0xd8, 0xd8, }, GP-IB (unknown -- not yet) */
    /* { 0xdc, 0xdf, }, MODEM (unknown -- not yet) */
    AM_RANGE( 0xe2, 0xe3) AM_WRITE( pc8801_write_extmem ) /* expand RAM select */
    AM_RANGE( 0xe4, 0xe4) AM_WRITE( pc8801_write_interrupt_level )
    AM_RANGE( 0xe6, 0xe6) AM_WRITE( pc8801_write_interrupt_mask )
    /* { 0xe7, 0xe7, }, (unknown -- not yet) */
    AM_RANGE( 0xe8, 0xeb) AM_WRITE( pc8801_write_kanji1 )
    AM_RANGE( 0xec, 0xed) AM_WRITE( pc8801_write_kanji2 ) /* JIS level2 Kanji ROM */
    /* { 0xf0, 0xf1, }, Kana to Kanji dictionary ROM select (not yet) */
    /* { 0xf3, 0xf3, }, DMA floppy (unknown -- not yet) */
    /* { 0xf4, 0xf7, }, DMA 5'floppy (may be not released) */
    /* { 0xf8, 0xfb, }, DMA 8'floppy (unknown -- not yet) */
    AM_RANGE( 0xfc, 0xff) AM_WRITE( ppi8255_0_w )

    AM_RANGE( 0x00, 0xff) AM_WRITE( pc8801_unknown_out )
ADDRESS_MAP_END

static INTERRUPT_GEN( pc8801fd_interrupt )
{
}

ADDRESS_MAP_START( pc8801fd_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x07ff) AM_READ( MRA8_ROM )
	AM_RANGE( 0x4000, 0x7fff) AM_READ( MRA8_RAM )
ADDRESS_MAP_END

ADDRESS_MAP_START( pc8801fd_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x07ff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0x4000, 0x7fff) AM_WRITE( MWA8_RAM )
ADDRESS_MAP_END

ADDRESS_MAP_START( pc8801fd_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0xf8, 0xf8) AM_READ( pc8801fd_nec765_tc )
	AM_RANGE( 0xfa, 0xfa) AM_READ( nec765_status_r )
	AM_RANGE( 0xfb, 0xfb) AM_READ( nec765_data_r )
	AM_RANGE( 0xfc, 0xff) AM_READ( ppi8255_1_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( pc8801fd_writeport , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0xfb, 0xfb) AM_WRITE( nec765_data_w )
    AM_RANGE( 0xfc, 0xff) AM_WRITE( ppi8255_1_w )
ADDRESS_MAP_END

ROM_START (pc88srl)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD ("n80.rom", 0x00000, 0x8000, CRC(27e1857d))
	ROM_LOAD ("n88.rom", 0x08000, 0x8000, CRC(a0fc0473))
	ROM_LOAD ("n88_0.rom", 0x10000, 0x2000, CRC(710a63ec))
	ROM_LOAD ("n88_1.rom", 0x12000, 0x2000, CRC(c0bd2aa6))
	ROM_LOAD ("n88_2.rom", 0x14000, 0x2000, CRC(af2b6efa))
	ROM_LOAD ("n88_3.rom", 0x16000, 0x2000, CRC(7713c519))
	ROM_REGION(0x10000,REGION_CPU2,0)
	ROM_LOAD ("disk.rom", 0x0000, 0x0800, CRC(2158d307))
	ROM_REGION(0x40000,REGION_GFX1,0)
	ROM_LOAD ("kanji1.rom", 0x00000, 0x20000, CRC(6178bd43))
	ROM_LOAD ("kanji2.rom", 0x20000, 0x20000, CRC(154803cc))
ROM_END

ROM_START (pc88srh)
	ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD ("n80.rom", 0x00000, 0x8000, CRC(27e1857d))
	ROM_LOAD ("n88.rom", 0x08000, 0x8000, CRC(a0fc0473))
	ROM_LOAD ("n88_0.rom", 0x10000, 0x2000, CRC(710a63ec))
	ROM_LOAD ("n88_1.rom", 0x12000, 0x2000, CRC(c0bd2aa6))
	ROM_LOAD ("n88_2.rom", 0x14000, 0x2000, CRC(af2b6efa))
	ROM_LOAD ("n88_3.rom", 0x16000, 0x2000, CRC(7713c519))
	ROM_REGION(0x10000,REGION_CPU2,0)
	ROM_LOAD ("disk.rom", 0x0000, 0x0800, CRC(2158d307))
	ROM_REGION(0x40000,REGION_GFX1,0)
	ROM_LOAD ("kanji1.rom", 0x00000, 0x20000, CRC(6178bd43))
	ROM_LOAD ("kanji2.rom", 0x20000, 0x20000, CRC(154803cc))
ROM_END

static struct beep_interface pc8801_beep_interface =
{
        1,
        { 10 }
};

static  READ8_HANDLER(opn_dummy_input){return 0xff;}

static struct YM2203interface ym2203_interface =
{
        1,
        3993600, /* Should be accurate */
        { YM2203_VOL(50,50) },
        { opn_dummy_input },
        { opn_dummy_input },
        { 0 },
        { 0 },
	{ pc88sr_sound_interupt }
};


static MACHINE_DRIVER_START( pc88srl )
	/* basic machine hardware */

	/* main CPU */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)        /* 4 Mhz */
	MDRV_CPU_PROGRAM_MAP(pc8801_readmem,pc8801_writemem)
	MDRV_CPU_IO_MAP(pc88sr_readport,pc88sr_writeport)
	MDRV_CPU_VBLANK_INT(pc8801_interrupt,1)

	/* sub CPU(5 inch floppy drive) */
	MDRV_CPU_ADD_TAG("sub", Z80, 4000000)		/* 4 Mhz */
	MDRV_CPU_PROGRAM_MAP(pc8801fd_readmem,pc8801fd_writemem)
	MDRV_CPU_IO_MAP(pc8801fd_readport,pc8801fd_writeport)
	MDRV_CPU_VBLANK_INT(pc8801fd_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(5000)

	MDRV_MACHINE_INIT( pc88srl )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER
		| VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_ASPECT_RATIO(8,5)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(18)
	MDRV_COLORTABLE_LENGTH(32)
	MDRV_PALETTE_INIT( pc8801 )

	MDRV_VIDEO_START(pc8801)
	MDRV_VIDEO_UPDATE(pc8801)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(BEEP, pc8801_beep_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc88srh )
	MDRV_IMPORT_FROM( pc88srl )
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_INTERLEAVE(6000)

	MDRV_MACHINE_INIT( pc88srh )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(8, 5)
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 400-1)
MACHINE_DRIVER_END

SYSTEM_CONFIG_START(pc88)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2, "d88\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_OR_READ, device_init_d88image_floppy, NULL, device_load_d88image_floppy, NULL, floppy_status)
SYSTEM_CONFIG_END


/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT		INIT	CONFIG	COMPANY	FULLNAME */
COMPX( 1985, pc88srl, 0,		0,		pc88srl,  pc88sr,	0,		pc88,	"Nippon Electronic Company",  "PC-8801 MKIISR (Lores display, VSYNC 15KHz)", 0 )
COMPX( 1985, pc88srh, pc88srl,	0,		pc88srh,  pc88sr,	0,		pc88,	"Nippon Electronic Company",  "PC-8801 MKIISR (Hires display, VSYNC 24KHz)", 0 )
