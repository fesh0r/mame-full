/***************************************************************************

  MOS video interface chip 6560 (and sound interface)

  main part in vidhrdw
***************************************************************************/
#include <math.h>
#include "driver.h"

#include "sound/streams.h"
#include "mame.h"

#define VERBOSE_DBG 0
#include "mess/includes/cbm.h"
#include "mess/includes/vic6560.h"

/*#define NEW_MODELL */
/*
 * assumed model:
 * each write to a ton/noise generated starts it new
 * each generator behaves like an timer
 * when it reaches 0, the next samplevalue is given out
 */

/*
 * noise channel
 * based on a document by diku0748@diku.dk (Asger Alstrup Nielsen)
 * 
 * 23 bit shift register
 * initial value (0x7ffff8)
 * after shift bit 0 is set to bit 22 xor bit 17
 * dac sample bit22 bit20 bit16 bit13 bit11 bit7 bit4 bit2(lsb)
 * 
 * emulation:
 * allocate buffer for 5 sec sampledata (fastest played frequency)
 * and fill this buffer in init with the required sample
 * fast turning off channel, immediate change of frequency
 */

#define NOISE_BUFFER_SIZE_SEC 5

#define TONE1_ON (vic6560[0xa]&0x80)
#define TONE2_ON (vic6560[0xb]&0x80)
#define TONE3_ON (vic6560[0xc]&0x80)
#define NOISE_ON (vic6560[0xd]&0x80)
#define VOLUME (vic6560[0xe]&0x0f)

#define TONE_FREQUENCY_MIN  (VIC656X_CLOCK/256/128)
#define TONE1_VALUE (8*(128-((vic6560[0xa]+1)&0x7f)))
#define TONE1_FREQUENCY (VIC656X_CLOCK/32/TONE1_VALUE)
#define TONE2_VALUE (4*(128-((vic6560[0xb]+1)&0x7f)))
#define TONE2_FREQUENCY (VIC656X_CLOCK/32/TONE2_VALUE)
#define TONE3_VALUE (2*(128-((vic6560[0xc]+1)&0x7f)))
#define TONE3_FREQUENCY (VIC656X_CLOCK/32/TONE3_VALUE)
#define NOISE_VALUE (32*(128-((vic6560[0xd]+1)&0x7f)))
#define NOISE_FREQUENCY (VIC656X_CLOCK/NOISE_VALUE)
#define NOISE_FREQUENCY_MAX (VIC656X_CLOCK/32/1)

static int channel, tone1pos = 0, tone2pos = 0, tone3pos = 0,
#ifdef NEW_MODELL
	ticks_per_sample,
	tone1count = 0, tone2count = 0, tone3count = 0, 
	tone1latch = 1, tone2latch = 1, tone3latch = 1,
#else
	tonesize, tone1samples = 1, tone2samples = 1, tone3samples = 1,
#endif
	noisesize,		  /* number of samples */
	noisepos = 0,         /* pos of tone */
	noisesamples = 1;	  /* count of samples to give out per tone */

#ifdef NEW_MODELL
static INT8 tone[32];

#else
static INT16 *tone;

#endif
static INT8 *noise;

void vic6560_soundport_w (int offset, int data)
{
    int old = vic6560[offset];

	switch (offset)
	{
#ifdef NEW_MODELL
	case 0xa:
		vic6560[offset] = data;
		tone1latch = TONE1_VALUE * ticks_per_sample;
		if ((data ^ old) & 0x80)
		{
			if (TONE1_ON)
			{
				tone1count = tone1latch;
				tone1pos = 0;
				tone1count = 0;
			}
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone1 %.2x f:%d c:%d\n",
								data, TONE1_FREQUENCY, TONE1_VALUE));
		break;
	case 0xb:
		vic6560[offset] = data;
		tone2latch = TONE2_VALUE * ticks_per_sample;
		if ((data ^ old) & 0x80)
		{
			if (TONE2_ON)
			{
				tone2count = tone2latch;
				tone2pos = 0;
				tone2count = 0;
			}
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone2 %.2x f:%d c:%d\n",
								data, TONE2_FREQUENCY, TONE2_VALUE));
		break;
	case 0xc:
		vic6560[offset] = data;
		tone3latch = TONE3_VALUE * ticks_per_sample;
		if ((data ^ old) & 0x80)
		{
			if (TONE3_ON)
			{
				tone3count = tone3latch;
				tone3pos = 0;
				tone3count = 0;
			}
		}
		SND_LOG (1, "vic6560", (errorlog, "tone3 %.2x f:%d c:%d\n",
								data, TONE3_FREQUENCY, TONE3_VALUE));
		break;
