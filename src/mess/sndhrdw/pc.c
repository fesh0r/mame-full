/***************************************************************************

    sndhrdw/pc.c

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/
#include "sound/streams.h"
#include "mess/machine/pc.h"

#define BASECLOCK	1193180

extern int PIT_clock[3];

static int channel = 0;
static int baseclock = BASECLOCK/440;
static signed char waveform0[8] = {  0,  0,  0,  0,  0,  0,  0,  0};
static signed char waveform1[8] = {127,127,127,127,127,127,127,127};
static signed char waveform2[8] = { 64, 90,127, 90, 64, 38,  0, 38};
static signed char *waveform = waveform0;

/************************************/
/* Sound handler start				*/
/************************************/
int pc_sh_start(void)
{
	if (errorlog) fprintf(errorlog, "pc_sh_start\n");
	channel = stream_init("PC speaker", 50, baseclock, 8, 0, pc_sh_update);
    return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void pc_sh_stop(void)
{
	if (errorlog) fprintf(errorlog, "pc_sh_stop\n");
}

void pc_sh_speaker(int mode)
{
	static int old_mode = 0;

	if (mode == old_mode)
		return;

    stream_update(channel,0);
    switch (mode) {
		case 0: /* completely off */
			SND_LOG(1,"PC_speaker",(errorlog,"off\n"));
			waveform = waveform0;
            break;
		case 1: /* completely on */
			SND_LOG(1,"PC_speaker",(errorlog,"on\n"));
			waveform = waveform1;
            break;
		case 2: /* play the tone */
			SND_LOG(1,"PC_speaker",(errorlog,"tone\n"));
			waveform = waveform2;
            break;
    }
    old_mode = mode;
}

/************************************/
/* Sound handler update 			*/
/************************************/
void pc_sh_update(int param, void *buffer, int length)
{
	static int cntr = 0, incr = 0;
	signed char *sample = buffer;
	int i;

    baseclock = PIT_clock[2];

    for( i = 0; i < length; i++ )
	{
		sample[i] = waveform[cntr];
		if( incr -= Machine->sample_rate < 0 )
		{
			incr += baseclock;
			cntr = ++cntr % sizeof(waveform0);
		}
	}
}

