/***************************************************************************
	beep.c

	This is used for computers/systems which can only output a constant tone.
	This tone can be turned on and off.
	e.g. PCW and PCW16 computer systems
	KT - 25-Jun-2000

	Sound handler
****************************************************************************/
#include "driver.h"
#include "sound/beep.h"


static int beep_stream;
/* enable beep */
static int beep_enable = 0;
/* set frequency - this can be changed using the appropiate function */
static int beep_frequency = 3250;
/* initial wave state */
static int beep_old_incr = 0;
static INT16 beep_signal = 0x7fff;

#define BEEP_SAMPLE_RATE 22050

/************************************/
/* Stream updater                   */
/************************************/
static void beep_sound_update( int num, INT16 *sample, int length )
{
	INT16 signal = beep_signal;
	int baseclock, rate = Machine->sample_rate/2;
    /* get progress through wave */
	int incr = beep_old_incr;

	baseclock = BEEP_SAMPLE_RATE/ (beep_frequency*2);

	/* if we're not enabled, just fill with 0 */
	if ( !beep_enable || Machine->sample_rate == 0)
	{
		memset( sample, 0, length * sizeof( INT16 ) );
		return;
	}

	/* fill in the sample */
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

	/* store progress through wave */
	beep_old_incr = incr;
	beep_signal = signal;
}

/************************************/
/* Sound handler start              */
/************************************/
int beep_sh_start( const struct MachineSound *msound ) {

	beep_stream = stream_init( "Generic Beep", 100, BEEP_SAMPLE_RATE, 0, beep_sound_update );

	return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void beep_sh_stop( void ) {
}

/************************************/
/* Sound handler update 			*/
/************************************/
void beep_sh_update( void ) {
}

void beep_set_state( int on )
{
	/* only update if new state is not the same as old state */
	if (beep_enable!=on)
	{
		stream_update( beep_stream, 0 );

		beep_enable = on;
		/* restart wave from beginning */
		beep_old_incr = 0;
		beep_signal = 0x07fff;
	}
}

void beep_set_frequency(int frequency)
{
	beep_frequency = frequency;
}

void	beep_set_volume(int volume)
{
	stream_update( beep_stream, 0 );

	volume = ( 100 / 7 ) * volume;

	mixer_set_volume( beep_stream, volume );
}