#else
	case 0xa:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE1_ON)
		{
			tone1pos = 0;
			tone1samples = options.samplerate / TONE1_FREQUENCY;
			if (tone1samples == 0)
				tone1samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone1 %.2x %d\n", data, TONE1_FREQUENCY));
		break;
	case 0xb:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE2_ON)
		{
			tone2pos = 0;
			tone2samples = options.samplerate / TONE2_FREQUENCY;
			if (tone2samples == 0)
				tone2samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone2 %.2x %d\n", data, TONE2_FREQUENCY));
		break;
	case 0xc:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE3_ON)
		{
			tone3pos = 0;
			tone3samples = options.samplerate / TONE3_FREQUENCY;
			if (tone2samples == 0)
				tone2samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone3 %.2x %d\n", data, TONE3_FREQUENCY));
		break;
#endif
	case 0xd:
		vic6560[offset] = data;
		if (NOISE_ON)
		{
			noisesamples = (int) ((double) NOISE_FREQUENCY_MAX * options.samplerate
								  * NOISE_BUFFER_SIZE_SEC / NOISE_FREQUENCY);
			DBG_LOG (1, "vic6560", (errorlog, "noise %.2x %d sample:%d\n",
									data, NOISE_FREQUENCY, noisesamples));
			if ((double) noisepos / noisesamples >= 1.0)
			{
				noisepos = 0;
			}
		}
		else
		{
			noisepos = 0;
		}
		break;
	case 0xe:
		vic6560[offset] = (old & ~0xf) | (data & 0xf);
		DBG_LOG (3, "vic6560", (errorlog, "volume %d\n", data & 0xf));
		break;
	}
/*  stream_update(channel,1.0/options.samplerate); */
}

/************************************/
/* Sound handler update             */
/************************************/
static void vic6560_update (int param, INT16 *buffer, int length)
{
	int i, v;


	for (i = 0; i < length; i++)
	{
		v = 0;
#ifdef NEW_MODELL
		if (TONE1_ON)
		{
			tone1count -= ticks_per_sample;
			while (tone1count <= 0)
			{
				tone1count += tone1latch;
				tone1pos = (tone1pos + 1) & 0x1f;
			}
			v += tone[tone1pos];
		}
		if (TONE2_ON)
		{
			tone2count -= ticks_per_sample;
			while (tone2count <= 0)
			{
				tone2count += tone2latch;
				tone2pos = (tone2pos + 1) & 0x1f;
			}
			v += tone[tone2pos];
		}
		if (TONE3_ON)
		{
			tone3count -= ticks_per_sample;
			while (tone3count <= 0)
			{
				tone3count += tone3latch;
				tone3pos = (tone3pos + 1) & 0x1f;
			}
			v += tone[tone3pos];
		}
#else
		if (TONE1_ON /*||(tone1pos!=0) */ )
		{
			v += tone[tone1pos * tonesize / tone1samples];
			tone1pos++;
#if 0
			tone1pos %= tone1samples;
#else
			if (tone1pos >= tone1samples)
			{
				tone1pos = 0;
				tone1samples = options.samplerate / TONE1_FREQUENCY;
				if (tone1samples == 0)
					tone1samples = 1;
			}
#endif
		}
		if (TONE2_ON /*||(tone2pos!=0) */ )
		{
			v += tone[tone2pos * tonesize / tone2samples];
			tone2pos++;
#if 0
			tone2pos %= tone2samples;
#else
			if (tone2pos >= tone2samples)
			{
				tone2pos = 0;
				tone2samples = options.samplerate / TONE2_FREQUENCY;
				if (tone2samples == 0)
					tone2samples = 1;
			}
#endif
		}
		if (TONE3_ON /*||(tone3pos!=0) */ )
		{
			v += tone[tone3pos * tonesize / tone3samples];
			tone3pos++;
#if 0
			tone3pos %= tone3samples;
#else
			if (tone3pos >= tone3samples)
			{
				tone3pos = 0;
				tone3samples = options.samplerate / TONE3_FREQUENCY;
				if (tone3samples == 0)
					tone3samples = 1;
			}
#endif
		}
#endif
		if (NOISE_ON)
		{
			v += noise[(int) ((double) noisepos * noisesize / noisesamples)];
			noisepos++;
			if ((double) noisepos / noisesamples >= 1.0)
			{
				noisepos = 0;
			}
		}
		v = (v * VOLUME) << 2;
		if (v > 32767)
			buffer[i] = 32767;
		else if (v < -32767)
			buffer[i] = -32767;
		else
			buffer[i] = v;



	}
}

