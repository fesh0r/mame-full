/***************************************************************************

    sndhrdw/pc.c

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/
#include "osd_cpu.h"
#include "sound/streams.h"
#include "includes/pcshare.h"

#include "includes/pit8253.h"

#define VERBOSE_SND 0		/* SND (sound / speaker) */
#if VERBOSE_SND
#define SND_LOG(n,m,a) \
	if(VERBOSE_SND>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define SND_LOG(n,m,a)
#endif


struct CustomSound_interface pc_sound_interface =
{
	pc_sh_custom_start,
	NULL,
	pc_sh_custom_update
};

#define BASECLOCK	1193180

static int channel = 0;
static int speaker_gate = 0;

/************************************/
/* Sound handler start				*/
/************************************/
static int pc_sh_start(void)
{
	logerror("pc_sh_start\n");
	channel = stream_init("PC speaker", 50, Machine->sample_rate, 0, pc_sh_update);
    return 0;
}

int pc_sh_custom_start(const struct MachineSound* driver)
{
	return pc_sh_start();
}

void pc_sh_speaker(int data)
{
	int mode = 0;
	switch( data )
	{
		case 0: mode=0; break;
		case 1: case 2: mode=1; break;
		case 3: mode=2; break;
	}

	if( mode == speaker_gate )
		return;

    stream_update(channel,0);

    switch( mode )
	{
		case 0: /* completely off */
			SND_LOG(1,"PC_speaker",("off\n"));
			speaker_gate = 0;
            break;
		case 1: /* completely on */
			SND_LOG(1,"PC_speaker",("on\n"));
			speaker_gate = 1;
            break;
		case 2: /* play the tone */
			SND_LOG(1,"PC_speaker",("tone\n"));
			speaker_gate = 2;
            break;
    }
}

void pc_sh_speaker_change_clock(double pc_clock)
{
    stream_update(channel,0);
}

void pc_sh_custom_update(void) {}

/************************************/
/* Sound handler update 			*/
/************************************/
void pc_sh_update(int param, INT16 *buffer, int length)
{
	static INT16 signal = 0x7fff;
    static int incr = 0;
	INT16 *sample = buffer;
	int baseclock, rate = Machine->sample_rate / 2;

	baseclock = pit8253_get_frequency(0, 2);

	switch( speaker_gate ) {
	case 0:
		/* speaker off */
		while( length-- > 0 )
			*sample++ = 0;
		break;

	case 1:
		/* speaker on */
		while( length-- > 0 )
			*sample++ = 0x7fff;
        break;

	case 2:
		/* speaker gate tone from PIT channel #2 */
		if (baseclock > rate)
		{
			/* if the tone is too high to play, don't play it */
			while( length-- > 0 )
				*sample++ = 0;
		}
		else
		{
			while( length-- > 0 )
			{
				*sample++ = signal;
				incr -= baseclock;
				while( incr < 0 )
				{
					incr += rate;
					signal = -signal;
				}
			}
		}
		break;
	}
}

