/******************************************************************************
 *
 *  kaypro.c
 *
 *  Driver for Kaypro 2x
 *
 *  Juergen Buchmueller, July 1998
 *  Benjamin C. W. Sittler, July 1998 (new keyboard)
 *
 ******************************************************************************/

#include "driver.h"
#include "includes/kaypro.h"
#include "devices/basicdsk.h"

static MEMORY_READ_START( readmem_kaypro )
    { 0x0000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_kaypro )
    { 0x0000, 0xffff, MWA_RAM },
MEMORY_END

PORT_READ_START( readport_kaypro )
    { BIOS_CONST,  BIOS_CONST,  kaypro_const_r },
    { BIOS_CONIN,  BIOS_CONIN,  kaypro_conin_r },
    { BIOS_CONOUT, BIOS_CONOUT, kaypro_conout_r },
    { BIOS_CMD,    BIOS_CMD,    cpm_bios_command_r },
PORT_END

PORT_WRITE_START( writeport_kaypro )
    { BIOS_CONST,  BIOS_CONST,  kaypro_const_w },
    { BIOS_CONIN,  BIOS_CONIN,  kaypro_conin_w },
    { BIOS_CONOUT, BIOS_CONOUT, kaypro_conout_w },
    { BIOS_CMD,    BIOS_CMD,    cpm_bios_command_w },
PORT_END

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

