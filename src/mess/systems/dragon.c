/******************************************************************************

 Mathis Rosenhauer
 Nate Woods

 ******************************************************************************/
#include "driver.h"
#include "machine/6821pia.h"

/* from machine/dragon.c */
extern void dragon32_init_machine(void);
extern void dragon64_init_machine(void);
extern void coco_init_machine(void);
extern void coco3_init_machine(void);
extern void dragon_stop_machine(void);
extern int coco_cassette_init(int id, const char *name);
extern int coco_floppy_init(int id, const char *name);
extern void coco_floppy_exit(int id);
extern int dragon32_rom_load(int id, const char *name);
extern int dragon64_rom_load(int id, const char *name);
extern int coco3_rom_load(int id, const char *name);

extern int dragon_mapped_irq_r(int offset);
extern int coco3_mapped_irq_r(int offset);
extern int dragon_disk_r(int offset);
extern void dragon_disk_w(int offset, int data);
extern void dragon64_enable_64k_w(int offset, int data);
extern void coco3_enable_64k_w(int offset, int data);
extern int coco3_mmu_r(int offset);
extern void coco3_mmu_w(int offset, int data);
extern int coco3_gime_r(int offset);
extern void coco3_gime_w(int offset, int data);
extern void dragon_speedctrl_w(int offset, int data);

/* from vidhrdw/dragon.c */
extern UINT8 *coco_ram;
extern int dragon_vh_start(void);
extern int coco_vh_start(void);
extern int coco3_vh_start(void);
extern void dragon_vh_stop(void);
extern void coco3_vh_stop(void);
extern void dragon_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern void coco3_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern void dragon_sam_display_offset(int offset, int data);
extern void dragon_sam_vdg_mode(int offset, int data);
extern int dragon_interrupt(void);
extern void coco_ram_w (int offset, int data);
extern int coco3_gimevh_r(int offset);
extern void coco3_gimevh_w(int offset, int data);
extern void coco3_palette_w(int offset, int data);

static struct MemoryReadAddress dragon32_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xfeff, MRA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, dragon_disk_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dragon32_writemem[] =
{
	{ 0x0000, 0x7fff, coco_ram_w, &coco_ram },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xfeff, MWA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, dragon_disk_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd6, 0xffd9, dragon_speedctrl_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress d64_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xfeff, MRA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, dragon_disk_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress d64_writemem[] =
{
	{ 0x0000, 0x7fff, coco_ram_w, &coco_ram },
	{ 0x8000, 0xfeff, MWA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, dragon_disk_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd6, 0xffd9, dragon_speedctrl_w },
	{ 0xffde, 0xffdf, dragon64_enable_64k_w },
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
	{ 0xe000, 0xfeff, MRA_BANK8 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xff40, 0xff5f, dragon_disk_r },
	{ 0xff90, 0xff97, coco3_gime_r },
	{ 0xff98, 0xff9f, coco3_gimevh_r },
	{ 0xffa0, 0xffaf, coco3_mmu_r },
	{ 0xffb0, 0xffbf, paletteram_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress coco3_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_BANK1 },
	{ 0x2000, 0x3fff, MWA_BANK2 },
	{ 0x4000, 0x5fff, MWA_BANK3 },
	{ 0x6000, 0x7fff, MWA_BANK4 },
	{ 0x8000, 0x9fff, MWA_BANK5 },
	{ 0xa000, 0xbfff, MWA_BANK6 },
	{ 0xc000, 0xdfff, MWA_BANK7 },
	{ 0xe000, 0xfeff, MWA_BANK8 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, dragon_disk_w },
	{ 0xff90, 0xff97, coco3_gime_w },
	{ 0xff98, 0xff9f, coco3_gimevh_w },
	{ 0xffa0, 0xffaf, coco3_mmu_w },
	{ 0xffb0, 0xffbf, coco3_palette_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ 0xffd6, 0xffd9, dragon_speedctrl_w },
	{ 0xffde, 0xffdf, coco3_enable_64k_w },
	{ -1 }	/* end of table */
};

static unsigned char palette[] = {
	0x00,0x00,0x00, /* BLACK */
	0x00,0xff,0x00, /* GREEN */
	0xff,0xff,0x00, /* YELLOW */
	0x00,0x00,0xff, /* BLUE */
	0xff,0x00,0x00, /* RED */
	0xff,0xff,0xff, /* BUFF */
	0x00,0xff,0xff, /* CYAN */
	0xff,0x00,0xff, /* MAGENTA */
	0xff,0x80,0x00, /* ORANGE */
};

/* Initialise the palette */
static void init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
}

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
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X|IPF_CENTER, 100, 10, 0, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y|IPF_CENTER, 100, 10, 0, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "right button", KEYCODE_RALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "left button", KEYCODE_LALT, IP_JOY_DEFAULT)

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
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X|IPF_CENTER, 100, 10, 0, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y|IPF_CENTER, 100, 10, 0, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "right button", KEYCODE_RALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "left button", KEYCODE_LALT, IP_JOY_DEFAULT)

