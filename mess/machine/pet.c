/***************************************************************************
	commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#include "vidhrdw/generic.h"
#include "mscommon.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/vc20tape.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "includes/pet.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/crtc6845.h"

/* keyboard lines */
static UINT8 pet_keyline[10] = { 0 };
static int pet_basic1=0; /* basic version 1 for quickloader */
static int superpet=0;
static int cbm8096=0;
static int pet_keyline_select;

int pet_font=0;
UINT8 *pet_memory;
UINT8 *superpet_memory;
UINT8 *pet_videoram;

static void pet_state(void);

/* pia at 0xe810
   port a
    7 sense input (low for diagnostics)
    6 ieee eoi in
    5 cassette 2 switch in
    4 cassette 1 switch in
    3..0 keyboard line select

  ca1 cassette 1 read
  ca2 ieee eoi out

  cb1 video sync in
  cb2 cassette 1 motor out
*/
static READ_HANDLER ( pet_pia0_port_a_read )
{
	int data=0xff;
	if (!cbm_ieee_eoi_r()) data&=~0x40;
	return data;
}

static WRITE_HANDLER ( pet_pia0_port_a_write )
{
	pet_keyline_select=data;  /*data is actually line here! */
}

static READ_HANDLER ( pet_pia0_port_b_read )
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

static WRITE_HANDLER( pet_pia0_ca2_out )
{
	cbm_ieee_eoi_w(0, data);
}

static void pet_irq (int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6502", ("irq %s\n", level ? "start" : "end"));
		if (superpet)
			cpu_set_irq_line (1, M6809_IRQ_LINE, level);
		cpu_set_irq_line (0, M6502_IRQ_LINE, level);
		old_level = level;
	}
}

/* pia at 0xe820 (ieee488)
   port a data in
   port b data out
  ca1 atn in
  ca2 ndac out
  cb1 srq in
  cb2 dav out
 */
static READ_HANDLER ( pet_pia1_port_a_read )
{
	return cbm_ieee_data_r();
}

static WRITE_HANDLER ( pet_pia1_port_b_write )
{
	cbm_ieee_data_w(0, data);
}

static READ_HANDLER ( pet_pia1_ca1_read )
{
	return cbm_ieee_atn_r();
}

static WRITE_HANDLER ( pet_pia1_ca2_write )
{
	cbm_ieee_ndac_w(0,data);
}

static WRITE_HANDLER ( pet_pia1_cb2_write )
{
	cbm_ieee_dav_w(0,data);
}

static READ_HANDLER ( pet_pia1_cb1_read )
{
	return cbm_ieee_srq_r();
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
	pet_pia0_port_b_read,
	NULL,
	NULL,
	NULL,
	NULL,
	pet_pia0_port_a_write,
	NULL,
	pet_pia0_ca2_out,
	NULL,
	NULL,
	pet_irq
}, pet_pia1= {
	pet_pia1_port_a_read,
	NULL,
    pet_pia1_ca1_read,
    pet_pia1_cb1_read,
	NULL,
	NULL,
	NULL,
    pet_pia1_port_b_write,
    pet_pia1_ca2_write,
    pet_pia1_cb2_write,
};

static WRITE_HANDLER( pet_address_line_11 )
{
	DBG_LOG (1, "address line", ("%d\n", data));
	if (data) pet_font|=1;
	else pet_font&=~1;
}

/* userport, cassettes, rest ieee488
   ca1 userport
   ca2 character rom address line 11
   pa user port

   pb0 ieee ndac in
   pb1 ieee nrfd out
   pb2 ieee atn out
   pb3 userport/cassettes
   pb4 cassettes
   pb5 userport/???
   pb6 ieee nrfd in
   pb7 ieee dav in

   cb1 cassettes
   cb2 user port
 */
static READ_HANDLER( pet_via_port_b_r )
{
	UINT8 data=0;
	if (cbm_ieee_ndac_r()) data|=1;
	if (cbm_ieee_nrfd_r()) data|=0x40;
	if (cbm_ieee_dav_r()) data|=0x80;
	return data;
}

static WRITE_HANDLER( pet_via_port_b_w )
{
	cbm_ieee_nrfd_w(0, data&2);
	cbm_ieee_atn_w(0, data&4);
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
	pet_via_port_b_r,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pet_via_port_b_w,
	pet_address_line_11
};

static struct {
	int bank; /* rambank to be switched in 0x9000 */
	int rom; /* rom socket 6502? at 0x9000 */
} spet= { 0 };

