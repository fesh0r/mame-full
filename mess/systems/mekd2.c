/******************************************************************************
	Motorola Evaluation Kit 6800 D2
	MEK6800D2

	system driver

	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

	memory map

	range		short	description
	0000-00ff	RAM 	256 bytes RAM
	0100-01ff	RAM 	optional 256 bytes RAM
	6000-63ff	PROM	optional PROM
	or
	6000-67ff	ROM 	optional ROM
	8004-8007	PIA
	8008		ACIA	cassette interface
	8020-8023	PIA 	keyboard interface
	a000-a07f	RAM 	128 bytes RAM (JBUG scratch)
	c000-c3ff	PROM	optional PROM
	or
	c000-c7ff	ROM 	optional ROM
	e000-e3ff	ROM 	JBUG monitor program
	e400-ffff	-/- 	not used

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/mekd2.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror(x)
#else
#define LOG(x)	/* x */
#endif

static READ_HANDLER(mekd2_pia_r) { return 0xff; }
static READ_HANDLER(mekd2_cas_r) { return 0xff; }
static READ_HANDLER(mekd2_kbd_r) { return 0xff; }
static READ_HANDLER(mekd2_mirror_r) { UINT8 *mem = memory_region(REGION_CPU1); return mem[0xe000+(offset&0x3ff)]; }

UINT8 pia[8];

static WRITE_HANDLER(mekd2_pia_w) { }
static WRITE_HANDLER(mekd2_cas_w) { }

static WRITE_HANDLER(mekd2_kbd_w)
{
	pia[offset] = data;
	switch( offset )
	{
	case 2:
		if( data & 0x20 )
		{
			videoram[0*2+0] = ~pia[0];
			videoram[0*2+1] = 14;
		}
		if( data & 0x10 )
		{
			videoram[1*2+0] = ~pia[0];
			videoram[1*2+1] = 14;
		}
		if( data & 0x08 )
		{
			videoram[2*2+0] = ~pia[0];
			videoram[2*2+1] = 14;
		}
		if( data & 0x04 )
		{
			videoram[3*2+0] = ~pia[0];
			videoram[3*2+1] = 14;
		}
		if( data & 0x02 )
		{
			videoram[4*2+0] = ~pia[0];
			videoram[4*2+1] = 14;
		}
		if( data & 0x01 )
		{
			videoram[5*2+0] = ~pia[0];
			videoram[5*2+1] = 14;
		}
		break;
	}
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
//	{ 0x0100, 0x01ff, MRA_RAM },	/* optional, set up in mekd2_init_machine */
//	{ 0x6000, 0x67ff, MRA_ROM },	/* -"- */
//	  { 0x8004, 0x8007, mekd2_pia_r },
//	  { 0x8008, 0x8008, mekd2_cas_r },
//	  { 0x8020, 0x8023, mekd2_kbd_r },
	{ 0xa000, 0xa07f, MRA_RAM },
//	{ 0xc000, 0xc7ff, MRA_RAM },	/* optional, set up in mekd2_init_machine */
	{ 0xe000, 0xe3ff, MRA_ROM },	/* JBUG ROM */
	{ 0xe400, 0xffff, mekd2_mirror_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
//	{ 0x0100, 0x01ff, MWA_RAM },	/* optional, set up in mekd2_init_machine */
//	{ 0x6000, 0x67ff, MWA_ROM },	/* -"- */
//	  { 0x8004, 0x8007, mekd2_pia_w },
//	  { 0x8008, 0x8008, mekd2_cas_w },
	{ 0x8020, 0x8023, mekd2_kbd_w },
	{ 0xa000, 0xa07f, MWA_RAM },
//	{ 0xc000, 0xc7ff, MWA_RAM },	/* optional, set up in mekd2_init_machine */
	{ 0xe000, 0xe3ff, MWA_ROM },	/* JBUG ROM */
MEMORY_END

INPUT_PORTS_START( mekd2 )
	PORT_START			/* IN0 keys row 0 */
INPUT_PORTS_END

static struct GfxLayout led_layout =
{
	18, 24, 	/* 16 x 24 LED 7segment displays */
	128,		/* 128 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15,
	 16,17 },
	{ 0*24, 1*24, 2*24, 3*24,
	  4*24, 5*24, 6*24, 7*24,
	  8*24, 9*24,10*24,11*24,
	 12*24,13*24,14*24,15*24,
	 16*24,17*24,18*24,19*24,
	 20*24,21*24,22*24,23*24,
	 24*24,25*24,26*24,27*24,
	 28*24,29*24,30*24,31*24 },
	24 * 24,	/* every LED code takes 32 times 18 (aligned 24) bit words */
};

static struct GfxLayout key_layout =
{
	24, 18, 	/* 24 * 18 keyboard icons */
	24, 		/* 24  codes */
	2,			/* 2 bit per pixel */
	{ 0, 1 },	/* two bitplanes */
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2,
	  8*2, 9*2,10*2,11*2,12*2,13*2,14*2,15*2,
	 16*2,17*2,18*2,19*2,20*2,21*2,22*2,23*2 },
	{ 0*24*2, 1*24*2, 2*24*2, 3*24*2, 4*24*2, 5*24*2, 6*24*2, 7*24*2,
	  8*24*2, 9*24*2,10*24*2,11*24*2,12*24*2,13*24*2,14*24*2,15*24*2,
	 16*24*2,17*24*2 },
	18 * 24 * 2,	/* every icon takes 18 rows of 24 * 2 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0, &led_layout, 0, 16 },
	{ 2, 0, &key_layout, 16*2, 2 },
	{ -1 } /* end of array */
};

static struct DACinterface dac_interface =
{
	1,			/* number of DACs */
	{ 100 } 	/* volume */
};

static struct MachineDriver machine_driver_mekd2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6800,
			614400, /* 614.4 kHz */
			readmem,writemem,0,0,
			mekd2_interrupt, 1
		}
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	mekd2_init_machine,
	NULL,			/* stop machine */

	/* video hardware (well, actually there was no video ;) */
	600, 768, { 0, 600 - 1, 0, 768 - 1},
	gfxdecodeinfo,
	256*3,
	256,
	mekd2_init_colors,		 /* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	mekd2_vh_start,
	mekd2_vh_stop,
	mekd2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START(mekd2)
	ROM_REGION(0x10000,REGION_CPU1)
		ROM_LOAD("jbug.rom",    0xe000, 0x0400, 0xa2a56502)
	ROM_REGION(128 * 24 * 3,REGION_GFX1)
		/* space filled with 7segement graphics by mekd2_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,REGION_GFX2)
		/* space filled with key icons by mekd2_init_driver */
ROM_END



static const struct IODevice io_mekd2[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"d2\0",             /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		mekd2_rom_id,		/* id */
		mekd2_rom_load, 	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	  FULLNAME */
CONS( 1977, mekd2,	   0,		mekd2,	  mekd2,	mekd2,	  "Motorola", "MEK6800D2" )

#ifdef RUNTIME_LOADER
extern void mekd2_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"mekd2")==0) drivers[i]=&driver_mekd2;
	}
}
#endif
