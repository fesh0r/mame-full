/**************************************************************************************
* Gameboy sound emulation (c) Anthony Kruize (trandor@labyrinth.net.au)
*
* Anyways, sound on the gameboy consists of 4 separate 'channels'
*   Sound1 = Quadrangular waves with SWEEP and ENVELOPE functions  (NR10,11,12,13,14)
*   Sound2 = Quadrangular waves with ENVELOPE functions (NR21,22,23,24)
*   Sound3 = Wave patterns from WaveRAM (NR30,31,32,33,34)
*   Sound4 = White noise with an envelope (NR41,42,43,44)
*
* Each sound channel has 2 modes, namely ON and OFF...  whoa
*
* These tend to be the two most important equations in
* converting between Hertz and GB frequency registers:
* (Sounds will have a 2.4% higher frequency on Super GB.)
*       gb = 2048 - (131072 / Hz)
*       Hz  = 131072 / (2048 - gb)
*
***************************************************************************************/

#include <math.h>
#include "driver.h"
#include "includes/gb.h"

#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23
#define NR50 0xFF24
#define NR51 0xFF25
#define NR52 0xFF26

#define GB_TO_HZ(x) (131072 / (2048 - x))
#define GBMODE3_TO_HZ(x) (65536 / (2048 - x))

static int channel = 1;
static int rate;

/* Table of wave dutyie.
   Represents wave duties of 12.5%, 25%, 50% and 75% */
static float wave_duty_table[4] = { 8, 4, 2, 1.333 };

/* Table of envelope lengths.
   Calculated with the formula: n * (1/64) and shifted to the left by 16 bits.
   Where n = 0-7 */
static INT32 env_length_table[8] = { 0, 1024, 2048, 3072, 4096, 5120, 6144, 7168 };

/* Table of sweep lengths.
   Calculated with the formula: n / 128hz
   Where n = 0-7 */
static INT32 swp_time_table[8] = { 0, 512, 1024, 1536, 2048, 2560, 3072, 3584 };

struct SOUND1
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	UINT32 frequency;
	INT32 count;
	INT8 signal;
	INT8 mode;
	INT8 duty;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
	INT32 swp_shift;
	INT32 swp_direction;
	INT32 swp_time;
	INT32 swp_count;
};

struct SOUND2
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	UINT32 frequency;
	INT32 count;
	INT8 signal;
	INT8 mode;
	INT8 duty;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
};

struct SOUND3
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	UINT32 frequency;
	INT32 count;
	INT8 signal;
	INT8 duty;
	INT8 mode;
	INT8 level;
};

struct SOUND4
{
	UINT8 on;
	INT32 length;
	INT32 frequency;
	INT32 count;
	INT16 signal;
	INT8 mode;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
	INT32 ply_frequency;
	INT32 ply_step;
	INT32 ply_ratio;
};

static struct SOUND1 snd_1;
static struct SOUND2 snd_2;
static struct SOUND3 snd_3;
static struct SOUND4 snd_4;

void gameboy_init_1(void)
{
	if( !snd_1.on )
		snd_1.pos = 0;
	snd_1.on = 1;
	snd_1.count = 0;
	snd_1.env_count = 0;
	snd_1.swp_count = 0;
	snd_1.signal = 0x1;
}

void gameboy_init_2(void)
{
	if( !snd_2.on )
		snd_2.pos = 0;
	snd_2.on = 1;
	snd_2.count = 0;
	snd_2.env_count = 0;
	snd_2.signal = 0x1;
}

void gameboy_3_init(void)
{
	if( !snd_3.on )
		snd_3.pos = 0;
	snd_3.on = 1;
	snd_3.count = 0;
	snd_3.signal = 0;
}

void gameboy_4_init(void)
{
	snd_4.on = 1;
	snd_4.count = 0;
	snd_4.env_count = 0;
}

void gameboy_update(int param, INT16 **buffer, int length);

