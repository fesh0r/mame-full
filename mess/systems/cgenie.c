/***************************************************************************
HAD to change the PORT_ANALOG defs in this file...	please check ;-)

Colour Genie memory map

CPU #1:
0000-3fff ROM basic & bios		  R   D0-D7

4000-bfff RAM
c000-dfff ROM dos				  R   D0-D7
e000-efff ROM extra 			  R   D0-D7
f000-f3ff color ram 			  W/R D0-D3
f400-f7ff font ram				  W/R D0-D7
f800-f8ff keyboard matrix		  R   D0-D7
ffe0-ffe3 floppy motor			  W   D0-D2
		  floppy head select	  W   D3
ffec-ffef FDC WD179x			  R/W D0-D7
		  ffec command			  W
		  ffec status			  R
		  ffed track			  R/W
		  ffee sector			  R/W
		  ffef data 			  R/W

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/cgenie.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"

static MEMORY_READ_START (readmem)
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
//	{ 0x8000, 0xbfff, MRA_RAM },	// only if 32K RAM is enabled
//	{ 0xc000, 0xdfff, MRA_ROM },	// installed in cgenie_init_machine
//	{ 0xe000, 0xefff, MRA_ROM },	// installed in cgenie_init_machine
	{ 0xf000, 0xf3ff, cgenie_colorram_r },
	{ 0xf400, 0xf7ff, cgenie_fontram_r	},
	{ 0xf800, 0xf8ff, cgenie_keyboard_r },
	{ 0xf900, 0xffdf, MRA_NOP },
	{ 0xffe0, 0xffe3, cgenie_irq_status_r },
	{ 0xffe4, 0xffeb, MRA_NOP },
	{ 0xffec, 0xffec, cgenie_status_r },
	{ 0xffe4, 0xffeb, MRA_NOP },
	{ 0xffed, 0xffed, cgenie_track_r },
	{ 0xffee, 0xffee, cgenie_sector_r },
	{ 0xffef, 0xffef, cgenie_data_r },
	{ 0xfff0, 0xffff, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START (writemem)
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, cgenie_videoram_w, &videoram },
//	{ 0x8000, 0xbfff, MWA_RAM },	// only if 32K RAM is enabled
//	{ 0xc000, 0xdfff, MWA_ROM },	// installed in cgenie_init_machine
//	{ 0xe000, 0xefff, MWA_ROM },	// installed in cgenie_init_machine
	{ 0xf000, 0xf3ff, cgenie_colorram_w, &colorram },
	{ 0xf400, 0xf7ff, cgenie_fontram_w, &cgenie_fontram },
	{ 0xf800, 0xf8ff, MWA_NOP },
	{ 0xf900, 0xffdf, MWA_NOP },
	{ 0xffe0, 0xffe3, cgenie_motor_w },
	{ 0xffe4, 0xffeb, MWA_NOP },
	{ 0xffec, 0xffec, cgenie_command_w },
	{ 0xffed, 0xffed, cgenie_track_w },
	{ 0xffee, 0xffee, cgenie_sector_w },
	{ 0xffef, 0xffef, cgenie_data_w },
	{ 0xfff0, 0xffff, MWA_NOP },
MEMORY_END

static PORT_READ_START (readport)
	{ 0xf8, 0xf8, cgenie_sh_control_port_r },
	{ 0xf9, 0xf9, cgenie_sh_data_port_r },
	{ 0xfa, 0xfa, cgenie_index_r },
	{ 0xfb, 0xfb, cgenie_register_r },
	{ 0xff, 0xff, cgenie_port_ff_r },
PORT_END

static PORT_WRITE_START (writeport)
	{ 0xf8, 0xf8, cgenie_sh_control_port_w },
	{ 0xf9, 0xf9, cgenie_sh_data_port_w },
	{ 0xfa, 0xfa, cgenie_index_w },
	{ 0xfb, 0xfb, cgenie_register_w },
	{ 0xff, 0xff, cgenie_port_ff_w },
PORT_END

INPUT_PORTS_START( cgenie )
	PORT_START /* IN0 */
	PORT_BITX(	  0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy Disc Drives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_BITX(	  0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "CG-DOS ROM C000-DFFF", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BITX(	  0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Extension  E000-EFFF", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_BITX(	  0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Video Display accuracy", KEYCODE_F5, IP_JOY_NONE )
	PORT_DIPSETTING(	0x10, "TV set" )
	PORT_DIPSETTING(	0x00, "RGB monitor" )
	PORT_BITX(	  0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Virtual tape support", KEYCODE_F6, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_BITX(	  0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Memory Size", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x04, "32K" )
	PORT_DIPSETTING(	0x00, "16K" )
	PORT_BIT(0x07, 0x07, IPT_UNUSED)

/**************************************************************************
   +-------------------------------+	 +-------------------------------+
   | 0	 1	 2	 3	 4	 5	 6	 7 |	 | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ |F-1|F-2|F-3|F-4|  |3 | x | y | z | { |F-5|F-6|F-7|F-8|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
***************************************************************************/

	PORT_START /* IN1 KEY ROW 0 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "0.0: @   ",   IP_KEY_NONE,         IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "0.1: A  a",   KEYCODE_A,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0.2: B  b",   KEYCODE_B,           IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "0.3: C  c",   KEYCODE_C,           IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "0.4: D  d",   KEYCODE_D,           IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "0.5: E  e",   KEYCODE_E,           IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0.6: F  f",   KEYCODE_F,           IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "0.7: G  g",   KEYCODE_G,           IP_JOY_NONE )

	PORT_START /* IN2 KEY ROW 1 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "1.0: H  h",   KEYCODE_H,           IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1.1: I  i",   KEYCODE_I,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "1.2: J  j",   KEYCODE_J,           IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "1.3: K  k",   KEYCODE_K,           IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "1.4: L  l",   KEYCODE_L,           IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "1.5: M  m",   KEYCODE_M,           IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "1.6: N  n",   KEYCODE_N,           IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1.7: O  o",   KEYCODE_O,           IP_JOY_NONE )

	PORT_START /* IN3 KEY ROW 2 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2.0: P  p",   KEYCODE_P,           IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "2.1: Q  q",   KEYCODE_Q,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2.2: R  r",   KEYCODE_R,           IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "2.3: S  s",   KEYCODE_S,           IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "2.4: T  t",   KEYCODE_T,           IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "2.5: U  u",   KEYCODE_U,           IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "2.6: V  v",   KEYCODE_V,           IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "2.7: W  w",   KEYCODE_W,           IP_JOY_NONE )

	PORT_START /* IN4 KEY ROW 3 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "3.0: X  x",   KEYCODE_X,           IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3.1: Y  y",   KEYCODE_Y,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "3.2: Z  z",   KEYCODE_Z,           IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3.3: [  {",   KEYCODE_OPENBRACE,   IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "3.4: F1 F5",  KEYCODE_F1,          IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "3.5: F2 F6",  KEYCODE_F2,          IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "3.6: F3 F7",  KEYCODE_F3,          IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "3.7: F4 F8",  KEYCODE_F4,          IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "3.4: \\  |",  KEYCODE_ASTERISK,    IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "3.5: ]  }",   KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "3.6: ^  ~",   KEYCODE_TILDE,       IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "3.7: _   ",   KEYCODE_EQUALS,      IP_JOY_NONE )

	PORT_START /* IN5 KEY ROW 4 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "4.0: 0   ",   KEYCODE_0,           IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "4.1: 1  !",   KEYCODE_1,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4.2: 2  \"",  KEYCODE_2,           IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "4.3: 3  #",   KEYCODE_3,           IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4.4: 4  $",   KEYCODE_4,           IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "4.5: 5  %",   KEYCODE_5,           IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "4.6: 6  &",   KEYCODE_6,           IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "4.7: 7  '",   KEYCODE_7,           IP_JOY_NONE )

		PORT_START /* IN6 KEY ROW 5 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "5.0: 8  (",   KEYCODE_8,           IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "5.1: 9  )",   KEYCODE_9,           IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "5.2: :  +",   KEYCODE_COLON,       IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5.3: ;  *",   KEYCODE_QUOTE,       IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "5.4: ,  <",   KEYCODE_COMMA,       IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5.5: -  =",   KEYCODE_MINUS,       IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "5.6: .  >",   KEYCODE_STOP,        IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "5.7: /  ?",   KEYCODE_SLASH,       IP_JOY_NONE )

	PORT_START /* IN7 KEY ROW 6 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "6.0: ENTER",  KEYCODE_ENTER,       IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "6.1: CLEAR",  KEYCODE_HOME,        IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "6.2: BREAK",  KEYCODE_END,         IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "6.3: UP",     KEYCODE_UP,          IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6.4: DOWN",   KEYCODE_DOWN,        IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "6.5: LEFT",   KEYCODE_LEFT,        IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6.6: RIGHT",  KEYCODE_RIGHT,       IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "6.7: SPACE",  KEYCODE_SPACE,       IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "6.5: (BSP)",  KEYCODE_BACKSPACE,   IP_JOY_NONE )

	PORT_START /* IN8 KEY ROW 7 */
		PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "7.0: SHIFT",  KEYCODE_LSHIFT,      IP_JOY_NONE )
		PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "7.1: MODSEL", KEYCODE_LALT,        IP_JOY_NONE )
		PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "7.2: (PGUP)", KEYCODE_PGUP,        IP_JOY_NONE )
		PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "7.3: (PGDN)", KEYCODE_PGDN,        IP_JOY_NONE )
		PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "7.4: (INS)",  KEYCODE_INSERT,      IP_JOY_NONE )
		PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7.5: (DEL)",  KEYCODE_DEL,         IP_JOY_NONE )
		PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "7.6: (CTRL)", KEYCODE_LCONTROL,    IP_JOY_NONE )
		PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7.7: (ALTGR)",KEYCODE_RALT,        IP_JOY_NONE )

	PORT_START /* IN9 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_X | IPF_PLAYER1,	40, 0, 0x00, 0xcf )

	PORT_START /* IN10 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1,  40, 0, 0x00, 0xcf )

	PORT_START /* IN11 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_X | IPF_PLAYER2,	40, 0, 0x00, 0xcf )

	PORT_START /* IN12 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER2,  40, 0, 0x00, 0xcf )

	/* Joystick Keypad */
	/* keypads were organized a 3 x 4 matrix and it looked	   */
	/* exactly like a our northern telephone numerical pads    */
	/* The addressing was done with a single clear bit 0..6    */
	/* on i/o port A,  while all other bits were set.		   */
	/* (e.g. 0xFE addresses keypad1 row 0, 0xEF keypad2 row 1) */

	/*		 bit  0   1   2   3   */
	/* FE/F7  0  [3] [6] [9] [#]  */
	/* FD/EF  1  [2] [5] [8] [0]  */
	/* FB/DF  2  [1] [4] [7] [*]  */

	PORT_START /* IN13 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 1 [3]", KEYCODE_3_PAD,     JOYCODE_1_BUTTON3  )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 1 [6]", KEYCODE_6_PAD,     JOYCODE_1_BUTTON6  )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 1 [9]", KEYCODE_9_PAD,     JOYCODE_OTHER      )
	PORT_BITX(0x08, 0x00, IPT_BUTTON2, "Joy 1 [#]", KEYCODE_SLASH_PAD, JOYCODE_1_BUTTON1  )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 1 [2]", KEYCODE_2_PAD,     JOYCODE_1_BUTTON2  )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 1 [5]", KEYCODE_5_PAD,     JOYCODE_1_BUTTON5  )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 1 [8]", KEYCODE_8_PAD,     JOYCODE_OTHER      )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 1 [0]", KEYCODE_0_PAD,     JOYCODE_OTHER      )

	PORT_START /* IN14 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 1 [1]", KEYCODE_1_PAD,     JOYCODE_1_BUTTON3  )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 1 [4]", KEYCODE_4_PAD,     JOYCODE_1_BUTTON6  )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 1 [7]", KEYCODE_7_PAD,     JOYCODE_OTHER      )
	PORT_BITX(0x08, 0x00, IPT_BUTTON1, "Joy 1 [*]", KEYCODE_ASTERISK,  JOYCODE_1_BUTTON1  )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 2 [3]", 0,                 JOYCODE_2_BUTTON2  )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 2 [6]", 0,                 JOYCODE_2_BUTTON5  )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 2 [9]", 0,                 JOYCODE_OTHER      )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 2 [#]", 0,                 JOYCODE_2_BUTTON1  )

	PORT_START /* IN15 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 2 [2]", 0,                 JOYCODE_2_BUTTON2  )
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 2 [5]", 0,                 JOYCODE_2_BUTTON5  )
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 2 [8]", 0,                 JOYCODE_OTHER      )
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "Joy 2 [0]", 0,                 JOYCODE_OTHER      )
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 2 [1]", 0,                 JOYCODE_2_BUTTON1  )
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 2 [4]", 0,                 JOYCODE_2_BUTTON4  )
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 2 [7]", 0,                 JOYCODE_OTHER      )
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 2 [*]", 0,                 JOYCODE_2_BUTTON1  )
INPUT_PORTS_END

struct GfxLayout cgenie_charlayout =
{
	8,8,		   /* 8*8 characters */
	384,		   /* 256 fixed + 128 defineable characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },   /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 		   /* every char takes 8 bytes */
};

