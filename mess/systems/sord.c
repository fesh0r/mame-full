/******************************************************************************

	sord m5
	system driver

	Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "machine/z80fmly.h"
#include "vidhrdw/tms9928a.h"
#include "sound/sn76496.h"
#include "cpu/z80/z80.h"

static void sordm5_video_int(int state)
{
	logerror("trigger 3 ctc w %02x\r\n ",state);

	z80ctc_0_trg3_w(0,1);
	z80ctc_0_trg3_w(0,0);
	TMS9928A_interrupt();
}


static void sord_m5_ctc_interrupt(int state)
{
	logerror("interrupting ctc %02x\r\n ",state);
    cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

static WRITE_HANDLER(sord_video_interrupt)
{
}

static z80ctc_interface	sord_m5_ctc_intf =
{
	1,
	{3800000},
	{0},
	{sord_m5_ctc_interrupt},
	{0},
	{0},
    {sord_video_interrupt}
};

int sord_m5_vh_init(void)
{
	return TMS9928A_start(TMS99x8A, 0x4000);
}

static READ_HANDLER ( sord_keyboard_r )
{
	return 0x00;
}

static READ_HANDLER ( sord_joystick_r )
{
	return 0x00;
}

static READ_HANDLER ( sord_misc_r)
{
	return 0x000;

}

MEMORY_READ_START( readmem_sord_m5 )
	{0x0000, 0x01fff, MRA_ROM},
	{0x2000, 0x03fff, MRA_ROM},
	{0x4000, 0x06fff, MRA_NOP},
	{0x7000, 0x0ffff, MRA_RAM},
//	{0x8000, 0x0ffff, MRA_NOP},	
MEMORY_END



MEMORY_WRITE_START( writemem_sord_m5 )
	{0x0000, 0x01fff, MWA_ROM},
	{0x2000, 0x03fff, MWA_ROM},
	{0x7000, 0x0ffff, MWA_RAM},
MEMORY_END

PORT_READ_START( readport_sord_m5 )
	{ 0x000, 0x003, z80ctc_0_r},
	{ 0x011, 0x011, TMS9928A_register_r},
	{ 0x010, 0x010, TMS9928A_vram_r},
//	{ 0x030, 0x036, sord_keyboard_r },
//	{ 0x037, 0x037, sord_joystick_r },
	{ 0x050, 0x050, sord_misc_r},
PORT_END

PORT_WRITE_START( writeport_sord_m5 )
	{ 0x000, 0x003, z80ctc_0_w},
	{ 0x011, 0x011, TMS9928A_register_w},
	{ 0x010, 0x010, TMS9928A_vram_w},
	{ 0x020, 0x020, SN76496_0_w},
PORT_END


static int sord_m5_interrupt(void)
{
	TMS9928A_interrupt();
	return ignore_interrupt();
}

void sord_m5_init_machine(void)
{
	z80ctc_init(&sord_m5_ctc_intf);
	TMS9928A_reset ();
	TMS9928A_int_callback (sordm5_video_int);
}


void sord_m5_shutdown_machine(void)
{
}

INPUT_PORTS_START(sord_m5)
	PORT_START
//  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 !", KEYCODE_1, IP_JOY_NONE)
  //PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
 // PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 %", KEYCODE_5, IP_JOY_NONE)
 // PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 '", KEYCODE_7, IP_JOY_NONE)
 // PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 )", KEYCODE_9, IP_JOY_NONE)
 // PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "- =", KEYCODE_MINUS, IP_JOY_NONE)
 // PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\ |", KEYCODE_BACKSLASH, IP_JOY_NONE)
 // PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NUMERICPAD7 PAGE )", KEYCODE_7_PAD, IP_JOY_NONE)
INPUT_PORTS_END


static Z80_DaisyChain sord_m5_daisy_chain[] =
{
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};


static struct SN76496interface sn76496_interface =
{
    1,  		/* 1 chip 		*/
    {3579545},  /* 3.579545 MHz */
    { 100 }
};


static struct MachineDriver machine_driver_sord_m5 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			3800000,
			readmem_sord_m5,		   /* MemoryReadAddress */
			writemem_sord_m5,		   /* MemoryWriteAddress */
			readport_sord_m5,		   /* IOReadPort */
			writeport_sord_m5,		   /* IOWritePort */
            sord_m5_interrupt, 1,
			0, 0,	/* every scanline */
			sord_m5_daisy_chain
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	sord_m5_init_machine,			   /* init machine */
	sord_m5_shutdown_machine,
	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	0,								
	TMS9928A_PALETTE_SIZE, TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,
	VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,								   /* MachineLayer */
	sord_m5_vh_init,
	TMS9928A_stop,
	TMS9928A_refresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(sordm5)
	ROM_REGION(0x010000, REGION_CPU1,0)
	ROM_LOAD("sordint.rom",0x0000, 0x02000, 0x78848d39)
	ROM_LOAD("sordbasi.rom",0x2000, 0x02000, 0x9a98e6ce)
//	ROM_LOAD("balloon.rom",0x2000, 0x03000, 0x01)
ROM_END

static const struct IODevice io_sordm5[] =
{

	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMPX( 1982, sordm5,      0,            sord_m5,          sord_m5,      0,       "Sord", "Sord M5",GAME_NOT_WORKING)