INPUT_PORTS_END

static struct DACinterface d_dac_interface =
{
	1,
	{ 100 }
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
	32*8,										/* screen width */
	16*12,									/* screen height (pixels doubled) */
	{ 0, 32*8-1, 0, 16*12-1},					/* visible_area */
	0,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	0,
	init_palette,								/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	dragon_vh_stop,
	dragon_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
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
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,		 /* frames per second, vblank duration */
	0,
	coco_init_machine,
	dragon_stop_machine,

	/* video hardware */
	32*8,										/* screen width */
	16*12,									/* screen height (pixels doubled) */
	{ 0, 32*8-1, 0, 16*12-1},					/* visible_area */
	0,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	0,
	init_palette,								/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	coco_vh_start,
	dragon_vh_stop,
	dragon_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
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
	640, /*32*8,*/										/* screen width */
	225, /*16*12,*/									/* screen height (pixels doubled) */
	{ 0, 639, /*32*8-1*/ 0, 224 /*16*12-1*/},					/* visible_area */
	0,							/* graphics decode info */
	17,	/* 16 colors + border color*/
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
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(dragon32)
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD("d32.rom",    0x8000,  0x4000, 0xe3879310)
ROM_END

ROM_START(coco)
     ROM_REGIONX(0x18000,REGION_CPU1)
     ROM_LOAD("coco.rom",  0x10000, 0x4000, 0x2ea0fb7f)
     ROM_LOAD("disk.rom",  0x14000, 0x2000, 0x7d48ba8e)
ROM_END

ROM_START(coco3)
     ROM_REGIONX(0x90000,REGION_CPU1)
     ROM_LOAD("coco3.rom", 0x80000, 0x7f00, BADCRC(0xdfce21e5))
     ROM_LOAD("disk.rom",  0x8C000, 0x2000, 0x7d48ba8e)
ROM_END

ROM_START(cp400)
     ROM_REGIONX(0x18000,REGION_CPU1)
     ROM_LOAD("cp400.rom",  0x10000, 0x60fe, 0xea9acb1e)
ROM_END

static const struct IODevice io_coco[] = {
	{
		IO_CARTSLOT,		/* type */
		2,					/* count */
		"pak\0", 		    /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		dragon64_rom_load,	/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
		IO_CASSETTE,		/* type */
		1,					/* count */
		"cas\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_cassette_init, /* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"dsk\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_floppy_init,	/* init */
		coco_floppy_exit,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

static const struct IODevice io_dragon32[] = {
	{
		IO_CARTSLOT,		/* type */
		2,					/* count */
		"pak\0",		    /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		dragon32_rom_load,	/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
		IO_CASSETTE,		/* type */
		1,					/* count */
		"cas\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_cassette_init, /* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

static const struct IODevice io_cp400[] = {
	{
		IO_CARTSLOT,		/* type */
		2,					/* count */
		"pak\0",		    /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		dragon64_rom_load,	/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
		IO_CASSETTE,		/* type */
		1,					/* count */
		"cas\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_cassette_init, /* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"dsk\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_floppy_init,	/* init */
		coco_floppy_exit,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

static const struct IODevice io_coco3[] = {
	{
		IO_CARTSLOT,		/* type */
		2,					/* count */
		"pak\0",		    /* file extensions */
        NULL,               /* private */
		NULL,				/* id */
		coco3_rom_load, 	/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
		IO_CASSETTE,		/* type */
		1,					/* count */
		"cas\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_cassette_init, /* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
	{
		IO_FLOPPY,			/* type */
		4,					/* count */
		"dsk\0",			/* file extensions */
        NULL,               /* private */
        NULL,               /* id */
		coco_floppy_init,	/* init */
		coco_floppy_exit,	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1982, coco,	  0,		coco,	  coco, 	0,		  "Tandy Radio Shack",  "Color Computer" )
COMP( 1986, coco3,	  coco, 	coco3,	  coco, 	0,		  "Tandy Radio Shack",  "Color Computer 3" )
COMP( 1982, dragon32, coco, 	dragon32, dragon32, 0,		  "Dragon Data Ltd",    "Dragon 32" )
COMP( 1984, cp400,	  coco, 	coco,	  coco, 	0,		  "Prologica",          "Prologica CP400" )

