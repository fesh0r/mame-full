/******************************************************************************

    kc.c
	system driver

	A big thankyou to Torsten Paul for his great help with this
	driver!


	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80fmly.h"
#include "cpu/z80/z80.h"
#include "includes/kc.h"

/* pio is last in chain and therefore has highest priority */
static Z80_DaisyChain kc85_daisy_chain[] =
{
        {z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
        {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
        {0,0,0,-1}
};


static READ_HANDLER(kc85_4_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			return kc85_module_r(offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);

	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE_HANDLER(kc85_4_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			kc85_module_w(offset,data);
//			return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


PORT_READ_START( readport_kc85_4 )
	{ 0x0000, 0x0ffff, kc85_4_port_r },
PORT_END

PORT_WRITE_START( writeport_kc85_4 )
	{0x0000, 0x0ffff, kc85_4_port_w},
PORT_END

static READ_HANDLER(kc85_4d_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x080:
			return kc85_module_r(offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);
		case 0x0f0:
		case 0x0f1:
		case 0x0f2:
		case 0x0f3:
			return kc85_disc_interface_ram_r(offset);

	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE_HANDLER(kc85_4d_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x080:
			kc85_module_w(offset,data);
			return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;

		case 0x0f0:
		case 0x0f1:
		case 0x0f2:
		case 0x0f3:
			kc85_disc_interface_ram_w(offset,data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}

PORT_READ_START( readport_kc85_4d )
	{0x0000, 0x0ffff, kc85_4d_port_r},
PORT_END

PORT_WRITE_START( writeport_kc85_4d )
	{0x0000, 0x0ffff, kc85_4d_port_w},
PORT_END

MEMORY_READ_START( readmem_kc85_4 )
	{0x00000, 0x03fff, MRA_BANK1},
	{0x04000, 0x07fff, MRA_BANK2},
	{0x08000, 0x0a7ff, MRA_BANK3},
	//{0x0a800, 0x0bfff, MRA_RAM},
	{0x0a800, 0x0bfff, MRA_BANK4},
	{0x0c000, 0x0dfff, MRA_BANK5},
	{0x0e000, 0x0ffff, MRA_BANK6},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_4 )
	{0x00000, 0x03fff, MWA_BANK7},
	{0x04000, 0x07fff, MWA_BANK8},
	{0x08000, 0x0a7ff, MWA_BANK9},
	//{0x0a800, 0x0bfff, MWA_RAM},
	{0x0a800, 0x0bfff, MWA_BANK10},
MEMORY_END

MEMORY_READ_START( readmem_kc85_3 )
	{0x00000, 0x03fff, MRA_BANK1},
	{0x04000, 0x07fff, MRA_BANK2},
	{0x08000, 0x0bfff, MRA_BANK3},
	{0x0c000, 0x0dfff, MRA_BANK4},
	{0x0e000, 0x0ffff, MRA_BANK5},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_3 )
	{0x00000, 0x03fff, MWA_BANK6},
	{0x04000, 0x07fff, MWA_BANK7},
	{0x08000, 0x0bfff, MWA_BANK8},
MEMORY_END

static READ_HANDLER(kc85_3_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			return kc85_module_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);
	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE_HANDLER(kc85_3_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			kc85_module_w(offset,data);
//			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


PORT_READ_START( readport_kc85_3 )
	{ 0x0000, 0x0ffff, kc85_3_port_r},
PORT_END

PORT_WRITE_START( writeport_kc85_3 )
	{ 0x0000, 0x0ffff, kc85_3_port_w},
PORT_END



INPUT_PORTS_START( kc85 )
	KC_KEYBOARD
INPUT_PORTS_END


/********************/
/** DISC INTERFACE **/

MEMORY_READ_START( readmem_kc85_disc_hw )
	{0x0000, 0x0ffff, MRA_RAM},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_disc_hw )
	{0x0000, 0x0ffff, MWA_RAM},
MEMORY_END

PORT_READ_START( readport_kc85_disc_hw )
	{0x0f0, 0x0f0, nec765_status_r},
	{0x0f1, 0x0f1, nec765_data_r},
	{0x0f2, 0x0f3, nec765_dack_r},
	{0x0f4, 0x0f5, kc85_disc_hw_input_gate_r},
	/*{0x0f6, 0x0f7, MRA_NOP},*/			/* for controller */
	{0x0fc, 0x0ff, kc85_disk_hw_ctc_r},
PORT_END

PORT_WRITE_START( writeport_kc85_disc_hw )
	{0x0f1, 0x0f1, nec765_data_w},
	{0x0f2, 0x0f3, nec765_dack_w},
	{0x0f8, 0x0f9, kc85_disc_hw_terminal_count_w}, /* terminal count */
	{0x0fc, 0x0ff, kc85_disk_hw_ctc_w},
PORT_END



static struct Speaker_interface kc_speaker_interface=
{
 1,
 {50},
};

static struct Wave_interface kc_wave_interface=
{
	1,	  /* number of cassette drives = number of waves to mix */
	{25},	/* default mixing level */

};

static struct MachineDriver machine_driver_kc85_4 =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80_MSX|CPU_16BIT_PORT,  /* type */
			KC85_4_CLOCK,
			readmem_kc85_4,		   /* MemoryReadAddress */
			writemem_kc85_4,		   /* MemoryWriteAddress */
			readport_kc85_4,		   /* IOReadPort */
			writeport_kc85_4,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
            kc85_daisy_chain
	    },
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	kc85_4_init_machine,			   /* init machine */
	kc85_4_shutdown_machine,
	/* video hardware */
	KC85_SCREEN_WIDTH,			   /* screen width */
	KC85_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_PALETTE_SIZE,								   /* total colours
									    */
	KC85_PALETTE_SIZE,								   /* color table len */
	kc85_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	kc85_4_vh_start,
	kc85_4_vh_stop,
	kc85_4_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
				SOUND_SPEAKER,
				&kc_speaker_interface
		},
		/* cassette sound is mixed with speaker sound */
		{
				SOUND_WAVE,
				&kc_wave_interface,
		}
	}
};


