/******************************************************************************

******************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/apf.h"

unsigned char *apf_video_ram;

/* TO BE CHECKED! THIS IS COPIED DIRECT FROM ATOM JUST TO TEST DRIVER */
static void apf_charproc(UINT8 c)
{
	m6847_inv_w(0,		(c & 0x80));
	m6847_as_w(0,		(c & 0x40));
	m6847_intext_w(0,	(c & 0x40));
}

int apf_vh_start(void)
{
	struct m6847_init_params p;

	/* allocate video memory ram */
	apf_video_ram = (unsigned char *)malloc(0x0400);

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL;
	p.ram = apf_video_ram;
	p.ramsize = 0x0400;
	p.charproc = apf_charproc;

	if (m6847_vh_start(&p))
		return 1;

	return (0);
}

void apf_vh_stop(void)
{
	/* free video memory ram */
	if (apf_video_ram!=NULL)
	{
		free(apf_video_ram);
		apf_video_ram = NULL;
	}

	m6847_vh_stop();
}

