/***************************************************************************

  MOS ted 7360 (and sound interface)

  main part in vidhrdw
***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#define VERBOSE_DBG 0
#include "mess/includes/cbm.h"
#include "mess/includes/c16.h"
#include "mess/includes/ted7360.h"

/*
 * assumed model:
 * each write to a ton/noise generated starts it new
 * 1 integrated samples: one for tone (sinus)
 * each generator behaves like an timer
 * when it reaches 0, the next samplevalue is given out
 */

/*
 * implemented model:
 * each write resets generator, amount of samples to give out for one
 * period.
 * 
 * the appropriate value is fetched from the sample
 * fast turning off of channel
 * changing note, when previous note finished
 * 
 * i hope streambuffer value are sign integer and the
 * DAC behaves like this
 */
/* noise channel: look into vic6560.c */
#define NOISE_BUFFER_SIZE_SEC 5

#define TONE_ON (!(ted7360[0x11]&0x80))		/* or tone update!? */
#define TONE1_ON ((ted7360[0x11]&0x10))
#define TONE1_VALUE (ted7360[0xe]|((ted7360[0x12]&3)<<8))
#define TONE2_ON ((ted7360[0x11]&0x20))
#define TONE2_VALUE (ted7360[0xf]|((ted7360[0x10]&3)<<8))
#define VOLUME (ted7360[0x11]&0x0f)
#define NOISE_ON (ted7360[0x11]&0x40)

/*
 * pal 111860.781
 * ntsc 111840.45
 */
#define TONE_FREQUENCY(reg) ((TED7360_CLOCK>>3)/(1024-reg))
#define TONE_FREQUENCY_MIN (TONE_FREQUENCY(0))
#define NOISE_FREQUENCY (TED7360_CLOCK/8/(1024-TONE2_VALUE))
#define NOISE_FREQUENCY_MAX (TED7360_CLOCK/8)

static int channel, tonesize, noisesize,	/* number of samples */
	tone1pos = 0, tone2pos = 0,		   /* pos of tone */
	tone1samples = 1, tone2samples = 1,   /* count of samples to give out per tone */
	noisepos = 0, noisesamples = 1;

static INT8 *tone;
static INT8 *noise;

void ted7360_soundport_w (int offset, int data)
{
	/*    int old=ted7360[offset]; */
	DBG_LOG (1, "sound", (errorlog, "write %.2x %.2x\n", offset, data));
	switch (offset)
	{
	case 0xe:
	case 0x12:
		if (offset == 0x12)
			ted7360[offset] = (ted7360[offset] & ~3) | (data & 3);
		else
			ted7360[offset] = data;
		tone1pos = 0;
		tone1samples = options.samplerate / TONE_FREQUENCY (TONE1_VALUE);
		if (tone1samples == 0)
			tone1samples = 1;		   /* plays more than complete tone in 1 outputsample */
		break;
	case 0xf:
	case 0x10:
		ted7360[offset] = data;
		tone2pos = 0;
		tone2samples = options.samplerate / TONE_FREQUENCY (TONE2_VALUE);
		if (tone2samples == 0)
			tone2samples = 1;
		noisesamples = (int) ((double) NOISE_FREQUENCY_MAX * options.samplerate
							  * NOISE_BUFFER_SIZE_SEC / NOISE_FREQUENCY);
		DBG_LOG (1, "vic6560", (errorlog, "noise %.2x %d sample:%d\n",
								data, NOISE_FREQUENCY, noisesamples));
		if (!NOISE_ON || ((double) noisepos / noisesamples >= 1.0))
		{
			noisepos = 0;
		}
		break;
	case 0x11:
		ted7360[offset] = data;
		if (!NOISE_ON)
			noisepos = 0;
		break;
	}
	/*    stream_update(channel,0); */
}

/************************************/
/* Sound handler update             */
/************************************/
void ted7360_update (int param, INT16 *buffer, int length)
{
	int i, v;

	for (i = 0; i < length; i++)
	{
		v = 0;
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
				tone1samples = options.samplerate / TONE_FREQUENCY (TONE1_VALUE);
				if (tone1samples == 0)
					tone1samples = 1;
			}
#endif
		}
		if (TONE2_ON || NOISE_ON /*||(tone2pos!=0) */ )
		{
			if (TONE2_ON)
			{						   /*higher priority ?! */
				v += tone[tone2pos * tonesize / tone2samples];
				tone2pos++;
#if 0
				tone2pos %= tone2samples;
#else
				if (tone2pos >= tone2samples)
				{
					tone2pos = 0;
					tone2samples = options.samplerate / TONE_FREQUENCY (TONE2_VALUE);
					if (tone2samples == 0)
						tone2samples = 1;
				}
#endif
			}
			else
			{
				v += noise[(int) ((double) noisepos * noisesize / noisesamples)];
				noisepos++;
				if ((double) noisepos / noisesamples >= 1.0)
				{
					noisepos = 0;
				}
			}
		}




		if (TONE_ON)
		{
			v = (v * VOLUME) << 2;
			if (v > 32767)
				buffer[i] = 32767;
			else if (v < -32767)
				buffer[i] = -32767;
			else
				buffer[i] = v;
		}
		else
			((char *) buffer)[i] = 0;
	}
}

/************************************/
/* Sound handler start              */
/************************************/
int ted7360_custom_start (const struct MachineSound *driver)
{
	int i;

	/* slowest played sample */
	tonesize = options.samplerate / TONE_FREQUENCY_MIN;

	channel = stream_init ("ted7360", 50, options.samplerate, 0, ted7360_update);

	tone = malloc (tonesize * sizeof (tone[0]));
	if (!tone)
		return 1;

	/* buffer for fastest played sample for 5 second
	 * so we have enough data for min 5 second */
	noisesize = NOISE_FREQUENCY_MAX * NOISE_BUFFER_SIZE_SEC;
	noise = malloc (noisesize * sizeof (noise[0]));
	if (!noise)
	{
		free (tone);
		return 1;
	}

	for (i = 0; i < tonesize; i++)
	{
		tone[i] = (sin (2 * M_PI * i / tonesize) * 127 + 0.5);
	}

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
	return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void ted7360_custom_stop (void)
{
	free (tone);
	free (noise);
}

void ted7360_custom_update (void)
{
}
