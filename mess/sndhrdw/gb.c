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
*       Hz = 131072 / (2048 - gb)
*
* Changes:
*
*	10/2/2002		AK - Preliminary sound code.
*	13/2/2002		AK - Added a hack for mode 4, other fixes.
*	23/2/2002		AK - Use lookup tables, added sweep to mode 1. Re-wrote the square
*						 wave generation.
*	13/3/2002		AK - Added mode 3, better lookup tables, other adjustments.
*	15/3/2002		AK - Mode 4 can now change frequencies.
*	31/3/2002		AK - Accidently forgot to handle counter/consecutive for mode 1.
*	 3/4/2002		AK - Mode 1 sweep can still occur if shift is 0.  Don't let frequency
*						 go past the maximum allowed value. Fixed Mode 3 length table.
*						 Slight adjustment to Mode 4's period table generation.
*	 5/4/2002		AK - Mode 4 is done correctly, using a polynomial counter instead
*						 of being a total hack.
*	 6/4/2002		AK - Slight tweak to mode 3's frequency calculation.
*	13/4/2002		AK - Reset envelope value when sound is initialized.
*	21/4/2002		AK - Backed out the mode 3 frequency calculation change.
*						 Merged init functions into gameboy_sound_w().
*	14/5/2002		AK - Removed magic numbers in the fixed point math.
*	12/6/2002		AK - Merged SOUNDx structs into one SOUND struct.
*  26/10/2002		AK - Finally fixed channel 3!
*
***************************************************************************************/

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

#define LEFT 1
#define RIGHT 2
#define MAX_FREQUENCIES 2048
#define FIXED_POINT 16

static sound_stream *channel;
static int rate;

/* Represents wave duties of 12.5%, 25%, 50% and 75% */
static float wave_duty_table[4] = { 8, 4, 2, 1.33 };
static INT32 env_length_table[8];
static INT32 swp_time_table[8];
static UINT32 period_table[MAX_FREQUENCIES];
static UINT32 period_mode3_table[MAX_FREQUENCIES];
static UINT32 period_mode4_table[8][16];
static UINT32 length_table[64];
static UINT32 length_mode3_table[256];

struct SOUND
{
	/* Common */
	UINT8  on;
	UINT8  channel;
	INT32  length;
	INT32  pos;
	UINT32 period;
	INT32  count;
	INT8   mode;
	/* Mode 1, 2, 3 */
	INT8   duty;
	/* Mode 1, 2, 4 */
	INT32  env_value;
	INT8   env_direction;
	INT32  env_length;
	INT32  env_count;
	INT8   signal;
	/* Mode 1 */
	UINT32 frequency;
	INT32  swp_shift;
	INT32  swp_direction;
	INT32  swp_time;
	INT32  swp_count;
	/* Mode 3 */
	INT8   level;
	UINT8  offset;
	UINT32 dutycount;
	/* Mode 4 */
	INT32  ply_step;
	INT16  ply_value;
};

struct SOUNDC
{
	UINT8 on;
	UINT8 vol_left;
	UINT8 vol_right;
	UINT8 mode1_left;
	UINT8 mode1_right;
	UINT8 mode2_left;
	UINT8 mode2_right;
	UINT8 mode3_left;
	UINT8 mode3_right;
	UINT8 mode4_left;
	UINT8 mode4_right;
};

static struct SOUND  snd_1;
static struct SOUND  snd_2;
static struct SOUND  snd_3;
static struct SOUND  snd_4;
static struct SOUNDC snd_control;

static void gameboy_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length);