static struct GfxLayout cgenie_gfxlayout =
{
	8,8,			/* 4*8 characters */
	256,			/* 256 graphics patterns */
	2,				/* 2 bits per pixel */
	{ 0, 1 },		/* two bitplanes; 2 bit per pixel */
	{ 0, 0, 2, 2, 4, 4, 6, 6}, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 			/* every char takes 8 bytes */
};

static struct GfxDecodeInfo cgenie_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &cgenie_charlayout,	  0, 3*16},
	{ REGION_GFX2, 0, &cgenie_gfxlayout, 3*16*2, 3*4},
	{ -1 } /* end of array */
};

static unsigned char cgenie_palette[] = {
	 0*4,  0*4,  0*4,  /* background   */

/* this is the 'RGB monitor' version, strong and clean */
	15*4, 15*4, 15*4,  /* gray		   */
	 0*4, 48*4, 48*4,  /* cyan		   */
	60*4,  0*4,  0*4,  /* red		   */
	47*4, 47*4, 47*4,  /* white 	   */
	55*4, 55*4,  0*4,  /* yellow	   */
	 0*4, 56*4,  0*4,  /* green 	   */
	42*4, 32*4,  0*4,  /* orange	   */
	63*4, 63*4,  0*4,  /* light yellow */
	 0*4,  0*4, 48*4,  /* blue		   */
	 0*4, 24*4, 63*4,  /* light blue   */
	60*4,  0*4, 38*4,  /* pink		   */
	38*4,  0*4, 60*4,  /* purple	   */
	31*4, 31*4, 31*4,  /* light gray   */
	 0*4, 63*4, 63*4,  /* light cyan   */
	58*4,  0*4, 58*4,  /* magenta	   */
	63*4, 63*4, 63*4,  /* bright white */

/* this is the 'TV screen' version, weak and blurred by repeating pixels */
	15*2+80, 15*2+80, 15*2+80,	/* gray 		*/
	 0*2+80, 48*2+80, 48*2+80,	/* cyan 		*/
	60*2+80,  0*2+80,  0*2+80,	/* red			*/
	47*2+80, 47*2+80, 47*2+80,	/* white		*/
	55*2+80, 55*2+80,  0*2+80,	/* yellow		*/
	 0*2+80, 56*2+80,  0*2+80,	/* green		*/
	42*2+80, 32*2+80,  0*2+80,	/* orange		*/
	63*2+80, 63*2+80,  0*2+80,	/* light yellow */
	 0*2+80,  0*2+80, 48*2+80,	/* blue 		*/
	 0*2+80, 24*2+80, 63*2+80,	/* light blue	*/
	60*2+80,  0*2+80, 38*2+80,	/* pink 		*/
	38*2+80,  0*2+80, 60*2+80,	/* purple		*/
	31*2+80, 31*2+80, 31*2+80,	/* light gray	*/
	 0*2+80, 63*2+80, 63*2+80,	/* light cyan	*/
	58*2+80,  0*2+80, 58*2+80,	/* magenta		*/
	63*2+80, 63*2+80, 63*2+80,	/* bright white */

	15*2+96, 15*2+96, 15*2+96,	/* gray 		*/
	 0*2+96, 48*2+96, 48*2+96,	/* cyan 		*/
	60*2+96,  0*2+96,  0*2+96,	/* red			*/
	47*2+96, 47*2+96, 47*2+96,	/* white		*/
	55*2+96, 55*2+96,  0*2+96,	/* yellow		*/
	 0*2+96, 56*2+96,  0*2+96,	/* green		*/
	42*2+96, 32*2+96,  0*2+96,	/* orange		*/
	63*2+96, 63*2+96,  0*2+96,	/* light yellow */
	 0*2+96,  0*2+96, 48*2+96,	/* blue 		*/
	 0*2+96, 24*2+96, 63*2+96,	/* light blue	*/
	60*2+96,  0*2+96, 38*2+96,	/* pink 		*/
	38*2+96,  0*2+96, 60*2+96,	/* purple		*/
	31*2+96, 31*2+96, 31*2+96,	/* light gray	*/
	 0*2+96, 63*2+96, 63*2+96,	/* light cyan	*/
	58*2+96,  0*2+96, 58*2+96,	/* magenta		*/
	63*2+96, 63*2+96, 63*2+96,	/* bright white */


};

