/****************************************************************************
 *
 * pulsar (vicdual.c)
 *
 * sound driver
 * juergen buchmueller <pullmoll@t-online.de>, aug 2000
 *
 * TODO:
 * fix bugs (some frequencies are off compared to the samples)
 * add heart-beat sound (a difficult to understand part in the schematics)
 *
 ****************************************************************************/
#include "driver.h"
#include <math.h>

#define CLANG		 0x01
#define KEY 		 0x02
#define ALIEN_HIT	 0x04
#define PLAYER_HIT	 0x08
#define ALIEN_SHOOT  0x10
#define PLAYER_SHOOT 0x20
#define BONUS		 0x40
#define HBEAT_RATE	 0x80	 /* currently not used */

#define SIZZLE		 0x01
#define GATE		 0x02
#define BIRTH		 0x04
#define HEART_BEAT	 0x08
#define MOVING_MAZE  0x10

static int sample_rate = 0;

static int sound_latch_1 = 0xff;
static int alien_shoot_on = 0;
static int player_shoot_on = 0;
static int player_shoot_freq = 0;
static int sound_latch_2 = 0xff;

static int channel = 0;
static int noise_count = 0;
static INT16 *decay = NULL;
static INT16 *noise = NULL;

INLINE int clang(void)
{
	static int counter_amp;
	static int amp;
	static int counter_1;
	static int counter_2;
	static int counter_3;
	static int output;
	static int mix[8] = {0,10922,10922,21845,10922,21845,21845,32767};
	static int sample;

	if ((sound_latch_1 & CLANG) == 0)
	{
		amp = 32767;
	}
	else
	if (amp > 0)
	{
		/* R1 = 120k, C1 = 0.047uF -> 5.64e-3 * 0.693 = 3.90852e-3 ~= 256Hz */
		counter_1 -= 256;
		while (counter_1 < 0)
		{
			counter_1 += sample_rate;
			output ^= 1;
		}
		/* R2 = 150k, C2 = 0.047uF -> 7.05e-3 * 0.693 = 4.88565e-3 ~= 205Hz */
		counter_2 -= 205;
		while (counter_2 < 0)
		{
			counter_2 += sample_rate;
			output ^= 2;
		}
		/* R3 = 220k, C3 = 0.047uF -> 10.34e-3 * 0.693 = 7.16562e-3 ~= 140Hz */
		counter_3 -= 140;
		while (counter_3 < 0)
		{
			counter_3 += sample_rate;
			output ^= 4;
		}

		counter_amp -= (int)(32768 / 2.5);
		while (counter_amp < 0)
		{
			counter_amp += sample_rate;
			amp--;
		}

		/* smoother signals (low pass filter !?) */
		if (sample < mix[output])
			sample = (mix[output] - sample) / 4 + 1;
		else
		if (sample > mix[output])
			sample = (sample - mix[output]) / 4 + 1;
	}

	return sample * amp / 32768;
}

INLINE int key(void)
{
	static int counter;
	static int sample;

	if (sound_latch_1 & KEY)
		return 0;

	/* filter R = 330k, C = 0.0022uF -> 7.26e-4 * 0.693 ~= 1988Hz */
	counter -= 1988;
	while (counter < 0)
	{
		sample = noise[noise_count];
		counter += sample_rate;
	}

	return sample;
}

INLINE int alien_hit(void)
{
	static int amp;
	static int counter_amp;
	static int counter;
	static int sample;

	if ((sound_latch_1 & ALIEN_HIT) == 0)
	{
		amp = 32767;
	}
	else
	if (amp > 0)
	{
		if (sound_latch_1 & ALIEN_HIT)
		{
			counter_amp -= (int)(32768 / 0.35);
			while (counter_amp < 0)
			{
				counter_amp += sample_rate;
				amp--;
			}
			/* filter frequency */
			counter -= 300;
			while (counter < 0)
			{
				counter += sample_rate;
				sample = amp & noise[noise_count];
			}
		}
	}

	return sample;
}

INLINE int player_hit(void)
{
	static int amp;
	static int counter_amp;
	static int counter;
	static int sample;

	if ((sound_latch_1 & PLAYER_HIT) == 0)
	{
		amp = 32767;
	}
	else
	if (amp > 0)
	{
		if (sound_latch_1 & PLAYER_HIT)
		{
			counter_amp -= (int)(32768 / 2.2);
			while (counter_amp < 0)
			{
				counter_amp += sample_rate;
				amp--;
			}
			/* filter frequency */
			counter -= 307;
			while (counter < 0)
			{
				counter += sample_rate;
				sample = amp & noise[noise_count];
			}
		}
	}
	return sample;
}

