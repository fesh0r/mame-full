/***************************************************************************
Colour Genie memory map

CPU #1:
0000-3fff ROM basic & bios        R   D0-D7

4000-bfff RAM
c000-dfff ROM dos                 R   D0-D7
e000-efff ROM extra               R   D0-D7
f000-f3ff color ram               W/R D0-D3
f400-f7ff font ram                W/R D0-D7
f800-f8ff keyboard matrix         R   D0-D7
ffe0-ffe3 floppy motor            W   D0-D2
          floppy head select      W   D3
ffec-ffef FDC WD179x              R/W D0-D7
          37ec command            W
          37ec status             R
          37ed track              R/W
          37ee sector             R/W
          37ef data               R/W

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6845.h"

extern  byte    *fontram;

extern int      cgenie_rom_load(void);
extern int      cgenie_rom_id(const char *name, const char * gamename);

extern int      cgenie_vh_start(void);
extern void     cgenie_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void     cgenie_sh_sound_init(const char * gamename);
extern void     cgenie_sh_control_port_w(int offset, int data);
extern void     cgenie_sh_data_port_w(int offset, int data);
extern int      cgenie_sh_control_port_r(int offset);
extern int      cgenie_sh_data_port_r(int offset);

extern int      cgenie_psg_port_a_r(int port);
extern int      cgenie_psg_port_b_r(int port);
extern void     cgenie_psg_port_a_w(int port, int val);
extern void     cgenie_psg_port_b_w(int port, int val);

extern void     cgenie_init_machine(void);

extern int      cgenie_colorram_r(int offset);
extern int      cgenie_fontram_r(int offset);

extern void     cgenie_dos_rom_w(int offset, int data);
extern void     cgenie_ext_rom_w(int offset, int data);
extern void     cgenie_colorram_w(int offset, int data);
extern void     cgenie_fontram_w(int offset, int data);

extern void     cgenie_port_ff_w(int offset, int data);
extern int      cgenie_port_ff_r(int offset);
extern int      cgenie_port_xx_r(int offset);

extern int      cgenie_timer_interrupt(void);
extern int      cgenie_frame_interrupt(void);

extern  int     cgenie_status_r(int offset);
extern  int     cgenie_track_r(int offset);
extern  int     cgenie_sector_r(int offset);
extern  int     cgenie_data_r(int offset);

extern  void    cgenie_command_w(int offset, int data);
extern  void    cgenie_track_w(int offset, int data);
extern  void    cgenie_sector_w(int offset, int data);
extern  void    cgenie_data_w(int offset, int data);

extern int      cgenie_irq_status_r(int offset);

extern void     cgenie_motor_w(int offset, int data);

extern int      cgenie_keyboard_r(int offset);
extern int      cgenie_videoram_r(int offset);
extern void     cgenie_videoram_w(int offset, int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf3ff, cgenie_colorram_r, &colorram },
	{ 0xf400, 0xf7ff, cgenie_fontram_r, &fontram },
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
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, cgenie_videoram_w, &videoram },
	{ 0x8000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xdfff, cgenie_dos_rom_w },
	{ 0xe000, 0xefff, cgenie_ext_rom_w },
	{ 0xf000, 0xf3ff, cgenie_colorram_w, &colorram },
	{ 0xf400, 0xf7ff, cgenie_fontram_w, &fontram },
	{ 0xf800, 0xf8ff, MWA_NOP },
	{ 0xf900, 0xffdf, MWA_NOP },
	{ 0xffe0, 0xffe3, cgenie_motor_w },
	{ 0xffe4, 0xffeb, MWA_NOP },
	{ 0xffec, 0xffec, cgenie_command_w },
	{ 0xffed, 0xffed, cgenie_track_w },
	{ 0xffee, 0xffee, cgenie_sector_w },
	{ 0xffef, 0xffef, cgenie_data_w },
	{ 0xfff0, 0xffff, MWA_NOP },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0xf8, 0xf8, cgenie_sh_control_port_r },
	{ 0xf9, 0xf9, cgenie_sh_data_port_r },
	{ 0xfa, 0xfa, m6845_index_r },
	{ 0xfb, 0xfb, m6845_register_r },
	{ 0xff, 0xff, cgenie_port_ff_r },
	{ -1 }
};

static struct IOWritePort writeport[] =
{
	{ 0xf8, 0xf8, cgenie_sh_control_port_w },
	{ 0xf9, 0xf9, cgenie_sh_data_port_w },
	{ 0xfa, 0xfa, m6845_index_w },
	{ 0xfb, 0xfb, m6845_register_w },
	{ 0xff, 0xff, cgenie_port_ff_w },
	{ -1 }
};

INPUT_PORTS_START( cgenie_input_ports )
	PORT_START /* IN0 */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Floppy Disc Drives", 0, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "DOS C000-DFFF", 0, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "ROM Enabled" )
	PORT_DIPSETTING(    0x00, "RAM" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Extension E000-EFFF", 0, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "ROM Enabled" )
	PORT_DIPSETTING(    0x00, "RAM" )
	PORT_BIT(0x1F, 0x1f, IPT_UNUSED)

/**************************************************************************
   +-------------------------------+     +-------------------------------+
   | 0   1   2   3   4   5   6   7 |     | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |   |
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
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "@",      OSD_KEY_ASTERISK,    IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "A",      OSD_KEY_A,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "B",      OSD_KEY_B,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "C",      OSD_KEY_C,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "D",      OSD_KEY_D,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "E",      OSD_KEY_E,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "F",      OSD_KEY_F,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "G",      OSD_KEY_G,           IP_JOY_NONE, 0)

	PORT_START /* IN2 KEY ROW 1 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "H",      OSD_KEY_H,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "I",      OSD_KEY_I,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "J",      OSD_KEY_J,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "K",      OSD_KEY_K,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "L",      OSD_KEY_L,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "M",      OSD_KEY_M,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "N",      OSD_KEY_N,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "O",      OSD_KEY_O,           IP_JOY_NONE, 0)

	PORT_START /* IN3 KEY ROW 2 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "P",      OSD_KEY_P,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "Q",      OSD_KEY_Q,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "R",      OSD_KEY_R,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "S",      OSD_KEY_S,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "T",      OSD_KEY_T,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "U",      OSD_KEY_U,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "V",      OSD_KEY_V,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "W",      OSD_KEY_W,           IP_JOY_NONE, 0)

	PORT_START /* IN4 KEY ROW 3 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "X",      OSD_KEY_X,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "Y",      OSD_KEY_Y,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "Z",      OSD_KEY_Z,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "[",      OSD_KEY_OPENBRACE,   IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "\\",     43,                  IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "]",      OSD_KEY_CLOSEBRACE,  IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "~",      OSD_KEY_TILDE,       IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "_",      OSD_KEY_EQUALS,      IP_JOY_NONE, 0)

	PORT_START /* IN5 KEY ROW 4 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "0",      OSD_KEY_0,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1",      OSD_KEY_1,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2",      OSD_KEY_2,           IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3",      OSD_KEY_3,           IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4",      OSD_KEY_4,           IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5",      OSD_KEY_5,           IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6",      OSD_KEY_6,           IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7",      OSD_KEY_7,           IP_JOY_NONE, 0)

	PORT_START /* IN6 KEY ROW 5 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "8",      OSD_KEY_8,           IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "9",      OSD_KEY_9,           IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, ":",      OSD_KEY_COLON,       IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, ";",      OSD_KEY_QUOTE,       IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, ",",      OSD_KEY_COMMA,       IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "-",      OSD_KEY_MINUS,       IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, ".",      OSD_KEY_STOP,        IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "/",      OSD_KEY_SLASH,       IP_JOY_NONE, 0)

	PORT_START /* IN7 KEY ROW 6 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "ENTER",  OSD_KEY_ENTER,       IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "CLEAR",  OSD_KEY_HOME,        IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "BREAK",  OSD_KEY_END,         IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "UP",     OSD_KEY_UP,          IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "DOWN",   OSD_KEY_DOWN,        IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "LEFT",   OSD_KEY_LEFT,        IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "BSP",    OSD_KEY_BACKSPACE,   IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "RIGHT",  OSD_KEY_RIGHT,       IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "SPACE",  OSD_KEY_SPACE,       IP_JOY_NONE, 0)

	PORT_START /* IN8 KEY ROW 7 */
	PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "SHIFT",  OSD_KEY_LSHIFT,      IP_JOY_NONE, 0)
	PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "(ALT)",  OSD_KEY_ALT,         IP_JOY_NONE, 0)
	PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "(PGUP)", OSD_KEY_PGUP,        IP_JOY_NONE, 0)
	PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "(PGDN)", OSD_KEY_PGDN,        IP_JOY_NONE, 0)
	PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "(INS)",  OSD_KEY_INSERT,      IP_JOY_NONE, 0)
	PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "(DEL)",  OSD_KEY_DEL,         IP_JOY_NONE, 0)
	PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "(CTRL)", OSD_KEY_LCONTROL,    IP_JOY_NONE, 0)
	PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "(ALTGR)",OSD_KEY_ALTGR,       IP_JOY_NONE, 0)

	PORT_START /* IN9 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_X | IPF_PLAYER1,  40, 0, 0x00, 0xcf )

	PORT_START /* IN10 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1,  40, 0, 0x00, 0xcf )

	PORT_START /* IN11 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_X | IPF_PLAYER2,  40, 0, 0x00, 0xcf )

	PORT_START /* IN12 */
	PORT_ANALOG( 0xff, 0x60, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER2,  40, 0, 0x00, 0xcf )

	/* Joystick Keypad */
	/* keypads were organized a 3 x 4 matrix and it looked     */
	/* exactly like a our northern telephone numerical pads    */
	/* The addressing was done with a single clear bit 0..6    */
	/* on i/o port A,  while all other bits were set.          */
	/* (e.g. 0xFE addresses keypad1 row 0, 0xEF keypad2 row 1) */

	/*       bit  0   1   2   3   */
	/* FE/F7  0  [3] [6] [9] [#]  */
	/* FD/EF  1  [2] [5] [8] [0]  */
	/* FB/DF  2  [1] [4] [7] [*]  */

	PORT_START /* IN13 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 1 [3]", OSD_KEY_3_PAD,     OSD_JOY_FIRE3,  0)
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 1 [6]", OSD_KEY_6_PAD,     OSD_JOY_FIRE6,  0)
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 1 [9]", OSD_KEY_9_PAD,     OSD_JOY_FIRE9,  0)
	PORT_BITX(0x08, 0x00, IPT_BUTTON2, "Joy 1 [#]", OSD_KEY_SLASH_PAD, OSD_JOY_FIRE,   0)
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 1 [2]", OSD_KEY_2_PAD,     OSD_JOY_FIRE2,  0)
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 1 [5]", OSD_KEY_5_PAD,     OSD_JOY_FIRE5,  0)
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 1 [8]", OSD_KEY_8_PAD,     OSD_JOY_FIRE8,  0)
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 1 [0]", OSD_KEY_0_PAD,     OSD_JOY_FIRE10, 0)

	PORT_START /* IN14 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 1 [1]", OSD_KEY_1_PAD,     OSD_JOY_FIRE3,  0)
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 1 [4]", OSD_KEY_4_PAD,     OSD_JOY_FIRE6,  0)
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 1 [7]", OSD_KEY_7_PAD,     OSD_JOY_FIRE9,  0)
	PORT_BITX(0x08, 0x00, IPT_BUTTON1, "Joy 1 [*]", OSD_KEY_ASTER_PAD, OSD_JOY_FIRE,   0)
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 2 [3]", 0,                 OSD_JOY2_FIRE2, 0)
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 2 [6]", 0,                 OSD_JOY2_FIRE5, 0)
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 2 [9]", 0,                 OSD_JOY2_FIRE8, 0)
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 2 [#]", 0,                 OSD_JOY2_FIRE,  0)

	PORT_START /* IN15 */
	PORT_BITX(0x01, 0x00, IPT_UNKNOWN, "Joy 2 [2]", 0,                 OSD_JOY2_FIRE2, 0)
	PORT_BITX(0x02, 0x00, IPT_UNKNOWN, "Joy 2 [5]", 0,                 OSD_JOY2_FIRE5, 0)
	PORT_BITX(0x04, 0x00, IPT_UNKNOWN, "Joy 2 [8]", 0,                 OSD_JOY2_FIRE8, 0)
	PORT_BITX(0x08, 0x00, IPT_UNKNOWN, "Joy 2 [0]", 0,                 OSD_JOY2_FIRE10,0)
	PORT_BITX(0x10, 0x00, IPT_UNKNOWN, "Joy 2 [1]", 0,                 OSD_JOY2_FIRE1, 0)
	PORT_BITX(0x20, 0x00, IPT_UNKNOWN, "Joy 2 [4]", 0,                 OSD_JOY2_FIRE4, 0)
	PORT_BITX(0x40, 0x00, IPT_UNKNOWN, "Joy 2 [7]", 0,                 OSD_JOY2_FIRE7, 0)
	PORT_BITX(0x80, 0x00, IPT_UNKNOWN, "Joy 2 [*]", 0,                 OSD_JOY2_FIRE,  0)
