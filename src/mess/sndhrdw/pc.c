/***************************************************************************

    sndhrdw/pc.c

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mess/machine/pc.h"

#define BASECLOCK	1193180

extern int PIT_clock[3];

static int channel = 0;
static int speaker_gate = 0;

/************************************/
/* Sound handler start				*/
/************************************/
int pc_sh_start(void)
{
	logerror("pc_sh_start\n");
	channel = stream_init("PC speaker", 50, Machine->sample_rate, 0, pc_sh_update);
    return 0;
}

int pc_sh_custom_start(const struct MachineSound* driver)
{
	return pc_sh_start();
}

/************************************/
/* Sound handler stop				*/
/************************************/
void pc_sh_stop(void)
{
	logerror("pc_sh_stop\n");
}

void pc_sh_speaker(int mode)
{
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

	if( PIT_clock[2] )
		baseclock = BASECLOCK / PIT_clock[2];
	else
		baseclock = BASECLOCK / 65536;

	switch( speaker_gate )
	{
	case 0: /* speaker off */
		while( length-- > 0 )
			*sample++ = 0;
		break;
	case 1: /* speaker on */
		while( length-- > 0 )
			*sample++ = 0x7fff;
        break;
	case 2: /* speaker gate tone from PIT channel #2 */
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
}

