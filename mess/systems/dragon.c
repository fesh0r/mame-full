/******************************************************************************

 Mathis Rosenhauer
 Nate Woods
 Tim Lindner

 TODO:
   - Support configurable RAM sizes:
     - coco & cocoe should allow 4k/16k/32k/64k
	 - coco2 & coco2b should allow 16k/64k
	 - coco3 & coco3h should allow 128k/512k (or even 2mb/8mb to show the hacks)
   - All systems capable of disk support (i.e. - not original coco) should also
     support DECB 1.0
 ******************************************************************************/
#include "driver.h"
#include "inputx.h"
#include "snprintf.h"
#include "machine/6821pia.h"
#include "vidhrdw/m6847.h"
#include "includes/6883sam.h"
#include "includes/dragon.h"
#include "devices/basicdsk.h"
#include "includes/6551.h"
#include "formats/coco_dsk.h"
#include "devices/printer.h"
#include "devices/messfmts.h"
#include "devices/cassette.h"
#include "devices/bitbngr.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/coco_vhd.h"

#define SHOW_FULL_AREA			0
#define JOYSTICK_DELTA			10
#define JOYSTICK_SENSITIVITY	100

static MEMORY_READ_START( coco_readmem )
	{ 0x0000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xfeff, MRA_BANK2 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, coco_pia_1_r },
	{ 0xff40, 0xff8f, coco_cartridge_r },
	{ 0xff90, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
MEMORY_END

static MEMORY_WRITE_START( coco_writemem )
	{ 0x0000, 0x7fff, MWA_BANK1 },
	{ 0x8000, 0xfeff, MWA_BANK2 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff8f, coco_cartridge_w },
	{ 0xff90, 0xffbf, MWA_NOP },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP},
MEMORY_END

static MEMORY_READ_START( coco3_readmem )
	{ 0x0000, 0x1fff, MRA_BANK1 },
	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x5fff, MRA_BANK3 },
	{ 0x6000, 0x7fff, MRA_BANK4 },
	{ 0x8000, 0x9fff, MRA_BANK5 },
	{ 0xa000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, MRA_BANK7 },
	{ 0xe000, 0xfdff, MRA_BANK8 },
	{ 0xfe00, 0xfeff, MRA_BANK9 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, coco3_pia_1_r },
	{ 0xff40, 0xff8f, coco_cartridge_r },
	{ 0xff90, 0xff97, coco3_gime_r },
	{ 0xff98, 0xff9f, coco3_gimevh_r },
	{ 0xffa0, 0xffaf, coco3_mmu_r },
	{ 0xffb0, 0xffbf, paletteram_r },
	{ 0xffc0, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, coco3_mapped_irq_r },
MEMORY_END

static MEMORY_READ_START( d64_readmem )
	{ 0x0000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xfeff, MRA_BANK3 },
	{ 0xff00, 0xff03, pia_0_r },
	{ 0xff04, 0xff07, acia_6551_r },
	{ 0xff08, 0xff0b, pia_0_r },
	{ 0xff0c, 0xff0f, acia_6551_r },
	{ 0xff10, 0xff13, pia_0_r },
	{ 0xff14, 0xff17, acia_6551_r },
	{ 0xff18, 0xff1b, pia_0_r },
	{ 0xff1c, 0xff1f, acia_6551_r },
	{ 0xff20, 0xff3f, coco_pia_1_r },
	{ 0xff40, 0xff8f, coco_cartridge_r },
	{ 0xff90, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
MEMORY_END

static MEMORY_WRITE_START( d64_writemem )
	{ 0x0000, 0x7fff, MWA_BANK1 },
	{ 0x8000, 0xbfff, MWA_BANK2 },
	{ 0xc000, 0xfeff, MWA_BANK3 },
	{ 0xff00, 0xff03, pia_0_w },
	{ 0xff04, 0xff07, acia_6551_w },
	{ 0xff08, 0xff0b, pia_0_w },
	{ 0xff0c, 0xff0f, acia_6551_w },
	{ 0xff10, 0xff13, pia_0_w },
	{ 0xff14, 0xff17, acia_6551_w },
	{ 0xff18, 0xff1b, pia_0_w },
	{ 0xff1c, 0xff1f, acia_6551_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff8f, coco_cartridge_w },
	{ 0xff90, 0xffbf, MWA_NOP },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP},
