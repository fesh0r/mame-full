#include "driver.h"
#include "includes/channelf.h"

static int channel;
static int sound_mode;
static int hrdwlim;

void channelf_sh_update(int param, INT16 *buffer, int length);

static int rate;
static int period_2V;
static int period_4V;
static int period_8V;
static int period_32V;
static int half_2V;
static int half_4V;
static int half_8V;
static int half_32V;

void channelf_sound_w(int mode)
{
	if (mode == sound_mode)
		return;

	sound_mode = mode;
    stream_update(channel,0);
}

static int channelf_sh_start(void)
{
	channel = stream_init("Digital out", 50, Machine->sample_rate, 0, channelf_sh_update);
	rate = Machine->sample_rate;

	/*
	 * 2V = 1000Hz ~= 3579535/224/16
	 * Note 2V on the schematic is not the 2V scanline counter -
	 *      it is the 2V vertical pixel counter
	 *      1 pixel = 4 scanlines high
	 */

	period_2V = rate/1000;
	period_4V = rate/500;
	period_8V = rate/250;
	period_32V = rate/62.5;
	half_2V = period_2V/2;
	half_4V = period_4V/2;
	half_8V = period_8V/2;
	half_32V = period_32V/2;
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

	INT16 *sample = buffer;

	switch( sound_mode )
	{
	case 0: /* sound off */
		hrdwlim=period_32V;  /* SKR - hardware only produces short tones */
		while( length-- > 0 )
			*sample++ = 0;
			incr++;
		break;
	case 1: /* high tone (2V) - 1000Hz */
		while( length-- > 0 )
		{
			if (((incr%period_2V) < half_2V) ||(hrdwlim<=0))
			{
				*sample++ = 0;
			}
			else
			{
				*sample++ = max;
				hrdwlim--;
			}
			incr++;
		}
	case 2: /* medium tone (4V) - 500Hz */
		while( length-- > 0 )
		{
			if (((incr%period_4V) < half_4V) || (hrdwlim<=0))
			{
				*sample++ = 0;
			}
			else
			{
				*sample++ = max;
				hrdwlim--;
			}
			incr++;
		}
		break;
	case 3: /* low (wierd) tone (32V & 8V) */
		while( length-- > 0 )
		{
			if ((((incr%period_32V) >= half_32V) && ((incr%period_8V) >= half_8V)) && (hrdwlim>0))
			{
				*sample++ = max;
				hrdwlim--;
			}
			else
			{
				*sample++ = 0x0;
			}
			incr++;
		}
		break;
	}
}

