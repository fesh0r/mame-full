/******************************************************************************
	SYM-1

	Peter.Trauner@jk.uni-linz.ac.at May 2000

******************************************************************************/
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6522via.h"
#include "includes/riot6532.h"
#include "includes/sym1.h"

/*
  8903 7segment display output, key pressed
  892c keyboard input only ????
*/

static int sym1_riot_a_r(int chip)
{
	int data = 0xff;
	if (!(riot_0_a_r(0)&0x80)) {
		data &= input_port_0_r(0);
	}
	if (!(riot_0_b_r(0)&1)) {
		data &= input_port_1_r(1);
	}
	if (!(riot_0_b_r(0)&2)) {
		data &= input_port_2_r(1);
	}
	if (!(riot_0_b_r(0)&4)) {
		data &= input_port_3_r(1);
	}
	if ( ((riot_0_a_r(0)^0xff)&(input_port_0_r(0)^0xff))&0x3f )
		data &= ~0x80;

	return data;
}

static int sym1_riot_b_r(int chip)
{
	int data = 0xff;
	if ( ((riot_0_a_r(0)^0xff)&(input_port_1_r(0)^0xff))&0x3f )
		data&=~1;
	if ( ((riot_0_a_r(0)^0xff)&(input_port_2_r(0)^0xff))&0x3f )
		data&=~2;
	if ( ((riot_0_a_r(0)^0xff)&(input_port_3_r(0)^0xff))&0x3f )
		data&=~4;
	data&=~0x80; // else hangs 8b02

	return data;
}

static void sym1_led_w(int chip, int data)
{

	if ((riot_0_b_r(0)&0xf)<6) {
		logerror("write 7seg(%d): %c%c%c%c%c%c%c\n",
				 data&7,
				 (riot_0_a_r(0) & 0x01) ? 'a' : '.',
				 (riot_0_a_r(0) & 0x02) ? 'b' : '.',
				 (riot_0_a_r(0) & 0x04) ? 'c' : '.',
				 (riot_0_a_r(0) & 0x08) ? 'd' : '.',
				 (riot_0_a_r(0) & 0x10) ? 'e' : '.',
				 (riot_0_a_r(0) & 0x20) ? 'f' : '.',
				 (riot_0_a_r(0) & 0x40) ? 'g' : '.');
		sym1_led[riot_0_b_r(0)] |= riot_0_a_r(0);
	}
}

static RIOT_CONFIG riot={
	1000000,
	{ sym1_riot_a_r,sym1_led_w },
	{ sym1_riot_b_r, sym1_led_w },
	0
};

static void sym1_irq(int level)
{
	cpu_set_irq_line(0, M6502_INT_IRQ, level);
}

static struct via6522_interface via0={
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
	0,
	0,
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,
	sym1_irq
},
via1 = { 0 },
via2 = { 0 };

void init_sym1(void)
{
	via_config(0, &via0);
	via_config(1, &via1);
	via_config(2, &via2);
	riot_config(0, &riot);
}

void sym1_init_machine(void)
{
	via_reset();
	riot_reset(0);
}

#if 0
int kim1_cassette_init(int id)
{
	const char magic[] = "KIM1";
	char buff[4];
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
		UINT16 addr, size;
		UINT8 ident, *RAM = memory_region(REGION_CPU1);

		osd_fread(file, buff, sizeof (buff));
		if (memcmp(buff, magic, sizeof (buff)))
		{
			logerror("kim1_rom_load: magic '%s' not found\n", magic);
			return INIT_FAIL;
		}
		osd_fread_lsbfirst(file, &addr, 2);
		osd_fread_lsbfirst(file, &size, 2);
		osd_fread(file, &ident, 1);
		logerror("kim1_rom_load: $%04X $%04X $%02X\n", addr, size, ident);
		while (size-- > 0)
			osd_fread(file, &RAM[addr++], 1);
		osd_fclose(file);
	}
	return INIT_PASS;
}

void kim1_cassette_exit(int id)
{
	/* nothing yet */
}

int kim1_cassette_id(int id)
{
	const char magic[] = "KIM1";
	char buff[4];
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
		osd_fread(file, buff, sizeof (buff));
		if (memcmp(buff, magic, sizeof (buff)) == 0)
		{
			logerror("kim1_rom_id: magic '%s' found\n", magic);
			return 1;
		}
	}
	return 0;
}
#endif

int sym1_interrupt(void)
{
	int i;

	/* decrease the brightness of the six 7segment LEDs */
	for (i = 0; i < 6; i++)
	{
		if (videoram[i * 2 + 1] > 0)
			videoram[i * 2 + 1] -= 1;
	}
	return ignore_interrupt();
}