MEMORY_END

/* Note that the CoCo 3 doesn't use the SAM VDG mode registers
 *
 * Also, there might be other SAM registers that are ignored in the CoCo 3;
 * I am not sure which ones are...
 *
 * Tepolt implies that $FFD4-$FFD7 and $FFDA-$FFDD are ignored on the CoCo 3,
 * which would make sense, but I'm not sure.
 */
static MEMORY_WRITE_START( coco3_writemem )
	{ 0x0000, 0x1fff, MWA_BANK1 },
	{ 0x2000, 0x3fff, MWA_BANK2 },
	{ 0x4000, 0x5fff, MWA_BANK3 },
	{ 0x6000, 0x7fff, MWA_BANK4 },
	{ 0x8000, 0x9fff, MWA_BANK5 },
	{ 0xa000, 0xbfff, MWA_BANK6 },
	{ 0xc000, 0xdfff, MWA_BANK7 },
	{ 0xe000, 0xfdff, MWA_BANK8 },
	{ 0xfe00, 0xfeff, MWA_BANK9 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff8f, coco_cartridge_w },
	{ 0xff90, 0xff97, coco3_gime_w },
	{ 0xff98, 0xff9f, coco3_gimevh_w },
	{ 0xffa0, 0xffaf, coco3_mmu_w },
	{ 0xffb0, 0xffbf, coco3_palette_w },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP },
MEMORY_END

/* Dragon keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk N/c N/c N/c N/c Shift
  PA5: X   Y   Z   Up  Dwn Lft Rgt Space
  PA4: P   Q   R   S   T   U   V   W
  PA3: H   I   J   K   L   M   N   O
  PA2: @   A   B   C   D   E   F   G
  PA1: 8   9   :   ;   ,   -   .   /
  PA0: 0   1   2   3   4   5   6   7
 */
