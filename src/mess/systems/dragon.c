#include "driver.h"
#include "machine/6821pia.h"

/* from machine/dragon.c */
extern void dragon_init_machine(void);
extern int dragon_rom_load(void);
extern int dragon_mapped_irq_r(int offset);

/* from vidhrdw/dragon.c */
extern UINT8 *dragon_ram;
extern int dragon_vh_start(void);
extern int coco_vh_start(void);
extern void dragon_vh_stop(void);
extern void dragon_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern void dragon_sam_display_offset(int offset, int data);
extern void dragon_sam_vdg_mode(int offset, int data);
extern int dragon_interrupt(void);
extern void dragon_ram_w (int offset, int data);

static struct MemoryReadAddress d32_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xfeff, MRA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, pia_1_r },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress d32_writemem[] =
{
	{ 0x0000, 0x7fff, dragon_ram_w, &dragon_ram },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xfeff, MWA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xffc0, 0xffc5, dragon_sam_vdg_mode },
	{ 0xffc6, 0xffd3, dragon_sam_display_offset },
	{ -1 }  /* end of table */
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
INPUT_PORTS_START( dragon_input_ports )
	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0   ", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":  *", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";  +", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",  <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/  ?", KEYCODE_SLASH, IP_JOY_NONE)

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
    PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X|IPF_CENTER, 50, 0, 0x80, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
    PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y|IPF_CENTER, 50, 0, 0x80, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "right button", KEYCODE_LALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "left button", KEYCODE_RALT, IP_JOY_DEFAULT)

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
INPUT_PORTS_START( coco_input_ports )
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
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0   ", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '", KEYCODE_7, IP_JOY_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":  *", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";  +", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",  <", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  >", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/  ?", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, IP_JOY_NONE)
 	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)

	PORT_START /* 7 */
    PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X|IPF_CENTER, 50, 0, 0x80, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
    PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y|IPF_CENTER, 50, 0, 0x80, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "right button", KEYCODE_LALT, IP_JOY_DEFAULT)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "left button", KEYCODE_RALT, IP_JOY_DEFAULT)

INPUT_PORTS_END

static struct DACinterface d_dac_interface =
{
	1,
	{ 100 }
};

static struct MachineDriver d32_machine_driver =
{
    /* basic machine hardware */
    {
        {
            CPU_M6809,
			894886,	/* 0,894886 Mhz */
			0,
			d32_readmem,d32_writemem,
			0, 0,
            dragon_interrupt, 1,
			0, 0,
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	dragon_init_machine,
    0,

    /* video hardware */
    32*8,                                       /* screen width */
	16*12, 									/* screen height (pixels doubled) */
	{ 0, 32*8-1, 0, 16*12-1},					/* visible_area */
	0,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	0,
	0,											/* convert color prom */

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

static struct MachineDriver coco_machine_driver =
{
    /* basic machine hardware */
    {
        {
            CPU_M6809,
			894886,	/* 0,894886 Mhz */
			0,
			d32_readmem,d32_writemem,
			0, 0,
            dragon_interrupt, 1,
			0, 0,
        },
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	0,
	dragon_init_machine,
    0,

    /* video hardware */
    32*8,                                       /* screen width */
	16*12, 									/* screen height (pixels doubled) */
	{ 0, 32*8-1, 0, 16*12-1},					/* visible_area */
	0,							/* graphics decode info */
	sizeof(palette) / sizeof(palette[0]) / 3,
	0,
	0,											/* convert color prom */

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


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(d32_rom)
	ROM_REGION(0x10000)
	ROM_LOAD("d32.rom",  0x8000, 0x4000, 0xe3879310)
ROM_END

struct GameDriver dragon32_driver =
{
	__FILE__,
	0,
	"dragon32",
	"Dragon 32",
	"1982",
	"Dragon Data Ltd",
	"Mathis Rosenhauer",
	GAME_COMPUTER,
    &d32_machine_driver,
	0,

	d32_rom,				/* rom module */
	dragon_rom_load,			/* load rom_file images */
	0,				/* identify rom images */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	1,						/* number of cassette drives supported */
	0,
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	dragon_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	0, 		                /* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};

ROM_START(coco_rom)
	ROM_REGION(0x10000)
	ROM_LOAD("coco.rom",  0x8000, 0x4000, 0x2ea0fb7f)
ROM_END

struct GameDriver coco_driver =
{
	__FILE__,
	0,
	"coco",
	"Color Computer",
	"1982",
	"Radio Shack",
	"Mathis Rosenhauer",
	GAME_COMPUTER,
    &coco_machine_driver,
	0,

	coco_rom,				/* rom module */
	dragon_rom_load,			/* load rom_file images */
	0,				/* identify rom images */
	1,						/* number of ROM slots */
	0,						/* number of floppy drives supported */
	0,						/* number of hard drives supported */
	1,						/* number of cassette drives supported */
	0,
	0,						/* opcode decoder */
	0,						/* pointer to sample names */
	0,						/* sound_prom */

	coco_input_ports,

	0,						/* color_prom */
	palette,				/* color palette */
	0,              		/* color lookup table */

	ORIENTATION_DEFAULT,	/* orientation */

	0,						/* hiscore load */
	0,						/* hiscore save */
};



