/***************************************************************************
	commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "vc20tape.h"
#include "machine/6821pia.h"
#include "mess/machine/6522via.h"
#include "mess/vidhrdw/pet.h"

#include "pet.h"

/* keyboard lines */
static UINT8 pet_keyline[10] = { 0 };
static int pet_basic1=0; /* basic version 1 for quickloader */
static int superpet=0;
static int pet_keyline_select;
static void *pet_clock;

UINT8 *pet_memory;
UINT8 *superpet_memory;
UINT8 *pet_videoram;

/* pia at 0xe810
   port a
    7 sense input (low for diagnostics)
    6
    5
    4
    3..0 keyboard line select
*/
static READ_HANDLER ( pet_pia0_port_a_read )
{
	return 0xff;
}

static WRITE_HANDLER ( pet_keyboard_line_select )
{
	pet_keyline_select=data;  /*data is actually line here! */
}

static READ_HANDLER ( pet_keyboard_line )
{
	int data=0;
	switch(pet_keyline_select) {
	case 0: data|=pet_keyline[0];break;
	case 1: data|=pet_keyline[1];break;
	case 2: data|=pet_keyline[2];break;
	case 3: data|=pet_keyline[3];break;
	case 4: data|=pet_keyline[4];break;
	case 5: data|=pet_keyline[5];break;
	case 6: data|=pet_keyline[6];break;
	case 7: data|=pet_keyline[7];break;
	case 8: data|=pet_keyline[8];break;
	case 9: data|=pet_keyline[9];break;
	}
	return data^0xff;
}

static void pet_irq (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6502", (errorlog, "irq %s\n", level ? "start" : "end"));
		if (superpet)
			cpu_set_irq_line (1, M6809_IRQ_LINE, level);
		cpu_set_irq_line (0, M6502_INT_IRQ, level);
		old_level = level;
	}
}

static struct pia6821_interface pet_pia0={
#if 0
	int (*in_a_func)(int offset);
	int (*in_b_func)(int offset);
	int (*in_ca1_func)(int offset);
	int (*in_cb1_func)(int offset);
	int (*in_ca2_func)(int offset);
	int (*in_cb2_func)(int offset);
	void (*out_a_func)(int offset, int val);
	void (*out_b_func)(int offset, int val);
	void (*out_ca2_func)(int offset, int val);
	void (*out_cb2_func)(int offset, int val);
	void (*irq_a_func)(int state);
	void (*irq_b_func)(int state);
#endif
	pet_pia0_port_a_read,
	pet_keyboard_line,
	NULL,
	NULL,
	NULL,
	NULL,
	pet_keyboard_line_select,
	NULL,
	NULL,
	NULL,
	NULL,
	pet_irq
}, pet_pia1= {
	NULL,
};

static void pet_address_line_11(int offset, int level)
{
	DBG_LOG (1, "address line", (errorlog, "%d\n", level));
	crtc6845_address_line_11(level);
}

static struct via6522_interface pet_via={
#if 0
	int (*in_a_func)(int offset);
	int (*in_b_func)(int offset);
	int (*in_ca1_func)(int offset);
	int (*in_cb1_func)(int offset);
	int (*in_ca2_func)(int offset);
	int (*in_cb2_func)(int offset);
	void (*out_a_func)(int offset, int val);
	void (*out_b_func)(int offset, int val);
	void (*out_ca2_func)(int offset, int val);
	void (*out_cb2_func)(int offset, int val);
	void (*irq_func)(int state);

    /* kludges for the Vectrex */
	void (*out_shift_func)(int val);
	void (*t2_callback)(double time);
#endif
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pet_address_line_11
};

static struct {
	int bank; /* rambank to be switched in 0x9000 */
	int rom; /* rom socket 6502? at 0x9000 */
} spet= { 0 };

extern READ_HANDLER(superpet_r)
{
	int data=0xff;
	switch (offset) {
	}
	return data;
}

extern WRITE_HANDLER(superpet_w)
{
	switch (offset) {
	case 6:case 7:
		spet.rom=data&1;
		
		break;
	case 4:case 5:
		spet.bank=data&0xf;
		cpu_setbank(3,superpet_memory+(spet.bank<<12));
		/* 7 low writeprotects systemlatch */
		break;
	case 0:case 1:case 2:case 3:
		/* 3: 1 pull down diagnostic pin on the userport
		   1: 1 if jumpered programable ram r/w 
		   0: 0 if jumpered programable m6809, 1 m6502 selected */
		break;
	}
}

static void pet_common_driver_init (void)
{
	int i;
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i=0; i<0x8000; i+=0x40) {
		memset (pet_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}
	memset(pet_memory+0xe800, 0xff, 0x800);

	pet_clock=timer_pulse(0.01, 0, pet_frame_interrupt);
	via_config(0,&pet_via);
	pia_config(0,PIA_8BIT,&pet_pia0);
	pia_config(1,PIA_8BIT,&pet_pia1);
}