static struct MachineDriver machine_driver_kc85_4d =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80_MSX | CPU_16BIT_PORT,  /* type */
			KC85_4_CLOCK,
			readmem_kc85_4,		   /* MemoryReadAddress */
			writemem_kc85_4,		   /* MemoryWriteAddress */
			readport_kc85_4d,		   /* IOReadPort */
			writeport_kc85_4d,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
            kc85_daisy_chain
	    },
		KC_DISC_INTERFACE_CPU 
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	2,								   /* cpu slices per frame */
	kc85_4d_init_machine,			   /* init machine */
	kc85_4d_shutdown_machine,
	/* video hardware */
	KC85_SCREEN_WIDTH,			   /* screen width */
	KC85_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_PALETTE_SIZE,								   /* total colours
									    */
	KC85_PALETTE_SIZE,								   /* color table len */
	kc85_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	kc85_4_vh_start,
	kc85_4_vh_stop,
	kc85_4_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
				SOUND_SPEAKER,
				&kc_speaker_interface
		},
		/* cassette sound is mixed with speaker sound */
		{
				SOUND_WAVE,
				&kc_wave_interface,
		}
	}
};


static struct MachineDriver machine_driver_kc85_3 =
{
		/* basic machine hardware */
	{
			/* MachineCPU */
		{
			CPU_Z80_MSX|CPU_16BIT_PORT,  /* type */
			KC85_3_CLOCK,
			readmem_kc85_3,		   /* MemoryReadAddress */
			writemem_kc85_3,		   /* MemoryWriteAddress */
			readport_kc85_3,		   /* IOReadPort */
			writeport_kc85_3,		   /* IOWritePort */
			0,		/* VBlank  Interrupt */
			0,				   /* vblanks per frame */
			0, 0,	/* every scanline */
            kc85_daisy_chain
	    },
	},
	50,								   /* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	kc85_3_init_machine,			   /* init machine */
	kc85_3_shutdown_machine,
	/* video hardware */
	KC85_SCREEN_WIDTH,			   /* screen width */
	KC85_SCREEN_HEIGHT,			   /* screen height */
	{0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /* graphics decode info */
	KC85_PALETTE_SIZE,								   /* total colours
									    */
	KC85_PALETTE_SIZE,								   /* color table len */
	kc85_init_palette,			   /* init palette */

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
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, 0xdfe34b08)
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, 0x57d9ab02)
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, 0xd64cd50b)
ROM_END


ROM_START(kc85_4d)
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, 0xdfe34b08)
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, 0x57d9ab02)
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, 0xd64cd50b)

	ROM_REGION(0x010000, REGION_CPU2,0)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x014000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, 0xdfe34b08)
	ROM_LOAD("caos__e0.853", 0x12000, 0x2000, 0x52bc2199)
ROM_END



#define KC85_IO \
{ \
   IO_QUICKLOAD,	   /* type */ \
   1,				   /* count */ \
   "kcc\0",       /*file extensions */ \
   0,	   /* reset if file changed */ \
   NULL,               /* id */ \
   kc_quickload_load,     /* init */ \
   NULL,     /* exit */ \
   NULL,               /* info */ \
   NULL,     /* open */ \
   NULL,               /* close */ \
   NULL,               /* status */ \
   NULL,               /* seek */ \
   NULL,               /* input */ \
   NULL,               /* output */ \
   NULL,               /* input_chunk */ \
   NULL                /* output_chunk */ \
}, \
IO_CASSETTE_WAVE(1,"wav\0",NULL,kc_cassette_device_init,kc_cassette_device_exit)


static const struct IODevice io_kc85_4[] =
{
	KC85_IO,
	{IO_END}
};

static const struct IODevice io_kc85_4d[] =
{
	KC85_IO,
	{
		IO_FLOPPY,				/* type */
		4,						/* count */
		"dsk\0",                /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		kc85_floppy_init, /* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{IO_END}
};
#define io_kc85_3 io_kc85_4

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 19??, kc85_3,   0,     kc85_3,  kc85,        0,                "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMPX( 19??, kc85_4,   kc85_3,     kc85_4,  kc85,        0,                "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMPX( 19??, kc85_4d,  kc85_3,     kc85_4d,  kc85,        0,                "VEB Mikroelektronik", "KC 85/4 + Disk Interface Module (D004)", GAME_NOT_WORKING)
