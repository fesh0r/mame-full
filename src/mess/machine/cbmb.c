/***************************************************************************
	commodore b series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6509.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "cia6526.h"
#include "tpi6525.h"
#include "c1551.h"
#include "vc1541.h"
#include "vc20tape.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/vidhrdw/crtc6845.h"
#include "mess/sndhrdw/sid6581.h"

#include "cbmb.h"

/* keyboard lines */
static UINT8 cbmb_keyline[12] = { 0 };
static int cbmb_keyline_a, cbmb_keyline_b, cbmb_keyline_c;
static void *cbmb_clock;

static int cbm500=0;
UINT8 *cbmb_memory;
UINT8 *cbmb_basic;
UINT8 *cbmb_kernal;
UINT8 *cbmb_chargen;
UINT8 *cbmb_videoram;
UINT8 *cbmb_colorram;

/* tpi at 0xfde00
 in interrupt mode
 irq to m6509 irq
 cb ?
 ca chargen rom address line 12
 i4 acia irq
 i3 ?
 i2 cia6526 irq
 i1 self pb0
 i0 tod (50 or 60 hertz frequency)
 */

/* tpi at 0xfdf00
  cbm 500
   port c7 video address lines?
   port c6 ?
  cbm 600
   port c7 low
   port c6 ntsc 1, 0 pal
  cbm 700
   port c7 high
   port c6 ?
  port c5 .. c0 keyboard line select
  port a7..a0 b7..b0 keyboard input */
static void cbmb_keyboard_line_select_a(int line)
{
	cbmb_keyline_a=line;
}

static void cbmb_keyboard_line_select_b(int line)
{
	cbmb_keyline_b=line;
}

static void cbmb_keyboard_line_select_c(int line)
{
	cbmb_keyline_c=line;
}

static int cbmb_keyboard_line_a(void)
{
	int data=0;
	if (!(cbmb_keyline_c&1)) data|=cbmb_keyline[0];
	if (!(cbmb_keyline_c&2)) data|=cbmb_keyline[2];
	if (!(cbmb_keyline_c&4)) data|=cbmb_keyline[4];
	if (!(cbmb_keyline_c&8)) data|=cbmb_keyline[6];
	if (!(cbmb_keyline_c&0x10)) data|=cbmb_keyline[8];
	if (!(cbmb_keyline_c&0x20)) data|=cbmb_keyline[10];

	return data^0xff;
}

static int cbmb_keyboard_line_b(void)
{
	int data=0;
	if (!(cbmb_keyline_c&1)) data|=cbmb_keyline[1];
	if (!(cbmb_keyline_c&2)) data|=cbmb_keyline[3];
	if (!(cbmb_keyline_c&4)) data|=cbmb_keyline[5];
	if (!(cbmb_keyline_c&8)) data|=cbmb_keyline[7];
	if (!(cbmb_keyline_c&0x10)) data|=cbmb_keyline[9];
	if (!(cbmb_keyline_c&0x20)) data|=cbmb_keyline[11];

	return data^0xff;
}

static int cbmb_keyboard_line_c(void)
{
	int data=0;

	if ( (cbmb_keyline[0]&~cbmb_keyline_a)||
		 (cbmb_keyline[1]&~cbmb_keyline_b)) data|=1;
	if ( (cbmb_keyline[2]&~cbmb_keyline_a)||
		 (cbmb_keyline[3]&~cbmb_keyline_b)) data|=2;
	if ( (cbmb_keyline[4]&~cbmb_keyline_a)||
		 (cbmb_keyline[5]&~cbmb_keyline_b)) data|=4;
	if ( (cbmb_keyline[6]&~cbmb_keyline_a)||
		 (cbmb_keyline[7]&~cbmb_keyline_b)) data|=8;
	if ( (cbmb_keyline[8]&~cbmb_keyline_a)||
		 (cbmb_keyline[9]&~cbmb_keyline_b)) data|=0x10;
	if ( (cbmb_keyline[10]&~cbmb_keyline_a)||
		 (cbmb_keyline[11]&~cbmb_keyline_b)) data|=0x20;

	if (!cbm500) {
		if (!VIDEO_NTSC) data|=0x40;
		if (!MODELL_700) data|=0x80;
	}
	return data^0xff;
}