WRITE8_HANDLER( gb_sound_w )
{
	offset += 0xFF10;

	/* change in registers so update first */
	stream_update(channel, 0);

	/* Only register NR52 is accessible if the sound controller is disabled */
	if( !snd_control.on && offset != NR52 )
	{
		return;
	}

	/* Store the value */
	gb_ram[offset] = data;

	switch( offset )
	{
	/*MODE 1 */
	case NR10: /* Sweep (R/W) */
		snd_1.swp_shift = data & 0x7;
		snd_1.swp_direction = (data & 0x8) >> 3;
		snd_1.swp_direction |= snd_1.swp_direction - 1;
		snd_1.swp_time = swp_time_table[ (data & 0x70) >> 4 ];
		break;
	case NR11: /* Sound length/Wave pattern duty (R/W) */
		snd_1.duty = (data & 0xC0) >> 6;
		snd_1.length = length_table[data & 0x3F];
		break;
	case NR12: /* Envelope (R/W) */
		snd_1.env_value = data >> 4;
		snd_1.env_direction = (data & 0x8) >> 3;
		snd_1.env_direction |= snd_1.env_direction - 1;
		snd_1.env_length = env_length_table[data & 0x7];
		break;
	case NR13: /* Frequency lo (R/W) */
		snd_1.frequency = ((gb_ram[NR14]&0x7)<<8) | gb_ram[NR13];
		snd_1.period = period_table[snd_1.frequency];
		break;
	case NR14: /* Frequency hi / Initialize (R/W) */
		snd_1.mode = (data & 0x40) >> 6;
		snd_1.frequency = ((gb_ram[NR14]&0x7)<<8) | gb_ram[NR13];
		snd_1.period = period_table[snd_1.frequency];
		if( data & 0x80 )
		{
			if( !snd_1.on )
				snd_1.pos = 0;
			snd_1.on = 1;
			snd_1.count = 0;
			snd_1.env_value = gb_ram[NR12] >> 4;
			snd_1.env_count = 0;
			snd_1.swp_count = 0;
			snd_1.signal = 0x1;
			gb_ram[NR52] |= 0x1;
		}
		break;

	/*MODE 2 */
	case NR21: /* Sound length/Wave pattern duty (R/W) */
		snd_2.duty = (data & 0xC0) >> 6;
		snd_2.length = length_table[data & 0x3F];
		break;
	case NR22: /* Envelope (R/W) */
		snd_2.env_value = data >> 4;
		snd_2.env_direction = (data & 0x8 ) >> 3;
		snd_2.env_direction |= snd_2.env_direction - 1;
		snd_2.env_length = env_length_table[data & 0x7];
		break;
	case NR23: /* Frequency lo (R/W) */
		snd_2.period = period_table[((gb_ram[NR24]&0x7)<<8) | gb_ram[NR23]];
		break;
	case NR24: /* Frequency hi / Initialize (R/W) */
		snd_2.mode = (data & 0x40) >> 6;
		snd_2.period = period_table[((gb_ram[NR24]&0x7)<<8) | gb_ram[NR23]];
		if( data & 0x80 )
		{
			if( !snd_2.on )
				snd_2.pos = 0;
			snd_2.on = 1;
			snd_2.count = 0;
			snd_2.env_value = gb_ram[NR22] >> 4;
			snd_2.env_count = 0;
			snd_2.signal = 0x1;
			gb_ram[NR52] |= 0x2;
		}
		break;

	/*MODE 3 */
	case NR30: /* Sound On/Off (R/W) */
		snd_3.on = (data & 0x80) >> 7;
		break;
	case NR31: /* Sound Length (R/W) */
		snd_3.length = length_mode3_table[data];
		break;
	case NR32: /* Select Output Level */
		snd_3.level = (data & 0x60) >> 5;
		break;
	case NR33: /* Frequency lo (W) */
		snd_3.period = period_mode3_table[((gb_ram[NR34]&0x7)<<8) + gb_ram[NR33]];
		break;
	case NR34: /* Frequency hi / Initialize (W) */
		snd_3.mode = (data & 0x40) >> 6;
		snd_3.period = period_mode3_table[((gb_ram[NR34]&0x7)<<8) + gb_ram[NR33]];
		if( data & 0x80 )
		{
			if( !snd_3.on )
			{
				snd_3.pos = 0;
				snd_3.offset = 0;
				snd_3.duty = 0;
			}
			snd_3.on = 1;
			snd_3.count = 0;
			snd_3.duty = 1;
			snd_3.dutycount = 0;
			gb_ram[NR52] |= 0x4;
		}
		break;

	/*MODE 4 */
	case NR41: /* Sound Length (R/W) */
		snd_4.length = length_table[data & 0x3F];
		break;
	case NR42: /* Envelope (R/W) */
		snd_4.env_value = data >> 4;
		snd_4.env_direction = (data & 0x8 ) >> 3;
		snd_4.env_direction |= snd_4.env_direction - 1;
		snd_4.env_length = env_length_table[data & 0x7];
		break;
	case NR43: /* Polynomial Counter/Frequency */
		snd_4.period = period_mode4_table[data & 0x7][(data & 0xF0) >> 4];
		snd_4.ply_step = (data & 0x8) >> 3;
		break;
	case NR44: /* Counter/Consecutive / Initialize (R/W)  */
		snd_4.mode = (data & 0x40) >> 6;
		if( data & 0x80 )
		{
			if( !snd_4.on )
				snd_4.pos = 0;
			snd_4.on = 1;
			snd_4.count = 0;
			snd_4.env_value = gb_ram[NR42] >> 4;
			snd_4.env_count = 0;
			snd_4.signal = rand();
			snd_4.ply_value = 0x7fff;
			gb_ram[NR52] |= 0x8;
		}
		break;

	/* CONTROL */
	case NR50: /* Channel Control / On/Off / Volume (R/W)  */
		snd_control.vol_left = data & 0x7;
		snd_control.vol_right = (data & 0x70) >> 4;
		break;
	case NR51: /* Selection of Sound Output Terminal */
		snd_control.mode1_right = data & 0x1;
		snd_control.mode1_left = (data & 0x10) >> 4;
		snd_control.mode2_right = (data & 0x2) >> 1;
		snd_control.mode2_left = (data & 0x20) >> 5;
		snd_control.mode3_right = (data & 0x4) >> 2;
		snd_control.mode3_left = (data & 0x40) >> 6;
		snd_control.mode4_right = (data & 0x8) >> 3;
		snd_control.mode4_left = (data & 0x80) >> 7;
		break;
	case NR52: /* Sound On/Off (R/W) */
		/* Only bit 7 is writable, writing to bits 0-3 does NOT enable or
		   disable sound.  They are read-only */
		snd_control.on = (data & 0x80) >> 7;
		if( !snd_control.on )
		{
			snd_1.on = 0;
			snd_2.on = 0;
			snd_3.on = 0;
			snd_4.on = 0;
			gb_ram[offset] = 0;
		}
		break;
	}
}



