/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/enterp.h"
#include "devices/basicdsk.h"
#include "includes/wd179x.h"
#include "sndhrdw/dave.h"
#include "image.h"

void Enterprise_SetupPalette(void);

MACHINE_INIT( enterprise )
{
	/* initialise the hardware */
	Enterprise_Initialise();
}

int enterprise_floppy_init(int id, mame_file *fp, int open_mode)
{
	if (fp == NULL)
		return INIT_PASS;

	if (basicdsk_floppy_load(id, fp, open_mode)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
