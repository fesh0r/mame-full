/******************************************************************************

    kc.c
	system driver

	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80fmly.h"
#include "cpu/z80/z80.h"
#include "includes/kc.h"

static Z80_DaisyChain kc85_4_daisy_chain[] =
{
        {z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};

static Z80_DaisyChain kc85_3_daisy_chain[] =
{
        {z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};

PORT_READ_START( readport_kc85_4 )
	{0x084, 0x084, kc85_4_84_r},
	{0x085, 0x085, kc85_4_84_r},
	{0x086, 0x086, kc85_4_86_r},
	{0x087, 0x087, kc85_4_86_r},
	{0x088, 0x089, kc85_4_pio_data_r},
	{0x08a, 0x08b, kc85_4_pio_control_r},
	{0x08c, 0x08f, kc85_4_ctc_r},
PORT_END

PORT_WRITE_START( writeport_kc85_4 )
	/* D05 decodes io ports on schematic */

	/* D08,D09 on schematic handle these ports */
	{0x084, 0x084, kc85_4_84_w},
	{0x085, 0x085, kc85_4_84_w},
	{0x086, 0x086, kc85_4_86_w},
	{0x087, 0x087, kc85_4_86_w},

	/* D06 on schematic handle these ports */
	{0x088, 0x089, kc85_4_pio_data_w},
	{0x08a, 0x08b, kc85_4_pio_control_w},

	/* D07 on schematic handle these ports */
	{0x08c, 0x08f, kc85_4_ctc_w },
PORT_END

MEMORY_READ_START( readmem_kc85_4 )
	{0x00000, 0x07fff, MRA_RAM},
	{0x08000, 0x0a7ff, MRA_BANK3},
	{0x0a800, 0x0bfff, MRA_RAM},
	{0x0c000, 0x0dfff, MRA_BANK1},
	{0x0e000, 0x0ffff, MRA_BANK2},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_4 )
	{0x00000, 0x07fff, MWA_RAM},
	{0x08000, 0x0a7ff, MWA_BANK3},
	{0x0a800, 0x0bfff, MWA_RAM},
	{0x0c000, 0x0dfff, MWA_NOP},
	{0x0e000, 0x0ffff, MWA_NOP},
MEMORY_END

MEMORY_READ_START( readmem_kc85_3 )
	{0x00000, 0x07fff, MRA_RAM},
	{0x08000, 0x0a7ff, MRA_BANK3},
	{0x0a800, 0x0bfff, MRA_RAM},
	{0x0c000, 0x0dfff, MRA_BANK1},
	{0x0e000, 0x0ffff, MRA_BANK2},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_3 )
	{0x00000, 0x07fff, MWA_RAM},
	{0x08000, 0x0a7ff, MWA_BANK3},
	{0x0a800, 0x0bfff, MWA_RAM},
	{0x0c000, 0x0dfff, MWA_NOP},
	{0x0e000, 0x0ffff, MWA_NOP},
MEMORY_END


PORT_READ_START( readport_kc85_3 )
	{0x088, 0x089, kc85_4_pio_data_r},
	{0x08a, 0x08b, kc85_4_pio_control_r},
	{0x08c, 0x08f, kc85_4_ctc_r},
PORT_END

PORT_WRITE_START( writeport_kc85_3 )
	{0x088, 0x089, kc85_4_pio_data_w},
	{0x08a, 0x08b, kc85_4_pio_control_w},
	{0x08c, 0x08f, kc85_4_ctc_w },
PORT_END



INPUT_PORTS_START( kc85_2 )
	KC_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START( kc85_3 )
	KC_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START( kc85_4 )
	KC_KEYBOARD
INPUT_PORTS_END

INPUT_PORTS_START( kc85_5 )
	KC_KEYBOARD
INPUT_PORTS_END

static struct MachineDriver machine_driver_kc85_4 =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80,  /* type */
			KC85_4_CLOCK,
			readmem_kc85_4,		   /* MemoryReadAddress */
			writemem_kc85_4,		   /* MemoryWriteAddress */
			readport_kc85_4,		   /* IOReadPort */
			writeport_kc85_4,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
            kc85_4_daisy_chain
	    },
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	kc85_4_init_machine,			   /* init machine */
	kc85_4_shutdown_machine,
	/* video hardware */
	KC85_4_SCREEN_WIDTH,			   /* screen width */
	KC85_4_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_4_SCREEN_WIDTH - 1), 0, (KC85_4_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_4_PALETTE_SIZE,								   /* total colours
									    */
	KC85_4_PALETTE_SIZE,								   /* color table len */
	kc85_4_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	kc85_4_vh_start,
	kc85_4_vh_stop,
	kc85_4_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
};


static struct MachineDriver machine_driver_kc85_3 =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80,  /* type */
			KC85_3_CLOCK,
			readmem_kc85_3,		   /* MemoryReadAddress */
			writemem_kc85_3,		   /* MemoryWriteAddress */
			readport_kc85_3,		   /* IOReadPort */
			writeport_kc85_3,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
            kc85_3_daisy_chain
	    },
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	kc85_3_init_machine,			   /* init machine */
	kc85_3_shutdown_machine,
	/* video hardware */
	KC85_3_SCREEN_WIDTH,			   /* screen width */
	KC85_3_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_3_SCREEN_WIDTH - 1), 0, (KC85_3_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_3_PALETTE_SIZE,								   /* total colours
									    */
	KC85_3_PALETTE_SIZE,								   /* color table len */
	kc85_3_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	kc85_3_vh_start,
	kc85_3_vh_stop,
	kc85_3_vh_screenrefresh,

		/* sound hardware */
	0,								   /* sh init */
	0,								   /* sh start */
	0,								   /* sh stop */
	0,								   /* sh update */
};



ROM_START(kc85_4)
	ROM_REGION(0x016000, REGION_CPU1,0)

    ROM_LOAD("kc854c.rom", 0x10000, 0x2000, 0x0dfe34b08)
    ROM_LOAD("kc854b.rom", 0x12000, 0x1000, 0x057d9ab02)
    ROM_LOAD("kc854a.rom", 0x14000, 0x2000, 0x0d64cd50b)
ROM_END



ROM_START(kc85_4b)
	ROM_REGION(0x016000, REGION_CPU1,0)

    ROM_LOAD("kc855c.rom", 0x10000, 0x2000, 0x0dfd594d1)
    ROM_LOAD("kc855b.rom", 0x12000, 0x1000, 0x0104ec9ef)
    ROM_LOAD("kc855a.rom", 0x14000, 0x2000, 0x0dfe34b08)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x016000, REGION_CPU1,0)

    ROM_LOAD("kc853b.rom", 0x12000, 0x1000, 0x057d9ab02)
	ROM_LOAD("kc853a.rom", 0x14000, 0x2000, 0x0dfe34b08)
ROM_END



static const struct IODevice io_kc85_4[] =
{
       {IO_END}
};

#define io_kc85_3 io_kc85_4
#define io_kc85_4b io_kc85_4

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 19??, kc85_3,   0,     kc85_3,  kc85_3,        0,                "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMPX( 19??, kc85_4,   0,     kc85_4,  kc85_4,        0,                "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMPX( 19??, kc85_4b,   0,     kc85_4,  kc85_4,        0,                "VEB Mikroelektronik", "KC 85/4 with 256k ram", GAME_NOT_WORKING)
