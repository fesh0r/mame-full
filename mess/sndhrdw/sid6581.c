/***************************************************************************

  MOS sound interface device sid6581

***************************************************************************/

/* uses Michael Schwendt's sidplay (copyright message in 6581_.cpp)
   problematic and much work to integrate, so better to redo bugfixes
   in the converted code

   now only 1 sid chip allowed!
   needs rework
*/

#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/sid6581.h"
#include "sid.h"

static SID6581 sid6581[MAX_SID6581]= {{0}};

void sid6581_set_type (int number, SIDTYPE type)
{
    sid6581[number].type=type;
    sidInitWaveformTables(type);
}

void sid6581_reset(int number)
{
	sidEmuReset(sid6581+number);
}

READ8_HANDLER ( sid6581_0_port_r )
{
	return sid6581_port_r(sid6581, offset);
}

READ8_HANDLER ( sid6581_1_port_r )
{
	return sid6581_port_r(sid6581+1, offset);
}

WRITE8_HANDLER ( sid6581_0_port_w )
{
	sid6581_port_w(sid6581, offset, data);
}

WRITE8_HANDLER ( sid6581_1_port_w )
{
	sid6581_port_w(sid6581+1, offset, data);
}

void sid6581_update(void)
{
	int i;
	for (i=0; i<MAX_SID6581; i++)
	{
		if (sid6581[i].on)
			stream_update(sid6581[i].mixer_channel,0);
	}
}

static void sid6581_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	sidEmuFillBuffer(sid6581 + (int) param, _buffer[0], length);
}



void *sid6581_custom_start (int clock, const struct CustomSound_interface *config)
{
	int i;
	const SID6581_interface *iface=(const SID6581_interface*)
		config;

	i = (int) config->extra_data;

	sid6581[i].mixer_channel = stream_create (0, 1, options.samplerate, (void *) i, sid6581_sh_update);

	sid6581[i].PCMfreq = options.samplerate;	
	sid6581[i].type = iface->chip.type;
	sid6581[i].clock = iface->chip.clock;
	sid6581[i].ad_read = iface->chip.ad_read;
	sid6581[i].on = 1;
	sid6581_init(sid6581 + i);

	return (void *) ~0;
}

