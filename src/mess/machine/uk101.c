/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"

void uk101_init_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "uk101_init\r\n");
}

void uk101_stop_machine(void)
{

}

/* || */

int uk101_rom_load(int id)
{
	return (0);
}

int uk101_rom_id(int id)
{
	return (1);
}