INPUT_PORTS_START( dragon32 )
	PORT_START /* KEY ROW 0 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "0   ", KEYCODE_0, CODE_NONE,			'0')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "1  !", KEYCODE_1, CODE_NONE,			'1',	'!')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, "2  \"",KEYCODE_2, CODE_NONE,			'2',	'\"')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, "3  #", KEYCODE_3, CODE_NONE,			'3',	'#')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, "4  $", KEYCODE_4, CODE_NONE,			'4',	'$')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "5  %", KEYCODE_5, CODE_NONE,			'5',	'%')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, "6  &", KEYCODE_6, CODE_NONE,			'6',	'&')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "7  '", KEYCODE_7, CODE_NONE,			'7',	'\'')

	PORT_START /* KEY ROW 1 */
	PORT_KEY2(0x01, IP_ACTIVE_LOW, "8  (", KEYCODE_8, CODE_NONE,			'8',	'(')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "9  )", KEYCODE_9, CODE_NONE,			'9',	')')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, ":  *", KEYCODE_COLON, CODE_NONE,		':',	'*')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, ";  +", KEYCODE_QUOTE, CODE_NONE,		';',	'+')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, ",  <", KEYCODE_COMMA, CODE_NONE,		',',	'<')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "-  =", KEYCODE_MINUS, CODE_NONE,		'-',	'=')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, ".  >", KEYCODE_STOP, CODE_NONE,			'.',	'>')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "/  ?", KEYCODE_SLASH, CODE_NONE,		'/',	'?')

	PORT_START /* KEY ROW 2 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "@", KEYCODE_ASTERISK, CODE_NONE,		'@')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "A", KEYCODE_A, CODE_NONE,				'A')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "B", KEYCODE_B, CODE_NONE,				'B')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "C", KEYCODE_C, CODE_NONE,				'C')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "D", KEYCODE_D, CODE_NONE,				'D')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "E", KEYCODE_E, CODE_NONE,				'E')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "F", KEYCODE_F, CODE_NONE,				'F')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "G", KEYCODE_G, CODE_NONE,				'G')

	PORT_START /* KEY ROW 3 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "H", KEYCODE_H, CODE_NONE,				'H')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "I", KEYCODE_I, CODE_NONE,				'I')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "J", KEYCODE_J, CODE_NONE,				'J')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "K", KEYCODE_K, CODE_NONE,				'K')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "L", KEYCODE_L, CODE_NONE,				'L')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "M", KEYCODE_M, CODE_NONE,				'M')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "N", KEYCODE_N, CODE_NONE,				'N')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "O", KEYCODE_O, CODE_NONE,				'O')

	PORT_START /* KEY ROW 4 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "P", KEYCODE_P, CODE_NONE,				'P')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Q", KEYCODE_Q, CODE_NONE,				'Q')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "R", KEYCODE_R, CODE_NONE,				'R')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "S", KEYCODE_S, CODE_NONE,				'S')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "T", KEYCODE_T, CODE_NONE,				'T')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "U", KEYCODE_U, CODE_NONE,				'U')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "V", KEYCODE_V, CODE_NONE,				'V')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "W", KEYCODE_W, CODE_NONE,				'W')

	PORT_START /* KEY ROW 5 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "X", KEYCODE_X, CODE_NONE,				'X')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Y", KEYCODE_Y, CODE_NONE,				'Y')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "Z", KEYCODE_Z, CODE_NONE,				'Z')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "UP", KEYCODE_UP, CODE_NONE,				'^')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "DOWN", KEYCODE_DOWN, CODE_NONE,			10)
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE,	8)
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "RIGHT", KEYCODE_RIGHT, CODE_NONE,		9)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "SPACE", KEYCODE_SPACE, CODE_NONE,		' ')

	PORT_START /* KEY ROW 6 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, CODE_NONE,		13)
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "CLEAR", KEYCODE_HOME, CODE_NONE,		12)
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "BREAK", KEYCODE_END, KEYCODE_ESC,		27)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), CODE_NONE, CODE_NONE)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE,	UCHAR_SHIFT_1)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE,	UCHAR_SHIFT_1)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, JOYCODE_2_BUTTON1)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x00, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Standard" )
	PORT_DIPSETTING(    0x02, "Reverse" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
INPUT_PORTS_END

/* CoCo keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk N/c N/c N/c N/c Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   Up  Dwn Lft Rgt Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */
INPUT_PORTS_START( coco )
	PORT_START /* KEY ROW 0 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "@", KEYCODE_ASTERISK, CODE_NONE,		'@')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "A", KEYCODE_A, CODE_NONE,				'A')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "B", KEYCODE_B, CODE_NONE,				'B')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "C", KEYCODE_C, CODE_NONE,				'C')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "D", KEYCODE_D, CODE_NONE,				'D')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "E", KEYCODE_E, CODE_NONE,				'E')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "F", KEYCODE_F, CODE_NONE,				'F')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "G", KEYCODE_G, CODE_NONE,				'G')

	PORT_START /* KEY ROW 1 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "H", KEYCODE_H, CODE_NONE,				'H')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "I", KEYCODE_I, CODE_NONE,				'I')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "J", KEYCODE_J, CODE_NONE,				'J')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "K", KEYCODE_K, CODE_NONE,				'K')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "L", KEYCODE_L, CODE_NONE,				'L')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "M", KEYCODE_M, CODE_NONE,				'M')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "N", KEYCODE_N, CODE_NONE,				'N')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "O", KEYCODE_O, CODE_NONE,				'O')

	PORT_START /* KEY ROW 2 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "P", KEYCODE_P, CODE_NONE,				'P')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Q", KEYCODE_Q, CODE_NONE,				'Q')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "R", KEYCODE_R, CODE_NONE,				'R')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "S", KEYCODE_S, CODE_NONE,				'S')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "T", KEYCODE_T, CODE_NONE,				'T')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "U", KEYCODE_U, CODE_NONE,				'U')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "V", KEYCODE_V, CODE_NONE,				'V')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "W", KEYCODE_W, CODE_NONE,				'W')

	PORT_START /* KEY ROW 3 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "X", KEYCODE_X, CODE_NONE,				'X')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Y", KEYCODE_Y, CODE_NONE,				'Y')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "Z", KEYCODE_Z, CODE_NONE,				'Z')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "UP", KEYCODE_UP, CODE_NONE,				'^')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "DOWN", KEYCODE_DOWN, CODE_NONE,			10)
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE,	8)
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "RIGHT", KEYCODE_RIGHT, CODE_NONE,		9)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "SPACE", KEYCODE_SPACE, CODE_NONE,		' ')

	PORT_START /* KEY ROW 4 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "0   ", KEYCODE_0, CODE_NONE,			'0')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "1  !", KEYCODE_1, CODE_NONE,			'1',	'!')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, "2  \"", KEYCODE_2, CODE_NONE,			'2',	'\"')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, "3  #", KEYCODE_3, CODE_NONE,			'3',	'#')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, "4  $", KEYCODE_4, CODE_NONE,			'4',	'$')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "5  %", KEYCODE_5, CODE_NONE,			'5',	'%')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, "6  &", KEYCODE_6, CODE_NONE,			'6',	'&')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "7  '", KEYCODE_7, CODE_NONE,			'7',	'\'')

	PORT_START /* KEY ROW 5 */
	PORT_KEY2(0x01, IP_ACTIVE_LOW, "8  (", KEYCODE_8, CODE_NONE,			'8',	'(')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "9  )", KEYCODE_9, CODE_NONE,			'9',	')')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, ":  *", KEYCODE_COLON, CODE_NONE,		':',	'*')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, ";  +", KEYCODE_QUOTE, CODE_NONE,		';',	'+')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, ",  <", KEYCODE_COMMA, CODE_NONE,		',',	'<')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "-  =", KEYCODE_MINUS, CODE_NONE,		'-',	'=')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, ".  >", KEYCODE_STOP, CODE_NONE,			'.',	'>')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "/  ?", KEYCODE_SLASH, CODE_NONE,		'/',	'?')

	PORT_START /* KEY ROW 6 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, CODE_NONE,		13)
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "CLEAR", KEYCODE_HOME, CODE_NONE,		12)
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "BREAK", KEYCODE_END, KEYCODE_ESC,		27)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), CODE_NONE, CODE_NONE)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE,	UCHAR_SHIFT_1)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE,	UCHAR_SHIFT_1)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, JOYCODE_2_BUTTON1)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Standard" )
	PORT_DIPSETTING(    0x02, "Reverse" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	
	PORT_START /* 13 */
	PORT_DIPNAME( 0x03, 0x00, "Real Time Clock" )
	PORT_DIPSETTING(    0x00, "Disto" )
	PORT_DIPSETTING(    0x01, "Cloud-9" )

