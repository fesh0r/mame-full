/******************************************************************************

 Mathis Rosenhauer
 Nate Woods

 ******************************************************************************/
#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/m6847.h"
#include "includes/dragon.h"
#include "includes/basicdsk.h"

static struct MemoryReadAddress dragon32_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xfeff, MRA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, coco_floppy_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dragon32_writemem[] =
{
	{ 0x0000, 0x7fff, coco_ram_w },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xfeff, MWA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, dragon_floppy_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd4, 0xffd5, dragon_sam_page_mode },
	{ 0xffd6, 0xffd9, dragon_sam_speedctrl },
	{ 0xffda, 0xffdd, dragon_sam_memory_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress d64_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xfeff, MRA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, coco_floppy_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress d64_writemem[] =
{
	{ 0x0000, 0x7fff, coco_ram_w},
	{ 0x8000, 0xfeff, MWA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, coco_floppy_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd4, 0xffd5, dragon_sam_page_mode },
	{ 0xffd6, 0xffd9, dragon_sam_speedctrl },
	{ 0xffda, 0xffdd, dragon_sam_memory_size },
	{ 0xffde, 0xffdf, dragon64_sam_himemmap },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress coco3_readmem[] =
{
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
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, coco3_floppy_r },
	{ 0xff90, 0xff97, coco3_gime_r },
	{ 0xff98, 0xff9f, coco3_gimevh_r },
	{ 0xffa0, 0xffaf, coco3_mmu_r },
	{ 0xffb0, 0xffbf, paletteram_r },
	{ 0xfff0, 0xffff, coco3_mapped_irq_r },
	{ -1 }	/* end of table */
};

/* Note that the CoCo 3 doesn't use the SAM VDG mode registers
 *
 * Also, there might be other SAM registers that are ignored in the CoCo 3;
 * I am not sure which ones are...
 *
 * Tepolt implies that $FFD4-$FFD7 and $FFDA-$FFDD are ignored on the CoCo 3,
 * which would make sense, but I'm not sure.
 */
static struct MemoryWriteAddress coco3_writemem[] =
{
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
	{ 0xff40, 0xff5f, coco3_floppy_w },
	{ 0xff90, 0xff97, coco3_gime_w },
	{ 0xff98, 0xff9f, coco3_gimevh_w },
	{ 0xffa0, 0xffaf, coco3_mmu_w },
	{ 0xffb0, 0xffbf, coco3_palette_w },
	{ 0xffc0, 0xffc5, MWA_NOP },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd4, 0xffd5, dragon_sam_page_mode },
	{ 0xffd6, 0xffd7, MWA_NOP },
	{ 0xffd8, 0xffd9, coco3_sam_speedctrl },
	{ 0xffda, 0xffdd, dragon_sam_memory_size },
	{ 0xffde, 0xffdf, coco3_sam_himemmap },
	{ -1 }	/* end of table */
};

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
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0	  ", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1	 !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2	 \"", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3	 #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4	 $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5	 %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6	 &", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7	 '", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8	 (", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9	 )", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":	 *", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";	 +", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",	 <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-	 =", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".	 >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/	 ?", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Backspace)", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Del)", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, IP_JOY_DEFAULT)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
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
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Backspace)", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Del)", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0	  ", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1	 !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2	 \"", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3	 #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4	 $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5	 %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6	 &", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7	 '", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8	 (", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9	 )", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":	 *", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";	 +", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",	 <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-	 =", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".	 >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/	 ?", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, IP_JOY_DEFAULT)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )

INPUT_PORTS_END

/* CoCo 3 keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk N/c N/c N/c N/c Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   Up  Dwn Lft Rgt Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */
INPUT_PORTS_START( coco3 )
	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Backspace)", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT (Del)", KEYCODE_DEL, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0	  ", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1	 !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2	 \"", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3	 #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4	 $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5	 %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6	 &", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7	 '", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8	 (", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9	 )", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":	 *", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";	 +", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",	 <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-	 =", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".	 >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/	 ?", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "ALT",   KEYCODE_LALT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",  KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",    KEYCODE_F1, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",    KEYCODE_F2, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button 1", KEYCODE_RALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Right Button 2", KEYCODE_RCONTROL, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button 1", KEYCODE_LALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Left Button 2", KEYCODE_LCONTROL, IP_JOY_DEFAULT)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Video type" )
	PORT_DIPSETTING(	0x00, "Composite" )
	PORT_DIPSETTING(	0x08, "RGB" )
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

