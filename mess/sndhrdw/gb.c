/**************************************************************************************
* Gameboy sound emulation (c) Ben Bruscella (ben@mame.net) <--- what shit!
*                           + Anthony Kruize (trandor@labyrinth.net.au)
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

static int channel = 1;
static int rate;

struct SOUND1
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	INT32 frequency;
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
};

struct SOUND2
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	INT32 frequency;
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
	INT32 frequency;
	INT32 count;
	INT8 signal;
	INT8 mode;
};

struct SOUND4
{
	UINT8 on;
	INT32 length;
	INT32 pos;
	INT32 frequency;
	INT32 count;
	INT16 signal;
	INT8 mode;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
	INT32 ply_count;
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
	snd_1.signal = (UINT8)0xff;
}

void gameboy_init_2(void)
{
	if( !snd_2.on )
		snd_2.pos = 0;
	snd_2.on = 1;
	snd_2.count = 0;
	snd_2.env_count = 0;
	snd_2.signal = (UINT8)0xff;
}

void snd_3_init(void)
{
	if( !snd_3.on )
		snd_3.pos = 0;
	snd_3.on = 1;
	snd_3.count = 0;

}

void snd_4_init(void)
{
	snd_4.on = 1;
	snd_4.pos = 0;
	snd_4.count = 0;
	snd_4.env_count = 0;
}

void gameboy_update(int param, INT16 **buffer, int length);

void gameboy_sound_w(int offset, int data)
{
	/* change in registers so update first */
	stream_update(channel,0);

	/* copy the data to ram */
	gb_ram[offset] = data;

	switch( offset )
	{
	/*MODE 1 */
	case NR10: /* Sweep (R/W) */
		snd_1.swp_shift = data & 0x7;
		snd_1.swp_direction = (data & 0x8) >> 3;
		snd_1.swp_direction |= snd_1.swp_direction - 1;
		snd_1.swp_time = (data & 0x70) >> 4;
		break;
	case NR11: /* Sound length/Wave pattern duty (R/W) */
		snd_1.duty = (data & 0xc0) >> 6;
		snd_1.length = (data & 0x3f) << 15;
		break;
	case NR12: /* Envelope (R/W) */
		snd_1.env_value = data >> 4;
		snd_1.env_direction = (data & 0x8) >> 3;
		snd_1.env_direction |= snd_1.env_direction - 1;
		snd_1.env_length = (data & 0x7) << 15;
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
		snd_2.length = (data & 0x3f) << 15;
		break;
	case NR22: /* Envelope (R/W) */
		snd_2.env_value = data >> 4;
		snd_2.env_direction = (data & 0x8 ) >> 3;
		snd_2.env_direction |= snd_2.env_direction - 1;
		snd_2.env_length = (data & 0x7) << 15;
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
		snd_3.on = (data & 0x80)>>7;
		break;
	case NR31: /* Sound Length (R/W) */
		snd_3.length = data;
		break;
	case NR32: /* Select Output Level */
		/* NEED TO FILL THIS IN */
		break;
	case NR33: /* Frequency lo (W) */
		snd_3.frequency = GB_TO_HZ(((gb_ram[NR34]&7)<<8) + gb_ram[NR33]);
		break;
	case NR34: /* Frequency hi / Initialize (W) */
		snd_3.frequency = GB_TO_HZ(((gb_ram[NR34]&7)<<8) + gb_ram[NR33]);
		if( data & 0x80 )
			snd_3_init();
		break;

	/*MODE 4 */
	case NR41: /* Sound Length (R/W) */
		snd_4.length = (data & 0x3f) << 2;
		break;
	case NR42: /* Envelope (R/W) */
		snd_4.env_value = data >> 4;
		snd_4.env_direction = (data & 0x8 ) >> 3;
		snd_4.env_direction |= snd_4.env_direction - 1;
		snd_4.env_length = (data & 0x7) << 15;
		break;
	case NR43: /* Polynomial Counter/Frequency */
		/* NEED TO SET POLYNOMIAL STUFF HERE */
		break;
	case NR44: /* Counter/Consecutive / Initialize (R/W)  */
		if( data & 0x80 )
			snd_4_init();
		break;

	/*GENERAL */
	case NR50: /* Channel Control / On/Off / Volume (R/W)  */
		break;
	case NR51: /* Selection of Sound Output Terminal */
		break;
	case NR52: /* Sound On/Off (R/W) */
		logerror("NR52 - %x\n",data);
		snd_1.on = (data & 0x01);
		/* logerror("Sound 1 = %02x\n",snd_1->on); */
		snd_2.on = (data & 0x02)>>1;
		/* logerror("Sound 2 = %02x\n",snd_2->on); */
		snd_3.on = (data & 0x04)>>2;
		/* logerror("Sound 3 = %02x\n",snd_3->on); */
		snd_4.on = (data & 0x08)>>3;
		/* logerror("Sound 4 = %02x\n",snd_4->on); */
		break;

 	/*   0xFF30 - 0xFF3F = Wave Pattern RAM for arbitrary sound data */
	}
}

void gameboy_update(int param, INT16 **buffer, int length)
{
	INT16 sample, left, right;
	INT32 clock;

	while( length-- > 0 )
	{
		left = right = 0;

		/* Mode 1 - Wave with Envelope and Sweep */
		if( snd_1.on )
		{
			/* TODO: Handle wave duty, counter/consecutive mode and sweep mode */
			clock = snd_1.frequency;
			sample = snd_1.signal & snd_1.env_value;
			snd_1.pos -= clock;
			while( snd_1.pos < 0 )
			{
				snd_1.pos += rate;
				snd_1.signal = -snd_1.signal;
			}

			if( snd_1.length )
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
					snd_1.env_value += snd_1.env_direction;
					if( snd_1.env_value < 0 )
						snd_1.env_value = 0;
					if( snd_1.env_value > 15 )
						snd_1.env_value = 15;
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
			/* TODO: Handle wave duty, counter/consecutive mode */
			clock = snd_2.frequency;
			sample = snd_2.signal & snd_2.env_value;
			snd_2.pos -= clock;
			while( snd_2.pos < 0 )
			{
				snd_2.pos += rate;
				snd_2.signal = -snd_2.signal;
			}

			if( snd_2.length )
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
				if( snd_2.env_count >= snd_2.env_length )
				{
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
			sample = 0;

			if( gb_ram[NR51] & 0x4 )
				right += sample;
			if( gb_ram[NR51] & 0x40 )
				left += sample;
		}

		/* Mode 4 - Noise with Envelope */
		if( snd_4.on )
		{
			/* TODO: Figure out how to do noise samples */
			sample = 0;

			if( snd_4.env_length )
			{
				snd_4.env_count++;
				if( snd_4.env_count >= snd_4.env_length )
				{
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
	const int volume[2] = { MIXER( 100, MIXER_PAN_LEFT ), MIXER( 100, MIXER_PAN_RIGHT ) };

	memset(&snd_1, 0, sizeof(snd_1));
	memset(&snd_2, 0, sizeof(snd_2));
	memset(&snd_3, 0, sizeof(snd_3));
	memset(&snd_4, 0, sizeof(snd_4));

	channel = stream_init_multi(2, names, volume, Machine->sample_rate, 0, gameboy_update);

	rate = Machine->sample_rate / 2;

	return 0;
}
