/******************************************************************************

atom.c

******************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"

static void atom_charproc(UINT8 c)
{
	m6847_inv_w(0,		(c & 0x80));
	m6847_as_w(0,		(c & 0x40));
	m6847_intext_w(0,	(c & 0x40));
}

int atom_vh_start(void)
{
	struct m6847_init_params p;

	p.version = M6847_VERSION_ORIGINAL;
	p.artifactdipswitch = -1;
	p.ram = memory_region(REGION_CPU1) + 0x8000;
	p.ramsize = 0x10000;
	p.charproc = atom_charproc;

	if (m6847_vh_start(&p))
		return 1;

	return (0);
}

