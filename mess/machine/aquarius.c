/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include <stdio.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "mess/includes/aquarius.h"

static	int	aquarius_ramsize = 0;

void aquarius_init_machine(void)
{
	logerror("aquarius_init\r\n");
	if (readinputport(9) != aquarius_ramsize)
	{
		aquarius_ramsize = readinputport(9);
		switch (aquarius_ramsize)
		{
			case 02:
				install_mem_write_handler(0, 0x8000, 0xffff, MWA_RAM);
				install_mem_read_handler(0, 0x8000, 0xffff, MRA_RAM);
				install_mem_write_handler(0, 0x4000, 0x7fff, MWA_RAM);
				install_mem_read_handler(0, 0x4000, 0x7fff, MRA_RAM);
				break;
			case 01:
				install_mem_write_handler(0, 0x8000, 0xffff, MWA_NOP);
				install_mem_read_handler(0, 0x8000, 0xffff, MRA_NOP);
				install_mem_write_handler(0, 0x4000, 0x7fff, MWA_RAM);
				install_mem_read_handler(0, 0x4000, 0x7fff, MRA_RAM);
				break;
			case 00:
				install_mem_write_handler(0, 0x8000, 0xffff, MWA_NOP);
				install_mem_read_handler(0, 0x8000, 0xffff, MRA_NOP);
				install_mem_write_handler(0, 0x4000, 0x7fff, MWA_NOP);
				install_mem_read_handler(0, 0x4000, 0x7fff, MRA_NOP);
				break;
		}
	}
}

void aquarius_stop_machine(void)
{
}

READ_HANDLER ( aquarius_port_ff_r )
{

	int loop, loop2, bc, rpl;

	bc = (cpu_get_reg (Z80_BC) >> 8);
	rpl = 0xff;

	for (loop = 0x80, loop2 = 7; loop; loop >>= 1, loop2--)
	{
		if (!(bc & loop))
		{
			rpl &= readinputport (loop2);
		}
	}

	return (rpl);
}

READ_HANDLER ( aquarius_port_fe_r )
{
	return (0xff);
}

WRITE_HANDLER ( aquarius_port_fc_w )
{
	speaker_level_w(0, data & 0x01);
}

WRITE_HANDLER ( aquarius_port_fe_w )
{
}

WRITE_HANDLER ( aquarius_port_ff_w )
{
}

