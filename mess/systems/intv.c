/************************************************************************
 *  Mattel Intellivision + Keyboard Component Drivers
 *
 *  Frank Palazzolo
 *  Kyle Davis
 *
 *  TBD:
 *		    Map game controllers correctly (right controller + 16 way)
 *		    Add tape support (intvkbd)
 *		    Add runtime tape loading
 *		    Fix memory system workaround
 *            (memory handler stuff in CP1610, debugger, and shared mem)
 *		    STIC
 *            reenable dirty support
 *		    Cleanup
 *			  Separate stic & vidhrdw better, get rid of *2 for kbd comp
 *		    Add better runtime cart loading
 *		    Switch to tilemap system
 *
 ************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/stic.h"
#include "includes/intv.h"
#include "devices/cartslot.h"

#ifndef VERBOSE
#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif
#endif

static unsigned char intv_palette[] =
{
	0x00, 0x00, 0x00, /* BLACK */
	0x00, 0x2D, 0xFF, /* BLUE */
	0xFF, 0x3D, 0x10, /* RED */
	0xC9, 0xCF, 0xAB, /* TAN */
	0x38, 0x6B, 0x3F, /* DARK GREEN */
	0x00, 0xA7, 0x56, /* GREEN */
	0xFA, 0xEA, 0x50, /* YELLOW */
	0xFF, 0xFC, 0xFF, /* WHITE */
	0xBD, 0xAC, 0xC8, /* GRAY */
	0x24, 0xB8, 0xFF, /* CYAN */
	0xFF, 0xB4, 0x1F, /* ORANGE */
	0x54, 0x6E, 0x00, /* BROWN */
	0xFF, 0x4E, 0x57, /* PINK */
	0xA4, 0x96, 0xFF, /* LIGHT BLUE */
	0x75, 0xCC, 0x80, /* YELLOW GREEN */
	0xB5, 0x1A, 0x58  /* PURPLE */
};

static PALETTE_INIT( intv )
{
	int i,j;

	/* Two copies of the palette */
	palette_set_colors(0, intv_palette, sizeof(intv_palette) / 3);
	palette_set_colors(sizeof(intv_palette) / 3, intv_palette, sizeof(intv_palette) / 3);

    /* Two copies of the color table */
    for(i=0;i<16;i++)
    {
    	for(j=0;j<16;j++)
    	{
    		*colortable++ = i;
    		*colortable++ = j;
		}
	}
    for(i=0;i<16;i++)
    {
    	for(j=0;j<16;j++)
    	{
    		*colortable++ = i+16;
    		*colortable++ = j+16;
		}
	}
}

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	3579545/2,	/* Colorburst/2 */
	{ 100 },
	{ intv_right_control_r },
	{ intv_left_control_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* graphics output */

struct GfxLayout intv_gromlayout =
{
	16, 16,
	256,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*16, 0*16, 1*16, 1*16, 2*16, 2*16, 3*16, 3*16,
	  4*16, 4*16, 5*16, 5*16, 6*16, 6*16, 7*16, 7*16 },
	8 * 16
};

struct GfxLayout intv_gramlayout =
{
	16, 16,
	64,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

struct GfxLayout intvkbd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo intv_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ -1 }
};

static struct	GfxDecodeInfo intvkbd_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ REGION_GFX1, 0x0000, &intvkbd_charlayout, 0, 256},
	{ -1 }
};

INPUT_PORTS_START( intv )
	PORT_START /* IN0 */	/* Right Player Controller Starts Here */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )

	PORT_START /* IN1 */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But1", KEYCODE_LCONTROL, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But2", KEYCODE_Z, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But3", KEYCODE_X, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "NA", KEYCODE_NONE, IP_JOY_NONE )

	PORT_START /* IN2 */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PGUP", KEYCODE_PGUP, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PGDN", KEYCODE_PGDN, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "END", KEYCODE_END, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "HOME", KEYCODE_HOME, IP_JOY_NONE )

	PORT_START /* IN3 */	/* Left Player Controller Starts Here */

	PORT_START /* IN4 */

	PORT_START /* IN5 */

INPUT_PORTS_END

/*
        Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0

 Row 0  NC      NC      NC      NC      NC      NC      CTRL    SHIFT
 Row 1  NC      NC      NC      NC      NC      NC      RPT     LOCK
 Row 2  NC      /       ,       N       V       X       NC      SPC
 Row 3  (right) .       M       B       C       Z       NC      CLS
 Row 4  (down)  ;       K       H       F       S       NC      TAB
 Row 5  ]       P       I       Y       R       W       NC      Q
 Row 6  (up)    -       9       7       5       3       NC      1
 Row 7  =       0       8       6       4       2       NC      [
 Row 8  (return)(left)  O       U       T       E       NC      ESC
 Row 9  DEL     '       L       J       G       D       NC      A
*/

