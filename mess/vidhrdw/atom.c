/******************************************************************************

atom.c

******************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"

int atom_vh_start(void)
{
	if (m6847_vh_start(M6847_VERSION_ORIGINAL))
		return (1);

	m6847_set_vram(memory_region(REGION_CPU1) + 0x8000, 0xffff);
	return (0);
}