static void gameboy_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	stream_sample_t sample, left, right, mode4_mask;

	while( length-- > 0 )
	{
		left = right = 0;

		/* Mode 1 - Wave with Envelope and Sweep */
		if( snd_1.on )
		{
			sample = snd_1.signal * snd_1.env_value;
			snd_1.pos++;
			if( snd_1.pos == (UINT32)(snd_1.period / wave_duty_table[snd_1.duty]) >> FIXED_POINT)
			{
				snd_1.signal = -snd_1.signal;
			}
			else if( snd_1.pos > (snd_1.period >> FIXED_POINT) )
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
					gb_ram[NR52] &= 0xFE;
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

			if( snd_1.swp_time )
			{
				snd_1.swp_count++;
				if( snd_1.swp_count >= snd_1.swp_time )
				{
					snd_1.swp_count = 0;
					if( snd_1.swp_direction > 0 )
					{
						snd_1.frequency -= snd_1.frequency / (1 << snd_1.swp_shift );
						if( snd_1.frequency <= 0 )
						{
							snd_1.on = 0;
							gb_ram[NR52] &= 0xFE;
						}
					}
					else
					{
						snd_1.frequency += snd_1.frequency / (1 << snd_1.swp_shift );
						if( snd_1.frequency >= MAX_FREQUENCIES )
						{
							snd_1.frequency = MAX_FREQUENCIES - 1;
						}
					}

					snd_1.period = period_table[snd_1.frequency];
				}
			}

			if( snd_control.mode1_left )
				left += sample;
			if( snd_control.mode1_right )
				right += sample;
		}

		/* Mode 2 - Wave with Envelope */
		if( snd_2.on )
		{
			sample = snd_2.signal * snd_2.env_value;
			snd_2.pos++;
			if( snd_2.pos == (UINT32)(snd_2.period / wave_duty_table[snd_2.duty]) >> FIXED_POINT)
			{
				snd_2.signal = -snd_2.signal;
			}
			else if( snd_2.pos > (snd_2.period >> FIXED_POINT) )
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
					gb_ram[NR52] &= 0xFD;
				}
			}

			if( snd_2.env_length )
			{
				snd_2.env_count++;
				if( snd_2.env_count >= snd_2.env_length )
				{
					snd_2.env_count = 0;
					snd_2.env_value += snd_2.env_direction;
					if( snd_2.env_value < 0 )
						snd_2.env_value = 0;
					if( snd_2.env_value > 15 )
						snd_2.env_value = 15;
				}
			}

			if( snd_control.mode2_left )
				left += sample;
			if( snd_control.mode2_right )
				right += sample;
		}

		/* Mode 3 - Wave patterns from WaveRAM */
		if( snd_3.on )
		{
			/* NOTE: This is extremely close, but not quite right.
			   The problem is for GB frequencies above 2000 the frequency gets
			   clipped. This is caused because snd_3.pos is never 0 at the test.*/
			sample = gb_ram[0xFF30 + (snd_3.offset/2)];
			if( !(snd_3.offset % 2) )
			{
				sample >>= 4;
			}
			sample = (sample & 0xF) - 8;

			if( snd_3.level )
				sample >>= (snd_3.level - 1);
			else
				sample = 0;

			snd_3.pos++;
			if( snd_3.pos >= ((UINT32)(((snd_3.period ) >> 21)) + snd_3.duty) )
			{
				snd_3.pos = 0;
				if( snd_3.dutycount == ((UINT32)(((snd_3.period ) >> FIXED_POINT)) % 32) )
				{
					snd_3.duty--;
				}
				snd_3.dutycount++;
				snd_3.offset++;
				if( snd_3.offset > 31 )
				{
					snd_3.offset = 0;
					snd_3.duty = 1;
					snd_3.dutycount = 0;
				}
			}

			if( snd_3.length && snd_3.mode )
			{
				snd_3.count++;
				if( snd_3.count >= snd_3.length )
				{
					snd_3.on = 0;
					gb_ram[NR52] &= 0xFB;
				}
			}

			if( snd_control.mode3_left )
				left += sample;
			if( snd_control.mode3_right )
				right += sample;
		}

		/* Mode 4 - Noise with Envelope */
		if( snd_4.on )
		{
			/* Similar problem to Mode 3, we seem to miss some notes */
			sample = snd_4.signal & snd_4.env_value;
			snd_4.pos++;
			if( snd_4.pos == (snd_4.period >> (FIXED_POINT + 1)) )
			{
				/* Using a Polynomial Counter (aka Linear Feedback Shift Register)
				   Mode 4 has a 7 bit and 15 bit counter so we need to shift the
				   bits around accordingly */
				mode4_mask = (((snd_4.ply_value & 0x2) >> 1) ^ (snd_4.ply_value & 0x1)) << (snd_4.ply_step ? 6 : 14);
				snd_4.ply_value >>= 1;
				snd_4.ply_value |= mode4_mask;
				snd_4.ply_value &= (snd_4.ply_step ? 0x7f : 0x7fff);
				snd_4.signal = (INT8)snd_4.ply_value;
			}
			else if( snd_4.pos > (snd_4.period >> FIXED_POINT) )
			{
				snd_4.pos = 0;
				mode4_mask = (((snd_4.ply_value & 0x2) >> 1) ^ (snd_4.ply_value & 0x1)) << (snd_4.ply_step ? 6 : 14);
				snd_4.ply_value >>= 1;
				snd_4.ply_value |= mode4_mask;
				snd_4.ply_value &= (snd_4.ply_step ? 0x7f : 0x7fff);
				snd_4.signal = (INT8)snd_4.ply_value;
			}

			if( snd_4.length && snd_4.mode )
			{
				snd_4.count++;
				if( snd_4.count >= snd_4.length )
				{
					snd_4.on = 0;
					gb_ram[NR52] &= 0xF7;
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

			if( snd_control.mode4_left )
				left += sample;
			if( snd_control.mode4_right )
				right += sample;
		}

		/* Adjust for master volume */
		left *= snd_control.vol_left;
		right *= snd_control.vol_right;

		/* pump up the volume */
		left <<= 6;
		right <<= 6;

		/* Update the buffers */
		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}

	gb_ram[NR52] = (gb_ram[NR52]&0xf0) | snd_1.on | (snd_2.on << 1) | (snd_3.on << 2) | (snd_4.on << 3);
}



