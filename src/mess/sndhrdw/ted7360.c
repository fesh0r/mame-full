/***************************************************************************

  MOS video interface chip 6560 (and sound interface)

  main part in vidhrdw
***************************************************************************/
#include "math.h"
#include "sound/streams.h"
#include "mess/machine/c16.h"
#include "mess/vidhrdw/ted7360.h"
#include "mame.h"

/*
 assumed model:
 each write to a ton/noise generated starts it new
 2 integrated samples: one for tone (sinus), now for noise (?)
 each generator behaves like an timer
 when it reaches 0, the next samplevalue is given out
 */

/*
 implemented model:
 each write resets generator, amount of samples to give out for one
 period.

 the appropriate value is fetched from the sample
 fast turning off of channel
 changing note, when previous note finished

 i hope streambuffer value are sign integer and the
 DAC behaves like this
 */

#define TONE_ON (!(ted7360[0x11]&0x80)) // or tone update!?
#define TONE1_ON ((ted7360[0x11]&0x10))
#define TONE1_VALUE (ted7360[0xe]|((ted7360[0x12]&3)<<8))
#define TONE2_ON ((ted7360[0x11]&0x20))
#define TONE2_VALUE (ted7360[0xf]|((ted7360[0x10]&3)<<8))
#define VOLUME (ted7360[0x11]&0x0f)
#define NOISE_ON (ted7360[0x11]&0x40)

/*
  pal 111860.781
  ntsc 111840.45
*/
#define TONE_FREQUENCY(reg) ((TED7360_CLOCK>>3)/(1024-reg))

static int channel,
	tonesize, noisesize, // number of samples
	tone1pos=0, tone2pos=0, // pos of tone
	tone1samples=1, tone2samples=1; // count of samples to give out per tone

static INT16 *tone,*noise;

void ted7360_soundport_w(int offset, int data)
{
//	int old=ted7360[offset];
	switch(offset) {
	case 0xe:
		ted7360[offset]=data;
		tone1pos=0;tone1samples=options.samplerate/TONE_FREQUENCY(TONE1_VALUE);
		if (tone1samples==0) tone1samples=1; // plays more than complete tone in 1 outputsample
		break;
	case 0xf:
		ted7360[offset]=data;
		tone2pos=0;tone2samples=options.samplerate/TONE_FREQUENCY(TONE2_VALUE);
		if (tone2samples==0) tone2samples=1;
		break;
	case 0x10:
		ted7360[offset]=data;
		tone2pos=0;tone2samples=options.samplerate/TONE_FREQUENCY(TONE2_VALUE);
		if (tone2samples==0) tone2samples=1;
		break;
	case 0x12:
		ted7360[offset]=(ted7360[offset]&~3)|(data&3);
		tone1pos=0;tone1samples=options.samplerate/TONE_FREQUENCY(TONE1_VALUE);
		if (tone1samples==0) tone1samples=1; // plays more than complete tone in 1 outputsample
		break;
	case 0x11:
		ted7360[offset]=data;
		break;
	}
//	stream_update(channel,0);
}

/************************************/
/* Sound handler update 			*/
/************************************/
void ted7360_update(int param, void *buffer, int length)
{
	int i, ton1=0, ton2=0, v;

	for (i=0; i<length; i++ ) {
		if (TONE1_ON/*||(tone1pos!=0)*/) {
			ton1=tone[tone1pos*tonesize/tone1samples];
			tone1pos++;
#if 0
			tone1pos%=tone1samples;
#else
			if (tone1pos>=tone1samples) {
				tone1pos=0;tone1samples=options.samplerate/TONE_FREQUENCY(TONE1_VALUE);
				if (tone1samples==0) tone1samples=1;
			}
#endif
		} else ton1=0;
		if (TONE2_ON||NOISE_ON/*||(tone2pos!=0)*/) {
			if (TONE2_ON) //higher priority ?!
				ton2=tone[tone2pos*tonesize/tone2samples];
			else
				ton2=noise[tone2pos*tonesize/tone2samples];
			tone2pos++;
#if 0
			tone2pos%=tone2samples;
#else
			if (tone2pos>=tone2samples) {
				tone2pos=0;tone2samples=options.samplerate/TONE_FREQUENCY(TONE2_VALUE);
				if (tone2samples==0) tone2samples=1;
			}
#endif
		} else ton2=0;
		if (1||TONE_ON) {
			v=((ton1+ton2)*VOLUME)>>8;
			if (v>127) ((char*)buffer)[i]=127;
			else if (v<-127) ((char*)buffer)[i]=-127;
			else ((char*)buffer)[i]=v;
		} else ((char*)buffer)[i]=0;
	}
}

/************************************/
/* Sound handler start				*/
/************************************/
int ted7360_custom_start(const struct MachineSound* driver)
{
	int i;

	// slowest played sample
	tonesize=options.samplerate/TONE_FREQUENCY(0);
	if (errorlog) fprintf(errorlog,"rate:%d bits:%d\n",options.samplerate, options.samplebits);
	if (errorlog) fprintf(errorlog,"ted7360 rate:%d bits:%d size:%d\n",options.samplerate, options.samplebits, tonesize);

	channel = stream_init("ted7360", 50, options.samplerate, 8, 0, ted7360_update);

	tone=malloc(tonesize*sizeof(tone[0]));
	if (!tone)
		return 1;

	noisesize=tonesize;
	noise=malloc(noisesize*sizeof(noise[0]));
	if (!noise) { free(tone);return 1; }

	for (i=0; i<tonesize; i++) {
		tone[i]=sin(2*M_PI*i/tonesize)*2048; // +- 2048, sum of 4 channels * volume make +- 30720
	}

	for (i=0; i<noisesize; i++) {
		noise[i]=(rand()*2.0/RAND_MAX-1)*2048;
	}
	return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void ted7360_custom_stop(void)
{
	free(tone);
	free(noise);
}

void ted7360_custom_update(void) {}

