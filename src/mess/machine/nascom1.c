/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

void nascom1_init_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "nascom1_init\r\n");

}

void nascom1_stop_machine(void)
{

}

