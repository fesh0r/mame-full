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

void Enterprise_SetupPalette(void);

MACHINE_INIT( enterprise )
{
	/* initialise the hardware */
	Enterprise_Initialise();
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
