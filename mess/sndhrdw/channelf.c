#include "driver.h"
#include "includes/channelf.h"

static int channel;
static int sound_mode;

void channelf_sh_update(int param, INT16 *buffer, int length);

void channelf_sound_w(int mode)
{
	if (mode == sound_mode)
		return;

    stream_update(channel,0);
	sound_mode = mode;
}

int channelf_sh_start(void)
{
	channel = stream_init("Digital out", 50, Machine->sample_rate, 0, channelf_sh_update);
    return 0;
}

int channelf_sh_custom_start(const struct MachineSound* driver)
{
	return channelf_sh_start();
}

void channelf_sh_stop(void)
{
}

void channelf_sh_custom_update(void) {}

void channelf_sh_update(int param, INT16 *buffer, int length)
{
	static INT16 max = 0x7fff;
    static int incr = 0;
	int rate = Machine->sample_rate / 2;
	int period_2V = rate/960;
	int period_4V = rate/480;
	int period_8V = rate/240;
	int period_32V = rate/60;
	int half_2V = period_2V/2;
	int half_4V = period_4V/2;
	int half_8V = period_8V/2;
	int half_32V = period_32V/2;
	INT16 *sample = buffer;

	switch( sound_mode )
	{
	case 0: /* sound off */
		while( length-- > 0 )
			*sample++ = 0;
			incr++;
		break;
	case 1: /* high tone (2V) */
		while( length-- > 0 )
		{
			if ((incr%period_2V) < half_2V)
				*sample++ = 0;
			else
				*sample++ = max;
			incr++;
		}
	case 2: /* medium tone (4V) */
		while( length-- > 0 )
		{
			if ((incr%period_4V) < half_4V)
				*sample++ = 0;
			else
				*sample++ = max;
			incr++;
		}
		break;
	case 3: /* low (wierd) tone (32V & 8V) */
		while( length-- > 0 )
		{
			if (((incr%period_32V) < half_32V) && ((incr%period_8V) < half_8V))
				*sample++ = max;
			else
				*sample++ = 0x0;
			incr++;
		}
		break;
	}
}
