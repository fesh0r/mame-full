/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/Z80/Z80.h"
#include "mess/machine/enterp.h"

extern unsigned char *Enterprise_RAM;

void Enterprise_SetupPalette(void);

const char *ep128_floppy_name[4] = {NULL,NULL,NULL,NULL};

void enterprise_init_machine(void)
{
	/* allocate memory. */
	/* I am allocating it because I control how the ram is
	 * accessed. Mame will not allocate any memory because all
	 * the memory regions have been defined as MRA_BANK?
	*/
	/* 128k memory, 32k for dummy read/write operations
	 * where memory bank is not defined
	 */
	Enterprise_RAM = malloc((128*1024)+32768);

	/* initialise the hardware */
	Enterprise_Initialise();
}

void enterprise_shutdown_machine(void)
{
	if (Enterprise_RAM != NULL)
		free(Enterprise_RAM);

	Enterprise_RAM = NULL;
}

int enterprise_floppy_init(int id)
{
	ep128_floppy_name[id] = device_filename(IO_FLOPPY,id);

    return 0;
}