INPUT_PORTS_START( kaypro )
    PORT_START /* IN0 */
    PORT_BIT(0xff, 0xff, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "ESC",       KEYCODE_ESC,         IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1  !",      KEYCODE_1,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2  \"",     KEYCODE_2,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3  #",      KEYCODE_3,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4  $",      KEYCODE_4,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5  %",      KEYCODE_5,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6  &",      KEYCODE_6,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7  '",      KEYCODE_7,           IP_JOY_NONE )

    PORT_START /* KEY ROW 1 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "8  *",      KEYCODE_8,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "9  (",      KEYCODE_9,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0  )",      KEYCODE_0,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "-  _",      KEYCODE_MINUS,       IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "=  +",      KEYCODE_EQUALS,      IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "`  ~",      KEYCODE_TILDE,       IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "BACK SPACE",KEYCODE_BACKSPACE,   IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "TAB",       KEYCODE_TAB,         IP_JOY_NONE )

    PORT_START /* KEY ROW 2 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "q  Q",      KEYCODE_Q,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "w  W",      KEYCODE_W,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "e  E",      KEYCODE_E,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "r  R",      KEYCODE_R,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "t  T",      KEYCODE_T,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "y  Y",      KEYCODE_Y,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "u  U",      KEYCODE_U,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "i  I",      KEYCODE_I,           IP_JOY_NONE )

    PORT_START /* KEY ROW 3 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "o  O",      KEYCODE_O,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "p  P",      KEYCODE_P,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "[  {",      KEYCODE_OPENBRACE,   IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "]  }",      KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "RETURN",    KEYCODE_ENTER,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "DEL",       KEYCODE_DEL,         IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "CTRL",      KEYCODE_LCONTROL,    IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD | IPF_TOGGLE,
                                        "CAPS LOCK", KEYCODE_CAPSLOCK,    IP_JOY_NONE )

    PORT_START /* KEY ROW 4 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "a  A",      KEYCODE_A,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "s  S",      KEYCODE_S,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "d  D",      KEYCODE_D,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "f  F",      KEYCODE_F,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "g  G",      KEYCODE_G,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "h  H",      KEYCODE_H,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "j  J",      KEYCODE_J,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "k  K",      KEYCODE_K,           IP_JOY_NONE )

    PORT_START /* KEY ROW 5 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "l  L",      KEYCODE_L,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, ";  :",      KEYCODE_COLON,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "'  \"",     KEYCODE_QUOTE,       IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "\\  |",     KEYCODE_ASTERISK,    IP_JOY_NONE )

    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "SHIFT (L)", KEYCODE_LSHIFT,      IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "z  Z",      KEYCODE_Z,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "x  X",      KEYCODE_X,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "c  C",      KEYCODE_C,           IP_JOY_NONE )

    PORT_START /* KEY ROW 6 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "v  V",      KEYCODE_V,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "b  B",      KEYCODE_B,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "n  N",      KEYCODE_N,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "m  M",      KEYCODE_M,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, ",  <",      KEYCODE_COMMA,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ".  >",      KEYCODE_STOP,        IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "/  ?",      KEYCODE_SLASH,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "SHIFT (R)", KEYCODE_RSHIFT,      IP_JOY_NONE )

    PORT_START /* KEY ROW 7 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "LINE FEED", IP_KEY_NONE,         IP_JOY_NONE )

    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "SPACE",     KEYCODE_SPACE,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "- (KP)",    KEYCODE_MINUS_PAD,   IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, ", (KP)",    KEYCODE_PLUS_PAD,    IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "ENTER (KP)",KEYCODE_ENTER_PAD,   IP_JOY_NONE )
//  PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ". (KP)",    KEYCODE_STOP_PAD,    IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0 (KP)",    KEYCODE_0_PAD,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1 (KP)",    KEYCODE_1_PAD,       IP_JOY_NONE )

    PORT_START /* KEY ROW 8 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2 (KP)",    KEYCODE_2_PAD,       IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3 (KP)",    KEYCODE_3_PAD,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4 (KP)",    KEYCODE_4_PAD,       IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5 (KP)",    KEYCODE_5_PAD,       IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6 (KP)",    KEYCODE_6_PAD,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7 (KP)",    KEYCODE_7_PAD,       IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "8 (KP)",    KEYCODE_8_PAD,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "9 (KP)",    KEYCODE_9_PAD,       IP_JOY_NONE )

    PORT_START /* KEY ROW 9 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "UP",        KEYCODE_UP,          IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "DOWN",      KEYCODE_DOWN,        IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "LEFT",      KEYCODE_LEFT,        IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "RIGHT",     KEYCODE_RIGHT,       IP_JOY_NONE )
    PORT_BITX(0xf0, 0x00, IPT_UNUSED,   0,           IP_KEY_NONE,         IP_JOY_NONE )
INPUT_PORTS_END

#define FW  ((KAYPRO_FONT_W+7)/8)*8
#define FH  KAYPRO_FONT_H

static struct GfxLayout charlayout =
{
    FW, FH,         /* 8*16 characters */
    4 * 256,        /* 4 * 256 characters */
    1,              /* 1 bits per pixel */
    { 0 },          /* no bitplanes; 1 bit per pixel */
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
    FW * FH         /* every char takes 16 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0, &charlayout, 0, 4},
MEMORY_END   /* end of array */

static unsigned char kaypro_palette[] =
{
      0,  0,  0,    /* black */
      0,240,  0,    /* green */
      0,120,  0,    /* dim green */
      0,240,  0,    /* flashing green */
      0,120,  0,    /* flashing dim green */

    240,  0,  0,    /* just to keep the frontend happy */
      0,  0,240,
    120,120,120,
    240,240,240,
};

static unsigned short kaypro_colortable[] =
{
    0,  1,      /* green on black */
    0,  2,      /* dim green on black */
    0,  3,      /* flashing green on black */
    0,  4,      /* flashing dim green on black */
};


/* Initialise the palette */
static PALETTE_INIT( kaypro )
{
	palette_set_colors(0, kaypro_palette, sizeof(kaypro_palette) / 3);
    memcpy(colortable, kaypro_colortable, sizeof(kaypro_colortable));
}

static MACHINE_DRIVER_START( kaypro )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)        /* 4 Mhz */
	MDRV_CPU_MEMORY(readmem_kaypro,writemem_kaypro)
	MDRV_CPU_PORTS(readport_kaypro,writeport_kaypro)
	MDRV_CPU_VBLANK_INT(kaypro_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_INIT( kaypro )
	MDRV_MACHINE_STOP( kaypro )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*KAYPRO_FONT_W, 25*KAYPRO_FONT_H)
	MDRV_VISIBLE_AREA(0*KAYPRO_FONT_W, 80*KAYPRO_FONT_W-1, 0*KAYPRO_FONT_H, 25*KAYPRO_FONT_H-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(kaypro_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(kaypro_colortable) / sizeof(kaypro_colortable[0]))
	MDRV_PALETTE_INIT( kaypro )

	MDRV_VIDEO_START( kaypro )
	MDRV_VIDEO_UPDATE( kaypro )
MACHINE_DRIVER_END


ROM_START (kaypro)
    ROM_REGION(0x10000,REGION_CPU1,0) /* 64K for the Z80 */
    /* totally empty :) */

    ROM_REGION(0x04000,REGION_GFX1,0)  /* 4 * 4K font ram */
    ROM_LOAD ("kaypro2x.fnt", 0x0000, 0x1000, 0x5f72da5b)

    ROM_REGION(0x01600,REGION_CPU2,0)  /* 5,5K for CCP and BDOS buffer */
    ROM_LOAD ("cpm62k.sys",   0x0000, 0x1600, 0xd10cd036)
ROM_END

SYSTEM_CONFIG_START(kaypro)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",	device_load_cpm_floppy)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1982, kaypro,   0,		0,		kaypro,   kaypro,   kaypro,   kaypro,   "Non Linear Systems",  "Kaypro 2x" )