void pet_driver_init (void)
{
	pet_common_driver_init ();
	raster2.display_state=pet_state;
	pet_init(pet_videoram);
}

void pet_basic1_driver_init (void)
{
	pet_basic1=1;
	pet_common_driver_init ();
	raster2.display_state=pet_state;
	pet_init(pet_videoram);
}

void pet40_driver_init (void)
{
	pet_common_driver_init ();
	raster2.display_state=pet_state;
	crtc6845_pet_init(pet_videoram);
}

void superpet_driver_init(void)
{
	superpet=1;
	pet_common_driver_init ();

	cpu_setbank(3, superpet_memory);
	memorycontextswap(1);
	cpu_setbank(1, pet_memory);
	cpu_setbank(2, pet_memory+0x8000);
	cpu_setbank(3, superpet_memory);
	memorycontextswap(0);

	raster2.display_state=pet_state;
	crtc6845_superpet_init(pet_videoram);
}

void pet_driver_shutdown (void)
{
}

void pet_init_machine (void)
{
	via_reset();
	pia_reset();

	if (superpet) {
		spet.rom=0;
		if (M6809_SELECT) {
			cpu_set_halt_line(0,1);
			cpu_set_halt_line(1,0);
		} else {
			cpu_set_halt_line(0,0);
			cpu_set_halt_line(1,1);
		}
	}

	switch (MEMORY) {
	case MEMORY_4:
		install_mem_write_handler (0, 0x1000, 0x1fff, MWA_NOP);
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_NOP);
		install_mem_write_handler (0, 0x4000, 0x7fff, MWA_NOP);
		break;
	case MEMORY_8:
		install_mem_write_handler (0, 0x1000, 0x1fff, MWA_RAM);
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_NOP);
		install_mem_write_handler (0, 0x4000, 0x7fff, MWA_NOP);
		break;
	case MEMORY_16:
		install_mem_write_handler (0, 0x1000, 0x1fff, MWA_RAM);
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_RAM);
		install_mem_write_handler (0, 0x4000, 0x7fff, MWA_NOP);
		break;
	case MEMORY_32:
		install_mem_write_handler (0, 0x1000, 0x1fff, MWA_RAM);
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_RAM);
		install_mem_write_handler (0, 0x4000, 0x7fff, MWA_RAM);
		break;
	}
}

void pet_shutdown_machine (void)
{
}

