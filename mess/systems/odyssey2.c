/***************************************************************************

  /systems/odyssey2.c

  Driver file to handle emulation of the Odyssey2.

***************************************************************************/

/**********************************************
8048 Ports:???????????
P1	Bit 0..1  - RAM bank select
	Bit 3..7  - Keypad input:

P2	Bit 0..3  - A8-A11
	Bit 4..7  - Sound control

T1	Mirror sync pulse

***********************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"

MEMORY_READ_START( readmem )
	{ 0x0000, 0x03FF, MRA_ROM },
	{ 0x0400, 0x14FF, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03FF, MWA_ROM },
	{ 0x0400, 0x14FF, MWA_ROM },
MEMORY_END

PORT_READ_START( readport )
	{ 0x00, 	0xff,	  odyssey2_MAINRAM_r},
	{ I8039_p1, I8039_p1, odyssey2_getp1 },
PORT_END

PORT_WRITE_START( writeport )
	{ 0x00, 	0xff,	  odyssey2_MAINRAM_w },
	{ I8039_p1, I8039_p1, odyssey2_putp1 },
PORT_END


INPUT_PORTS_START( odyssey2 )
	PORT_START		/* IN0 */
INPUT_PORTS_END

static struct MachineDriver machine_driver_odyssey2 =
{
	/* basic machine hardware */
	{
		{
			CPU_I8048,
			1790000,  /* 1.79 MHz */
			readmem,writemem,readport,writeport,
			ignore_interrupt,1
		}
	},
	8*15, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	odyssey2_init_machine,	/* init_machine */
	0,						/* stop_machine */

	/* video hardware */
	262,240, {0,262-1,0,240-1},
	NULL,
	(8+2)*3,
	8*2,
	odyssey2_vh_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	odyssey2_vh_start,
	odyssey2_vh_stop,
	odyssey2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};


ROM_START (odyssey2)
	ROM_REGION(0x10000,REGION_CPU1,0)    /* 64 k Internal RAM */
	ROM_LOAD ("o2bios.rom", 0x0000, 0x400, 0x8016a315)
ROM_END

static const struct IODevice io_odyssey2[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",            /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		NULL,				/* id */
		odyssey2_load_rom,	/* init */
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

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
CONSX( 1982, odyssey2, 0,		odyssey2, odyssey2, 0,		  "Magnavox",  "ODYSSEY 2", GAME_NO_SOUND )