INLINE int alien_shoot(void)
{
	static int counter_1;
	static int sample_1;
	static int counter_2;
	static int sample_2;
	static int sample;

	if (alien_shoot_on)
	{
		alien_shoot_on--;
		/* filter R = 680k, C = 0.01uF -> 6.8e-3 * 0.693 = 4.7124e-3 ~= 212Hz */
		counter_1 -= 212;
		while (counter_1 < 0)
		{
			counter_1 += sample_rate;
			sample_1 ^= 32767;
		}
		/* filter R = 330k, C = 0.0047uF -> 1.551e-3 * 0.693 = 1.074843e-3 ~= 930Hz */
		counter_2 -= 930;
		while (counter_2 < 0)
		{
			counter_2 += sample_rate;
			sample_2 ^= 32767;
		}
		sample = (sample_1 | sample_2) & noise[noise_count];
	}
	else
		return 0;

	return sample;
}

INLINE int player_shoot(void)
{
	static int counter_freq;
	static int counter_555;
	static int output;

	if (player_shoot_on > 0)
	{
		player_shoot_on--;
		/*
		 * Ra is 10k, Rb is 10k, C is 0.022uF
		 * charge time t1 = 0.693 * (Ra + Rb) * C -> 3.0492e-4 s ~= 3280Hz
		 * discharge time t2 = 0.693 * Rb * C -> 1.5246e-4 ~= 6560Hz
		 */
		if (output)
		{
			counter_555 -= 6560 + 6550 * decay[player_shoot_freq] / 32768;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output = 0;
			}
		}
		else
		{
			counter_555 -= 3280 + 3280 * decay[player_shoot_freq] / 32768;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output = 1;
			}
		}
		if (player_shoot_freq > 0)
		{
			counter_freq -= 32768*6;
			while (counter_freq < 0)
			{
				counter_freq += sample_rate;
				if (player_shoot_freq > 0)
					player_shoot_freq--;
			}
		}
	}

	/* return half amp only */
	return output ? 16834 : 0;
}

INLINE int bonus(void)
{
	static int counter_freq;
	static int output_freq;
	static int counter_555;
	static int output_555;
	static int counter_amp;
	static int amp;

	if ((sound_latch_1 & BONUS) == 0)
	{
		amp = 32767;
	}

    if (amp > 0)
	{
		/* R = 1M, C = 0.1uF */
		counter_freq -= (int)(32768 / 0.0693);
		while (counter_freq < 0)
		{
			counter_freq += sample_rate;
			output_freq++;
		}

		/*
		 * Ra is 47k, Rb is 100k, C is 0.0047uF
		 * charge time t1 = 0.693 * (Ra + Rb) * C -> 4.787937e-4 s ~= 2089Hz
		 * discharge time t2 = 0.693 * Rb * C -> 3.2571e-4 ~= 3070Hz
		 */
		if (output_555)
		{
			counter_555 -= (output_freq & 0x8000) ? 2089 - (2089*1/3): 2089;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output_555 = 0;
			}
		}
		else
		{
			counter_555 -= (output_freq & 0x8000) ? 3070 - (3070*1/3): 3070;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output_555 = 1;
			}
		}

		counter_amp -= (int)(32768/2.2);
		while (counter_amp < 0)
		{
			counter_amp += sample_rate;
			if (amp > 0)
				amp--;
		}
	}

	/* return half amp only */
	return output_555 ? amp : 0;
}

/* resistor values at the outputs of a counter/1-of-10 demux 4017 */
static int birth_4017[10] = {
	 22000, 100000,  33000,  68000,  10000,
	150000,  47000, 220000,  82000,  33000
};

INLINE int sizzle(void)
{
	static int counter_1;
	static int counter_2;
	static int output;
	static int mix[4] = {0,10477,22290,32767};
	static int sample;

	if (sound_latch_2 & SIZZLE)
		return 0;

	/* R1 = 820k, C1 = 0.01uF -> 8.2e-3 * 0.693 = 5.6826e-3 ~= 176Hz */
	counter_1 -= 176;
	while (counter_1 < 0)
	{
		counter_1 += sample_rate;
		output ^= 1;
		sample = (mix[output] * 3 + noise[noise_count]) / 4;
	}
	/* R1 = 470k, C1 = 0.047uF -> 0.02209 * 0.693 = 0.01530837 ~= 65Hz */
	counter_2 -= 65;
	while (counter_2 < 0)
	{
		counter_2 += sample_rate;
		output ^= 2;
		sample = (mix[output] * 3 + noise[noise_count]) / 4;
	}

	return sample;
}

