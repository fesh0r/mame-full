/***************************************************************************
	beep.c

	This is used for computers/systems which can only output a constant tone.
	This tone can be turned on and off.
	e.g. PCW and PCW16 computer systems
	KT - 25-Jun-2000

	Sound handler
****************************************************************************/
#include "driver.h"

struct beep_sound
{
        int beep_stream;
        /* enable beep */
        int beep_enable;
        /* set frequency - this can be changed using the appropiate function */
        int beep_frequency;
        /* initial wave state */
        int beep_old_incr;
        INT16 beep_signal;
};

static struct beep_interface *intf;
static struct beep_sound beeps[4];

#define BEEP_SAMPLE_RATE 22050

/************************************/
/* Stream updater                   */
/************************************/
static void beep_sound_update( int num, INT16 *sample, int length )
{
        INT16 signal = beeps[num].beep_signal;
	int baseclock, rate = Machine->sample_rate/2;
    /* get progress through wave */
        int incr = beeps[num].beep_old_incr;

        baseclock = BEEP_SAMPLE_RATE/ (beeps[num].beep_frequency*2);

	/* if we're not enabled, just fill with 0 */
        if ( !beeps[num].beep_enable || Machine->sample_rate == 0)
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
        beeps[num].beep_old_incr = incr;
        beeps[num].beep_signal = signal;
}

/************************************/
/* Sound handler start              */
/************************************/
int beep_sh_start( const struct MachineSound *msound ) 
{
	int i;

	intf = msound->sound_interface;

	for (i=0; i<intf->num; i++)
	{
		struct beep_sound *pBeep = &beeps[i];
		char buf[32];

	      if( intf->num > 1 )
			sprintf(buf, "Beep #%d", i+1);
		else
			strcpy(buf, "Beep");

                pBeep->beep_stream = stream_init( "Generic Beep", 100, BEEP_SAMPLE_RATE, i, beep_sound_update );
        	pBeep->beep_enable = 0;
        	pBeep->beep_frequency = 3250;
        	pBeep->beep_old_incr = 0;
        	pBeep->beep_signal = 0x07fff;
	}
	return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void beep_sh_stop(void )
{
}

/************************************/
/* Sound handler update 			*/
/************************************/
void beep_sh_update(void )
{
     int i;

     for (i=0; i<intf->num; i++)
     {
        stream_update( beeps[i].beep_stream, i );
     }
}

/* changing state to on from off will restart tone */
void beep_set_state( int num, int on )
{
	/* only update if new state is not the same as old state */
        if (beeps[num].beep_enable!=on)
	{
                stream_update( beeps[num].beep_stream, num );

                beeps[num].beep_enable = on;
		/* restart wave from beginning */
                beeps[num].beep_old_incr = 0;
                beeps[num].beep_signal = 0x07fff;
	}
}

/* setting new frequency starts from beginning */
void beep_set_frequency(int num,int frequency)
{
        if (beeps[num].beep_frequency!=frequency)
        {
             stream_update(beeps[num].beep_stream,num);
             beeps[num].beep_frequency = frequency;
             beeps[num].beep_signal = 0x07fff;
             beeps[num].beep_old_incr = 0;
        }
}

void    beep_set_volume(int num,int volume)
{
        stream_update( beeps[num].beep_stream, num );

        volume = ( 100 / 7 ) * volume;

        mixer_set_volume( beeps[num].beep_stream, volume );
}