static struct MachineDriver machine_driver_dragon32 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			894886,	/* 0,894886 Mhz */
			dragon32_readmem,dragon32_writemem,
			0, 0,
			dragon_interrupt, 1,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	dragon32_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			894886,	/* 0,894886 Mhz */
			d64_readmem,d64_writemem,
			0, 0,
			dragon_interrupt, 1,
			0, 0,
		},
	},
	60, 0,		 /* frames per second, vblank duration */
	0,
	coco_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco3 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			894886,	/* 0,894886 Mhz */
			coco3_readmem,coco3_writemem,
			0, 0,
			dragon_interrupt, 1,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	coco3_init_machine,
	dragon_stop_machine,

	/* video hardware */
	640,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 639, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	19,	/* 16 colors + border color + 2 artifact colors */
	0,
	NULL,								/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	coco3_vh_start,
	coco3_vh_stop,
	coco3_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco3h =
{
	/* basic machine hardware */
	{
		{
			CPU_HD6309,
			894886,	/* 0,894886 Mhz */
			coco3_readmem,coco3_writemem,
			0, 0,
			dragon_interrupt, 1,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	coco3_init_machine,
	dragon_stop_machine,

	/* video hardware */
	640,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 639, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	19,	/* 16 colors + border color + 2 artifact colors */
	0,
	NULL,								/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	coco3_vh_start,
	coco3_vh_stop,
	coco3_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

/***************************************************************************

  Game driver(s)

  Note - These should probably be split up into different ROM files to more
  correctly match the chip layout in the repsective machines.
***************************************************************************/

ROM_START(dragon32)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("d32.rom",    0x8000,  0x4000, 0xe3879310)
ROM_END

ROM_START(coco)
     ROM_REGION(0x18000,REGION_CPU1)
     ROM_LOAD(			"bas12.rom",	0x12000, 0x2000, 0x54368805)
     ROM_LOAD_OPTIONAL(	"extbas11.rom",	0x10000, 0x2000, 0xa82a6254)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x14000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco3)
     ROM_REGION(0x90000,REGION_CPU1)
	 ROM_LOAD(			"coco3.rom",	0x80000, 0x8000, 0xb4c88d6c)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x8C000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(cp400)
     ROM_REGION(0x18000,REGION_CPU1)
     ROM_LOAD("cp400bas.rom",  0x10000, 0x4000, 0x878396a5)
     ROM_LOAD("cp400dsk.rom",  0x14000, 0x2000, 0xe9ad60a0)
ROM_END

#define rom_coco3h	rom_coco3

#define IO_FLOPPY_COCO \
	{\
		IO_FLOPPY,\
		4,\
		"dsk\0",\
		IO_RESET_NONE,\
        basicdsk_floppy_id,\
                dragon_floppy_init,\
                basicdsk_floppy_exit,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL \
    }

#define IO_SNAPSHOT_COCOPAK(loadproc) \
	{\
		IO_SNAPSHOT,\
		1,\
		"pak\0",\
		IO_RESET_ALL,\
        NULL,\
		loadproc,\
		NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL\
    }

static const struct IODevice io_coco[] = {
	IO_SNAPSHOT_COCOPAK(dragon64_rom_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
    { IO_END }
};

static const struct IODevice io_dragon32[] = {
	IO_SNAPSHOT_COCOPAK(dragon32_rom_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
    { IO_END }
};

static const struct IODevice io_cp400[] = {
	IO_SNAPSHOT_COCOPAK(dragon64_rom_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
    { IO_END }
};

static const struct IODevice io_coco3[] = {
	IO_SNAPSHOT_COCOPAK(coco3_rom_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
    { IO_END }
};

#define io_coco3h io_coco3

/*     YEAR  NAME       PARENT  MACHINE    INPUT     INIT     COMPANY               FULLNAME */
COMP(  1982, coco,      0,		coco,      coco,     0,		  "Tandy Radio Shack",  "Color Computer" )
COMP(  1986, coco3,     coco, 	coco3,	   coco3,    0,		  "Tandy Radio Shack",  "Color Computer 3" )
COMP(  1982, dragon32,  coco, 	dragon32,  dragon32, 0,		  "Dragon Data Ltd",    "Dragon 32" )
COMP(  1984, cp400,     coco, 	coco,      coco,     0,		  "Prologica",          "Prologica CP400" )
COMPX( 19??, coco3h,	coco,	coco3h,    coco3,	 0, 	  "Tandy Radio Shack",  "Color Computer 3 (6309)", GAME_NOT_WORKING|GAME_COMPUTER_MODIFIED|GAME_ALIAS)