INLINE int gate(void)
{
	static int counter_1;
	static int counter_2;
	static int output;
	static int mix[4] = {0,16383,16383,32767};
	static int sample;

	if (sound_latch_2 & GATE)
		return 0;

	/* R1 = 680k, C1 = 0.01uF -> 6.8e-3 * 0.693 = 4.7124e-3 ~= 212Hz */
	counter_1 -= 212;
	while (counter_1 < 0)
	{
		counter_1 += sample_rate;
		output ^= 1;
	}
	/* R1 = 820k, C1 = 0.01uF -> 8.2e-3 * 0.693 = 5.6826e-3 ~= 176Hz */
	counter_2 -= 176;
	while (counter_2 < 0)
	{
		counter_2 += sample_rate;
		output ^= 2;
	}

	/* smoother signals (low pass filters) */
	if (sample < mix[output])
		sample = (mix[output] - sample) / 2 + 1;
	else
	if (sample > mix[output])
		sample = (sample - mix[output]) / 2 + 1;

	return sample;
}

INLINE int birth(void)
{
	static int counter_4017;
	static int counter_555;
	static int counter_amp;
	static int step_c21;
	static int output;
	static int amp = 0;
	static int index_4017;

	/* clock for the 4017 is R18 = 470k and C8 0.01uF
	 * period -> 3.2571e-3 ~= 307Hz */
	if ((counter_4017 -= 64) < 0)
	{
		counter_4017 += sample_rate;
		index_4017 = ++index_4017 % 10;
		step_c21 = birth_4017[index_4017];
	}

	if ((sound_latch_2 & BIRTH) == 0)
		amp = 32767;

	if (amp)
	{
		if (sound_latch_2 & BIRTH)
		{
			/* discharge C 2.2uF through two times 1M */
			counter_amp -= (int)(32768 / 4.4 * 0.693);
			while (counter_amp < 0)
			{
				counter_amp += sample_rate;
				amp--;
			}
		}
		/*
		 * Ra is 10k, Rb is 100k, C is 0.022uF
		 * charge time t1 = 0.693 * (Ra + Rb) * C -> 1.67706e-3 s ~= 596Hz
		 * discharge time t2 = 0.693 * Rb * C -> 1.5246e-4 ~= 6560Hz
		 */
		if (output)
		{
			counter_555 -= 6560 * step_c21 / 200000;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output = 0;
			}
		}
		else
		{
			counter_555 -= 596 * step_c21 / 200000;
			while (counter_555 < 0)
			{
				counter_555 += sample_rate;
				output = 1;
			}
		}
	}

	return output ? decay[amp] : 0;
}

/* resistor values at 5 outputs of a counter/1-of-10 demux 4017 (reset at count 5) */
static int heart_beat_4017_1[10] = {
	100000,  62000,  47000,  33000,  22000,
};

INLINE int heart_beat(void)
{
	static int counter_4017_1;
	static int index_4017_1;
	static int counter_555;
	static int output_555;
	static int index_4017_2;
	static int counter_filter_1;
	static int counter_filter_2;
	static int sample;

	if (sound_latch_2 & HEART_BEAT)
		return 0;

	/* R = 100k, C = 0.05uF -> 0.005 ~= 200Hz
	 * with HBEAT_RATE there's another 1M resistor, so probably(?)
	 * R = 90.9k, C = 0.05uF -> 0.004545 ~= 220Hz ?
	 */
	counter_4017_1 -= (sound_latch_1 & HBEAT_RATE) ? 220 : 200;
	while (counter_4017_1 < 0)
	{
		counter_4017_1 += sample_rate;
		index_4017_1 = ++index_4017_1 % 5;
	}
	/*
	 * Ra is 10k, Rb is 470k, C is 0.1uF
	 * charge time t1 = 0.693 * (Ra + Rb) * C -> 0.033264 ~= 30Hz
	 * discharge time t2 = 0.693 * Rb * C -> 0.032571 ~= 31 Hz
	 */
	if (output_555)
	{
		counter_555 -= 31 * heart_beat_4017_1[index_4017_1] / 100000;
		while (counter_555 < 0)
		{
			counter_555 += sample_rate;
			/* count the second 4017 in the heart beat section */
			index_4017_2 = ++index_4017_2 % 10;
			output_555 = 0;
        }
	}
	else
	{
		counter_555 -= 30 * heart_beat_4017_1[index_4017_1] / 100000;
		while (counter_555 < 0)
		{
			counter_555 += sample_rate;
			output_555 = 1;
        }
	}

    /* R = 100k, C = 0.033uF -> ~= 440Hz */
	counter_filter_1 -= 440;
	while (counter_filter_1 < 0)
	{
		counter_filter_1 += sample_rate;
		/* for counter values 0 and 4 the upper filter is enabled. */
		if (index_4017_2 == 0 || index_4017_2 == 4)
			sample = noise[noise_count];
	}

	/* R = 470k, C = 0.01uF -> ~= 307Hz */
	counter_filter_2 -= 307;
	while (counter_filter_2 < 0)
	{
		counter_filter_2 += sample_rate;
		/* for counter value 1 the lower filter is enabled. */
		if (index_4017_2 == 1)
			sample = noise[noise_count];
	}

	return sample;
}

