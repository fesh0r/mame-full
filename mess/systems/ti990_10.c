/*
	Experimental ti990/10 driver

	This driver boots the DX10 build tape.  However, this will not help much until a terminal is
	emulated.

TODO :
* finish CPU emulation
* finish tape emulation
* write disk emulation
* emulate a 911 or 913 VDT terminal
* add additionnal bells and whistles as need appears
*/

/*
	CRU map:

	990/10 CPU board:
	1fa0-1fbe: map file CRU interface
	1fc0-1fde: error interrupt register
	1fe0-1ffe: control panel

	optional hardware (default configuration):
	0000-001e: 733 ASR
	0020-003e: PROM programmer
	0040-005e: card reader
	0060-007e: line printer
	0080-00be: FD800 floppy disc
	00c0-00ee: 913 VDT
	0100-013e: 913 VDT #2
	0140-017e: 913 VDT #3


	TPCS map:
	1ff800: hard disk
	(1ff810: FD1000 floppy)
	1ff880: tape unit


	interrupt map (default configuration):
	0,1,2: CPU board
	4: card reader
	5: line clock
	6: 733 ASR
	7: FD800 floppy (or FD1000 floppy)
	9: 913 VDT #3
	10: 913 VDT #2
	11: 913 VDT
	13: hard disk
	14 line printer
	15: PROM programmer (actually not used)
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/mt3200.h"

static void clear_load(int dummy)
{
	cpu_set_nmi_line(0, CLEAR_LINE);
}

static void ti990_10_init_machine(void)
{
	cpu_set_nmi_line(0, ASSERT_LINE);
	timer_set(TIME_IN_MSEC(100), 0, clear_load);

	mt3200_reset();
}

static void ti990_10_stop_machine(void)
{

}

static int ti990_10_vblank_interrupt(void)
{


	return ignore_interrupt();
}

/*
three panel types
* operator panel
* programmer panel
* MDU

Operator panel :
* Power led
* Fault led
* Off/On/Load switch

Programmer panel :
* 16 status light, 32 switches, IDLE, RUN led
* interface to a low-level debugger in ROMs

* MDU :
* includes a tape unit, possibly other stuff

output :
0-7 : lights 0-7
8 : increment scan
9 : clear scan
A : run light
B : fault light
C : Memory Error Interrupt clear
D : Start panel timer
E : Set SIE function (interrupt after 2 instructions are executed)
F : flag

input :
0-7 : switches 0-7 (or data from MDU tape)
8 : scan count bit 1
9 : scan count bit 0
A : timer active
B : programmer panel not present
C : char in MDU tape unit buffer ?
D : unused ?
E : MDU tape unit present ?
F : flag

*/

static READ16_HANDLER ( ti990_10_panel_read )
{
	if (offset == 1)
		return 0x48;

	return 0;
}

static WRITE16_HANDLER ( ti990_10_panel_write )
{
}

/*
  TI990/10 video emulation.

  I guess there was text terminal and CRT terminals.
*/


static void ti990_10_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *dummy)
{
/*	memcpy(palette, & ti990_10_palette, sizeof(ti990_10_palette));
	memcpy(colortable, & ti990_10_colortable, sizeof(ti990_10_colortable));*/
}

static int ti990_10_vh_start(void)
{
	return 0; /*generic_vh_start();*/
}

/*#define ti990_10_vh_stop generic_vh_stop*/

static void ti990_10_vh_stop(void)
{
}

static void ti990_10_vh_refresh(struct mame_bitmap *bitmap, int full_refresh)
{

}

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }	/* end of array */
};


/*
  Memory map - see description above
*/

static MEMORY_READ16_START (ti990_10_readmem)

	{ 0x000000, 0x0fffff, MRA16_RAM },		/* let's say we have 1MB of RAM */
	{ 0x100000, 0x1ff7ff, MRA16_NOP },		/* free TILINE space */
	{ 0x1ff800, 0x1ff81f, MRA16_NOP },		/* disk controller TPCS - not implemented yet */
	{ 0x1ff820, 0x1ff87f, MRA16_NOP },		/* free TPCS */
	{ 0x1ff880, 0x1ff89f, mt3200_r },		/* tape controller TPCS */
	{ 0x1ff8a0, 0x1ffbff, MRA16_NOP },		/* free TPCS */
	{ 0x1ffc00, 0x1fffff, MRA16_ROM },		/* LOAD ROM */

MEMORY_END

