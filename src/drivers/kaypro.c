/******************************************************************************
 *
 *	kaypro.c
 *
 *	Driver for Kaypro 2x
 *
 *	Juergen Buchmueller, July 1998
 *      Benjamin C. W. Sittler, July 1998 (new keyboard)
 *
 ******************************************************************************/

#include "driver.h"
#include "machine/kaypro.h"
#include "vidhrdw/kaypro.h"

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xffff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xffff, MWA_RAM },
	{ -1 }
};

static struct IOReadPort readport[] =
{
	{ BIOS_CONST,  BIOS_CONST,	kaypro_const_r },
    { BIOS_CONIN,  BIOS_CONIN,  kaypro_conin_r },
    { BIOS_CONOUT, BIOS_CONOUT, kaypro_conout_r },
    { BIOS_CMD,    BIOS_CMD,    cpm_bios_command_r },
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{ BIOS_CONST,  BIOS_CONST,	kaypro_const_w },
    { BIOS_CONIN,  BIOS_CONIN,  kaypro_conin_w },
    { BIOS_CONOUT, BIOS_CONOUT, kaypro_conout_w },
    { BIOS_CMD,    BIOS_CMD,    cpm_bios_command_w },
    { -1 }
};

/*
 * The KAYPRO keyboard has roughly the following layout:
 *
 *                                                                  [up] [down] [left] [right]
 *         [ESC] [1!] [2@] [3#] [4$] [5%] [6^] [7&] [8*] [9(] [0)] [-_] [=+] [`~] [BACK SPACE]
 *           [TAB] [qQ] [wW] [eE] [rR] [tT] [yY] [uU] [iI] [oO] [pP] [[{] []}]       [DEL]
 *[CTRL] [CAPS LOCK] [aA] [sS] [dD] [fF] [gG] [hH] [jJ] [kK] [lL] [;:] ['"] [RETURN] [\|]
 *            [SHIFT] [zZ] [xX] [cC] [vV] [bB] [nN] [mM] [,<] [.>] [/?] [SHIFT] [LINE FEED]
 *                      [                 SPACE                     ]
 *
 * [7] [8] [9] [-]
 * [4] [5] [6] [,]
 * [1] [2] [3]
 * [  0  ] [.] [ENTER]
 *
 * Notes on Appearance
 * -------------------
 * The RETURN key is shaped like a backwards "L". The keypad ENTER key is actually
 * oriented vertically, not horizontally. The alpha keys are marked with the uppercase letters
 * only. Other keys with two symbols have the shifted symbol printed above the unshifted symbol.
 * The keypad is actually located to the right of the main keyboard; it is shown separately here
 * as a convenience to users of narrow listing windows. The arrow keys are actually marked with
 * arrow graphics pointing in the appropriate directions. The F and J keys are specially shaped,
 * since they are the "home keys" for touch-typing. The CAPS LOCK key has a build-in red indicator
 * which is lit when CAPS LOCK is pressed once, and extinguished when the key is pressed again.
 *
 * Technical Notes
 * ---------------
 * The keyboard interfaces to the computer using a serial protocol. Modifier keys are handled
 * inside the keyboards, as is the CAPS LOCK key. The arrow keys and the numeric keypad send
 * non-ASCII codes which are not affected by the modifier keys. The remaining keys send the
 * appropriate ASCII values.
 *
 * The keyboard has a built-in buzzer which is activated briefly by a non-modifier keypress,
 * producing a "clicking" sound for user feedback. Additionally, this buzzer can be activated
 * for a longer period by sending a 0x04 byte to the keyboard. This is used by the ROM soft
 * terminal to alert the user in case of a BEL.
 */

