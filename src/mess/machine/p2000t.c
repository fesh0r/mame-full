/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

void p2000t_init_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "p2000t_init\r\n");

}

void p2000t_stop_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "p2000t_stop\r\n");
}
