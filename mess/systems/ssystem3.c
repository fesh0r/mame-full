/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at November 2000
******************************************************************************/


/* 
chess champion super system III
-------------------------------
(germany chess champion mk III)

6502
6522 ($6000?)
2x2114 (1kx4 sram) ($0000)
signetics 7947e c19081e ss-3-lrom (real empty!!!!????)
signetics 7945e c19082 ss-3-hrom ($d000??)
(both connected to the same pins!,
look into mess source mess/messroms/rddil24.c for notes)

optional printer (special serial connection)
optional board display (special serial connection)
internal expansion/cartridge port
 (special power saving pack)
*/

#include "driver.h"

#include "includes/ssystem3.h"
#include "machine/6522via.h"
#include "cpu/m6502/m6502.h"

static int ssystem3_frame_int(void);
static void ssystem3_machine_init(void);

static MEMORY_READ_START( ssystem3_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x600f, via_0_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( ssystem3_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
// 0x4000, 0x40ff, lcd chip!?
	{ 0x6000, 0x600f, via_0_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( ssystem3 )
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
#if 0
	PORT_START
	DIPS_HELPER( 0x001, "Test 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x002, "Test 2", KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "Test 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x008, "Test 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "Test 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x020, "Test 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "Test 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x080, "Test 8", KEYCODE_8_PAD, CODE_NONE)
#endif
INPUT_PORTS_END

static struct DACinterface ssystem3_dac={ 1, {80}}; // silence is golden

static struct MachineDriver machine_driver_ssystem3 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			ssystem3_readmem, ssystem3_writemem,0,0,
			ssystem3_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION, // lcd!
	1,				/* single CPU */
	ssystem3_machine_init,
	0,//pc1401_machine_stop,

	728, 437, { 0, 728 - 1, 0, 437 - 1},
	0, 			   /* graphics decode info */
	sizeof (ssystem3_palette) / sizeof (ssystem3_palette[0]) ,
	sizeof (ssystem3_colortable) / sizeof(ssystem3_colortable[0][0]),
	ssystem3_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    ssystem3_vh_start,
	ssystem3_vh_stop,
	ssystem3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ SOUND_DAC, &ssystem3_dac }
    }
};

ROM_START(ssystem3)
	ROM_REGION(0x10000,REGION_CPU1,0)
//	ROM_LOAD("ss3lrom", 0xc000, 0x1000, 0x0) // damned zero dump
	ROM_LOAD("ss3hrom", 0xf000, 0x1000, 0x52741e0b) // not sure if complete
	ROM_RELOAD(0xd000, 0x1000)
/* 0xd450 reset,irq,nmi

   d7c7 outputs 2e..32 to serial port 1
 */

ROM_END

static const struct IODevice io_ssystem3[] = {
    { IO_END }
};


/*
  port b
   bit 0: ??

    hi speed serial 1 
   bit 1: output data
   bit 2: output clock (hi data is taken)

	bit 6: input clocks!?

 */

struct via6522_interface config=
{
	0,//mem_read_handler in_a_func;
		0,//mem_read_handler in_b_func;
		0,//mem_read_handler in_ca1_func;
		0,//mem_read_handler in_cb1_func;
		0,//mem_read_handler in_ca2_func;
		0,//mem_read_handler in_cb2_func;
		0,//mem_write_handler out_a_func;
		0,//mem_write_handler out_b_func;
		0,//mem_write_handler out_ca2_func;
		0,//mem_write_handler out_cb2_func;
		0,//void (*irq_func)(int state);
};

void init_ssystem3(void)
{
	via_config(0,&config);
}

static int ssystem3_frame_int(void)
{
	static int toggle=0;
	via_set_input_b(0,toggle?0x40:0);
	toggle^=0;
	return 0;
}

static void ssystem3_machine_init(void)
{
	via_reset();
}


CONS( 1979,	ssystem3,	0, 		ssystem3,	ssystem3,	ssystem3,	  "NOVAG Industries Ltd.",  "Chess Champion Super System III") 
//chess champion MK III in germany

#ifdef RUNTIME_LOADER
extern void ssystem3_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"ssystem3")==0) drivers[i]=&driver_ssystem3;
	}
}
#endif