void gameboy_sound_w(int offset, int data)
{
	/* change in registers so update first */
	stream_update(channel,0);

	switch( offset )
	{
	/*MODE 1 */
	case NR10: /* Sweep (R/W) */
		snd_1.swp_shift = data & 0x7;
		snd_1.swp_direction = (data & 0x8) >> 3;
		snd_1.swp_direction |= snd_1.swp_direction - 1;
		snd_1.swp_time = ( swp_time_table[ (data & 0x70) >> 4 ] * rate ) >> 15;
		break;
	case NR11: /* Sound length/Wave pattern duty (R/W) */
		snd_1.duty = (data & 0xc0) >> 6;
		snd_1.length = ((64 - (data & 0x3f)) * ((1<<16)/256) * rate) >> 16;
		break;
	case NR12: /* Envelope (R/W) */
		snd_1.env_value = data >> 4;
		snd_1.env_direction = (data & 0x8) >> 3;
		snd_1.env_direction |= snd_1.env_direction - 1;
		snd_1.env_length = ( env_length_table[data & 0x7] * rate ) >> 16;
		break;
	case NR13: /* Frequency lo (R/W) */
		snd_1.frequency = GB_TO_HZ((((gb_ram[NR14]&7)<<8) | gb_ram[NR13]));
		break;
	case NR14: /* Frequency hi / Initialize (R/W) */
		snd_1.frequency = GB_TO_HZ((((gb_ram[NR14]&7)<<8) | gb_ram[NR13]));
		if( data & 0x80 )
			gameboy_init_1();
		break;

	/*MODE 2 */
	case NR21: /* Sound length/Wave pattern duty (R/W) */
		snd_2.duty = (data & 0xc0) >> 6;
		snd_2.length = ((64 - (data & 0x3f)) * ((1<<16)/256) * rate) >> 16;
		break;
	case NR22: /* Envelope (R/W) */
		snd_2.env_value = data >> 4;
		snd_2.env_direction = (data & 0x8 ) >> 3;
		snd_2.env_direction |= snd_2.env_direction - 1;
		snd_2.env_length = ( env_length_table[data & 0x7] * rate ) >> 16;
		break;
	case NR23: /* Frequency lo (R/W) */
		snd_2.frequency = GB_TO_HZ((((gb_ram[NR24]&7)<<8) | gb_ram[NR23]));
		break;
	case NR24: /* Frequency hi / Initialize (R/W) */
		snd_2.mode = (data & 0x40) >> 6;
		snd_2.frequency = GB_TO_HZ((((gb_ram[NR24]&7)<<8) | gb_ram[NR23]));
		if( data & 0x80 )
			gameboy_init_2();
		break;

	/*MODE 3 */
	case NR30: /* Sound On/Off (R/W) */
		snd_3.on = (data & 0x80) >> 7;
		break;
	case NR31: /* Sound Length (R/W) */
		snd_3.length = ((256 - data) * ((1<<16)/2) * rate) >> 16;
		break;
	case NR32: /* Select Output Level */
		snd_3.level = (data & 0x60) >> 5;
		break;
	case NR33: /* Frequency lo (W) */
		snd_3.frequency = GBMODE3_TO_HZ(((gb_ram[NR34]&7)<<8) + gb_ram[NR33]);
		break;
	case NR34: /* Frequency hi / Initialize (W) */
		snd_3.mode = (data & 0x40) >> 6;
		snd_3.frequency = GBMODE3_TO_HZ(((gb_ram[NR34]&7)<<8) + gb_ram[NR33]);
		if( data & 0x80 )
			gameboy_3_init();
		break;

	/*MODE 4 */
	case NR41: /* Sound Length (R/W) */
		snd_4.length = ((64 - (data & 0x3f)) * ((1<<16)/256) * rate) >> 16;
		break;
	case NR42: /* Envelope (R/W) */
		snd_4.env_value = data >> 4;
		snd_4.env_direction = (data & 0x8 ) >> 3;
		snd_4.env_direction |= snd_4.env_direction - 1;
		snd_4.env_length = ( env_length_table[data & 0x7] * rate ) >> 16;
		break;
	case NR43: /* Polynomial Counter/Frequency */
		/* NEED TO SET POLYNOMIAL STUFF HERE */
		break;
	case NR44: /* Counter/Consecutive / Initialize (R/W)  */
		snd_4.mode = (data & 0x40) >> 6;
		if( data & 0x80 )
			gameboy_4_init();
		break;

	/*GENERAL */
	case NR50: /* Channel Control / On/Off / Volume (R/W)  */
		break;
	case NR51: /* Selection of Sound Output Terminal */
		break;
	case NR52: /* Sound On/Off (R/W) */
		logerror("NR52 - %x\n",data);
		snd_1.on = (data & 0x01);
		snd_2.on = (data & 0x02)>>1;
		snd_3.on = (data & 0x04)>>2;
		snd_4.on = (data & 0x08)>>3;
		break;

 	/*   0xFF30 - 0xFF3F = Wave Pattern RAM for arbitrary sound data */
	}
}

