/***************************************************************************

  machine.c

  Functions to emulate general aspects of PMD-85 (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cassette.h"
#include "cpu/i8085/i8085.h"
#include "includes/pmd85.h"
#include "machine/8255ppi.h"
#include "includes/msm8251.h"


UINT8 pmd85_ppi_port_outputs[1][3];

static UINT8 startup_mem_map = 0;

static READ_HANDLER ( pmd85_ppi_0_porta_r )
{
	return 0xff;
}

static READ_HANDLER ( pmd85_ppi_0_portb_r )
{
	return readinputport(pmd85_ppi_port_outputs[0][0]&0x0f);
}

static READ_HANDLER ( pmd85_ppi_0_portc_r )
{
	logerror ("Port C (SPKLED) read\n");
	return 	pmd85_ppi_port_outputs[0][2];
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
	logerror ("Port C (SPKLED) write, data: %02x\n", data);
	pmd85_ppi_port_outputs[0][2] = data;
}


/* I/O */
READ_HANDLER ( pmd85_io_r )
{
	if (startup_mem_map)
	{
		return 0xff;
	}
	else
	{
		if ((offset & 0x80) && !(offset & 0x08))
			return ppi8255_0_r(offset & 0x03);
		logerror ("Reading from unmapped port: %02x\n", offset);
		return 0xff;
	}
}

WRITE_HANDLER ( pmd85_io_w )
{
	if (startup_mem_map)
	{
		startup_mem_map = 0;

		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(1, 0, MWA_BANK1);

		cpu_setbank(1, mess_ram);
	}

	if ((offset & 0x80) && !(offset & 0x08))
		ppi8255_0_w(offset & 0x03, data);
	else
		logerror ("Writing to unmapped port: %02x, value: %02x\n", offset, data);
}


static ppi8255_interface pmd85_ppi8255_interface =
{
	1,
	{pmd85_ppi_0_porta_r},
	{pmd85_ppi_0_portb_r},
	{pmd85_ppi_0_portc_r},
	{pmd85_ppi_0_porta_w},
	{pmd85_ppi_0_portb_w},
	{pmd85_ppi_0_portc_w}
};

MACHINE_INIT( pmd85 )
{
	ppi8255_init(&pmd85_ppi8255_interface);

	startup_mem_map = 1;

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);
	memory_set_bankhandler_r(5, 0, MRA_BANK5);

	memory_set_bankhandler_w(1, 0, MWA_ROM);
	memory_set_bankhandler_w(2, 0, MWA_BANK2);
	memory_set_bankhandler_w(3, 0, MWA_ROM);
	memory_set_bankhandler_w(4, 0, MWA_ROM);
	memory_set_bankhandler_w(5, 0, MWA_BANK5);
	
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
	cpu_setbank(2, mess_ram + 0x1000);
	cpu_setbank(3, memory_region(REGION_CPU1) + 0x010000);
	cpu_setbank(4, memory_region(REGION_CPU1) + 0x010000);
	cpu_setbank(5, mess_ram + 0xc000);
}

