/******************************************************************************

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/apf.h"

unsigned char *apf_video_ram;

static void apf_charproc(UINT8 c)
{
	/* this seems to be the same so far, as it gives the same result as vapf */
	m6847_inv_w(0,		(c & 0x040));
	m6847_as_w(0,		(c & 0x080));	
}



READ8_HANDLER(apf_video_r)
{
	return apf_video_ram[offset];
}

WRITE8_HANDLER(apf_video_w)
{
	apf_video_ram[offset] = data;
	m6847_touch_vram(offset);
	
	schedule_full_refresh();
}

extern unsigned int apf_ints;

static WRITE8_HANDLER(apf_vsync_int)
{
	if (data!=0)
	{
		apf_ints|=(1<<4);
	}
	else
	{
		apf_ints&=~(1<<4);
	}

	apf_update_ints();
}

VIDEO_START( apf )
{
	struct m6847_init_params p;

	/* allocate video memory ram */
	apf_video_ram = (unsigned char *)auto_malloc(0x0400);

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL_NTSC;
	p.ram = apf_video_ram+0x0200;
	p.ramsize = 0x0400;
	p.charproc = apf_charproc;
	p.fs_func = apf_vsync_int;

	if (video_start_m6847(&p))
		return 1;

	return (0);
}