INPUT_PORTS_START( intvkbd )
	PORT_START /* IN0 */	/* Keyboard Row 0 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE )

	PORT_START /* IN1 */	/* Keyboard Row 1 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "REPEAT", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "LOCK", KEYCODE_RSHIFT, IP_JOY_NONE )

	PORT_START /* IN2 */	/* Keyboard Row 2 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, " ", KEYCODE_SPACE, IP_JOY_NONE )

	PORT_START /* IN3 */	/* Keyboard Row 3 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right", KEYCODE_RIGHT, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cls", KEYCODE_LALT, IP_JOY_NONE )

	PORT_START /* IN4 */	/* Keyboard Row 4 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Down", KEYCODE_DOWN, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE )

	PORT_START /* IN5 */	/* Keyboard Row 5 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )

	PORT_START /* IN6 */	/* Keyboard Row 6 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Up", KEYCODE_UP, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", KEYCODE_MINUS, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )

	PORT_START /* IN7 */	/* Keyboard Row 7 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE )

	PORT_START /* IN8 */	/* Keyboard Row 8 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left", KEYCODE_LEFT, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Esc", KEYCODE_ESC, IP_JOY_NONE )

	PORT_START /* IN9 */	/* Keyboard Row 9 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BS)", KEYCODE_BACKSPACE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )

	PORT_START /* IN10 */	/* For tape drive testing... */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_0_PAD, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_1_PAD, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_2_PAD, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_3_PAD, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_4_PAD, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_5_PAD, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_6_PAD, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_7_PAD, IP_JOY_NONE )
INPUT_PORTS_END

static ADDRESS_MAP_START( readmem , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READ( stic_r )
    AM_RANGE(0x0100, 0x01ef) AM_READ( intv_ram8_r )
    AM_RANGE(0x01f0, 0x01ff) AM_READ( AY8914_directread_port_0_lsb_r )
 	AM_RANGE(0x0200, 0x035f) AM_READ( intv_ram16_r )
	AM_RANGE(0x1000, 0x1fff) AM_READ( MRA16_ROM )		/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_READ( MRA16_ROM ) 		/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READ( intv_gram_r )		/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x7fff) AM_READ( MRA16_ROM )		/* Cartridges? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_WRITE( stic_w )
    AM_RANGE(0x0100, 0x01ef) AM_WRITE( intv_ram8_w )
    AM_RANGE(0x01f0, 0x01ff) AM_WRITE( AY8914_directwrite_port_0_lsb_w )
	AM_RANGE(0x0200, 0x035f) AM_WRITE( intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_WRITE( MWA16_ROM ) 		/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_WRITE( MWA16_ROM )		/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_WRITE( intv_gram_w )	/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x7fff) AM_WRITE( MWA16_ROM )		/* Cartridges? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_kbd , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READ( stic_r )
    AM_RANGE(0x0100, 0x01ef) AM_READ( intv_ram8_r )
    AM_RANGE(0x01f0, 0x01ff) AM_READ( AY8914_directread_port_0_lsb_r )
 	AM_RANGE(0x0200, 0x035f) AM_READ( intv_ram16_r )
	AM_RANGE(0x1000, 0x1fff) AM_READ( MRA16_ROM )		/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_READ( MRA16_ROM ) 		/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READ( intv_gram_r )		/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x6fff) AM_READ( MRA16_ROM )		/* Cartridges? */
	AM_RANGE(0x7000, 0x7fff) AM_READ( MRA16_ROM )		/* Keyboard ROM */
	AM_RANGE(0x8000, 0xbfff) AM_READ( intvkbd_dualport16_r )	/* Dual-port RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_kbd , ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_WRITE( stic_w )
    AM_RANGE(0x0100, 0x01ef) AM_WRITE( intv_ram8_w )
    AM_RANGE(0x01f0, 0x01ff) AM_WRITE( AY8914_directwrite_port_0_lsb_w )
	AM_RANGE(0x0200, 0x035f) AM_WRITE( intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_WRITE( MWA16_ROM ) 		/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_WRITE( MWA16_ROM )		/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_WRITE( intv_gram_w )	/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x6fff) AM_WRITE( MWA16_ROM )		/* Cartridges? */
	AM_RANGE(0x7000, 0x7fff) AM_WRITE( MWA16_ROM )		/* Keyboard ROM */
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( intvkbd_dualport16_w )	/* Dual-port RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem2 , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(0xff) )  /* Required because of probing */
	AM_RANGE( 0x0000, 0x3fff) AM_READ( intvkbd_dualport8_lsb_r ) /* Dual-port RAM */
	AM_RANGE( 0x4000, 0x7fff) AM_READ( intvkbd_dualport8_msb_r ) /* Dual-port RAM */
	AM_RANGE( 0xb7f8, 0xb7ff) AM_READ( MRA8_RAM ) /* ??? */
	AM_RANGE( 0xb800, 0xbfff) AM_READ( &videoram_r ) /* Text Display */
	AM_RANGE( 0xc000, 0xffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem2 , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff) AM_WRITE( intvkbd_dualport8_lsb_w ) /* Dual-port RAM */
	AM_RANGE( 0x4000, 0x7fff) AM_WRITE( intvkbd_dualport8_msb_w ) /* Dual-port RAM */
	AM_RANGE( 0xb7f8, 0xb7ff) AM_WRITE( MWA8_RAM ) /* ??? */
	AM_RANGE( 0xb800, 0xbfff) AM_WRITE( &videoram_w ) /* Text Display */
	AM_RANGE( 0xc000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

static INTERRUPT_GEN( intv_interrupt2 )
{
	cpu_set_irq_line(1, 0, PULSE_LINE);
}

static MACHINE_DRIVER_START( intv )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", CP1610, 3579545/4)        /* Colorburst/4 */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(intv_interrupt,1)
	MDRV_FRAMES_PER_SECOND(59.92)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( intv )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 24*8)
	MDRV_VISIBLE_AREA(0, 40*8-1, 0, 24*8-1)
	MDRV_GFXDECODE( intv_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(2 * 2 * 16 * 16)
	MDRV_PALETTE_INIT( intv )

	MDRV_VIDEO_START( intv )
	MDRV_VIDEO_UPDATE( intv )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( intvkbd )
	MDRV_IMPORT_FROM( intv )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(readmem_kbd,writemem_kbd)

	MDRV_CPU_ADD(M6502, 3579545/2)	/* Colorburst/2 */
	MDRV_CPU_PROGRAM_MAP(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(intv_interrupt2,1)

	MDRV_INTERLEAVE(100)

    /* video hardware */
	MDRV_GFXDECODE( intvkbd_gfxdecodeinfo )
	MDRV_VIDEO_START( intvkbd )
	MDRV_VIDEO_UPDATE( intvkbd )
MACHINE_DRIVER_END

ROM_START(intv)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, CRC(cbce86f7 ))
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158 ))
ROM_END