void pet_keyboard_business(void)
{
	int value;

	value = 0;
	/*if (KEY_ESCAPE) value|=0x80; // nothing, not blocking */
	/*if (KEY_REVERSE) value|=0x40; // nothing, not blocking */
	if (KEY_B_CURSOR_RIGHT) value |= 0x20;
	if (KEY_B_PAD_8) value |= 0x10;
	if (KEY_B_MINUS) value |= 8;
	if (KEY_B_8) value |= 4;
	if (KEY_B_5) value |= 2;
	if (KEY_B_2) value |= 1;
	pet_keyline[0] = value;

	value = 0;
	if (KEY_B_PAD_9) value |= 0x80;
	/*if (KEY_ESCAPE) value|=0x40; // nothing, not blocking */
	if (KEY_B_ARROW_UP) value |= 0x20;
	if (KEY_B_PAD_7) value |= 0x10;
	if (KEY_B_PAD_0) value |= 8;
	if (KEY_B_7) value |= 4;
	if (KEY_B_4) value |= 2;
	if (KEY_B_1) value |= 1;
	pet_keyline[1] = value;

	value = 0;
	if (KEY_B_PAD_5) value |= 0x80;
	if (KEY_B_SEMICOLON) value |= 0x40;
	if (KEY_B_K) value |= 0x20;
	if (KEY_B_CLOSEBRACE) value |= 0x10;
	if (KEY_B_H) value |= 8;
	if (KEY_B_F) value |= 4;
	if (KEY_B_S) value |= 2;
	if (KEY_B_ESCAPE) value |= 1; /* upper case */
	pet_keyline[2] = value;

	value = 0;
	if (KEY_B_PAD_6) value |= 0x80;
	if (KEY_B_ATSIGN) value |= 0x40;
	if (KEY_B_L) value |= 0x20;
	if (KEY_B_RETURN) value |= 0x10;
	if (KEY_B_J) value |= 8;
	if (KEY_B_G) value |= 4;
	if (KEY_B_D) value |= 2;
	if (KEY_B_A) value |= 1;
	pet_keyline[3] = value;

	value = 0;
	if (KEY_B_DEL) value |= 0x80;
	if (KEY_B_P) value |= 0x40;
	if (KEY_B_I) value |= 0x20;
	if (KEY_B_BACKSLASH) value |= 0x10;
	if (KEY_B_Y) value |= 8;
	if (KEY_B_R) value |= 4;
	if (KEY_B_W) value |= 2;
	if (KEY_B_TAB) value |= 1;
	pet_keyline[4] = value;

	value = 0;
	if (KEY_B_PAD_4) value |= 0x80;
	if (KEY_B_OPENBRACE) value |= 0x40;
	if (KEY_B_O) value |= 0x20;
	if (KEY_B_CURSOR_DOWN) value |= 0x10;
	if (KEY_B_U) value |= 8;
	if (KEY_B_T) value |= 4;
	if (KEY_B_E) value |= 2;
	if (KEY_B_Q) value |= 1;
	pet_keyline[5] = value;

	value = 0;
	if (KEY_B_PAD_3) value |= 0x80;
	if (KEY_B_RIGHT_SHIFT) value |= 0x40;
	/*if (KEY_REVERSE) value |= 0x20; // clear line */
	if (KEY_B_PAD_POINT) value |= 0x10;
	if (KEY_B_POINT) value |= 8;
	if (KEY_B_B) value |= 4;
	if (KEY_B_C) value |= 2;
	if (KEY_B_LEFT_SHIFT) value |= 1;
	pet_keyline[6] = value;

	value = 0;
	if (KEY_B_PAD_2) value |= 0x80;
	if (KEY_B_REPEAT) value |= 0x40;
	/*if (KEY_REVERSE) value |= 0x20; // blocking */
	if (KEY_B_0) value |= 0x10;
	if (KEY_B_COMMA) value |= 8;
	if (KEY_B_N) value |= 4;
	if (KEY_B_V) value |= 2;
	if (KEY_B_Z) value |= 1;
	pet_keyline[7] = value;

	value = 0;
	if (KEY_B_PAD_1) value |= 0x80;
	if (KEY_B_SLASH) value |= 0x40;
	/*if (KEY_ESCAPE) value |= 0x20; // special clear */
	if (KEY_B_HOME) value |= 0x10;
	if (KEY_B_M) value |= 8;
	if (KEY_B_SPACE) value |= 4;
	if (KEY_B_X) value |= 2;
	if (KEY_B_REVERSE) value |= 1; /* lower case */
	pet_keyline[8] = value;

	value = 0;
	/*if (KEY_ESCAPE) value |= 0x80; // blocking */
	/*if (KEY_REVERSE) value |= 0x40; // blocking */
	if (KEY_B_COLON) value |= 0x20;
	if (KEY_B_STOP) value |= 0x10;
	if (KEY_B_9) value |= 8;
	if (KEY_B_6) value |= 4;
	if (KEY_B_3) value |= 2;
	if (KEY_B_ARROW_LEFT) value |= 1;
	pet_keyline[9] = value;
}