INPUT_PORTS_END

/* CoCo 3 keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk Alt Ctr F1  F2 Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   Up  Dwn Lft Rgt Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */
INPUT_PORTS_START( coco3 )
	PORT_START /* KEY ROW 0 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "@", KEYCODE_ASTERISK, CODE_NONE,		'@')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "A", KEYCODE_A, CODE_NONE,				'A')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "B", KEYCODE_B, CODE_NONE,				'B')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "C", KEYCODE_C, CODE_NONE,				'C')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "D", KEYCODE_D, CODE_NONE,				'D')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "E", KEYCODE_E, CODE_NONE,				'E')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "F", KEYCODE_F, CODE_NONE,				'F')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "G", KEYCODE_G, CODE_NONE,				'G')

	PORT_START /* KEY ROW 1 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "H", KEYCODE_H, CODE_NONE,				'H')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "I", KEYCODE_I, CODE_NONE,				'I')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "J", KEYCODE_J, CODE_NONE,				'J')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "K", KEYCODE_K, CODE_NONE,				'K')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "L", KEYCODE_L, CODE_NONE,				'L')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "M", KEYCODE_M, CODE_NONE,				'M')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "N", KEYCODE_N, CODE_NONE,				'N')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "O", KEYCODE_O, CODE_NONE,				'O')

	PORT_START /* KEY ROW 2 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "P", KEYCODE_P, CODE_NONE,				'P')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Q", KEYCODE_Q, CODE_NONE,				'Q')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "R", KEYCODE_R, CODE_NONE,				'R')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "S", KEYCODE_S, CODE_NONE,				'S')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "T", KEYCODE_T, CODE_NONE,				'T')
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "U", KEYCODE_U, CODE_NONE,				'U')
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "V", KEYCODE_V, CODE_NONE,				'V')
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "W", KEYCODE_W, CODE_NONE,				'W')

	PORT_START /* KEY ROW 3 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "X", KEYCODE_X, CODE_NONE,				'X')
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "Y", KEYCODE_Y, CODE_NONE,				'Y')
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "Z", KEYCODE_Z, CODE_NONE,				'Z')
	PORT_KEY1(0x08, IP_ACTIVE_LOW, "UP", KEYCODE_UP, CODE_NONE,				'^')
	PORT_KEY1(0x10, IP_ACTIVE_LOW, "DOWN", KEYCODE_DOWN, CODE_NONE,			10)
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE,	8)
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "RIGHT", KEYCODE_RIGHT, CODE_NONE,		9)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "SPACE", KEYCODE_SPACE, CODE_NONE,		' ')

	PORT_START /* KEY ROW 4 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "0   ", KEYCODE_0, CODE_NONE,			'0')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "1  !", KEYCODE_1, CODE_NONE,			'1',	'!')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, "2  \"", KEYCODE_2, CODE_NONE,			'2',	'\"')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, "3  #", KEYCODE_3, CODE_NONE,			'3',	'#')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, "4  $", KEYCODE_4, CODE_NONE,			'4',	'$')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "5  %", KEYCODE_5, CODE_NONE,			'5',	'%')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, "6  &", KEYCODE_6, CODE_NONE,			'6',	'&')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "7  '", KEYCODE_7, CODE_NONE,			'7',	'\'')

	PORT_START /* KEY ROW 5 */
	PORT_KEY2(0x01, IP_ACTIVE_LOW, "8  (", KEYCODE_8, CODE_NONE,			'8',	'(')
	PORT_KEY2(0x02, IP_ACTIVE_LOW, "9  )", KEYCODE_9, CODE_NONE,			'9',	')')
	PORT_KEY2(0x04, IP_ACTIVE_LOW, ":  *", KEYCODE_COLON, CODE_NONE,		':',	'*')
	PORT_KEY2(0x08, IP_ACTIVE_LOW, ";  +", KEYCODE_QUOTE, CODE_NONE,		';',	'+')
	PORT_KEY2(0x10, IP_ACTIVE_LOW, ",  <", KEYCODE_COMMA, CODE_NONE,		',',	'<')
	PORT_KEY2(0x20, IP_ACTIVE_LOW, "-  =", KEYCODE_MINUS, CODE_NONE,		'-',	'=')
	PORT_KEY2(0x40, IP_ACTIVE_LOW, ".  >", KEYCODE_STOP, CODE_NONE,			'.',	'>')
	PORT_KEY2(0x80, IP_ACTIVE_LOW, "/  ?", KEYCODE_SLASH, CODE_NONE,		'/',	'?')

	PORT_START /* KEY ROW 6 */
	PORT_KEY1(0x01, IP_ACTIVE_LOW, "ENTER", KEYCODE_ENTER, CODE_NONE,		13)
	PORT_KEY1(0x02, IP_ACTIVE_LOW, "CLEAR", KEYCODE_HOME, CODE_NONE,		12)
	PORT_KEY1(0x04, IP_ACTIVE_LOW, "BREAK", KEYCODE_END, KEYCODE_ESC,		27)
	PORT_KEY0(0x08, IP_ACTIVE_LOW, "ALT",   KEYCODE_LALT, CODE_NONE)
	PORT_KEY0(0x10, IP_ACTIVE_LOW, "CTRL",  KEYCODE_LCONTROL, CODE_NONE)
	PORT_KEY1(0x20, IP_ACTIVE_LOW, "F1",    KEYCODE_F1, CODE_NONE,			UCHAR_MAMEKEY(F1))
	PORT_KEY1(0x40, IP_ACTIVE_LOW, "F2",    KEYCODE_F2, CODE_NONE,			UCHAR_MAMEKEY(F2))
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE,	UCHAR_SHIFT_1)
	PORT_KEY1(0x80, IP_ACTIVE_LOW, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE,	UCHAR_SHIFT_1)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, JOYSTICK_SENSITIVITY, JOYSTICK_DELTA, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button 1", KEYCODE_RCONTROL, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Right Button 2", CODE_NONE,        JOYCODE_1_BUTTON2)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button 1",  KEYCODE_RALT,     JOYCODE_2_BUTTON1)
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Left Button 2",  CODE_NONE,        JOYCODE_2_BUTTON2)


	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Standard" )
	PORT_DIPSETTING(    0x02, "Reverse" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Video type" )
	PORT_DIPSETTING(	0x00, "Composite" )
	PORT_DIPSETTING(	0x08, "RGB" )
	PORT_DIPNAME( 0x30, 0x00, "Joystick Type" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x10, "Hi-Res Interface" )
	PORT_DIPSETTING(	0x30, "Hi-Res Interface (CoCoMax 3 Style)" )
	
	PORT_START /* 13 */
	PORT_DIPNAME( 0x03, 0x00, "Real Time Clock" )
	PORT_DIPSETTING(    0x00, "Disto" )
	PORT_DIPSETTING(    0x01, "Cloud-9" )
INPUT_PORTS_END

static struct DACinterface d_dac_interface =
{
	1,
	{ 100 }
};

static struct Wave_interface d_wave_interface = {
	1,			/* number of waves */
	{ 25 }		/* mixing levels */
};

static MACHINE_DRIVER_START( dragon32 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(coco_readmem, coco_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( dragon32 )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_M6847_PAL( dragon )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dragon64 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(d64_readmem, d64_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( dragon64 )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_M6847_PAL( dragon )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coco )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(coco_readmem, coco_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( coco )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_M6847_NTSC( dragon )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coco2 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(coco_readmem, coco_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( coco2 )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_M6847_PAL( dragon )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coco2b )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(coco_readmem, coco_writemem)
	MDRV_CPU_VBLANK_INT(m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( coco2 )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_M6847_NTSC( coco2b )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coco3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, COCO_CPU_SPEED_HZ)        /* 0,894886 Mhz */
	MDRV_CPU_MEMORY(coco3_readmem, coco3_writemem)
	MDRV_CPU_VBLANK_INT(coco3_vh_interrupt, M6847_INTERRUPTS_PER_FRAME)
	MDRV_FRAMES_PER_SECOND(COCO_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_INIT( coco3 )
	MDRV_MACHINE_STOP( coco )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(M6847_VIDEO_TYPE | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(640, 263)
#if SHOW_FULL_AREA
	MDRV_VISIBLE_AREA(0,639,0,262)
#else
	MDRV_VISIBLE_AREA(0,639,11,250)
#endif
	MDRV_PALETTE_LENGTH(64+M6847_ARTIFACT_COLOR_COUNT)
	MDRV_VIDEO_START(coco3)
	MDRV_VIDEO_UPDATE(coco3)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, d_dac_interface)
	MDRV_SOUND_ADD(WAVE, d_wave_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coco3h )
	MDRV_IMPORT_FROM( coco3 )
	MDRV_CPU_REPLACE( "main", HD6309, COCO_CPU_SPEED_HZ)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(dragon32)
	ROM_REGION(0x8000,REGION_CPU1,0)
	ROM_LOAD(           "d32.rom",      0x0000,  0x4000, 0xe3879310)
	ROM_LOAD_OPTIONAL(  "ddos10.rom",   0x4000,  0x2000, 0xb44536f6)
ROM_END

ROM_START(dragon64)
	ROM_REGION(0xC000,REGION_CPU1,0)
	ROM_LOAD(           "d64_1.rom",    0x0000,  0x4000, 0x60a4634c)
	ROM_LOAD(           "d64_2.rom",    0x8000,  0x4000, 0x17893a42)
	ROM_LOAD_OPTIONAL(  "ddos10.rom",   0x4000,  0x2000, 0xb44536f6)
ROM_END

ROM_START(coco)
     ROM_REGION(0x8000,REGION_CPU1,0)
     ROM_LOAD(			"bas10.rom",	0x2000, 0x2000, 0x00b50aaa)
ROM_END

ROM_START(cocoe)
     ROM_REGION(0x8000,REGION_CPU1,0)
     ROM_LOAD(			"bas11.rom",	0x2000, 0x2000, 0x6270955a)
     ROM_LOAD(	        "extbas10.rom",	0x0000, 0x2000, 0x6111a086)
     ROM_LOAD_OPTIONAL(	"disk10.rom",	0x4000, 0x2000, 0xb4f9968e)
ROM_END

ROM_START(coco2)
     ROM_REGION(0x8000,REGION_CPU1,0)
     ROM_LOAD(			"bas12.rom",	0x2000, 0x2000, 0x54368805)
     ROM_LOAD(      	"extbas11.rom",	0x0000, 0x2000, 0xa82a6254)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x4000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco2b)
     ROM_REGION(0x8000,REGION_CPU1,0)
     ROM_LOAD(			"bas13.rom",	0x2000, 0x2000, 0xd8f4d15e)
     ROM_LOAD(      	"extbas11.rom",	0x0000, 0x2000, 0xa82a6254)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x4000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco3)
     ROM_REGION(0x10000,REGION_CPU1,0)
	 ROM_LOAD(			"coco3.rom",	0x0000, 0x8000, 0xb4c88d6c)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0xC000, 0x2000, 0x0b9c5415)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0xE000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco3p)
     ROM_REGION(0x10000,REGION_CPU1,0)
	 ROM_LOAD(			"coco3p.rom",	0x0000, 0x8000, 0xff050d80)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0xC000, 0x2000, 0x0b9c5415)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0xE000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(cp400)
     ROM_REGION(0x8000,REGION_CPU1,0)
     ROM_LOAD("cp400bas.rom",  0x0000, 0x4000, 0x878396a5)
     ROM_LOAD("cp400dsk.rom",  0x4000, 0x2000, 0xe9ad60a0)