static void cbmb_irq (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6509", (errorlog, "irq %s\n", level ? "start" : "end"));
		cpu_set_irq_line (0, M6509_INT_IRQ, level);
		old_level = level;
	}
}

struct cia6526_interface cbmb_cia =
{
	0,/*c64_cia0_port_a_r, */
	0,/*c64_cia0_port_b_r, */
	0,/*c64_cia0_port_a_w, */
	0,/*c64_cia0_port_b_w, */
	0,								   /*c64_cia0_pc_w */
	0,								   /*c64_cia0_sp_r */
	0,								   /*c64_cia0_sp_w */
	0,								   /*c64_cia0_cnt_r */
	0,								   /*c64_cia0_cnt_w */
	tpi6525_0_irq2_level,
	0xff, 0xff, 0
};

void cbmb_colorram_w(int offset, int data)
{
	cbmb_colorram[offset]=data|0xf0;
}

static int cbmb_dma_read(int offset)
{
	if (offset>=0x1000)
		return cbmb_videoram[offset&0x3ff];
	else
		return cbmb_chargen[offset&0xfff];
}

static int cbmb_dma_read_color(int offset)
{
	return cbmb_colorram[offset&0x3ff];
}

static void cbmb_common_driver_init (void)
{
	/*    memset(c64_memory, 0, 0xfd00); */

#if 0
	sid6581_0_init (c64_paddle_read);
#else
	sid6581_0_init (NULL, 1);
#endif
	cbmb_cia.todin50hz = 0;
	cia6526_config (0, &cbmb_cia);

	tpi6525[0].ca.output=crtc6845_address_line_12;
	tpi6525[0].interrupt.output=cbmb_irq;
	tpi6525[1].a.read=cbmb_keyboard_line_a;
	tpi6525[1].b.read=cbmb_keyboard_line_b;
	tpi6525[1].c.read=cbmb_keyboard_line_c;
	tpi6525[1].a.output=cbmb_keyboard_line_select_a;
	tpi6525[1].b.output=cbmb_keyboard_line_select_b;
	tpi6525[1].c.output=cbmb_keyboard_line_select_c;
	cbmb_clock=timer_pulse(0.01, 0, cbmb_frame_interrupt);
}

void cbm600_driver_init (void)
{
	cbmb_common_driver_init ();
	raster2.display_state=cbmb_state;
	crtc6845_cbm600_init(cbmb_videoram);
}

void cbm600pal_driver_init (void)
{
	cbmb_common_driver_init ();
	raster2.display_state=cbmb_state;
	crtc6845_cbm600pal_init(cbmb_videoram);
}

void cbm700_driver_init (void)
{
	cbmb_common_driver_init ();
	raster2.display_state=cbmb_state;
	crtc6845_cbm700_init(cbmb_videoram);
}

void cbm500_driver_init (void)
{
	cbmb_common_driver_init ();
	cbm500=1;
	raster1.display_state=cbmb_state;
	vic6567_init (0, 0, cbmb_dma_read, cbmb_dma_read_color, NULL);
}

void cbmb_driver_shutdown (void)
{
}

void cbmb_init_machine (void)
{
	sid6581_0_reset();
	cia6526_reset ();
	tpi6525_0_reset();
	tpi6525_1_reset();
}

void cbmb_shutdown_machine (void)
{
}