INPUT_PORTS_START( kaypro_input_ports )
	PORT_START /* IN0 */
	PORT_BIT(0xff, 0xff, IPT_UNUSED)

	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "ESC",       OSD_KEY_ESC,         IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1  !",      OSD_KEY_1,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2  \"",     OSD_KEY_2,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3  #",      OSD_KEY_3,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4  $",      OSD_KEY_4,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5  %",      OSD_KEY_5,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6  &",      OSD_KEY_6,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7  '",      OSD_KEY_7,           IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "8  *",      OSD_KEY_8,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "9  (",      OSD_KEY_9,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0  )",      OSD_KEY_0,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "-  _",      OSD_KEY_MINUS,       IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "=  +",      OSD_KEY_EQUALS,      IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "`  ~",      OSD_KEY_TILDE,       IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "BACK SPACE",OSD_KEY_BACKSPACE,   IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "TAB",       OSD_KEY_TAB,         IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "q  Q",      OSD_KEY_Q,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "w  W",      OSD_KEY_W,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "e  E",      OSD_KEY_E,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "r  R",      OSD_KEY_R,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "t  T",      OSD_KEY_T,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "y  Y",      OSD_KEY_Y,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "u  U",      OSD_KEY_U,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "i  I",      OSD_KEY_I,           IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "o  O",      OSD_KEY_O,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "p  P",      OSD_KEY_P,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "[  {",      OSD_KEY_OPENBRACE,   IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "]  }",      OSD_KEY_CLOSEBRACE,  IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "RETURN",    OSD_KEY_ENTER,       IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "DEL",       OSD_KEY_DEL,         IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "CTRL",      OSD_KEY_LCONTROL,    IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD | IPF_TOGGLE,
                                            "CAPS LOCK", OSD_KEY_CAPSLOCK, IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "a  A",      OSD_KEY_A,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "s  S",      OSD_KEY_S,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "d  D",      OSD_KEY_D,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "f  F",      OSD_KEY_F,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "g  G",      OSD_KEY_G,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "h  H",      OSD_KEY_H,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "j  J",      OSD_KEY_J,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "k  K",      OSD_KEY_K,           IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "l  L",      OSD_KEY_L,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, ";  :",      OSD_KEY_COLON,       IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "'  \"",     OSD_KEY_QUOTE,       IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "\\  |",     OSD_KEY_ASTERISK,    IP_JOY_NONE, 0)

	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "SHIFT (L)", OSD_KEY_LSHIFT,      IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "z  Z",      OSD_KEY_Z,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "x  X",      OSD_KEY_X,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "c  C",      OSD_KEY_C,           IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "v  V",      OSD_KEY_V,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "b  B",      OSD_KEY_B,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "n  N",      OSD_KEY_N,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "m  M",      OSD_KEY_M,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, ",  <",      OSD_KEY_COMMA,       IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ".  >",      OSD_KEY_STOP,        IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "/  ?",      OSD_KEY_SLASH,       IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "SHIFT (R)", OSD_KEY_RSHIFT,      IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 7 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "LINE FEED", IP_KEY_NONE,         IP_JOY_NONE, 0)

	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "SPACE",     OSD_KEY_SPACE,       IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "- (KP)",    OSD_KEY_MINUS_PAD,   IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, ", (KP)",    OSD_KEY_PLUS_PAD,    IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "ENTER (KP)",OSD_KEY_ENTER_PAD,   IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ". (KP)",    OSD_KEY_STOP_PAD,    IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0 (KP)",    OSD_KEY_0_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1 (KP)",    OSD_KEY_1_PAD,       IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 8 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2 (KP)",    OSD_KEY_2_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3 (KP)",    OSD_KEY_3_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4 (KP)",    OSD_KEY_4_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5 (KP)",    OSD_KEY_5_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6 (KP)",    OSD_KEY_6_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7 (KP)",    OSD_KEY_7_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "8 (KP)",    OSD_KEY_8_PAD,       IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "9 (KP)",    OSD_KEY_9_PAD,       IP_JOY_NONE, 0)

	PORT_START /* KEY ROW 9 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "UP",        OSD_KEY_UP,          IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "DOWN",      OSD_KEY_DOWN,        IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "LEFT",      OSD_KEY_LEFT,        IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "RIGHT",     OSD_KEY_RIGHT,       IP_JOY_NONE, 0)
	PORT_BITX(0xf0, 0x00, IPT_UNUSED, 0, IP_KEY_NONE, IP_JOY_NONE, 0)
INPUT_PORTS_END

#define FW  ((KAYPRO_FONT_W+7)/8)*8
#define FH	KAYPRO_FONT_H

static struct GfxLayout charlayout =
{
	FW, FH, 		/* 8*16 characters */
	4 * 256,		/* 4 * 256 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
    { 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15,
	 16,17,18,19,20,21,22,23,
	 24,25,26,27,28,29,30,31 },
	/* y offsets */
    { 0*FW, 1*FW, 2*FW, 3*FW, 4*FW, 5*FW, 6*FW, 7*FW,
	  8*FW, 9*FW,10*FW,11*FW,12*FW,13*FW,14*FW,15*FW,
	 16*FW,17*FW,18*FW,19*FW,20*FW,21*FW,22*FW,23*FW,
	 24*FW,25*FW,26*FW,27*FW,28*FW,29*FW,30*FW,31*FW },
	FW * FH 		/* every char takes 16 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0, &charlayout, 0, 4},
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	  0,  0,  0,	/* black */
	  0,240,  0,	/* green */
	  0,120,  0,	/* dim green */
	  0,240,  0,	/* flashing green */
	  0,120,  0,	/* flashing dim green */

	240,  0,  0,	/* just to keep the frontend happy */
	  0,  0,240,
	120,120,120,
    240,240,240,
};

