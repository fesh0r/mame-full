/***************************************************************************

  machine.c

  Functions to emulate general aspects of PMD-85 (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "devices/cassette.h"
#include "cpu/i8085/i8085.h"
#include "includes/pmd85.h"
#include "machine/8255ppi.h"
#include "includes/msm8251.h"
#include "includes/pit8253.h"

static UINT8 pmd85_rom_module_support;
static UINT8 pmd85_rom_module_present = 0;

static UINT8 pmd85_ppi_port_outputs[4][3];

static UINT8 pmd85_startup_mem_map = 0;
static void (*pmd85_update_memory) (void);

enum {PMD85_LED_1, PMD85_LED_2, PMD85_LED_3};

static void pmd851_update_memory (void)
{
	if (pmd85_startup_mem_map)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK5);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK10);

		cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(3, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(5, mess_ram + 0xc000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(8, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(10, mess_ram + 0xc000);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, MWA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, MWA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, MWA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK5);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK10);

		cpu_setbank(1, mess_ram);
		cpu_setbank(2, mess_ram + 0x1000);
		cpu_setbank(3, mess_ram + 0x2000);
		cpu_setbank(4, mess_ram + 0x3000);
		cpu_setbank(5, mess_ram + 0x4000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(8, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(10, mess_ram + 0xc000);
	}
}

static void pmd852a_update_memory (void)
{
	if (pmd85_startup_mem_map)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, MWA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, MWA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK5);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, MWA8_BANK7);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, MWA8_BANK9);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK10);

		cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(2, mess_ram + 0x9000);
		cpu_setbank(3, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(4, mess_ram + 0xb000);
		cpu_setbank(5, mess_ram + 0xc000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(7, mess_ram + 0x9000);
		cpu_setbank(8, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(9, mess_ram + 0xb000);
		cpu_setbank(10, mess_ram + 0xc000);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, MWA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, MWA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, MWA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK5);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, MWA8_BANK7);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, MWA8_BANK9);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK10);

		cpu_setbank(1, mess_ram);
		cpu_setbank(2, mess_ram + 0x1000);
		cpu_setbank(3, mess_ram + 0x2000);
		cpu_setbank(4, mess_ram + 0x5000);
		cpu_setbank(5, mess_ram + 0x4000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(7, mess_ram + 0x9000);
		cpu_setbank(8, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(9, mess_ram + 0xb000);
		cpu_setbank(10, mess_ram + 0xc000);
	}
}

static void alfa_update_memory (void)
{
	if (pmd85_startup_mem_map)
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x33ff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3400, 0x3fff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xb3ff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb400, 0xbfff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK8);

		cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(2, memory_region(REGION_CPU1) + 0x011000);
		cpu_setbank(4, mess_ram + 0xc000);
		cpu_setbank(5, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x011000);
		cpu_setbank(8, mess_ram + 0xc000);
	}
	else
	{
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, MWA8_BANK1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x33ff, 0, MWA8_BANK2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3400, 0x3fff, 0, MWA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, MWA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0xb3ff, 0, MWA8_ROM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb400, 0xbfff, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xffff, 0, MWA8_BANK8);

		cpu_setbank(1, mess_ram);
		cpu_setbank(2, mess_ram + 0x1000);
		cpu_setbank(3, mess_ram + 0x3400);
		cpu_setbank(4, mess_ram + 0x4000);
		cpu_setbank(5, memory_region(REGION_CPU1) + 0x010000);
		cpu_setbank(6, memory_region(REGION_CPU1) + 0x011000);
		cpu_setbank(8, mess_ram + 0xc000);
	}
}

/*******************************************************************************

	Motherboard 8255
	----------------
		keyboard, LEDs, speaker

*******************************************************************************/

static READ_HANDLER ( pmd85_ppi_0_porta_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_0_portb_r )
{
	return readinputport(pmd85_ppi_port_outputs[0][0]&0x0f) & readinputport(0x0f);
}

static READ_HANDLER ( pmd85_ppi_0_portc_r )
{
	return 	0xff;
}

static WRITE_HANDLER ( pmd85_ppi_0_porta_w )
{
	pmd85_ppi_port_outputs[0][0] = data;
}

static WRITE_HANDLER ( pmd85_ppi_0_portb_w )
{
	pmd85_ppi_port_outputs[0][1] = data;
}

static WRITE_HANDLER ( pmd85_ppi_0_portc_w )
{
	pmd85_ppi_port_outputs[0][2] = data;
	set_led_status(PMD85_LED_2, (data & 0x08) ? 1 : 0);
	set_led_status(PMD85_LED_3, (data & 0x04) ? 1 : 0);
}