static WRITE_HANDLER(cbm8096_io_w)
{
	if (offset<0x10) ;
	else if (offset<0x14) pia_0_w(offset&3,data);
	else if (offset<0x20) ;
	else if (offset<0x24) pia_1_w(offset&3,data);
	else if (offset<0x40) ;
	else if (offset<0x50) via_0_w(offset&0xf,data);
	else if (offset<0x80) ;
	else if (offset<0x82) crtc6845_0_port_w(offset&1,data);
}

static READ_HANDLER(cbm8096_io_r)
{
	int data=0xff;
	if (offset<0x10) ;
	else if (offset<0x14) data=pia_0_r(offset&3);
	else if (offset<0x20) ;
	else if (offset<0x24) data=pia_1_r(offset&3);
	else if (offset<0x40) ;
	else if (offset<0x50) data=via_0_r(offset&0xf);
	else if (offset<0x80) ;
	else if (offset<0x82) data=crtc6845_0_port_r(offset&1);
	return data;
}

/*
65520        8096 memory control register
        bit 0    1=write protect $8000-BFFF
        bit 1    1=write protect $C000-FFFF
        bit 2    $8000-BFFF bank select
        bit 3    $C000-FFFF bank select
        bit 5    1=screen peek through
        bit 6    1=I/O peek through
        bit 7    1=enable expansion memory

*/
WRITE_HANDLER(cbm8096_w)
{
	read8_handler rh;
	write8_handler wh;

	if (data&0x80)
	{
		if (data&0x40)
		{
			rh = cbm8096_io_r;
			wh = cbm8096_io_w;
		}
		else
		{
			rh = MRA8_BANK7;
			if (!(data&2))
				wh = MWA8_BANK7;
			else
				wh = MWA8_NOP;
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, rh);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, wh);

		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xe7ff, 0, 0, (data & 2) == 0 ? MWA8_BANK6 : MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffef, 0, 0, (data & 2) == 0 ? MWA8_BANK8 : MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfff1, 0xffff, 0, 0, (data & 2) == 0 ? MWA8_BANK9 : MWA8_NOP);

		if (data&0x20)
		{
			cpu_setbank(1, pet_memory+0x8000);
			wh = videoram_w;
		}
		else
		{
			if (!(data&1))
				wh = MWA8_BANK1;
			else
				wh = MWA8_NOP;
		}
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, wh);

		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, 0, (data & 1) == 0 ? MWA8_BANK2 : MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, (data & 1) == 0 ? MWA8_BANK3 : MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, 0, (data & 1) == 0 ? MWA8_BANK4 : MWA8_NOP);


		if (data&4) {
			if (!(data&0x20)) {
				cpu_setbank(1,pet_memory+0x14000);
			}
			cpu_setbank(2,pet_memory+0x15000);
			cpu_setbank(3,pet_memory+0x16000);
			cpu_setbank(4,pet_memory+0x17000);
		} else {
			if (!(data&0x20)) {
				cpu_setbank(1,pet_memory+0x10000);
			}
			cpu_setbank(2,pet_memory+0x11000);
			cpu_setbank(3,pet_memory+0x12000);
			cpu_setbank(4,pet_memory+0x13000);
		}
		if (data&8) {
			if (!(data&0x40)) {
				cpu_setbank(7,pet_memory+0x1e800);
			}
			cpu_setbank(6, pet_memory+0x1c000);
			cpu_setbank(8, pet_memory+0x1f000);
			cpu_setbank(9, pet_memory+0x1fff1);
		} else {
			if (!(data&0x40)) {
				cpu_setbank(7,pet_memory+0x1a800);
			}
			cpu_setbank(6, pet_memory+0x18000);
			cpu_setbank(8, pet_memory+0x1b000);
			cpu_setbank(9, pet_memory+0x1bff1);
		}
	}
	else
	{
		cpu_setbank(1, pet_memory + 0x8000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, videoram_w);

		cpu_setbank(2, pet_memory + 0x9000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, 0, MWA8_ROM);

		cpu_setbank(3, pet_memory + 0xa000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, MWA8_ROM);

		cpu_setbank(4,pet_memory+0xb000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, 0, MWA8_ROM);

		cpu_setbank(6,pet_memory+0xc000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xe7ff, 0, 0, MWA8_ROM);

		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, cbm8096_io_r);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, cbm8096_io_w);

		cpu_setbank(8,pet_memory+0xf000);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffef, 0, 0, MWA8_ROM);

		cpu_setbank(9,pet_memory+0xfff1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfff1, 0xffff, 0, 0, MWA8_ROM);
	}
}