INLINE int moving_maze(void)
{
	static int counter_1;
	static int counter_2;
	static int counter_3;
	static int output;
	static int mix[8] = {0,10922,10922,21845,10922,21845,21845,32767};
	static int sample;

	if (sound_latch_2 & MOVING_MAZE)
		return 0;

	/* R1 = 120k, C1 = 0.047uF -> 5.64e-3 * 0.693 = 3.90852e-3 ~= 256Hz */
	counter_1 -= 256;
	while (counter_1 < 0)
	{
		counter_1 += sample_rate;
		output ^= 1;
	}
	/* R2 = 150k, C2 = 0.047uF -> 7.05e-3 * 0.693 = 4.88565e-3 ~= 205Hz */
	counter_2 -= 205;
	while (counter_2 < 0)
	{
		counter_2 += sample_rate;
		output ^= 2;
	}
	/* R3 = 220k, C3 = 0.047uF -> 10.34e-3 * 0.693 = 7.16562e-3 ~= 140Hz */
	counter_3 -= 140;
	while (counter_3 < 0)
	{
		counter_3 += sample_rate;
		output ^= 4;
	}

	/* smoother signals (low pass filters) */
	if (sample < mix[output])
		sample = (mix[output] - sample) / 2 + 1;
	else
	if (sample > mix[output])
		sample = (sample - mix[output]) / 2 + 1;

	return sample;
}

WRITE_HANDLER( pulsar_sh_port1_w )
{
	if (data == sound_latch_1)
		return;
	stream_update(channel,0);
	sound_latch_1 = data;

	if ((sound_latch_1 & ALIEN_SHOOT) == 0)
		alien_shoot_on = sample_rate / 4;

	if ((sound_latch_1 & PLAYER_SHOOT) == 0)
	{
		player_shoot_on = sample_rate / 4;
		player_shoot_freq = 32767;
	}
}

WRITE_HANDLER( pulsar_sh_port2_w )
{
	if (data == sound_latch_2)
		return;
	stream_update(channel,0);
	sound_latch_2 = data;
}

static void pulsar_update(int param, INT16 **buffer, int length)
{
	int offs;

	for(offs = 0; offs < length; offs++)
	{
        noise_count = ++noise_count & 32767;
		buffer[ 0][offs] = clang();
		buffer[ 1][offs] = key();
		buffer[ 2][offs] = alien_hit();
		buffer[ 3][offs] = player_hit();
		buffer[ 4][offs] = alien_shoot();
		buffer[ 5][offs] = player_shoot();
		buffer[ 6][offs] = bonus();
		buffer[ 7][offs] = sizzle();
		buffer[ 8][offs] = gate();
		buffer[ 9][offs] = birth();
		buffer[10][offs] = heart_beat();
        buffer[11][offs] = moving_maze();
	}
}

static const char *channel_names[] = {
	"CLANG",
	"KEY",
	"ALIEN HIT",
	"PLAYER HIT",
	"ALIEN SHOOT",
	"PLAYER SHOOT",
	"BONUS",
	"SIZZLE",
	"GATE",
	"BIRTH",
	"HEART BEAT",
	"MOVING MAZE"
};

static int channel_mixing_levels[] = {
	25, /* CLANG		*/
	25, /* KEY			*/
	25, /* ALIEN HIT	*/
	25, /* PLAYER HIT	*/
	25, /* ALIEN SHOOT	*/
	25, /* PLAYER SHOOT */
	25, /* BONUS		*/
	25, /* SIZZLE		*/
	25, /* GATE 		*/
	25, /* BIRTH		*/
	25, /* HEART-BEAT	*/
	25	/* MOVING MAZE	*/
};

int pulsar_sh_start(const struct MachineSound *msound)
{
	int i;

	decay = (INT16 *)malloc(32768 * sizeof(INT16));
	if( !decay )
		return 1;

	noise = (INT16 *)malloc(32768 * sizeof(INT16));
	if( !noise )
		return 1;

	for( i = 0; i < 0x8000; i++ )
		decay[0x7fff-i] = (INT16) (0x7fff/exp(1.0*i/4096));

	for( i = 0; i < 0x8000; i++ )
		noise[i] = rand();

	sample_rate = Machine->sample_rate;

	channel = stream_init_multi(12,
		channel_names, channel_mixing_levels, sample_rate, 0, pulsar_update);

	return 0;
}

void pulsar_sh_stop(void)
{
	if( decay )
		free(decay);
	decay = NULL;
	if( noise )
		free(noise);
	noise = NULL;
}

void pulsar_sh_update(void)
{
	stream_update(channel,0);
}