/*******************************************************************************

	I/O board 8255
	--------------
		GPIO/0 (K3 connector), GPIO/1 (K4 connector)

*******************************************************************************/

static READ_HANDLER ( pmd85_ppi_1_porta_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_1_portb_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_1_portc_r )
{
	return 0xff;
}

static WRITE_HANDLER ( pmd85_ppi_1_porta_w )
{
	pmd85_ppi_port_outputs[1][0] = data;
}

static WRITE_HANDLER ( pmd85_ppi_1_portb_w )
{
	pmd85_ppi_port_outputs[1][1] = data;
}

static WRITE_HANDLER ( pmd85_ppi_1_portc_w )
{
	pmd85_ppi_port_outputs[1][2] = data;
}

/*******************************************************************************

	I/O board 8255
	--------------
		IMS-2 (K5 connector)

	- 8251 - cassette recorder and V.24/IFSS (selectable by switch)

	- external interfaces connector (K2)

*******************************************************************************/

static READ_HANDLER ( pmd85_ppi_2_porta_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_2_portb_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_2_portc_r )
{
	return 0xff;
}

static WRITE_HANDLER ( pmd85_ppi_2_porta_w )
{
	pmd85_ppi_port_outputs[2][0] = data;
}

static WRITE_HANDLER ( pmd85_ppi_2_portb_w )
{
	pmd85_ppi_port_outputs[2][1] = data;
}

static WRITE_HANDLER ( pmd85_ppi_2_portc_w )
{
	pmd85_ppi_port_outputs[2][2] = data;
}

/*******************************************************************************

	I/O board 8251
	--------------
		cassette recorder and V.24/IFSS (selectable by switch)

*******************************************************************************/

static struct msm8251_interface pmd85_msm8251_interface =
{
	NULL,
	NULL,
	NULL
};

/*******************************************************************************

	I/O board 8253
	--------------

*******************************************************************************/

static struct pit8253_config pmd85_pit8253_interface =
{
	TYPE8253,
	{
		{ 0,		NULL,	NULL },
		{ 2000000,	NULL,	NULL },
		{ 1,		NULL,	NULL }
	}
};


/*******************************************************************************

	I/O board external interfaces connector (K2)
	--------------------------------------------

*******************************************************************************/



/*******************************************************************************

	ROM Module 8255
	---------------
		port A - data read
		ports B, C - address select

*******************************************************************************/

static READ_HANDLER ( pmd85_ppi_3_porta_r )
{
	return memory_region(REGION_USER1)[pmd85_ppi_port_outputs[3][1]|(pmd85_ppi_port_outputs[3][2]<<8)];
}

static READ_HANDLER ( pmd85_ppi_3_portb_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_3_portc_r )
{
	return 0xff;
}

static WRITE_HANDLER ( pmd85_ppi_3_porta_w )
{
	pmd85_ppi_port_outputs[3][0] = data;
}

static WRITE_HANDLER ( pmd85_ppi_3_portb_w )
{
	pmd85_ppi_port_outputs[3][1] = data;
}

static WRITE_HANDLER ( pmd85_ppi_3_portc_w )
{
	pmd85_ppi_port_outputs[3][2] = data;
}

/*******************************************************************************

	I/O ports
	---------

	I/O board
	1xxx11aa	external interfaces connector (K2)
					
	0xxx11aa	I/O board interfaces
		000111aa	8251 (casette recorder, V24)
		010011aa	8255 (GPIO/0, GPIO/1)
		010111aa	8253
		011111aa	8255 (IMS-2)

	Motherboard
	1xxx01aa	8255 (keyboard, speaker. LEDS)

	ROM Module
	1xxx10aa	8255 (ROM reading)

*******************************************************************************/