void cbmb_frame_interrupt (int param)
{
	static int level=0;
	static int quickload = 0;
	int value;

	tpi6525_0_irq0_level(level);
	level=!level;
	if (level) return ;

	sid6581_update();
	if (!quickload && QUICKLOAD) {
		if (cbm500) cbm500_quick_open(0, 0, cbmb_memory);
		else cbmb_quick_open (0, 0, cbmb_memory);
	}
	quickload = QUICKLOAD;

	value = 0;
	if (KEY_STOP)
		value |= 0x80;
	if (KEY_GRAPH)
		value |= 0x40;
	if (KEY_REVERSE)
		value |= 0x20;
	if (KEY_HOME)
		value |= 0x10;
	if (KEY_CURSOR_UP)
		value |= 8;
	if (KEY_CURSOR_DOWN)
		value |= 4;
	if (KEY_F10)
		value |= 2;
	if (KEY_F9)
		value |= 1;
	cbmb_keyline[0] = value;

	value = 0;
	if (KEY_F8)
		value |= 0x80;
	if (KEY_F7)
		value |= 0x40;
	if (KEY_F6)
		value |= 0x20;
	if (KEY_F5)
		value |= 0x10;
	if (KEY_F4)
		value |= 8;
	if (KEY_F3)
		value |= 4;
	if (KEY_F2)
		value |= 2;
	if (KEY_F1)
		value |= 1;
	cbmb_keyline[1] = value;

	value = 0;
	if (KEY_PAD_SLASH)
		value |= 0x80;
	if (KEY_PAD_ASTERIX)
		value |= 0x40;
	if (KEY_PAD_CE)
		value |= 0x20;
	if (KEY_PAD_HELP)
		value |= 0x10;
	if (KEY_CURSOR_LEFT)
		value |= 8;
	if (KEY_EQUALS)
		value |= 4;
	if (KEY_0)
		value |= 2;
	if (KEY_9)
		value |= 1;
	cbmb_keyline[2] = value;

	value = 0;
	if (KEY_8)
		value |= 0x80;
	if (KEY_7)
		value |= 0x40;
	if (KEY_5)
		value |= 0x20;
	if (KEY_4)
		value |= 0x10;
	if (KEY_3)
		value |= 8;
	if (KEY_2)
		value |= 4;
	if (KEY_1)
		value |= 2;
	if (KEY_ESC)
		value |= 1;
	cbmb_keyline[3] = value;

	value = 0;
	if (KEY_PAD_MINUS)
		value |= 0x80;
	if (KEY_PAD_9)
		value |= 0x40;
	if (KEY_PAD_8)
		value |= 0x20;
	if (KEY_PAD_7)
		value |= 0x10;
	if (KEY_CURSOR_RIGHT)
		value |= 8;
	if (KEY_ARROW_LEFT)
		value |= 4;
	if (KEY_MINUS)
		value |= 2;
	if (KEY_O)
		value |= 1;
	cbmb_keyline[4] = value;

	value = 0;
	if (KEY_I)
		value |= 0x80;
	if (KEY_U)
		value |= 0x40;
	if (KEY_6)
		value |= 0x20;
	if (KEY_R)
		value |= 0x10;
	if (KEY_E)
		value |= 8;
	if (KEY_W)
		value |= 4;
	if (KEY_Q)
		value |= 2;
	if (KEY_TAB)
		value |= 1;
	cbmb_keyline[5] = value;

	value = 0;
	if (KEY_PAD_PLUS)
		value |= 0x80;
	if (KEY_PAD_6)
		value |= 0x40;
	if (KEY_PAD_5)
		value |= 0x20;
	if (KEY_PAD_4)
		value |= 0x10;
	if (KEY_DEL)
		value |= 8;
	if (KEY_CLOSEBRACE)
		value |= 4;
	if (KEY_P)
		value |= 2;
	if (KEY_L)
		value |= 1;
	cbmb_keyline[6] = value;

	value = 0;
	if (KEY_K)
		value |= 0x80;
	if (KEY_J)
		value |= 0x40;
	if (KEY_Y)
		value |= 0x20;
	if (KEY_T)
		value |= 0x10;
	if (KEY_D)
		value |= 8;
	if (KEY_S)
		value |= 4;
	if (KEY_A)
		value |= 2;
	cbmb_keyline[7] = value;

	value = 0;
	if (KEY_PAD_ENTER)
		value |= 0x80;
	if (KEY_PAD_3)
		value |= 0x40;
	if (KEY_PAD_2)
		value |= 0x20;
	if (KEY_PAD_1)
		value |= 0x10;
	if (KEY_CBM)
		value |= 8;
	if (KEY_RETURN)
		value |= 4;
	if (KEY_OPENBRACE)
		value |= 2;
	if (KEY_SEMICOLON)
		value |= 1;
	cbmb_keyline[8] = value;

	value = 0;
	if (KEY_COMMA)
		value |= 0x80;
	if (KEY_M)
		value |= 0x40;
	if (KEY_H)
		value |= 0x20;
	if (KEY_G)
		value |= 0x10;
	if (KEY_F)
		value |= 8;
	if (KEY_X)
		value |= 4;
	if (KEY_Z)
		value |= 2;
	if (KEY_SHIFT)
		value |= 1;
	cbmb_keyline[9] = value;

	value = 0;
	if (KEY_PAD_00)
		value |= 0x40;
	if (KEY_PAD_POINT)
		value |= 0x20;
	if (KEY_PAD_0)
		value |= 0x10;
	if (KEY_PI)
		value |= 4;
	if (KEY_APOSTROPH)
		value |= 2;
	if (KEY_SLASH)
		value |= 1;
	cbmb_keyline[10] = value;

	value = 0;
	if (KEY_POINT)
		value |= 0x80;
	if (KEY_SPACE)
		value |= 0x40;
	if (KEY_N)
		value |= 0x20;
	if (KEY_B)
		value |= 0x10;
	if (KEY_V)
		value |= 8;
	if (KEY_C)
		value |= 4;
	if (KEY_CTRL)
		value |= 1;
	cbmb_keyline[11] = value;

#if 0
	value = 0xff;
	if (JOYSTICK1||JOYSTICK1_2BUTTON) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_RIGHT)
			value &= ~8;
		if (JOYSTICK_1_LEFT)
			value &= ~4;
		if (JOYSTICK_1_DOWN)
			value &= ~2;
		if (JOYSTICK_1_UP)
			value &= ~1;
	} else if (PADDLES12) {
		if (PADDLE2_BUTTON)
			value &= ~8;
		if (PADDLE1_BUTTON)
			value &= ~4;
	} else if (MOUSE1) {
		if (JOYSTICK_1_BUTTON)
			value &= ~0x10;
		if (JOYSTICK_1_BUTTON2)
			value &= ~1;
	}
	cbmb_keyline[8] = value;

	value2 = 0xff;
	if (JOYSTICK2||JOYSTICK2_2BUTTON) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_RIGHT)
			value2 &= ~8;
		if (JOYSTICK_2_LEFT)
			value2 &= ~4;
		if (JOYSTICK_2_DOWN)
			value2 &= ~2;
		if (JOYSTICK_2_UP)
			value2 &= ~1;
	} else if (PADDLES34) {
		if (PADDLE4_BUTTON)
			value2 &= ~8;
		if (PADDLE3_BUTTON)
			value2 &= ~4;
	} else if (MOUSE2) {
		if (JOYSTICK_2_BUTTON)
			value2 &= ~0x10;
		if (JOYSTICK_2_BUTTON2)
			value2 &= ~1;
	}
	cbmb_keyline[9] = value2;
#endif

	vic2_frame_interrupt ();

	osd_led_w (1 /*KB_CAPSLOCK_FLAG */ , KEY_SHIFTLOCK ? 1 : 0);
#if 0
	osd_led_w (0 /*KB_NUMLOCK_FLAG */ , JOYSTICK_SWAP ? 1 : 0);
#endif
}

void cbmb_state(PRASTER *this)
{
#if VERBOSE_DBG
	int y;
	char text[70];

	y = Machine->gamedrv->drv->visible_area.max_y + 1 - Machine->uifont->height;

	snprintf(text, sizeof(text),
			 "%.2x %.2x",
			 MODELL_700, VIDEO_NTSC);
	praster_draw_text (this, text, &y);

	crtc6845_status(text, sizeof(text));
	praster_draw_text (this, text, &y);

#if 0
	cia6526_status (text, sizeof (text));
	praster_draw_text (this, text, &y);
#endif
#endif
}
