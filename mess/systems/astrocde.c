
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
#include "devices/cartslot.h"

/****************************************************************************
 * Bally Astrocade
 ****************************************************************************/

MEMORY_READ_START( astrocade_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( astrocade_writemem )
	{ 0x0000, 0x0fff, astrocade_magicram_w },
	{ 0x1000, 0x3fff, MWA_ROM },  /* Star Fortress writes in here?? */
	{ 0x4000, 0x4fff, astrocade_videoram_w, &astrocade_videoram, &videoram_size },	/* ASG */
MEMORY_END

PORT_READ_START( astrocade_readport )
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

PORT_END

PORT_WRITE_START( astrocade_writeport )
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

PORT_END

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
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "%   div       [   ]   LIST", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/   x     J   K   L   NEXT", KEYCODE_SLASH_PAD, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "x   -     V   W   X   IF", KEYCODE_ASTERISK, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-   +     &   @   *   GOTO", KEYCODE_MINUS_PAD, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+   =     #   %   :   PRINT", KEYCODE_PLUS_PAD, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=   WORDS Shift", KEYCODE_ENTER_PAD, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN5 */	/* Keypad Column 1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "dwn HALT              RUN", KEYCODE_PGDN, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CH  9     G   H   I   STEP", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9   6     S   T   U   RND", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6   3     up  .   dwn BOX", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3   ERASE (   ;   )      ", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".   BLUE Shift", KEYCODE_STOP, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN6 */	/* Keypad Column 2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "up  PAUSE     /   \\      ", KEYCODE_PGUP, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MS  8     D   E   F   TO", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8   5     P   Q   R   RETN", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5   2     lft '   rgt LINE", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2   0     <   \"   >   INPUT", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0   RED Shift", KEYCODE_0, IP_JOY_NONE )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN7 */	/* Keypad Column 3 (left) */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C   GO                +10", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "MR  7     A   B   C   FOR", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7   4     M   N   O   GOSB", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4   1     Y   Z   !   CLEAR", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1   SPACE $   ,   ?      ", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CE  GREEN Shift", KEYCODE_E, IP_JOY_NONE )
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


static MACHINE_DRIVER_START( astrocde )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789000)        /* 1.789 Mhz */
	MDRV_CPU_MEMORY(astrocade_readmem,astrocade_writemem)
	MDRV_CPU_PORTS(astrocade_readport,astrocade_writeport)
	MDRV_CPU_VBLANK_INT(astrocade_interrupt,256)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(8*32)
	MDRV_COLORTABLE_LENGTH(8)
	MDRV_PALETTE_INIT( astrocade )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_1chip_interface)
MACHINE_DRIVER_END


ROM_START( astrocde )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "astro.bin",  0x0000, 0x2000, CRC(ebc77f3a ))
ROM_END

ROM_START( astrocdw )
    ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD( "bioswhit.bin",  0x0000, 0x2000, CRC(6eb53e79 ))
ROM_END

SYSTEM_CONFIG_START(astrocde)
	CONFIG_DEVICE_CARTSLOT_OPT( 1, "bin\0", NULL, NULL, device_load_astrocade_rom, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT	COMPAT	  MACHINE   INPUT     INIT	CONFIG		COMPANY			FULLNAME */
CONS( 1978, astrocde, 0,	0,	  astrocde, astrocde, 0,	astrocde,	"Bally Manufacturing", "Bally Professional Arcade")
CONS( 1977, astrocdw, astrocde, 0, astrocde, astrocde, 0,        astrocde,       "Bally Manufacturing", "Bally Computer System")
