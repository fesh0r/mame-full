/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at September 2000
******************************************************************************/

#include "driver.h"

#include "includes/mk2.h"
#include "includes/rriot.h"
#include "cpu/m6502/m6502.h"

/*
chess champion mk II

MOS MPS 6504 2179
MOS MPS 6530 024 1879
 layout of 6530 dumped with my adapter
 0x1300-0x133f io
 0x1380-0x13bf ram
 0x1400-0x17ff rom

2x2111 ram (256x4?)
MOS MPS 6332 005 2179
74145 bcd to decimal encoder (10 low active select lines)
(7400)

4x 7 segment led display (each with dot)
4 single leds
21 keys
*/

/*
  83, 84 contains display variables
 */
// only lower 12 address bits on bus!
static struct MemoryReadAddress mk2_readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM }, // 2 2111, should be mirrored
	{ 0x8009, 0x8009, MRA_NOP },// bit $8009 (ora #$80) causes false accesses
	{ 0x8b00, 0x8f0f, rriot_0_r },
	{ 0x8b80, 0x8bbf, MRA_RAM }, // rriot ram
	{ 0x8c00, 0x8fff, MRA_ROM }, // rriot rom
	{ 0xf000, 0xffff, MRA_ROM },
	MEMORY_TABLE_END
};

static struct MemoryWriteAddress mk2_writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x8b00, 0x8f0f, rriot_0_w },
	{ 0x8b80, 0x8bbf, MWA_RAM },
	{ 0x8c00, 0x8fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
	MEMORY_TABLE_END
};

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( mk2 )
	PORT_START
DIPS_HELPER( 0x001, "NEW GAME", KEYCODE_F3, CODE_NONE) // seams to be direct wired to reset
	DIPS_HELPER( 0x002, "CLEAR", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x004, "ENTER", KEYCODE_ENTER, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "Black A    Black", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x002, "Black B    Field", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x004, "Black C    Time?", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x008, "Black D    Time?", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x010, "Black E    Time off?", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x020, "Black F    LEVEL", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x040, "Black G    Swap", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x080, "Black H    White", KEYCODE_H, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "White 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x002, "White 2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x004, "White 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x008, "White 4", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x010, "White 5", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x020, "White 6", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x040, "White 7", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x080, "White 8", KEYCODE_8, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "White 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x002, "White 2", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "White 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x008, "White 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "White 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x020, "White 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "White 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x080, "White 8", KEYCODE_8_PAD, CODE_NONE)
INPUT_PORTS_END

static int mk2_frame_int(void)
{
	return 0;
}

static void mk2_machine_init(void)
{
	rriot_reset(0);
}

static struct DACinterface mk2_dac={ 1, {80}}; // silence is golden

static struct MachineDriver machine_driver_mk2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502, // 6504
			1000000,
			mk2_readmem,mk2_writemem,0,0,
			mk2_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION, // lcd!
	1,				/* single CPU */
	mk2_machine_init,
	0,//pc1401_machine_stop,

	252, 559, { 0, 252 - 1, 0, 559 - 1},
	0, 			   /* graphics decode info */
	sizeof (mk2_palette) / sizeof (mk2_palette[0]) ,
	sizeof (mk2_colortable) / sizeof(mk2_colortable[0][0]),
	mk2_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    mk2_vh_start,
	mk2_vh_stop,
	mk2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_DAC, &mk2_dac }
    }
};

ROM_START(mk2)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("024_1879", 0xbc00, 0x0400, 0x4f28c443)
	ROM_LOAD("005_2179", 0xf000, 0x1000, 0x6f10991b) // chess mate 7.5
ROM_END

static const struct IODevice io_mk2[] = {
    { IO_END }
};

/*
   port a
   0..7 led output
   0..6 keyboard input

   port b
    0..5 outputs
	0 speaker out
	6 as chipselect used!?
	7 interrupt out?

 	c4, c5, keyboard polling
	c0, c1, c2, c3 led output

*/

static int mk2_read_a(int chip)
{
	int data=0xff;
	int help=input_port_1_r(0)|input_port_2_r(0); // looks like white and black keys are the same!

	switch (rriot_0_b_r(0)&0xf) {
	case 4:
		if (help&0x20) data&=~0x1; //F
		if (help&0x10) data&=~0x2; //E
		if (help&8) data&=~0x4; //D
		if (help&4) data&=~0x8; // C
		if (help&2) data&=~0x10; // B
		if (help&1) data&=~0x20; // A
#if 0
		if (input_port_3_r(0)&1) data&=~0x40; //?
#endif
		break;
	case 5:
#if 0
		if (input_port_3_r(0)&2) data&=~0x1; //?
		if (input_port_3_r(0)&4) data&=~0x2; //?
		if (input_port_3_r(0)&8) data&=~0x4; //?
#endif
		if (input_port_0_r(0)&4) data&=~0x8; // Enter
		if (input_port_0_r(0)&2) data&=~0x10; // Clear
		if (help&0x80) data&=~0x20; // H
		if (help&0x40) data&=~0x40; // G
		break;
	case 0xc:
		if (help&0x20) data&=~0x1; //F
		if (help&0x10) data&=~0x2; //E
		if (help&8) data&=~0x4; //D
		if (help&4) data&=~0x8; // C
		if (help&2) data&=~0x10; // B
		if (help&1) data&=~0x20; // A
#if 0
		if (input_port_3_r(0)&1) data&=~0x40; //?
#endif
		break;
	case 0xd:
#if 0
		if (input_port_3_r(0)&2) data&=~0x1; //?
		if (input_port_3_r(0)&4) data&=~0x2; //?
		if (input_port_3_r(0)&8) data&=~0x4; //?
#endif
		if (input_port_0_r(0)&4) data&=~0x8; // Enter
		if (input_port_0_r(0)&2) data&=~0x10; // Clear
		if (help&0x80) data&=~0x20; // H
		if (help&0x40) data&=~0x40; // G
		break;
	}
	return data;
}

static void mk2_write_a(int chip, int value)
{
	int temp=rriot_0_b_r(0);

//	if ((temp&0xc)==0)
		mk2_led[temp&3]=value;
}

static int mk2_read_b(int chip)
{
	return 0xff&~0x40; // chip select mapped to pb6
}

static void mk2_write_b(int chip, int value)
{
	if ((value&0xe)==6)
		DAC_data_w(0,value&1?80:0);
}

static void mk2_irq(int chip, int level)
{
	cpu_set_irq_line(0, M6502_INT_IRQ, level);
}

static RRIOT_CONFIG riot={
	1000000,
	{ mk2_read_a, mk2_write_a },
	{ mk2_read_b, mk2_write_b },
	mk2_irq
};

void init_mk2(void)
{
	rriot_config(0,&riot);
}

// seams to be developed by mostek (MK)
CONS( 1979,	mk2,	0, 		mk2,	mk2,	mk2,	  "Quelle International",  "Chess Champion MK II")