static short colortable[] =
{
	0,	1,		/* green on black */
	0,	2,		/* dim green on black */
	0,	3,		/* flashing green on black */
	0,	4,		/* flashing dim green on black */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,
			readport,writeport,
			kaypro_interrupt,1, 	/* one interrupt per frame */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	4,
	kaypro_init_machine,
	kaypro_stop_machine,

	/* video hardware */
	80*KAYPRO_FONT_W, 					/* screen width */
	25*KAYPRO_FONT_H, 					/* screen height */
	{ 0*KAYPRO_FONT_W, 80*KAYPRO_FONT_W-1,
	  0*KAYPRO_FONT_H, 25*KAYPRO_FONT_H-1}, /* visible_area */
	gfxdecodeinfo,						/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,	/* palette */
	sizeof(colortable) / sizeof(colortable[0]), /* colortable */
	0,											/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	kaypro_vh_start,
	kaypro_vh_stop,
	kaypro_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START (kaypro_rom)
	ROM_REGION (0x11600)	/* 64K for the Z80 */
	/* totally empty :) */
	
	ROM_REGION (0x4000) 	/* 4 * 4K font ram */
	ROM_LOAD ("kaypro2x.fnt", 0x0000, 0x1000, 0x381b1a9f)

	ROM_REGION (0x1600) 	/* 5,5K for CCP and BDOS buffer */
	ROM_LOAD ("cpm62k.sys",   0x0000, 0x1600, 0x6b517357)
ROM_END

static void kaypro_rom_decode(void)
{
  byte * FONT = Machine->memory_region[1];
  int i;
  
  /* copy font, but add underline in last line */
  for (i = 0; i < 0x1000; i++)
    FONT[0x1000 + i] = ((i % KAYPRO_FONT_H) == (KAYPRO_FONT_H - 1)) ?
	FONT[i] ^ 0xff : FONT[i];

  /* copy font inverted */
  for (i = 0; i < 0x2000; i++)
    FONT[0x2000 + i] = FONT[i] ^ 0xff;
}

struct GameDriver kaypro_driver =
{
	__FILE__,
	0,
	"kaypro",
	"Kaypro 2x",
	"19??",
	"Kaypro",
	"Juergen Buchmueller (MESS driver)\nBenjamin C. W. Sittler (terminal)\nChi-Yuan Lin (CP/M info)",
	0,
	&machine_driver,

	kaypro_rom,
	kaypro_rom_load,		/* load rom_file images */
	kaypro_rom_id,			/* identify rom images */
	1,                      /* number of ROM slots - in this case, a CMD binary */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	0,						/* number of cassette drives supported */
	kaypro_rom_decode,		/* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	kaypro_input_ports, 	/* input ports */

	0,                      /* color_prom */
	palette,				/* color palette */
	colortable, 			/* color lookup table */

	ORIENTATION_DEFAULT,    /* orientation */

	0,                      /* hiscore load */
	0,                      /* hiscore save */
	0,                      /* state load */
	0                       /* state save */
};