INPUT_PORTS_END

struct GfxLayout cgenie_charlayout =
{
	8,8,           /* 8*8 characters */
	384,           /* 256 fixed + 128 defineable characters */
	1,             /* 1 bits per pixel */
	{ 0 },         /* no bitplanes; 1 bit per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },   /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8            /* every char takes 8 bytes */
};

static struct GfxLayout cgenie_gfxlayout =
{
	8,8,            /* 4*8 characters */
	256,            /* 256 graphics patterns */
	2,              /* 2 bits per pixel */
	{ 0, 1 },       /* two bitplanes; 2 bit per pixel */
	{ 0, 0, 2, 2, 4, 4, 6, 6}, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8             /* every char takes 8 bytes */
};

static struct GfxDecodeInfo cgenie_gfxdecodeinfo[] =
{
	{ 1, 0, &cgenie_charlayout, 0, 32},
	{ 2, 0, &cgenie_gfxlayout, 32,  4},
	{ -1 } /* end of array */
};

static unsigned char cgenie_palette[17*3] = {
	 0*4,  0*4,  0*4,  /* background   */
	15*4, 15*4, 15*4,  /* gray         */
	 0*4, 48*4, 48*4,  /* cyan         */
	60*4,  0*4,  0*4,  /* red          */
	47*4, 47*4, 47*4,  /* white        */
	55*4, 55*4,  0*4,  /* yellow       */
	 0*4, 56*4,  0*4,  /* green        */
	42*4, 32*4,  0*4,  /* orange       */
	63*4, 63*4,  0*4,  /* light yellow */
	 0*4,  0*4, 48*4,  /* blue         */
	 0*4, 24*4, 63*4,  /* light blue   */
	60*4,  0*4, 38*4,  /* pink         */
	38*4,  0*4, 60*4,  /* purple       */
	31*4, 31*4, 31*4,  /* light gray   */
	 0*4, 63*4, 63*4,  /* light cyan   */
	58*4,  0*4, 58*4,  /* magenta      */
	63*4, 63*4, 63*4,  /* bright white */
};