void pet_keyboard_normal(void)
{
	int value;
	value = 0;
	if (KEY_CURSOR_RIGHT) value |= 0x80;
	if (KEY_HOME) value |= 0x40;
	if (KEY_ARROW_LEFT) value |= 0x20;
	if (KEY_9) value |= 0x10;
	if (KEY_7) value |= 8;
	if (KEY_5) value |= 4;
	if (KEY_3) value |= 2;
	if (KEY_1) value |= 1;
	pet_keyline[0] = value;

	value = 0;
	if (KEY_PAD_DEL) value |= 0x80;
	if (KEY_CURSOR_DOWN) value |= 0x40;
	/*if (KEY_3) value |= 0x20; // not blocking */
	if (KEY_0) value |= 0x10;
	if (KEY_8) value |= 8;
	if (KEY_6) value |= 4;
	if (KEY_4) value |= 2;
	if (KEY_2) value |= 1;
	pet_keyline[1] = value;

	value = 0;
	if (KEY_PAD_9)
		value |= 0x80;
	if (KEY_PAD_7)
		value |= 0x40;
	if (KEY_ARROW_UP) value |= 0x20;
	if (KEY_O)
		value |= 0x10;
	if (KEY_U)
		value |= 8;
	if (KEY_T)
		value |= 4;
	if (KEY_E)
		value |= 2;
	if (KEY_Q)
		value |= 1;
	pet_keyline[2] = value;

	value = 0;
	if (KEY_PAD_SLASH) value |= 0x80;
	if (KEY_PAD_8)
		value |= 0x40;
	/*if (KEY_5) value |= 0x20; // not blocking */
	if (KEY_P)
		value |= 0x10;
	if (KEY_I)
		value |= 8;
	if (KEY_Y)
		value |= 4;
	if (KEY_R)
		value |= 2;
	if (KEY_W)
		value |= 1;
	pet_keyline[3] = value;

	value = 0;
	if (KEY_PAD_6)
		value |= 0x80;
	if (KEY_PAD_4)
		value |= 0x40;
	/* if (KEY_6) value |= 0x20; // not blocking */
	if (KEY_L)
		value |= 0x10;
	if (KEY_J)
		value |= 8;
	if (KEY_G)
		value |= 4;
	if (KEY_D)
		value |= 2;
	if (KEY_A)
		value |= 1;
	pet_keyline[4] = value;

	value = 0;
	if (KEY_PAD_ASTERIX) value |= 0x80;
	if (KEY_PAD_5)
		value |= 0x40;
	/*if (KEY_8) value |= 0x20; // not blocking */
	if (KEY_COLON) value |= 0x10;
	if (KEY_K)
		value |= 8;
	if (KEY_H)
		value |= 4;
	if (KEY_F)
		value |= 2;
	if (KEY_S)
		value |= 1;
	pet_keyline[5] = value;

	value = 0;
	if (KEY_PAD_3)
		value |= 0x80;
	if (KEY_PAD_1) value |= 0x40;
	if (KEY_RETURN)
		value |= 0x20;
	if (KEY_SEMICOLON)
		value |= 0x10;
	if (KEY_M)
		value |= 8;
	if (KEY_B)
		value |= 4;
	if (KEY_C)
		value |= 2;
	if (KEY_Z)
		value |= 1;
	pet_keyline[6] = value;

	value = 0;
	if (KEY_PAD_PLUS) value |= 0x80;
	if (KEY_PAD_2)
		value |= 0x40;
	/*if (KEY_2) value |= 0x20; // non blocking */
	if (KEY_QUESTIONMARK) value |= 0x10;
	if (KEY_COMMA)
		value |= 8;
	if (KEY_N)
		value |= 4;
	if (KEY_V)
		value |= 2;
	if (KEY_X)
		value |= 1;
	pet_keyline[7] = value;

	value = 0;
	if (KEY_PAD_MINUS)
		value |= 0x80;
	if (KEY_PAD_0) value |= 0x40;
    if (KEY_RIGHT_SHIFT) value |= 0x20;
	if (KEY_BIGGER) value |= 0x10;
	/*if (KEY_5) value |= 8; // non blocking */
	if (KEY_CLOSEBRACE) value |= 4;
	if (KEY_ATSIGN) value |= 2;
	if (KEY_LEFT_SHIFT) value |= 1;
	pet_keyline[8] = value;

	value = 0;
	if (KEY_PAD_EQUALS) value |= 0x80;
	if (KEY_PAD_POINT)
		value |= 0x40;
	/*if (KEY_6) value |= 0x20; // non blocking */
	if (KEY_STOP) value |= 0x10;
	if (KEY_SMALLER) value |= 8;
	if (KEY_SPACE) value |= 4;
	if (KEY_OPENBRACE)
		value |= 2;
	if (KEY_REVERSE) value |= 1;
	pet_keyline[9] = value;
}

void pet_frame_interrupt (int param)
{
	static int level=0;
	static int quickload = 0;

	pia_0_cb1_w(0,level);
	level=!level;
	if (level) return;

	if (superpet) {
		if (M6809_SELECT) {
			cpu_set_halt_line(0,1);
			cpu_set_halt_line(1,0);
			crtc6845_address_line_12(1);
		} else {
			cpu_set_halt_line(0,0);
			cpu_set_halt_line(1,1);
			crtc6845_address_line_12(0);
		}
	}

	if (BUSINESS_KEYBOARD) {
		pet_keyboard_business();
	} else {
		pet_keyboard_normal();
	}

	if (!quickload && QUICKLOAD) {
		if (pet_basic1)
			cbm_pet1_quick_open (0, 0, pet_memory);
		else
			cbm_pet_quick_open (0, 0, pet_memory);
	}
	quickload = QUICKLOAD;

	osd_led_w (1 /*KB_CAPSLOCK_FLAG */ , KEY_B_SHIFTLOCK ? 1 : 0);
}

void pet_state(PRASTER *this)
{
#if VERBOSE_DBG
	int y;
	char text[70];

	y = Machine->gamedrv->drv->visible_area.max_y + 1 - Machine->uifont->height;

#if 0
	snprintf(text, sizeof(text),
			 "%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x",
			 pet_keyline[0],
			 pet_keyline[1],
			 pet_keyline[2],
			 pet_keyline[3],
			 pet_keyline[4],
			 pet_keyline[5],
			 pet_keyline[6],
			 pet_keyline[7],
			 pet_keyline[8],
			 pet_keyline[9]);
	praster_draw_text (this, text, &y);
#endif
#endif
}
