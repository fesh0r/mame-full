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


UINT8 pmd85_ppi_port_outputs[1][3];

static UINT8 startup_mem_map = 0;

static OPBASE_HANDLER (pmd85_opbaseoverride)
{
	if ( startup_mem_map )
	{
		switch ( address & 0x4000 )
		{
			case 0x0000:
				switch ( address & 0x1000 )
				{
					case 0x0000:
						OP_ROM = OP_RAM =  memory_region(REGION_CPU1)+0x010000 - (address & 0xa000);
						return -1;
					case 0x1000:
						logerror ("Illegal opcode address\n");
						return -1;
				}
				break;
			case 0x4000:
				OP_ROM = OP_RAM = mess_ram + 0x8000 - (address & 0xc000);
				return -1;
		}
	}
	else
	{
		switch ( address & 0x8000 )
		{
			case 0x0000:
				OP_ROM = OP_RAM = mess_ram;
			        return -1;
			case 0x8000:
				switch ( address & 0x4000 )
				{
					case 0x0000:
						switch ( address & 0x1000 )
						{
							case 0x0000:
								OP_ROM = OP_RAM = memory_region(REGION_CPU1) + 0x010000 - (address & 0xa000);
								return -1;
							case 0x1000:
								logerror ("Illegal opcode address\n");
								return -1;
						}
						break;
					case 0x4000:
						OP_ROM = OP_RAM = mess_ram + 0x8000 - (address & 0xc000);
						return -1;
				}
				break;
		}
	}
	return address;
}

READ_HANDLER( pmd85_mem_r )
{
	if ( startup_mem_map )
	{
		switch ( offset & 0x4000 )
		{
			case 0x0000:
				switch ( offset & 0x1000 )
				{
					case 0x0000:
						return memory_region(REGION_CPU1)[0x010000 + (offset&0x0fff)];
						break;
					case 0x1000:
						return 0xff;
						break;
				}
				break;
			case 0x4000:
				return mess_ram[0x8000+(offset&0x3fff)];
				break;
		}
	}
	else
	{
		switch ( offset & 0x8000 )
		{
			case 0x0000:
				return mess_ram[offset&0x7fff];
			        break;
			case 0x8000:
				switch ( offset & 0x4000 )
				{
					case 0x0000:
						switch ( offset & 0x1000 )
						{
							case 0x0000:
								return memory_region(REGION_CPU1)[0x010000 + (offset&0x0fff)];
								break;
							case 0x1000:
								return 0xff;
								break;
						}
						break;
					case 0x4000:
						return mess_ram[0x8000+(offset&0x3fff)];
						break;
				}
				break;
		}
	}
	return 0xff;
}

WRITE_HANDLER( pmd85_mem_w )
{
	if ( startup_mem_map )
	{
		switch ( offset & 0x4000 )
		{
			case 0x0000:
				switch ( offset & 0x1000 )
				{
					case 0x0000:
						logerror ("Writing to ROM\n");
						break;
					case 0x1000:
						logerror ("Writing to unampped memory\n");
						break;
				}
				break;
			case 0x4000:
				mess_ram[0x8000+(offset&0x3fff)] = data;
				break;
		}
	}
	else
	{
		switch ( offset & 0x8000 )
		{
			case 0x0000:
				mess_ram[offset&0x7fff] = data;
			        break;
			case 0x8000:
				switch ( offset & 0x4000 )
				{
					case 0x0000:
						switch ( offset & 0x1000 )
						{
							case 0x0000:
								logerror ("Writing to ROM\n");
								break;
							case 0x1000:
								logerror ("Writing to unampped memory\n");
								break;
						}
						break;
					case 0x4000:
						mess_ram[0x8000+(offset&0x3fff)] = data;
						break;
				}
			break;
		}
	}
}

static struct msm8251_interface pmd85_uart_interface=
{
	NULL,
	NULL,
	NULL
};

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

	if ((offset & 0x80) && !(offset & 0x08))	/* 8255 */
		return ppi8255_0_r(offset & 0x03);
	if ((offset & 0xf8) == 0x18)			/* 8251 */
	{
		switch (offset & 0x01)
		{
			case 0x00: return msm8251_data_r(offset & 0x01);
			case 0x01: return msm8251_status_r(offset & 0x01);
		}
	}

	logerror ("Reading from unmapped port: %02x\n", offset);
	return 0xff;
}

WRITE_HANDLER ( pmd85_io_w )
{
	if (startup_mem_map)
		startup_mem_map = 0;

	if ((offset & 0x80) && !(offset & 0x08))	/* 8255 */
		ppi8255_0_w(offset & 0x03, data);

	if ((offset & 0xf8) == 0x18)			/* 8251 */
	{
		switch (offset & 0x01)
		{
			case 0x00: msm8251_data_w(offset & 0x01, data);
			case 0x01: msm8251_control_w(offset & 0x01, data);
		}
	}
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
	msm8251_init(&pmd85_uart_interface);

	memory_set_opbase_handler(0, pmd85_opbaseoverride);

	startup_mem_map = 1;
}