static MEMORY_WRITE16_START (ti990_10_writemem)

	{ 0x000000, 0x0fffff, MWA16_RAM },		/* let's say we have 1MB of RAM */
	{ 0x100000, 0x1ff7ff, MWA16_NOP },		/* free TILINE space */
	{ 0x1ff800, 0x1ff81f, MWA16_NOP },		/* disk controller TPCS - not implemented yet */
	{ 0x1ff820, 0x1ff87f, MWA16_NOP },		/* free TPCS */
	{ 0x1ff880, 0x1ff89f, mt3200_w },		/* tape controller TPCS */
	{ 0x1ff8a0, 0x1ffbff, MWA16_NOP },		/* free TPCS */
	{ 0x1ffc00, 0x1fffff, MWA16_ROM },		/* LOAD ROM */

MEMORY_END


/*
  CRU map
*/

static PORT_WRITE16_START ( ti990_10_writeport )

	{ 0xfd0 << 1, 0xfdf << 1, ti990_10_mapper_cru_w },

	{ 0xff0 << 1, 0xfff << 1, ti990_10_panel_write },

PORT_END

static PORT_READ16_START ( ti990_10_readport )

	{ 0x1fa << 1, 0x1fb << 1, ti990_10_mapper_cru_r },

	{ 0x1fe << 1, 0x1ff << 1, ti990_10_panel_read },

PORT_END

static struct MachineDriver machine_driver_ti990_10 =
{
	/* basic machine hardware */
	{
		{
			CPU_TI990_10,
			4000000,	/* unknown */
			ti990_10_readmem, ti990_10_writemem, ti990_10_readport, ti990_10_writeport,
			ti990_10_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,
	ti990_10_init_machine,
	ti990_10_stop_machine,

	/* video hardware - no screen emulated */
	200,						/* screen width */
	200,						/* screen height */
	{ 0, 200-1, 0, 200-1},		/* visible_area */
	gfxdecodeinfo,				/* graphics decode info (???)*/
	0/*TI990_10_PALETTE_SIZE*/,		/* palette is 3*total_colors bytes long */
	0/*TI990_10_COLORTABLE_SIZE*/,	/* length in shorts of the color lookup table */
	ti990_10_init_palette,		/* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti990_10_vh_start,
	ti990_10_vh_stop,
	ti990_10_vh_refresh,

	/* sound hardware */
	0,
	0,0,0,

#if 0
	{ /* no sound ! */
	}
#endif
};


/*
  ROM loading
*/
ROM_START(ti990_10)
	/*CPU memory space*/

#if 0

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : older boot ROMs for floppy-disk */
	ROM_LOAD16_BYTE("975383.31", 0x1FFC00, 0x100, 0x64fcd040)
	ROM_LOAD16_BYTE("975383.32", 0x1FFC01, 0x100, 0x64277276)
	ROM_LOAD16_BYTE("975383.29", 0x1FFE00, 0x100, 0xaf92e7bf)
	ROM_LOAD16_BYTE("975383.30", 0x1FFE01, 0x100, 0xb7b40cdc)

#elif 1

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : newer "universal" boot ROMs  */
	ROM_LOAD16_BYTE("975383.45", 0x1FFC00, 0x100, 0x391943c7)
	ROM_LOAD16_BYTE("975383.46", 0x1FFC01, 0x100, 0xf40f7c18)
	ROM_LOAD16_BYTE("975383.47", 0x1FFE00, 0x100, 0x1ba571d8)
	ROM_LOAD16_BYTE("975383.48", 0x1FFE01, 0x100, 0x8852b09e)

#else

	ROM_REGION16_BE(0x202000, REGION_CPU1,0)

	/* TI990/12 ROMs - actually incompatible with TI990/10, but I just wanted to disassemble them. */
	ROM_LOAD16_BYTE("ti2025-7", 0x1FFC00, 0x1000, 0x4824f89c)
	ROM_LOAD16_BYTE("ti2025-8", 0x1FFC01, 0x1000, 0x51fef543)
	/* the other half of this ROM is not loaded - it makes no sense as TI990/12 machine code, as
	it is microcode... */

#endif

ROM_END

static void init_ti990_10(void)
{
#if 0
	/* load specific ti990/12 rom page */
	const int page = 3;

	memmove(memory_region(REGION_CPU1)+0x1FFC00, memory_region(REGION_CPU1)+0x1FFC00+(page*0x400), 0x400);
#endif
}

static const struct IODevice io_ti990_10[] =
{
	/* tape reader - only IO device we support */
	{
		IO_CASSETTE,			/* type */
		2,						/* count */
		"tap\0",				/* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		mt3200_tape_init,		/* init */
		mt3200_tape_exit,		/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

INPUT_PORTS_START(ti990_10)
INPUT_PORTS_END

/*		YEAR			NAME			PARENT	MACHINE		INPUT	INIT	COMPANY	FULLNAME */
COMP( circa 1975,	ti990_10,	0,			ti990_10,	ti990_10,	ti990_10,	"Texas Instruments",	"TI990/10" )