static unsigned short cgenie_colortable[] =
{
	0, 1, 0, 2, 0, 3, 0, 4, /* RGB monitor set of text colors */
	0, 5, 0, 6, 0, 7, 0, 8,
	0, 9, 0,10, 0,11, 0,12,
	0,13, 0,14, 0,15, 0,16,

	0,17, 0,18, 0,19, 0,20, /* TV set text colors: darker */
	0,21, 0,22, 0,23, 0,24,
	0,25, 0,26, 0,27, 0,28,
	0,29, 0,30, 0,31, 0,32,

	0,33, 0,34, 0,35, 0,36, /* TV set text colors: a bit brighter */
	0,37, 0,38, 0,39, 0,40,
	0,41, 0,42, 0,43, 0,44,
	0,45, 0,46, 0,47, 0,48,

	0,	  9,	7,	  6,	/* RGB monitor graphics colors */
	0,	  25,	23,   22,	/* TV set graphics colors: darker */
	0,	  41,	39,   38,	/* TV set graphics colors: a bit brighter */
};

/* Initialise the palette */
static PALETTE_INIT( cgenie )
{
	palette_set_colors(0, cgenie_palette, sizeof(cgenie_palette) / 3);
	memcpy(colortable, cgenie_colortable, sizeof(cgenie_colortable));
}