static short cgenie_colortable[32+4] = {
	0, 1,
	0, 2,
	0, 3,
	0, 4,
	0, 5,
	0, 6,
	0, 7,
	0, 8,
	0, 9,
	0,10,
	0,11,
	0,12,
	0,13,
	0,14,
	0,15,
	0,16,
	0, 9, 7, 6,
};

static struct AY8910interface ay8910_interface =
{
	1,	      /* 1 chip */
	2000000,	/* 2 MHz */
	{ 255 },	/* volume */
	{ cgenie_psg_port_a_r },
	{ cgenie_psg_port_b_r },
	{ cgenie_psg_port_a_w },
	{ cgenie_psg_port_b_w }
};

static struct DACinterface DAC_interface =
{
    1,          /* number of DACs */
    11025,      /* DAC sampling rate */
    {160, },    /* volume */
    {0, }       /* filter rate */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2216800,	/* 2.2168 Mhz */
			0,
			readmem,writemem,
			readport,writeport,
			cgenie_frame_interrupt,1,
			cgenie_timer_interrupt,40
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	4,
	cgenie_init_machine,

	/* video hardware */
	48*8,                                   /* screen width */
	(32)*8,                                 /* screen height */
	{ 0*8, 48*8-1,0*8,32*8-1},              /* visible_area */
	cgenie_gfxdecodeinfo,                   /* graphics decode info */
	17, 32+4,                               /* colors */
	0,                                      /* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	m6845_vh_start,
	m6845_vh_stop,
	m6845_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&DAC_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver cgenie_driver =
{
	"EACA Colour Genie 2000",
	"cgenie",
	"Juergen Buchmueller",
	&machine_driver,

	cgenie_rom_load,        /* load rom_file images */
	cgenie_rom_id,          /* identify rom images */
	1,                      /* number of ROM slots - in this case, a CMD binary */
	4,                      /* number of floppy drives supported */
	0,                      /* number of hard drives supported */
	1,                      /* number of cassette drives supported */
	0,                      /* rom decoder */
	0,                      /* opcode decoder */
	0,                      /* pointer to sample names */
	0,                      /* sound_prom */

	cgenie_input_ports,

	0,                      /* color_prom */
	cgenie_palette,         /* color palette */
	cgenie_colortable,      /* color lookup table */

	ORIENTATION_DEFAULT,    /* orientation */

	0,                      /* hiscore load */
	0                       /* hiscore save */
};