READ_HANDLER(superpet_r)
{
	return 0xff;
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

static void pet_interrupt(int param)
{
	static int level=0;

	pia_0_cb1_w(0,level);
	level=!level;
}

static void pet_common_driver_init (void)
{
	int i;
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i=0; i<0x8000; i+=0x40) {
		memset (pet_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}
	memset(pet_memory+0xe800, 0xff, 0x800);

	/* pet clock */
	timer_pulse(0.01, 0, pet_interrupt);

	via_config(0,&pet_via);
	pia_config(0,PIA_STANDARD_ORDERING,&pet_pia0);
	pia_config(1,PIA_STANDARD_ORDERING,&pet_pia1);

	statetext_add_function(pet_state);

	cbm_drive_open ();

	cbm_ieee_open();
}

void pet_driver_init (void)
{
	pet_common_driver_init ();
	pet_vh_init();
}

void pet_basic1_driver_init (void)
{
	pet_basic1=1;
	pet_common_driver_init ();
	pet_vh_init();
}

static struct crtc6845_config crtc_pet= { 800000 /*?*/};

void pet40_driver_init (void)
{
	pet_common_driver_init ();
	pet_vh_init();
	crtc6845_init(&crtc_pet);
}

void cbm80_driver_init (void)
{
	cbm8096=1;
	pet_common_driver_init ();
	videoram_size=0x800;
	pet80_vh_init();
	crtc6845_init(&crtc_pet);
}

void superpet_driver_init(void)
{
	superpet=1;
	pet_common_driver_init ();

	superpet_memory=memory_region(REGION_CPU2+0x10000);

	cpu_setbank(3, superpet_memory);
	memory_set_context(1);
	cpu_setbank(1, pet_memory);
	cpu_setbank(2, pet_memory+0x8000);
	cpu_setbank(3, superpet_memory);
	memory_set_context(0);

	superpet_vh_init();
	crtc6845_init(&crtc_pet);
}

MACHINE_INIT( pet )
{
	via_reset();
	pia_reset();

	if (superpet) {
		spet.rom=0;
		if (M6809_SELECT) {
			cpu_set_halt_line(0,1);
			cpu_set_halt_line(1,0);
			pet_font=2;
		} else {
			cpu_set_halt_line(0,0);
			cpu_set_halt_line(1,1);
			pet_font=0;
		}
	}

	switch (MEMORY) {
	case MEMORY_4:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_NOP);
		break;
	case MEMORY_8:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_NOP);
		break;
	case MEMORY_16:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_NOP);
		break;
	case MEMORY_32:
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MWA8_RAM);
		break;
	}

	if (cbm8096) {
		if (CBM8096_MEMORY) {
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfff0, 0xfff0, 0, 0, cbm8096_w);
		} else {
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfff0, 0xfff0, 0, 0, MWA8_NOP);
		}
		cbm8096_w(0,0);
	}

	cbm_drive_0_config (IEEE8ON ? IEEE : 0, 8);
	cbm_drive_1_config (IEEE9ON ? IEEE : 0, 9);

	pet_rom_load();
}



void pet_rom_load(void)
{
	int i;

	for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
			 &&(cbm_rom[i].size!=0); i++) {
		memcpy(pet_memory+cbm_rom[i].addr, cbm_rom[i].chip, cbm_rom[i].size);
	}
}

static void pet_keyboard_business(void)
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

static void pet_keyboard_normal(void)
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

INTERRUPT_GEN( pet_frame_interrupt )
{
	if (superpet)
	{
		if (M6809_SELECT) {
			cpu_set_halt_line(0,1);
			cpu_set_halt_line(1,0);
			pet_font|=2;
		} else {
			cpu_set_halt_line(0,0);
			cpu_set_halt_line(1,1);
			pet_font&=~2;
		}
	}

	if (BUSINESS_KEYBOARD)
		pet_keyboard_business();
	else
		pet_keyboard_normal();

	set_led_status (1 /*KB_CAPSLOCK_FLAG */ , KEY_B_SHIFTLOCK ? 1 : 0);
}

void pet_state(void)
{
	char text[70];

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
	statetext_display_text(text);
#endif

	cbm_drive_0_status (text, sizeof (text));
	statetext_display_text(text);

	cbm_drive_1_status (text, sizeof (text));
	statetext_display_text(text);
}