ROM_START(intvsrs)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "searsexc.bin", 0x1000<<1, 0x2000, CRC(ea552a22 ))
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158 ))
ROM_END

ROM_START(intvkbd)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, CRC(cbce86f7 ))
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158 ))
		ROM_LOAD16_WORD( "024.u60",  0x7000<<1, 0x1000, CRC(4f7998ec ))
		ROM_LOAD16_BYTE( "4d72.u62", 0x7800<<1, 0x0800, CRC(aa57c594 ))
		ROM_LOAD16_BYTE( "4d71.u63", (0x7800<<1)+1, 0x0800, CRC(069b2f0b ))

	ROM_REGION(0x10000,REGION_CPU2,0)
		ROM_LOAD( "0104.u20",  0xc000, 0x1000, CRC(5c6f1256))
		ROM_RELOAD( 0xe000, 0x1000 )
		ROM_LOAD("cpu2d.u21",  0xd000, 0x1000, CRC(2c2dba33))
		ROM_RELOAD( 0xf000, 0x1000 )

	ROM_REGION(0x00800,REGION_GFX1,0)
		ROM_LOAD( "4c52.u34",  0x0000, 0x0800, CRC(cbeb2e96))

ROM_END

SYSTEM_CONFIG_START(intv)
	CONFIG_DEVICE_CARTSLOT_REQ( 1, "int\0rom\0", device_init_intv_cart, NULL, device_load_intv_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(intvkbd)
	CONFIG_DEVICE_CARTSLOT_OPT( 2, "int\0rom\0bin\0", NULL, NULL, device_load_intvkbd_cart, NULL, NULL, NULL)
#if 0
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "tap\0", DEVICE_LOAD_RESETS_CPU, 0, NULL, NULL, NULL, NULL, NULL)
#endif
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT	COMPAT	MACHINE   INPUT     INIT		CONFIG		COMPANY      FULLNAME */
CONSX( 1979, intv,		0,		0,		intv,     intv, 	intv,		intv,		"Mattel",    "Intellivision", GAME_NOT_WORKING )
CONSX( 1981, intvsrs,	0,		0,		intv,     intv, 	intv,		intv,		"Mattel",    "Intellivision (Sears)", GAME_NOT_WORKING )
COMPX( 1981, intvkbd,	0,		0,		intvkbd,  intvkbd, 	intvkbd,	intvkbd,	"Mattel",    "Intellivision Keyboard Component (Unreleased)", GAME_NOT_WORKING)