void *gameboy_sh_start(int clock, const struct CustomSound_interface *config)
{
	int I, J;

	memset(&snd_1, 0, sizeof(snd_1));
	memset(&snd_2, 0, sizeof(snd_2));
	memset(&snd_3, 0, sizeof(snd_3));
	memset(&snd_4, 0, sizeof(snd_4));

	channel = stream_create(0, 2, Machine->sample_rate, 0, gameboy_update);
	rate = Machine->sample_rate;

	/* Calculate the envelope and sweep tables */
	for( I = 0; I < 8; I++ )
	{
		env_length_table[I] = (I * ((1 << FIXED_POINT) / 64) * rate) >> FIXED_POINT;
		swp_time_table[I] = (((I << FIXED_POINT) / 128) * rate) >> (FIXED_POINT - 1);
	}

	/* Calculate the period tables */
	for( I = 0; I < MAX_FREQUENCIES; I++ )
	{
		period_table[I] = ((1 << FIXED_POINT) / (131072 / (2048 - I))) * rate;
		period_mode3_table[I] = ((1 << FIXED_POINT) / (65536 / (2048 - I))) * rate;
	}
	/* Calculate the period table for mode 4 */
	for( I = 0; I < 8; I++ )
	{
		for( J = 0; J < 16; J++ )
		{
			/* I is the dividing ratio of frequencies
			   J is the shift clock frequency */
			period_mode4_table[I][J] = ((1 << FIXED_POINT) / (524288 / ((I == 0)?0.5:I) / (1 << (J + 1)))) * rate;
		}
	}

	/* Calculate the length table */
	for( I = 0; I < 64; I++ )
	{
		length_table[I] = ((64 - I) * ((1 << FIXED_POINT)/256) * rate) >> FIXED_POINT;
	}
	/* Calculate the length table for mode 3 */
	for( I = 0; I < 256; I++ )
	{
		length_mode3_table[I] = ((256 - I) * ((1 << FIXED_POINT)/256) * rate) >> FIXED_POINT;
	}

	return 0;
}