READ_HANDLER ( pmd85_io_r )
{
	if (pmd85_startup_mem_map)
	{
		return 0xff;
	}

	switch (offset & 0x0c)
	{
		case 0x04:	/* Motherboard */
				switch (offset & 0x80)
				{
					case 0x80:	/* Motherboard 8255 */
							return ppi8255_0_r(offset & 0x03);
				}
				break;
		case 0x08:	/* ROM module connector */
				if (pmd85_rom_module_present)
				{
					switch (offset & 0x80)
					{
						case 0x80:	/* ROM module 8255 */
								return ppi8255_3_r(offset & 0x03);
					}
				}
				break;
		case 0x0c:	/* I/O board */
				switch (offset & 0x80)
				{
					case 0x00:	/* I/O board interfaces */
							switch (offset & 0x70)
							{
								case 0x10:	/* 8251 (casette recorder, V24) */
										switch (offset & 0x01)
										{
											case 0x00: return msm8251_data_r(offset & 0x01);
											case 0x01: return msm8251_status_r(offset & 0x01);
										}
										break;
								case 0x40:      /* 8255 (GPIO/0, GPIO/0) */
										return ppi8255_1_r(offset & 0x03);
								case 0x50:	/* 8253 */
										return pit8253_0_r (offset & 0x03);
								case 0x70:	/* 8255 (IMS-2) */
										return ppi8255_2_r(offset & 0x03);
							}
							break;
					case 0x80:	/* external interfaces */
							break;
				}
				break;
	}

	logerror ("Reading from unmapped port: %02x\n", offset);
	return 0xff;
}

WRITE_HANDLER ( pmd85_io_w )
{
	if (pmd85_startup_mem_map)
	{
		pmd85_startup_mem_map = 0;
		pmd85_update_memory();
	}

	switch (offset & 0x0c)
	{
		case 0x04:	/* Motherboard */
				switch (offset & 0x80)
				{
					case 0x80:	/* Motherboard 8255 */
							ppi8255_0_w(offset & 0x03, data);
							break;
				}
				break;
		case 0x08:	/* ROM module connector */
				if (pmd85_rom_module_present)
				{
					switch (offset & 0x80)
					{
						case 0x80:	/* ROM module 8255 */
								ppi8255_3_w(offset & 0x03, data);
								break;
					}
				}
				break;
		case 0x0c:	/* I/O board */
				switch (offset & 0x80)
				{
					case 0x00:	/* I/O board interfaces */
							switch (offset & 0x70)
							{
								case 0x10:	/* 8251 (casette recorder, V24) */
										switch (offset & 0x01)
										{
											case 0x00: msm8251_data_w(offset & 0x01, data);
											case 0x01: msm8251_control_w(offset & 0x01, data);
										}
										break;
								case 0x40:      /* 8255 (GPIO/0, GPIO/0) */
										ppi8255_1_w(offset & 0x03, data);
										break;
								case 0x50:	/* 8253 */
										// pit8253_0_w (offset & 0x03, data);
										break;
								case 0x70:	/* 8255 (IMS-2) */
										ppi8255_2_w(offset & 0x03, data);
										break;
							}
							break;
					case 0x80:	/* external interfaces */
							break;
				}
				break;
	}
}

static ppi8255_interface pmd85_ppi8255_interface =
{
	4,
	{pmd85_ppi_0_porta_r, pmd85_ppi_1_porta_r, pmd85_ppi_2_porta_r, pmd85_ppi_3_porta_r},
	{pmd85_ppi_0_portb_r, pmd85_ppi_1_portb_r, pmd85_ppi_2_portb_r, pmd85_ppi_3_portb_r},
	{pmd85_ppi_0_portc_r, pmd85_ppi_1_portc_r, pmd85_ppi_2_portc_r, pmd85_ppi_3_portc_r},
	{pmd85_ppi_0_porta_w, pmd85_ppi_1_porta_w, pmd85_ppi_2_porta_w, pmd85_ppi_3_porta_w},
	{pmd85_ppi_0_portb_w, pmd85_ppi_1_portb_w, pmd85_ppi_2_portb_w, pmd85_ppi_3_portb_w},
	{pmd85_ppi_0_portc_w, pmd85_ppi_1_portc_w, pmd85_ppi_2_portc_w, pmd85_ppi_3_portc_w}
};

DRIVER_INIT ( pmd851 )
{
	pmd85_update_memory = pmd851_update_memory;
	pmd85_rom_module_support = 1;
}

DRIVER_INIT ( pmd852a )
{
	pmd85_update_memory = pmd852a_update_memory;
	pmd85_rom_module_support = 1;
}

DRIVER_INIT ( alfa )
{
	pmd85_update_memory = alfa_update_memory;
	pmd85_rom_module_support = 0;

	pit8253_init(1);
	pit8253_config(0, &pmd85_pit8253_interface);
}

MACHINE_INIT( pmd85 )
{
	ppi8255_init(&pmd85_ppi8255_interface);
	msm8251_init(&pmd85_msm8251_interface);

	if (pmd85_rom_module_support)
		pmd85_rom_module_present = (readinputport(0x11)&0x01) ? 1 : 0;

	pmd85_startup_mem_map = 1;
	pmd85_update_memory();
}