static struct AY8910interface ay8910_interface =
{
	1,						/* 1 chip */
	2000000,				/* 2 MHz */
	{ 75 }, 				/* mixing level */
	{ cgenie_psg_port_a_r },
	{ cgenie_psg_port_b_r },
	{ cgenie_psg_port_a_w },
	{ cgenie_psg_port_b_w }
};

static struct DACinterface DAC_interface =
{
	1,			/* number of DACs */
	{ 25 }		/* volume */
};

static MACHINE_DRIVER_START( cgenie )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2216800)        /* 2,2168 Mhz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(cgenie_frame_interrupt,1)
	MDRV_CPU_PERIODIC_INT(cgenie_timer_interrupt,40)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_INIT( cgenie )
	MDRV_MACHINE_STOP( cgenie )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(48*8, (32)*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1,0*8,32*8-1)
	MDRV_GFXDECODE( cgenie_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(cgenie_palette) / sizeof(cgenie_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(cgenie_colortable) / sizeof(cgenie_colortable[0]))
	MDRV_PALETTE_INIT( cgenie )

	MDRV_VIDEO_START( cgenie )
	MDRV_VIDEO_UPDATE( cgenie )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, DAC_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (cgenie)
	ROM_REGION(0x13000,REGION_CPU1,0)
	ROM_LOAD ("cgenie.rom",  0x00000, 0x4000, 0xd359ead7)
	ROM_LOAD ("cgdos.rom",   0x10000, 0x2000, 0x2a96cf74)

	ROM_REGION(0x0c00,REGION_GFX1,0)
	ROM_LOAD ("cgenie1.fnt", 0x0000, 0x0800, 0x4fed774a)

	/* Empty memory region for the character generator */
	ROM_REGION(0x0800,REGION_GFX2,0)

ROM_END

SYSTEM_CONFIG_START(cgenie)
	CONFIG_DEVICE_CARTSLOT_OPT		(1, "rom\0", NULL, NULL, device_load_cgenie_cart, NULL, NULL, NULL)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0", device_load_cgenie_floppy)
	CONFIG_DEVICE_LEGACY			(IO_CASSETTE, 1, "cas\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_NONE, NULL, NULL, device_load_cgenie_cassette, NULL, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT	  CONFIG     COMPANY	FULLNAME */
COMP( 1982, cgenie,   0,		0,		cgenie,   cgenie,	cgenie,   cgenie,    "EACA Computers Ltd.",  "Colour Genie EG2000" )
