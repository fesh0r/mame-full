/******************************************************************************

atom.c

******************************************************************************/

#include "mess/vidhrdw/m6847.h"
#include "vidhrdw/generic.h"

int atom_vh_start(void)
{
	if (m6847_vh_start())
		return (1);

	m6847_set_vram(memory_region(REGION_CPU1) + 0x8000, 0xffff);
	return (0);
}