ROM_END

#define rom_coco3h	rom_coco3

/***************************************************************************
  Bitbanger port
***************************************************************************/

static int coco_bitbanger_filter(mess_image *img, const int *pulses, int total_pulses, int total_duration)
{
	int i;
	int result = 0;
	int word;
	int pos;
	int pulse_type;
	int c;

	if (total_duration >= 11)
	{
		word = 0;
		pos = 0;
		pulse_type = 0;
		result = 1;

		for (i = 0; i < total_pulses; i++)
		{
			if (pulse_type)
				word |= ((1 << pulses[i]) - 1) << pos;
			pulse_type ^= 1;
			pos += pulses[i];
		}

		c = (word >> 1) & 0xff;
		printer_output(img, c);
	}
	return result;
}

static const struct bitbanger_config coco_bitbanger_config =
{
	coco_bitbanger_filter,
	1.0 / 10.0,
	0.2,
	2,
	10,
	0,
	0
};

/* ----------------------------------------------------------------------- */

static GET_CUSTOM_DEVICENAME( coco )
{
	const char *name = NULL;
	switch(image_devtype(img)) {
	case IO_VHD:
		name = "Virtual Hard Disk";
		break;

	case IO_FLOPPY:
		/* CoCo people like their floppy drives zero counted */
		snprintf(buf, bufsize, "Floppy #%d", image_index_in_devtype(img));
		name = buf;
		break;
	}
	return name;
}

