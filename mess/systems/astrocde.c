
/****************************************************************************

	Bally Astrocade Driver

	09/23/98 - Added sound, added player 2 pot					FMP
			   Added MWA_ROM to fix Star Fortress problem
			   Added cartridge support

	08/02/98 - First release based on original wow.c in MAME	FMP
			   Added palette generation based on a function
			   Fixed collision detection
                           Fixed shifter operation
                           Fixed clock speed
                           Fixed Interrupt Rate and handling
                           (No Light pen support yet)
                           (No sound yet)

        Original header follows, some comments don't apply      FMP

 ****************************************************************************/

 /****************************************************************************

   Bally Astrocade style games

   02.02.98 - New IO port definitions				MJC
              Dirty Rectangle handling
              Sparkle Circuit for Gorf
              errorlog output conditional on MAME_DEBUG

   03/04 98 - Extra Bases driver 				ATJ
	      	  Wow word driver

 ****************************************************************************/

#include "driver.h"
#include "sound/astrocde.h"
#include "vidhrdw/generic.h"
#include "includes/astrocde.h"

/****************************************************************************
 * Bally Astrocade
 ****************************************************************************/

static struct MemoryReadAddress astrocade_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress astrocade_writemem[] =
{
	{ 0x0000, 0x0fff, astrocade_magicram_w },
	{ 0x1000, 0x3fff, MWA_ROM },  /* Star Fortress writes in here?? */
	{ 0x4000, 0x4fff, astrocade_videoram_w, &astrocade_videoram, &videoram_size },	/* ASG */
	{ -1 }	/* end of table */
};

static struct IOReadPort astrocade_readport[] =
{
	{ 0x08, 0x08, astrocade_intercept_r },
	{ 0x0e, 0x0e, astrocade_video_retrace_r },
	/*{ 0x0f, 0x0f, astrocade_horiz_r }, */
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
  	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ 0x14, 0x14, input_port_4_r },
	{ 0x15, 0x15, input_port_5_r },
  	{ 0x16, 0x16, input_port_6_r },
	{ 0x17, 0x17, input_port_7_r },

	{ 0x1c, 0x1c, input_port_8_r },
	{ 0x1d, 0x1d, input_port_9_r },
	{ 0x1e, 0x1e, input_port_10_r },
	{ 0x1f, 0x1f, input_port_11_r },

	{ -1 }	/* end of table */
};

static struct IOWritePort astrocade_writeport[] =
{
	{ 0x00, 0x07, astrocade_colour_register_w },
	{ 0x08, 0x08, astrocade_mode_w },
	{ 0x09, 0x09, astrocade_colour_split_w },
	{ 0x0a, 0x0a, astrocade_vertical_blank_w },
	{ 0x0b, 0x0b, astrocade_colour_block_w },
	{ 0x0c, 0x0c, astrocade_magic_control_w },
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x0e, 0x0e, astrocade_interrupt_enable_w },
	{ 0x0f, 0x0f, astrocade_interrupt_w },
	{ 0x10, 0x18, astrocade_sound1_w }, /* Sound Stuff */
	{ 0x19, 0x19, astrocade_magic_expand_color_w },

	{ -1 }	/* end of table */
};

INPUT_PORTS_START( astrocde )
	PORT_START /* IN0 */	/* Player 1 Handle */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN1 */	/* Player 2 Handle */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN2 */	/* Player 3 Handle */

	PORT_START /* IN3 */	/* Player 4 Handle */

	PORT_START /* IN4 */	/* Keypad Column 0 (right) */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "%", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_PGDN, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+", KEYCODE_PGUP, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_Q, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN5 */	/* Keypad Column 1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Down", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CH", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_COMMA, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN6 */	/* Keypad Column 2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Up", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MS", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN7 */	/* Keypad Column 3 (left) */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MR", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CE", KEYCODE_E, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN8 */	/* Player 1 Knob */
#if 0
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE , 25, 0, 255, KEYCODE_X, KEYCODE_Z, 0,0,4 )

	PORT_START /* IN9 */	/* Player 2 Knob */
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE , 25, 0, 255, KEYCODE_N, KEYCODE_M, 0,0,4 )
#else
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE|IPF_REVERSE , 4, 25, 0, 255, KEYCODE_Z, KEYCODE_X, CODE_NONE, CODE_NONE )

	PORT_START /* IN9 */	/* Player 2 Knob */
	PORT_ANALOGX( 0xff, 0x00, IPT_PADDLE , 4, 25, 0, 255, KEYCODE_M, KEYCODE_N, CODE_NONE, CODE_NONE )
#endif
	PORT_START /* IN10 */	/* Player 3 Knob */

	PORT_START /* IN11 */	/* Player 4 Knob */

INPUT_PORTS_END

static struct astrocade_interface astrocade_1chip_interface =
{
	1,			/* Number of chips */
	1789773,	/* Clock speed */
	{255}		/* Volume */
};


static struct MachineDriver machine_driver_astrocde =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789000,	/* 1.789 Mhz */
			astrocade_readmem,astrocade_writemem,astrocade_readport,astrocade_writeport,
			astrocade_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,
	0,

	/* video hardware */
	160, 204, { 0, 160-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	8*32,8,
	astrocade_init_palette,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	astrocade_vh_screenrefresh,

	/* sound hardware */
	0,             	/* Initialise audio hardware */
	0,   			/* Start audio  */
	0,     			/* Stop audio   */
	0,               /* Update audio */
	{
		{
			SOUND_ASTROCADE,
			&astrocade_1chip_interface
		}
 	}
};

ROM_START( astrocde )
    ROM_REGION( 0x10000, REGION_CPU1 )
    ROM_LOAD( "astro.bin",  0x0000, 0x2000, 0xebc77f3a )
ROM_END

static const struct IODevice io_astrocde[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",            /* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
        astrocade_id_rom,   /* id */
		astrocade_load_rom, /* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL                /* output_chunk */
	},
	{ IO_END }
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
CONS( 1978, astrocde, 0,		astrocde, astrocde, 0,		  "Bally Manufacturing", "Bally Pro Arcade/Astrocade")