void gameboy_update(int param, INT16 **buffer, int length)
{
	INT16 sample, left, right;
	UINT32 /*clock,*/ period;

	while( length-- > 0 )
	{
		left = right = 0;

		/* Mode 1 - Wave with Envelope and Sweep */
		if( snd_1.on )
		{
			period = (1 << 16) / (snd_1.frequency) * rate;
			sample = snd_1.signal & snd_1.env_value;
			snd_1.pos++;
			if( snd_1.pos == (UINT32)(period / wave_duty_table[snd_1.duty]) >> 16)
			{
				snd_1.signal = -snd_1.signal;
			}
			else if( snd_1.pos > (period >> 16) )
			{
				snd_1.pos = 0;
				snd_1.signal = -snd_1.signal;
			}

			if( snd_1.length && snd_1.mode )
			{
				snd_1.count++;
				if( snd_1.count >= snd_1.length )
				{
					snd_1.on = 0;
				}
			}

			if( snd_1.env_length )
			{
				snd_1.env_count++;
				if( snd_1.env_count >= snd_1.env_length )
				{
					snd_1.env_count = 0;
					snd_1.env_value += snd_1.env_direction;
					if( snd_1.env_value < 0 )
						snd_1.env_value = 0;
					if( snd_1.env_value > 15 )
						snd_1.env_value = 15;
				}
			}

			/* Is this correct? */
			if( snd_1.swp_time && snd_1.swp_shift )
			{
				snd_1.swp_count++;
				if( snd_1.swp_count >= snd_1.swp_time )
				{
					snd_1.swp_count = 0;
					if( snd_1.swp_direction > 0 )
					{
						snd_1.frequency = snd_1.frequency - snd_1.frequency / (pow( 2.0, (double)snd_1.swp_shift));
						if( snd_1.frequency <= 0 )
							snd_1.on = 0;
					}
					else
						snd_1.frequency = snd_1.frequency + snd_1.frequency / (pow( 2.0, (double)snd_1.swp_shift));

				}
			}

			if( gb_ram[NR51] & 0x1 )
				right += sample;
			if( gb_ram[NR51] & 0x10 )
				left += sample;
		}

		/* Mode 2 - Wave with Envelope */
		if( snd_2.on )
		{
			period = (1 << 16) / (snd_2.frequency) * rate;
			sample = snd_2.signal & snd_2.env_value;
			snd_2.pos++;
			if( snd_2.pos == (UINT32)(period / wave_duty_table[snd_2.duty]) >> 16)
			{
				snd_2.signal = -snd_2.signal;
			}
			else if( snd_2.pos > (period >> 16) )
			{
				snd_2.pos = 0;
				snd_2.signal = -snd_2.signal;
			}

			if( snd_2.length && snd_2.mode )
			{
				snd_2.count++;
				if( snd_2.count >= snd_2.length )
				{
					snd_2.on = 0;
				}
			}

			if( snd_2.env_length )
			{
				snd_2.env_count++;
				if( snd_2.env_count > snd_2.env_length )
				{
					snd_2.env_count = 0;
					snd_2.env_value += snd_2.env_direction;
					if( snd_2.env_value < 0 )
						snd_2.env_value = 0;
					if( snd_2.env_value > 15 )
						snd_2.env_value = 15;
				}
			}

			if( gb_ram[NR51] & 0x2 )
				right += sample;
			if( gb_ram[NR51] & 0x20 )
				left += sample;
		}

		/* Mode 3 - Wave patterns from WaveRAM */
		if( snd_3.on )
		{
			/* TODO: Figure out how to use wave ram samples */

/*			period = (1 << 16) / (snd_3.frequency) * (rate / 5);
			sample = gb_ram[0xFF30 + snd_3.signal];
			if( snd_3.duty & 0x1 )
			{
				sample &= 0xf;
			}
			else
			{
				sample >> 4;
			}

			sample &= 0xf;
			sample -= 8;

			sample >>= snd_3.level - 1;

			snd_3.pos++;
			snd_3.duty != snd_3.duty;
			if( snd_3.pos > (UINT32)(period >> 16) )
			{
				snd_3.pos = 0;
				if( !snd_3.duty )
				{
					snd_3.signal++;
					if( snd_3.signal > 15 )
						snd_3.signal = 0;
				}
			}

			if( !snd_3.level )
			{
				sample = 0;
			}

			if( snd_3.length && snd_3.mode )
			{
				snd_3.count++;
				if( snd_3.count >= snd_3.length )
				{
					snd_3.on = 0;
				}
			}*/

			/* output nothing for now */
			sample = 0;

			if( gb_ram[NR51] & 0x4 )
				right += sample;
			if( gb_ram[NR51] & 0x40 )
				left += sample;
		}

		/* Mode 4 - Noise with Envelope */
		if( snd_4.on )
		{
			/* Below is a hack, a big hack, but it will do until we figure out
			   a proper polynomial white noise generator. */
			sample = rand() & snd_4.env_value;

			if( snd_4.length && snd_4.mode )
			{
				snd_4.count++;
				if( snd_4.count >= snd_4.length )
				{
					snd_4.on = 0;
				}
			}

			if( snd_4.env_length )
			{
				snd_4.env_count++;
				if( snd_4.env_count >= snd_4.env_length )
				{
					snd_4.env_count = 0;
					snd_4.env_value += snd_4.env_direction;
					if( snd_4.env_value < 0 )
						snd_4.env_value = 0;
					if( snd_4.env_value > 15 )
						snd_4.env_value = 15;
				}
			}

			if( gb_ram[NR51] & 0x8 )
				right += sample;
			if( gb_ram[NR51] & 0x80 )
				left += sample;
		}

		/* Adjust for volume */
		left *= (gb_ram[NR50] & 0x7);
		right *= ((gb_ram[NR50] & 0x70)>>4);

		left <<= 4;
		right <<= 4;

		/* Update the buffers */
		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}

	gb_ram[NR52] = (gb_ram[NR52]&0xf0) | snd_1.on | (snd_2.on<<1) | (snd_3.on<<2) | (snd_4.on<<3);
}

int gameboy_sh_start(const struct MachineSound* driver)
{
	const char *names[2] = { "Gameboy left", "Gameboy right" };
	const int volume[2] = { MIXER( 50, MIXER_PAN_LEFT ), MIXER( 50, MIXER_PAN_RIGHT ) };

	memset(&snd_1, 0, sizeof(snd_1));
	memset(&snd_2, 0, sizeof(snd_2));
	memset(&snd_3, 0, sizeof(snd_3));
	memset(&snd_4, 0, sizeof(snd_4));

	channel = stream_init_multi(2, names, volume, Machine->sample_rate, 0, gameboy_update);

	rate = Machine->sample_rate;

	return 0;
}
