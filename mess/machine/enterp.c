/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/enterp.h"
#include "includes/basicdsk.h"
#include "includes/wd179x.h"
#include "sndhrdw/dave.h"

extern unsigned char *Enterprise_RAM;

void Enterprise_SetupPalette(void);

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
	if (!Enterprise_RAM) return;

	/* initialise the hardware */
	Enterprise_Initialise();
}

void enterprise_shutdown_machine(void)
{
	wd179x_exit();

	if (Enterprise_RAM != NULL)
		free(Enterprise_RAM);

	Enterprise_RAM = NULL;

	Dave_Finish();
}

int enterprise_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY, id)==NULL)
		return INIT_PASS;

	if (strlen(device_filename(IO_FLOPPY, id))==0)
		return INIT_PASS;

	if (basicdsk_floppy_init(id)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1, 0);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