/************************************/
/* Sound handler start          */
/************************************/
int vic6560_custom_start (const struct MachineSound *driver)
{
	int i;

#ifdef NEW_MODELL
	/* problems with sound disabled samplerate=0 */
	ticks_per_sample = VIC656X_CLOCK / options.samplerate;
#endif

	channel = stream_init ("VIC6560", 50, options.samplerate, 0, vic6560_update);


	/* buffer for fastest played sample for 5 second
	 * so we have enough data for min 5 second */
	noisesize = NOISE_FREQUENCY_MAX * NOISE_BUFFER_SIZE_SEC;
	noise = malloc (noisesize * sizeof (noise[0]));
	if (!noise)
		return 1;
	{
		int noiseshift = 0x7ffff8;
		char data;

		for (i = 0; i < noisesize; i++)
		{
			data = 0;
			if (noiseshift & 0x400000)
				data |= 0x80;
			if (noiseshift & 0x100000)
				data |= 0x40;
			if (noiseshift & 0x010000)
				data |= 0x20;
			if (noiseshift & 0x002000)
				data |= 0x10;
			if (noiseshift & 0x000800)
				data |= 0x08;
			if (noiseshift & 0x000080)
				data |= 0x04;
			if (noiseshift & 0x000010)
				data |= 0x02;
			if (noiseshift & 0x000004)
				data |= 0x01;
			noise[i] = data;
			if (((noiseshift & 0x400000) == 0) != ((noiseshift & 0x002000) == 0))
				noiseshift = (noiseshift << 1) | 1;
			else
				noiseshift <<= 1;
		}
	}
#ifdef NEW_MODELL
	for (i = 0; i < 32 / 2; i++)
	{
		tone[i] = (int) (sin (2 * M_PI * i / 32) * 127 + 0.5);
		tone[16 + i] = -tone[i];
	}
#else
	tonesize = options.samplerate / TONE_FREQUENCY_MIN;

	tone = malloc (tonesize * sizeof (tone[0]));
	if (!tone)
	{
		free (noise);
		return 1;
	}
	for (i = 0; i < tonesize; i++)
	{
		tone[i] = (sin (2 * M_PI * i / tonesize) * 127 + 0.5);
	}
#endif
	return 0;
}

/************************************/
/* Sound handler stop           */
/************************************/
void vic6560_custom_stop (void)
{
#ifndef NEW_MODELL
	free (tone);
#endif
	free (noise);
}

void vic6560_custom_update (void)
{
}
