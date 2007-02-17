/***************************************************************************

    Sega G-80 raster hardware

    Across these games, there's a mixture of discrete sound circuitry,
    speech boards, ADPCM samples, and a TMS3617 music chip.

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "cpu/i8039/i8039.h"
#include "segag80r.h"
#include "machine/8255ppi.h"
#include "sound/samples.h"
#include "sound/tms36xx.h"
#include "sound/dac.h"
#include "sound/custom.h"



/*************************************
 *
 *  Constants
 *
 *************************************/

#define SEGA005_555_TIMER_FREQ		(1.44 / ((15e3 + 2 * 4.7e3) * 1.5e-6))
#define SEGA005_COUNTER_FREQ		(100000)	/* unknown, just a guess */



/*************************************
 *
 *  Local sound states
 *
 *************************************/

static UINT8 sound_state[2];
static UINT8 sound_rate;
static UINT16 sound_addr;
static UINT8 sound_data;

static UINT8 square_state;
static UINT8 square_count;

static mame_timer *sega005_sound_timer;
static sound_stream *sega005_stream;

static UINT8 n7751_command;
static UINT8 n7751_busy;



/*************************************
 *
 *  Astro Blaster sound hardware
 *
 *************************************/

static SOUND_START( astrob );

/*
    Description of Astro Blaster sounds (in the hope of future discrete goodness):

    CD4017 = decade counter with one output per decoded stage (10 outputs altogether)
    CD4024 = 7-bit counter with 7 outputs


    "V" signal
    ----------
        CD4017 @ U15:
            reset by RATE RESET signal = 1
            clocked by falling edge of ATTACK signal
            +12V output from here goes through a diode and one of 10 resistors:
                0 = 120k
                1 = 82k
                2 = 62k
                3 = 56k
                4 = 47k
                5 = 39k
                6 = 35k
                7 = 27k
                8 = 24k
                9 = 22k
            and then in series through a 22k resistor

        Op-amp @ U6 takes the WARP signal and the output of CD4017 @ U15
            and forms the signal "V" which is used to control the invader
            sounds


        How to calculate the output voltage at U16 labeled (V).
        (Derrick Renaud)

        First you have an inverting amp.  To get the gain you
        use G=-Rf/Ri, where Rf=R178=22k.  Ri is the selected
        resistor on the output of U15.

        The input voltage to the amp (pin 6) will always be
        about 12V - 0.5V (diode drop in low current circuit) =
        11.5V.

        Now you need to calculate the refrence voltage on the
        + input (pin 5).  Depending on the state of WARP...

        If the warp data is 0, then U31 inverts it to an Open
        Collector high, meaning WARP is out of circuit. So:
        Vref = 12V * (R163)/(R162+R163)
             = 12V * 10k/(10K+4.7k)
             = 8.163V

        When warp data is 1, then U31 inverts it to low,
        grounding R164 putting it in parallel with R163,
        giving:
        Vref = 12V * (R163||R164)/(R163||R164 +R162)
             = 12V * 5k/(5k+4.7k)
             = 6.186V

        Now to get the control voltage V:
        V = (Vi - Vref) * G + Vref
          = (11.5V - Vref) * G + Vref

        That gives you the control voltage at V.  From there I
        would have to millman the voltage with the internal
        voltage/resistors of the 555 to get the actual used
        control voltage.

        But it seems you just want a range, so just use the
        above info to get the highest and lowest voltages
        generated, and create the frequency shift you desire.
        Remember as the control voltage (V) lowers, the
        frequency increases.



    INVADER-1 output
    ----------------




    INVADER-2 output
    ----------------
        555 timer @ U13 in astable mode with the following parameters:
            R1 = 10k
            R2 = 100k
            C = 0.0022u
            CV = "V" signal
            Reset = (PORT076 & 0x02)
        Output goes to CD4024 @ U12

        CD4024 @ U12:
            reset through some unknown means
            clocked by 555 timer @ U13
            +12 output from here goes through a resistor ladder:
                Q1 -> 82k
                Q2 -> 39k
                Q3 -> 22k
                Q4 -> 10k
        Summed output from here is INVADER-2


    INVADER-3 output
    ----------------
        555 timer at U17 in astable mode with the following parameters:
            R1 = 10k
            R2 = 68k
            C = 0.1u
            CV = some combination of "V" and "W" signals
            Reset = (PORT076 & 0x04)
        Output from here is INVADER-3

*/

