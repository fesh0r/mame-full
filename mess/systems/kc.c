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
#include "devices/cassette.h"

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
	{0x00000, 0x03fff, MRA8_BANK1},
	{0x04000, 0x07fff, MRA8_BANK2},
	{0x08000, 0x0a7ff, MRA8_BANK3},
	//{0x0a800, 0x0bfff, MRA8_RAM},
	{0x0a800, 0x0bfff, MRA8_BANK4},
	{0x0c000, 0x0dfff, MRA8_BANK5},
	{0x0e000, 0x0ffff, MRA8_BANK6},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_4 )
	{0x00000, 0x03fff, MWA8_BANK7},
	{0x04000, 0x07fff, MWA8_BANK8},
	{0x08000, 0x0a7ff, MWA8_BANK9},
	//{0x0a800, 0x0bfff, MWA8_RAM},
	{0x0a800, 0x0bfff, MWA8_BANK10},
MEMORY_END

MEMORY_READ_START( readmem_kc85_3 )
	{0x00000, 0x03fff, MRA8_BANK1},
	{0x04000, 0x07fff, MRA8_BANK2},
	{0x08000, 0x0bfff, MRA8_BANK3},
	{0x0c000, 0x0dfff, MRA8_BANK4},
	{0x0e000, 0x0ffff, MRA8_BANK5},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_3 )
	{0x00000, 0x03fff, MWA8_BANK6},
	{0x04000, 0x07fff, MWA8_BANK7},
	{0x08000, 0x0bfff, MWA8_BANK8},
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
			kc85_3_pio_data_w(port-0x088, data);
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
	{0x0000, 0x0ffff, MRA8_RAM},
MEMORY_END

MEMORY_WRITE_START( writemem_kc85_disc_hw )
	{0x0000, 0x0ffff, MWA8_RAM},
MEMORY_END

PORT_READ_START( readport_kc85_disc_hw )
	{0x0f0, 0x0f0, nec765_status_r},
	{0x0f1, 0x0f1, nec765_data_r},
	{0x0f2, 0x0f3, nec765_dack_r},
	{0x0f4, 0x0f5, kc85_disc_hw_input_gate_r},
	/*{0x0f6, 0x0f7, MRA8_NOP},*/			/* for controller */
	{0x0fc, 0x0ff, kc85_disk_hw_ctc_r},
PORT_END

PORT_WRITE_START( writeport_kc85_disc_hw )
	{0x0f1, 0x0f1, nec765_data_w},
	{0x0f2, 0x0f3, nec765_dack_w},
	{0x0f8, 0x0f9, kc85_disc_hw_terminal_count_w}, /* terminal count */
	{0x0fc, 0x0ff, kc85_disk_hw_ctc_w},
PORT_END

MACHINE_DRIVER_START( cpu_kc_disc )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(readmem_kc85_disc_hw, writemem_kc85_disc_hw)
	MDRV_CPU_PORTS(readport_kc85_disc_hw, writeport_kc85_disc_hw)
MACHINE_DRIVER_END


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


static MACHINE_DRIVER_START( kc85_3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, KC85_3_CLOCK)
	MDRV_CPU_FLAGS( CPU_16BIT_PORT )
	MDRV_CPU_MEMORY(readmem_kc85_3, writemem_kc85_3)
	MDRV_CPU_PORTS(readport_kc85_3, writeport_kc85_3)
	MDRV_CPU_CONFIG(kc85_daisy_chain)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( kc85_3 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(KC85_SCREEN_WIDTH, KC85_SCREEN_HEIGHT)
	MDRV_VISIBLE_AREA(0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1))
	MDRV_PALETTE_LENGTH(KC85_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(KC85_PALETTE_SIZE)
	MDRV_PALETTE_INIT( kc85 )

	MDRV_VIDEO_START( kc85_3 )
	MDRV_VIDEO_UPDATE( kc85_3 )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, kc_speaker_interface)
	MDRV_SOUND_ADD(WAVE, kc_wave_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4 )
	MDRV_IMPORT_FROM( kc85_3 )

	MDRV_CPU_REPLACE("main", Z80, KC85_4_CLOCK)
	MDRV_CPU_FLAGS( CPU_16BIT_PORT )
	MDRV_CPU_MEMORY(readmem_kc85_4, writemem_kc85_4)
	MDRV_CPU_PORTS(readport_kc85_4, writeport_kc85_4)

	MDRV_MACHINE_INIT( kc85_4 )
	MDRV_VIDEO_START( kc85_4 )
	MDRV_VIDEO_UPDATE( kc85_4 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4d )
	MDRV_IMPORT_FROM( kc85_4 )
	MDRV_IMPORT_FROM( cpu_kc_disc )
	MDRV_INTERLEAVE( 2 )
	MDRV_MACHINE_INIT( kc85_4d )
MACHINE_DRIVER_END


ROM_START(kc85_4)
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b))
ROM_END


ROM_START(kc85_4d)
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b))

	ROM_REGION(0x010000, REGION_CPU2,0)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x014000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08))
	ROM_LOAD("caos__e0.853", 0x12000, 0x2000, CRC(52bc2199))
ROM_END

SYSTEM_CONFIG_START(kc85)
	CONFIG_DEVICE_CASSETTE	(1, NULL)
	CONFIG_DEVICE_QUICKLOAD	(	"kcc\0",	kc)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(kc85d)
	CONFIG_IMPORT_FROM(kc85)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",	device_load_kc85_floppy)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT	COMPAT	MACHINE  INPUT     INIT  CONFIG  COMPANY   FULLNAME */
COMPX( 1987, kc85_3,   0,		0,		kc85_3,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMPX( 1989, kc85_4,   kc85_3,  0,		kc85_4,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMPX( 1989, kc85_4d,  kc85_3,  0,		kc85_4d, kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/4 + Disk Interface Module (D004)", GAME_NOT_WORKING)
