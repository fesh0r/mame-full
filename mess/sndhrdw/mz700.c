/***************************************************************************
 *	Sharp MZ700
 *
 *	sound hardware
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 ***************************************************************************/

#include "driver.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

static int channel;
static int baseclock = 1;

static void mz700_sound_output(int param, INT16 *buffer, int length)
{
	static INT16 signal = 0x7fff;
    static int incr = 0;
	int rate = Machine->sample_rate / 2;

	while( length-- > 0 )
	{
		*buffer++ = signal;
		incr -= baseclock;
		while( incr < 0 )
		{
			incr += rate;
			signal = -signal;
		}
	}
}

int mz700_sh_start(const struct MachineSound* driver)
{
	logerror("pc_sh_start\n");
	channel = stream_init("PC speaker", 50, Machine->sample_rate, 0, mz700_sound_output);
    return 0;
}

void mz700_sh_stop(void)
{
}

void mz700_sh_update(void)
{
	stream_update(channel, 0);
}

void mz700_sh_set_clock(int clock)
{
	stream_update(channel, 0);
    baseclock = clock;
	LOG(1,"mz700_sh_set_clock",("%d Hz\n", clock));
}