/* ----------------------------------------------------------------------- */

SYSTEM_CONFIG_START( generic_coco )
	/* bitbanger port */
	CONFIG_DEVICE_BITBANGER (1, &coco_bitbanger_config )

	/* cassette */
	CONFIG_DEVICE_CASSETTE	(1, "cas\0", coco_cassette_init)

	/* floppy */
	CONFIG_DEVICE_FLOPPY	(4, coco, coco_jvc )

	/* custom devicename */
	CONFIG_GET_CUSTOM_DEVICENAME( coco )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( generic_coco12 )
	CONFIG_IMPORT_FROM			( generic_coco )
	CONFIG_DEVICE_CARTSLOT_OPT	( 1, "ccc\0rom\0", NULL, NULL, coco_rom_load, coco_rom_unload, NULL, NULL )
	CONFIG_DEVICE_SNAPSHOT		(    "pak\0", coco_pak )
SYSTEM_CONFIG_END

/* ----------------------------------------------------------------------- */

SYSTEM_CONFIG_START( coco )
	CONFIG_IMPORT_FROM		( generic_coco12 )
	CONFIG_RAM				(4 * 1024)
	CONFIG_RAM				(16 * 1024)
	CONFIG_RAM				(32 * 1024)
	CONFIG_RAM_DEFAULT		(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(coco2)
	CONFIG_IMPORT_FROM		( generic_coco12 )
	CONFIG_RAM				(16 * 1024)
	CONFIG_RAM_DEFAULT		(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(coco3)
	CONFIG_IMPORT_FROM			( generic_coco )
	CONFIG_DEVICE_CARTSLOT_OPT	( 1, "ccc\0rom\0", NULL, NULL, coco3_rom_load, coco3_rom_unload, NULL, NULL )
	CONFIG_DEVICE_SNAPSHOT		(    "pak\0", coco3_pak )
	CONFIG_DEVICE_COCOVHD
	CONFIG_RAM					(128 * 1024)
	CONFIG_RAM_DEFAULT			(512 * 1024)
	CONFIG_RAM					(2048 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(dragon32)
	CONFIG_IMPORT_FROM		( generic_coco12 )
	CONFIG_RAM_DEFAULT		(32 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(dragon64)
	CONFIG_IMPORT_FROM		( generic_coco12 )
	CONFIG_RAM_DEFAULT		(64 * 1024)
SYSTEM_CONFIG_END

/*     YEAR		NAME		PARENT	COMPAT	MACHINE    INPUT     INIT     CONFIG	COMPANY					FULLNAME */
COMP(  1980,	coco,		0,		0,		coco,      coco,     0,		  coco,		"Tandy Radio Shack",	"Color Computer" )
COMP(  1981,	cocoe,		coco,	0,		coco,      coco,     0,		  coco,		"Tandy Radio Shack",	"Color Computer (Extended BASIC 1.0)" )
COMP(  1983,	coco2,		coco,	0,		coco2,     coco,     0,		  coco2,	"Tandy Radio Shack",	"Color Computer 2" )
COMP(  1985?,	coco2b,		coco,	0,		coco2b,    coco,     0,		  coco2,	"Tandy Radio Shack",	"Color Computer 2B" )
COMP(  1986,	coco3,		coco,	0,	 	coco3,	   coco3,    0,		  coco3,	"Tandy Radio Shack",	"Color Computer 3 (NTSC)" )
COMP(  1986,	coco3p,		coco, 	0,		coco3,	   coco3,    0,		  coco3,	"Tandy Radio Shack",	"Color Computer 3 (PAL)" )
COMPX( 19??,	coco3h,		coco,	0,		coco3h,    coco3,	 0, 	  coco3,	"Tandy Radio Shack",	"Color Computer 3 (NTSC; HD6309)", GAME_COMPUTER_MODIFIED|GAME_ALIAS)
COMP(  1982,	dragon32,	coco,	0,		dragon32,  dragon32, 0,		  dragon32,	"Dragon Data Ltd",    "Dragon 32" )
COMP(  1983,	dragon64,	coco,	0,		dragon64,  dragon32, 0,		  dragon64,	"Dragon Data Ltd",    "Dragon 64" )
COMP(  1984,	cp400,		coco, 	0,		coco,	coco,     0,		  coco,		"Prologica",          "CP400" )