static const char *astrob_sample_names[] =
{
	"*astrob",
	"invadr1.wav",		/* 0 */
	"winvadr1.wav",		/* 1 */
	"invadr2.wav",		/* 2 */
	"winvadr2.wav",		/* 3 */
	"invadr3.wav",		/* 4 */
	"winvadr3.wav",		/* 5 */
	"invadr4.wav",		/* 6 */
	"winvadr4.wav",		/* 7 */
	"asteroid.wav",		/* 8 */
	"refuel.wav",		/* 9 */
	"pbullet.wav",		/* 10 */
	"ebullet.wav",		/* 11 */
	"eexplode.wav",		/* 12 */
	"pexplode.wav",		/* 13 */
	"deedle.wav",		/* 14 */
	"sonar.wav",		/* 15 */
	0
};


static struct Samplesinterface astrob_samples_interface =
{
	11,
	astrob_sample_names
};


MACHINE_DRIVER_START( astrob_sound_board )

	MDRV_SOUND_START(astrob)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(astrob_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END



/*************************************
 *
 *  Startup configuration
 *
 *************************************/

static SOUND_START( astrob )
{
	state_save_register_global_array(sound_state);
	state_save_register_global(sound_rate);
	return 0;
}



/*************************************
 *
 *  Astro Blaster sound triggers
 *
 *************************************/

WRITE8_HANDLER( astrob_sound_w )
{
	static const float attack_resistor[10] =
	{
		120.0f, 82.0f, 62.0f, 56.0f, 47.0f, 39.0f, 33.0f, 27.0f, 24.0f, 22.0f
	};
	float freq_factor;

	UINT8 diff = data ^ sound_state[offset];
	sound_state[offset] = data;

	switch (offset)
	{
		case 0:
			/* INVADER-1: channel 0 */
			if ((diff & 0x01) && !(data & 0x01)) sample_start(0, (data & 0x80) ? 0 : 1, TRUE);
			if ((data & 0x01) && sample_playing(0)) sample_stop(0);

			/* INVADER-2: channel 1 */
			if ((diff & 0x02) && !(data & 0x02)) sample_start(1, (data & 0x80) ? 2 : 3, TRUE);
			if ((data & 0x02) && sample_playing(1)) sample_stop(1);

			/* INVADER-3: channel 2 */
			if ((diff & 0x04) && !(data & 0x04)) sample_start(2, (data & 0x80) ? 4 : 5, TRUE);
			if ((data & 0x04) && sample_playing(2)) sample_stop(2);

			/* INVADER-4: channel 3 */
			if ((diff & 0x08) && !(data & 0x08)) sample_start(3, (data & 0x80) ? 6 : 7, TRUE);
			if ((data & 0x08) && sample_playing(3)) sample_stop(3);

			/* ASTROIDS: channel 4 */
			if ((diff & 0x10) && !(data & 0x10)) sample_start(4, 8, TRUE);
			if ((data & 0x10) && sample_playing(4)) sample_stop(4);

			/* MUTE */
			sound_global_enable(!(data & 0x20));

			/* REFILL: channel 5 */
			if (!(data & 0x40) && !sample_playing(5)) sample_start(5, 9, FALSE);
			if ( (data & 0x40) && sample_playing(5))  sample_stop(5);

			/* WARP: changes which sample is played for the INVADER samples above */
			if (diff & 0x80)
			{
				if (sample_playing(0)) sample_start(0, (data & 0x80) ? 0 : 1, TRUE);
				if (sample_playing(1)) sample_start(1, (data & 0x80) ? 2 : 3, TRUE);
				if (sample_playing(2)) sample_start(2, (data & 0x80) ? 4 : 5, TRUE);
				if (sample_playing(3)) sample_start(3, (data & 0x80) ? 6 : 7, TRUE);
			}
			break;

		case 1:
			/* LASER #1: channel 6 */
			if ((diff & 0x01) && !(data & 0x01)) sample_start(6, 10, FALSE);

			/* LASER #2: channel 7 */
			if ((diff & 0x02) && !(data & 0x02)) sample_start(7, 11, FALSE);

			/* SHORT EXPL: channel 8 */
			if ((diff & 0x04) && !(data & 0x04)) sample_start(8, 12, FALSE);

			/* LONG EXPL: channel 8 */
			if ((diff & 0x08) && !(data & 0x08)) sample_start(8, 13, FALSE);

			/* ATTACK RATE */
			if ((diff & 0x10) && !(data & 0x10)) sound_rate = (sound_rate + 1) % 10;

			/* RATE RESET */
			if (!(data & 0x20)) sound_rate = 0;

			/* BONUS: channel 9 */
			if ((diff & 0x40) && !(data & 0x40)) sample_start(9, 14, FALSE);

			/* SONAR: channel 10 */
			if ((diff & 0x80) && !(data & 0x80)) sample_start(10, 15, FALSE);
			break;
	}

	/* the samples were recorded with sound_rate = 0, so we need to scale */
	/* the frequency as a fraction of that; these equations come from */
	/* Derrick's analysis above; we compute the inverted scale factor to */
	/* account for the fact that frequency goes up as CV goes down */
	/* WARP is already taken into account by the differing samples above */
	freq_factor  = (11.5f - 8.163f) * (-22.0f / attack_resistor[0]) + 8.163f;
	freq_factor /= (11.5f - 8.163f) * (-22.0f / attack_resistor[sound_rate]) + 8.163f;

	/* adjust the sample rate of invader sounds based the sound_rate */
	/* this is an approximation */
	if (sample_playing(0)) sample_set_freq(0, sample_get_base_freq(0) * freq_factor);
	if (sample_playing(1)) sample_set_freq(1, sample_get_base_freq(1) * freq_factor);
	if (sample_playing(2)) sample_set_freq(2, sample_get_base_freq(2) * freq_factor);
	if (sample_playing(3)) sample_set_freq(3, sample_get_base_freq(3) * freq_factor);
}



/*************************************
 *
 *  005 sound hardware
 *
 *************************************/

static SOUND_START( 005 );
static void *sega005_custom_start(int clock, const struct CustomSound_interface *config);
static void sega005_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
static void sega005_auto_timer(int param);

/*
    005

    The Sound Board consists of the following:

    An 8255:
        Port A controls the sounds that use discrete circuitry
            A0 - Large Expl. Sound Trig
            A1 - Small Expl. Sound Trig
            A2 - Drop Sound Bomb Trig
            A3 - Shoot Sound Pistol Trig
            A4 - Missile Sound Trig
            A5 - Helicopter Sound Trig
            A6 - Whistle Sound Trig
            A7 - <unused>

      Port B controls the melody generator (described below)

      Port C is apparently unused


    Melody Generator:

        555 timer frequency = 1.44/((R1 + 2R2)*C)
        R1 = 15e3
        R2 = 4.7e3
        C=1.5e-6
        Frequency = 39.344 Hz

        Auto timer is enabled if port B & 0x20 == 1
        Auto timer is reset if 2716 value & 0x20 == 0

        Manual timer is enabled if port B & 0x20 == 0
        Manual timer is clocked if port B & 0x40 goes from 0 to 1

        Both auto and manual timers clock LS393 counter
        Counter is held to 0 if port B & 0x10 == 1

        Output of LS393 >> 1 selects low 7 bits of lookup in 2716.
        High 4 bits come from port B bits 0-3.

        Low 5 bits of output from 2716 look up value in 6331 PROM at U8 (32x8)

        8-bit output of 6331 at U8 is loaded into pair of LS161 counters whenever they overflow.
        LS161 counters are clocked somehow (not clear how)

        Carry output from LS161 counters (overflowing 8 bits) goes to the B
            input on the LS293 counter at U14.
        Rising edge of B input clocks bit 1 of counter (effectively adding 2).
        Output B (bit 1) is mixed with output D (bit 3) with different weights
            through a small RC circuit and fed into the 4391 input at U32.

        The 4391 output is the final output.
*/

static const char *sega005_sample_names[] =
{
	"*005",
	"lexplode.wav",		/* 0 */
	"sexplode.wav",		/* 1 */
	"dropbomb.wav",		/* 2 */
	"shoot.wav",		/* 3 */
	"missile.wav",		/* 4 */
	"heilcopt.wav",		/* 5 */
	"whistle.wav",		/* 6 */
	0
};


static struct Samplesinterface sega005_samples_interface =
{
	7,
	sega005_sample_names
};


static struct CustomSound_interface sega005_custom_interface =
{
	sega005_custom_start
};


MACHINE_DRIVER_START( 005_sound_board )

	/* sound hardware */
	MDRV_SOUND_START(005)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(sega005_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(sega005_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END



/*************************************
 *
 *  Startup configuration
 *
 *************************************/

static SOUND_START( 005 )
{
	static const ppi8255_interface ppi_intf =
	{
		1,
		{ 0 },
		{ 0 },
		{ 0 },
		{ sega005_sound_a_w },
		{ sega005_sound_b_w },
		{ 0 }
	};
	ppi8255_init(&ppi_intf);

	state_save_register_global_array(sound_state);
	state_save_register_global(sound_addr);
	state_save_register_global(sound_data);
	state_save_register_global(square_state);
	state_save_register_global(square_count);
	return 0;
}



/*************************************
 *
 *  005 sound triggers
 *
 *************************************/

WRITE8_HANDLER( sega005_sound_a_w )
{
	UINT8 diff = data ^ sound_state[0];
	sound_state[0] = data;

	/* LARGE EXPL: channel 0 */
	if ((diff & 0x01) && !(data & 0x01)) sample_start(0, 0, FALSE);

	/* SMALL EXPL: channel 1 */
	if ((diff & 0x02) && !(data & 0x02)) sample_start(1, 1, FALSE);

	/* DROP BOMB: channel 2 */
	if ((diff & 0x04) && !(data & 0x04)) sample_start(2, 2, FALSE);

	/* SHOOT PISTOL: channel 3 */
	if ((diff & 0x08) && !(data & 0x08)) sample_start(3, 3, FALSE);

	/* MISSILE: channel 4 */
	if ((diff & 0x10) && !(data & 0x10)) sample_start(4, 4, FALSE);

	/* HELICOPTER: channel 5 */
	if ((diff & 0x20) && !(data & 0x20) && !sample_playing(5)) sample_start(5, 5, TRUE);
	if ((diff & 0x20) &&  (data & 0x20)) sample_stop(5);

	/* WHISTLE: channel 6 */
	if ((diff & 0x40) && !(data & 0x40) && !sample_playing(6)) sample_start(6, 6, TRUE);
	if ((diff & 0x40) &&  (data & 0x40)) sample_stop(6);
}


INLINE void sega005_update_sound_data(void)
{
	UINT8 newval = memory_region(REGION_SOUND1)[sound_addr];
	UINT8 diff = newval ^ sound_data;

	//mame_printf_debug("  [%03X] = %02X\n", sound_addr, newval);

	/* latch the new value */
	sound_data = newval;

	/* if bit 5 goes high, we reset the timer */
	if ((diff & 0x20) && !(newval & 0x20))
	{
		//mame_printf_debug("Stopping timer\n");
		timer_adjust(sega005_sound_timer, TIME_NEVER, 0, TIME_NEVER);
	}

	/* if bit 5 goes low, we start the timer again */
	if ((diff & 0x20) && (newval & 0x20))
	{
		//mame_printf_debug("Starting timer\n");
		timer_adjust(sega005_sound_timer, TIME_IN_HZ(SEGA005_555_TIMER_FREQ), 0, TIME_IN_HZ(SEGA005_555_TIMER_FREQ));
	}
}


WRITE8_HANDLER( sega005_sound_b_w )
{
	/*
           D6: manual timer clock (0->1)
           D5: 0 = manual timer, 1 = auto timer
           D4: 1 = hold/reset address counter to 0
        D3-D0: upper 4 bits of ROM address
    */
	UINT8 diff = data ^ sound_state[1];
	sound_state[1] = data;

	//mame_printf_debug("sound[%d] = %02X\n", 1, data);

	/* force a stream update */
	stream_update(sega005_stream);

	/* ROM address */
	sound_addr = ((data & 0x0f) << 7) | (sound_addr & 0x7f);

	/* reset both sound address and square wave counters */
	if (data & 0x10)
	{
		sound_addr &= 0x780;
		square_state = 0;
	}

	/* manual clock */
	if ((diff & 0x40) && (data & 0x40) && !(data & 0x20) && !(data & 0x10))
		sound_addr = (sound_addr & 0x780) | ((sound_addr + 1) & 0x07f);

	/* update the sound data */
	sega005_update_sound_data();
}



/*************************************
 *
 *  005 custom sound generation
 *
 *************************************/

static void *sega005_custom_start(int clock, const struct CustomSound_interface *config)
{
	/* create the stream */
	sega005_stream = stream_create(0, 1, SEGA005_COUNTER_FREQ, NULL, sega005_stream_update);

	/* create a timer for the 555 */
	sega005_sound_timer = timer_alloc(sega005_auto_timer);

	/* set the initial sound data */
	sound_data = 0x00;
	sega005_update_sound_data();

	return auto_malloc(1);
}


static void sega005_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	const UINT8 *sound_prom = memory_region(REGION_PROMS);
	int i;

	/* no implementation yet */
	for (i = 0; i < samples; i++)
	{
		if (!(sound_state[1] & 0x10) && (++square_count & 0xff) == 0)
		{
			square_count = sound_prom[sound_data & 0x1f];

			/* hack - the RC should filter this out */
			if (square_count != 0xff)
				square_state += 2;
		}

		outputs[0][i] = (square_state & 2) ? 0x7fff : 0x0000;
	}
}


static void sega005_auto_timer(int param)
{
	/* force an update then clock the sound address if not held in reset */
	stream_update(sega005_stream);
	if ((sound_state[1] & 0x20) && !(sound_state[1] & 0x10))
	{
		sound_addr = (sound_addr & 0x780) | ((sound_addr + 1) & 0x07f);
		sega005_update_sound_data();
	}
}



/*************************************
 *
 *  Space Odyssey sound hardware
 *
 *************************************/

static SOUND_START( spaceod );

const char *spaceod_sample_names[] =
{
	"*spaceod",
	"fire.wav",			/* 0 */
	"bomb.wav",			/* 1 */
	"eexplode.wav", 	/* 2 */
	"pexplode.wav",		/* 3 */
	"warp.wav", 		/* 4 */
	"birth.wav", 		/* 5 */
	"scoreup.wav", 		/* 6 */
	"ssound.wav",		/* 7 */
	"accel.wav", 		/* 8 */
	"damaged.wav", 		/* 9 */
	"erocket.wav",		/* 10 */
	0
};


static struct Samplesinterface spaceod_samples_interface =
{
	11,
	spaceod_sample_names
};


MACHINE_DRIVER_START( spaceod_sound_board )

	/* sound hardware */
	MDRV_SOUND_START(spaceod)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(spaceod_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END



/*************************************
 *
 *  Startup configuration
 *
 *************************************/

static SOUND_START( spaceod )
{
	state_save_register_global_array(sound_state);
	return 0;
}



/*************************************
 *
 *  Space Odyssey sound triggers
 *
 *************************************/

WRITE8_HANDLER( spaceod_sound_w )
{
	UINT8 diff = data ^ sound_state[offset];
	sound_state[offset] = data;

	switch (offset)
	{
		case 0:
			/* BACK G: channel 0 */
			if ((diff & 0x01) && !(data & 0x01) && !sample_playing(0)) sample_start(0, 7, TRUE);
			if ((diff & 0x01) &&  (data & 0x01)) sample_stop(0);

			/* SHORT EXP: channel 1 */
			if ((diff & 0x04) && !(data & 0x04)) sample_start(1, 2, FALSE);

			/* ACCELERATE: channel 2 */
			if ((diff & 0x10) && !(data & 0x10)) sample_start(2, 8, FALSE);

			/* BATTLE STAR: channel 3 */
			if ((diff & 0x20) && !(data & 0x20)) sample_start(3, 10, FALSE);

			/* D BOMB: channel 4 */
			if ((diff & 0x40) && !(data & 0x40)) sample_start(4, 1, FALSE);

			/* LONG EXP: channel 5 */
			if ((diff & 0x80) && !(data & 0x80)) sample_start(5, 3, FALSE);
			break;

		case 1:
			/* SHOT: channel 6 */
			if ((diff & 0x01) && !(data & 0x01)) sample_start(6, 0, FALSE);

			/* BONUS UP: channel 7 */
			if ((diff & 0x02) && !(data & 0x02)) sample_start(7, 6, FALSE);

			/* WARP: channel 8 */
			if ((diff & 0x08) && !(data & 0x08)) sample_start(8, 4, FALSE);

			/* APPEARANCE UFO: channel 9 */
			if ((diff & 0x40) && !(data & 0x40)) sample_start(9, 5, FALSE);

			/* BLACK HOLE: channel 10 */
			if ((diff & 0x80) && !(data & 0x80)) sample_start(10, 9, FALSE);
			break;
	}
}



/*************************************
 *
 *  Monster Bash sound hardware
 *
 *************************************/

static SOUND_START( monsterb );
static WRITE8_HANDLER( monsterb_sound_a_w );
static WRITE8_HANDLER( monsterb_sound_b_w );
static READ8_HANDLER( n7751_status_r );
static WRITE8_HANDLER( n7751_command_w );
static WRITE8_HANDLER( n7751_rom_offset_w );
static WRITE8_HANDLER( n7751_rom_select_w );
static READ8_HANDLER( n7751_rom_r );
static READ8_HANDLER( n7751_command_r );
static WRITE8_HANDLER( n7751_busy_w );
static READ8_HANDLER( n7751_t1_r );

/*
    Monster Bash

    The Sound Board is a fairly complex mixture of different components.
    An 8255A-5 controls the interface to/from the sound board.
    Port A connects to a TMS3617 (basic music synthesizer) circuit.
    Port B connects to two sounds generated by discrete circuitry.
    Port C connects to a NEC7751 (8048 CPU derivative) to control four "samples".
*/


static const char *monsterb_sample_names[] =
{
	"*monsterb",
	"zap.wav",
	"jumpdown.wav",
	0
};


static struct Samplesinterface monsterb_samples_interface =
{
	2,
	monsterb_sample_names
};


static struct TMS36XXinterface monsterb_tms3617_interface =
{
	TMS3617,
	{0.5,0.5,0.5,0.5,0.5,0.5}  /* decay times of voices */
};



/*************************************
 *
 *  N7751 memory maps
 *
 *************************************/

ADDRESS_MAP_START( monsterb_7751_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_ROM
ADDRESS_MAP_END

ADDRESS_MAP_START( monsterb_7751_portmap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(n7751_t1_r)
	AM_RANGE(I8039_p2, I8039_p2) AM_READ(n7751_command_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(n7751_rom_r)
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(DAC_0_data_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(n7751_busy_w)
	AM_RANGE(I8039_p4, I8039_p6) AM_WRITE(n7751_rom_offset_w)
	AM_RANGE(I8039_p7, I8039_p7) AM_WRITE(n7751_rom_select_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

MACHINE_DRIVER_START( monsterb_sound_board )

	/* basic machine hardware */
	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_PROGRAM_MAP(monsterb_7751_map,0)
	MDRV_CPU_IO_MAP(monsterb_7751_portmap,0)

	/* sound hardware */
	MDRV_SOUND_START(monsterb)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(monsterb_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(TMS36XX, 247)
	MDRV_SOUND_CONFIG(monsterb_tms3617_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  Startup configuration
 *
 *************************************/

static SOUND_START( monsterb )
{
	static const ppi8255_interface ppi_intf =
	{
		1,
		{ 0 },
		{ 0 },
		{ n7751_status_r },
		{ monsterb_sound_a_w },
		{ monsterb_sound_b_w },
		{ n7751_command_w }
	};
	ppi8255_init(&ppi_intf);

	state_save_register_global_array(sound_state);
	state_save_register_global(sound_addr);
	state_save_register_global(n7751_command);
	state_save_register_global(n7751_busy);
	return 0;
}



/*************************************
 *
 *  TMS3617 access
 *
 *************************************/

static WRITE8_HANDLER( monsterb_sound_a_w )
{
	int enable_val;

	/* Lower four data lines get decoded into 13 control lines */
	tms36xx_note_w(0, 0, data & 15);

	/* Top four data lines address an 82S123 ROM that enables/disables voices */
	enable_val = memory_region(REGION_SOUND2)[(data & 0xF0) >> 4];
	tms3617_enable_w(0, enable_val >> 2);
}



/*************************************
 *
 *  Discrete sound triggers
 *
 *************************************/

static WRITE8_HANDLER( monsterb_sound_b_w )
{
	UINT8 diff = data ^ sound_state[1];
	sound_state[1] = data;

	/* SHOT: channel 0 */
	if ((diff & 0x01) && !(data & 0x01)) sample_start(0, 0, FALSE);

	/* DIVE: channel 1 */
	if ((diff & 0x02) && !(data & 0x02)) sample_start(1, 1, FALSE);

    /* TODO: D7 on Port B might affect TMS3617 output (mute?) */
}



/*************************************
 *
 *  N7751 connections
 *
 *************************************/

static READ8_HANDLER( n7751_status_r )
{
	return n7751_busy << 4;
}


static WRITE8_HANDLER( n7751_command_w )
{
	/*
        Z80 7751 control port

        D0-D2 = connected to 7751 port C
        D3    = /INT line
    */
	n7751_command = data & 0x07;
	cpunum_set_input_line(1, 0, ((data & 0x08) == 0) ? ASSERT_LINE : CLEAR_LINE);
	cpu_boost_interleave(0, TIME_IN_USEC(100));
}


static WRITE8_HANDLER( n7751_rom_offset_w )
{
	/* P4 - address lines 0-3 */
	/* P5 - address lines 4-7 */
	/* P6 - address lines 8-11 */
	int mask = (0xf << (4 * offset)) & 0x3fff;
	int newdata = (data << (4 * offset)) & mask;
	sound_addr = (sound_addr & ~mask) | newdata;
}


static WRITE8_HANDLER( n7751_rom_select_w )
{
	/* P7 - ROM selects */
	int numroms = memory_region_length(REGION_SOUND1) / 0x1000;
	sound_addr &= 0xfff;
	if (!(data & 0x01) && numroms >= 1) sound_addr |= 0x0000;
	if (!(data & 0x02) && numroms >= 2) sound_addr |= 0x1000;
	if (!(data & 0x04) && numroms >= 3) sound_addr |= 0x2000;
	if (!(data & 0x08) && numroms >= 4) sound_addr |= 0x3000;
}


static READ8_HANDLER( n7751_rom_r )
{
	/* read from BUS */
	return memory_region(REGION_SOUND1)[sound_addr];
}


static READ8_HANDLER( n7751_command_r )
{
	/* read from P2 - 8255's PC0-2 connects to 7751's S0-2 (P24-P26 on an 8048) */
	/* bit 0x80 is an alternate way to control the sample on/off; doesn't appear to be used */
	return 0x80 | ((n7751_command & 0x07) << 4);
}


static WRITE8_HANDLER( n7751_busy_w )
{
	/* write to P2 */
	/* output of bit $80 indicates we are ready (1) or busy (0) */
	/* no other outputs are used */
	n7751_busy = data >> 7;
}


static READ8_HANDLER( n7751_t1_r )
{
	/* T1 - labelled as "TEST", connected to ground */
	return 0;
}
