/***************************************************************************

	Philips SAA1099 Sound driver

	This driver emulates a few of the SAA1099 functions

	Not implemented:
		* Noise Generators
		* Envelope Generators

	SAA1099 register layout:
	========================

	offs | 7654 3210 | description
	-----+-----------+---------------------------	
	0x00 | ---- xxxx | Amplitud channel 0 (left)
	0x00 | xxxx ---- | Amplitud channel 0 (right)	
	0x01 | ---- xxxx | Amplitud channel 1 (left)
	0x01 | xxxx ---- | Amplitud channel 1 (right)	
	0x02 | ---- xxxx | Amplitud channel 2 (left)
	0x02 | xxxx ---- | Amplitud channel 2 (right)	
	0x03 | ---- xxxx | Amplitud channel 3 (left)
	0x03 | xxxx ---- | Amplitud channel 3 (right)
	0x04 | ---- xxxx | Amplitud channel 4 (left)
	0x04 | xxxx ---- | Amplitud channel 4 (right)
	0x05 | ---- xxxx | Amplitud channel 5 (left)
	0x05 | xxxx ---- | Amplitud channel 5 (right)
		 |			 |
	0x08 | xxxx xxxx | Frequency channel 0
	0x09 | xxxx xxxx | Frequency channel 1
	0x0a | xxxx xxxx | Frequency channel 2
	0x0b | xxxx xxxx | Frequency channel 3
	0x0c | xxxx xxxx | Frequency channel 4
	0x0d | xxxx xxxx | Frequency channel 5
		 |			 |
	0x10 | ---- -xxx | Channel 0 octave select
	0x10 | -xxx ---- | Channel 1 octave select
	0x11 | ---- -xxx | Channel 2 octave select
	0x11 | -xxx ---- | Channel 3 octave select
	0x12 | ---- -xxx | Channel 4 octave select
	0x12 | -xxx ---- | Channel 5 octave select
		 |			 |
	0x14 | ---- ---x | Channel 0 frequency enable (0 = off, 1 = on)
	0x14 | ---- --x- | Channel 1 frequency enable (0 = off, 1 = on)
	0x14 | ---- -x-- | Channel 2 frequency enable (0 = off, 1 = on)
	0x14 | ---- x--- | Channel 3 frequency enable (0 = off, 1 = on)
	0x14 | ---x ---- | Channel 4 frequency enable (0 = off, 1 = on)
	0x14 | --x- ---- | Channel 5 frequency enable (0 = off, 1 = on)
		 |			 |
  	0x15 | ---- ---x | Channel 0 noise enable (0 = off, 1 = on)
	0x15 | ---- --x- | Channel 1 noise enable (0 = off, 1 = on)
	0x15 | ---- -x-- | Channel 2 noise enable (0 = off, 1 = on)
	0x15 | ---- x--- | Channel 3 noise enable (0 = off, 1 = on)
	0x15 | ---x ---- | Channel 4 noise enable (0 = off, 1 = on)
	0x15 | --x- ---- | Channel 5 noise enable (0 = off, 1 = on)
		 |			 |
	0x16 | ---- --xx | Noise generator parameters 0
	0x16 | --xx ---- | Noise generator parameters 1
		 |			 |
	0x18 | --xx xxxx | Envelope generator 0 parameters
	0x18 | x--- ---- | Envelope generator 0 control enable (0 = off, 1 = on)
	0x19 | --xx xxxx | Envelope generator 1 parameters
	0x19 | x--- ---- | Envelope generator 1 control enable (0 = off, 1 = on)
		 |			 |
	0x1c | ---- ---x | All channels enable (0 = off, 1 = on)
	0x1c | ---- --x- | Synch & Reset generators

***************************************************************************/

#include "driver.h"
#include <math.h>


#define LEFT	0x00
#define RIGHT	0x01

/* this structure defines a channel */
typedef struct
{	
	int frequency;					/* frequency (0x00..0xff) */
	int freq_enable;				/* frequency enable */
	int noise_enable;				/* noise enable */
	int octave;						/* octave (0x00..0x07) */
	int amplitude[2];				/* amplitude (0x00..0x0f) */

	/* vars to simulate the square wave */
	double counter;
	double incr;
	int level;
} saa1099_channel;

/* this structure defines a SAA1099 chip */
typedef struct
{
	int stream;						/* our stream */
	int noise_params[2];			/* noise generators parameters */
	int env_gen_enable[2];			/* envelope generators enable */
	int env_params[2];				/* envelope generators parameters */
	int all_ch_enable;				/* all channels enable */
	int sync_state;					/* sync all channels */
	saa1099_channel channels[6];	/* channels */
} sound_chip;

/* saa1099 chips */
static sound_chip chip_list[MAX_SAA1099];

static int selected_reg[MAX_SAA1099];	/* selected register */

/* global parameters */
static int sample_rate;



static void saa1099_update(int chip, INT16 **buffer, int length)
{
	int j, ch;
	int output1, output2;

	/* init output data */
	memset(buffer[LEFT],0,length*sizeof(INT16));
	memset(buffer[RIGHT],0,length*sizeof(INT16));

	/* if the channels are disabled we're done */
	if (!chip_list[chip].all_ch_enable) return;

	/* update incr based on the actual frequency and octave */
	for (ch = 0; ch < 6; ch++)
	{
		chip_list[chip].channels[ch].incr = ((double)(15625 << chip_list[chip].channels[ch].octave)/(511.0 - (double)chip_list[chip].channels[ch].frequency))/(double)sample_rate;
	}

	/* fill all data needed */
	for( j = 0; j < length; j++ )
	{
		output1 = 0;
		output2 = 0;

		/* for each channel */
		for (ch = 0; ch < 6; ch++)
		{
			/* if the square wave is disabled we're done */
			if (!chip_list[chip].channels[ch].freq_enable) continue;

			if (!chip_list[chip].sync_state)
			{
				/* check the actual position in the square wave */
				chip_list[chip].channels[ch].counter += chip_list[chip].channels[ch].incr;
				if (chip_list[chip].channels[ch].counter > 1.0)
				{
					chip_list[chip].channels[ch].counter -= 1.0;
					chip_list[chip].channels[ch].level ^= 0x016c;
				}

				/* calculate wave output taking the amplitude registers into account */
				output1 += chip_list[chip].channels[ch].amplitude[LEFT]*chip_list[chip].channels[ch].level;		/* range 0x0000..0x1554 for each channel */
				output2 += chip_list[chip].channels[ch].amplitude[RIGHT]*chip_list[chip].channels[ch].level;	/* range 0x0000..0x1554 for each channel */				
			}
		}

		/* write sound data to the buffer */
		buffer[LEFT][j] = output1/6;
		buffer[RIGHT][j] = output2/6;
	}
}



int saa1099_sh_start(const struct MachineSound *msound)
{
	int i, j;
	const struct saa1099_interface *intf = msound->sound_interface;	

	/* bag on a 0 sample_rate */
	if (Machine->sample_rate == 0)
		return 0;

	/* copy global parameters */
	sample_rate = Machine->sample_rate;

	/* for each chip allocate one stream */
	for (i = 0; i < intf->numchips; i++)
	{
		int vol[2];
		char buf[2][64];
		const char *name[2];

		memset(&chip_list[i], 0, sizeof(chip_list[i]));
		selected_reg[i] = 0;

		for (j = 0; j < 2; j++)
		{
			sprintf(buf[j], "SAA1099 #%d", i);
			name[j] = buf[j];
			vol[j] = MIXER(intf->volume[i][j], j ? MIXER_PAN_RIGHT : MIXER_PAN_LEFT);
		}
		chip_list[i].stream = stream_init_multi(2, name, vol, sample_rate, i, saa1099_update);
	}

	return 0;
}

void saa1099_sh_stop(void)
{
}

static void saa1099_control_port_w( int chip, int reg, int data )
{
	if ((data & 0xff) > 0x1c)
	{
		/* Error! */
		logerror("%04x: (SAA1099 #%d) Unknown register selected\n",cpu_get_pc(), chip);
	}
	
	selected_reg[chip] = data & 0x1f;
}


static void saa1099_write_port_w( int chip, int offset, int data )
{
	int reg = selected_reg[chip];

	switch (selected_reg[chip])
	{
	/* channel i amplitude */
	case 0x00:	case 0x01:	case 0x02:	case 0x03:	case 0x04:	case 0x05:
		chip_list[chip].channels[reg & 0x07].amplitude[LEFT] = data & 0x0f;
		chip_list[chip].channels[reg & 0x07].amplitude[RIGHT] = (data >> 4) & 0x0f;
		break;
	/* channel i frequency */
	case 0x08:	case 0x09:	case 0x0a:	case 0x0b:	case 0x0c:	case 0x0d:
		reg -= 0x08;
		if (chip_list[chip].channels[reg & 0x07].frequency != (data & 0xff))
		{
			chip_list[chip].channels[reg & 0x07].frequency = data & 0xff;
			chip_list[chip].channels[reg & 0x07].counter = 0.0;
		}
		break;
	/* channel i octave */
	case 0x10:	case 0x11:	case 0x12:
		reg -= 0x10;
		if (chip_list[chip].channels[(reg << 1) + 0].octave != (data & 0x07))
		{
			chip_list[chip].channels[(reg << 1) + 0].octave = data & 0x07;
			chip_list[chip].channels[(reg << 1) + 0].counter = 0.0;
		}
		if (chip_list[chip].channels[(reg << 1) + 1].octave != ((data >> 4) & 0x07))
		{
			chip_list[chip].channels[(reg << 1) + 1].octave = (data >> 4) & 0x07;
			chip_list[chip].channels[(reg << 1) + 1].counter = 0.0;
		}
		break;
	/* channel i frequency enable */
	case 0x14:
		chip_list[chip].channels[0].freq_enable = data & 0x01;
		chip_list[chip].channels[1].freq_enable = data & 0x02;
		chip_list[chip].channels[2].freq_enable = data & 0x04;
		chip_list[chip].channels[3].freq_enable = data & 0x08;
		chip_list[chip].channels[4].freq_enable = data & 0x10;
		chip_list[chip].channels[5].freq_enable = data & 0x20;
		break;
	/* channel i noise enable */
	case 0x15:
		if (data & 0xff)
		{
			logerror("%04x: (SAA1099 #%d) -reg 0x15- Noise not supported (%02x)\n",cpu_get_pc(), chip, data);
		}
		chip_list[chip].channels[0].noise_enable = data & 0x01;
		chip_list[chip].channels[1].noise_enable = data & 0x02;
		chip_list[chip].channels[2].noise_enable = data & 0x04;
		chip_list[chip].channels[3].noise_enable = data & 0x08;
		chip_list[chip].channels[4].noise_enable = data & 0x10;
		chip_list[chip].channels[5].noise_enable = data & 0x20;
		break;
	/* noise generators parameters */
	case 0x16:
		logerror("%04x: (SAA1099 #%d) -reg 0x16- Noise not supported (%02x)\n",cpu_get_pc(), chip, data);
		chip_list[chip].noise_params[0] = data & 0x03;
		chip_list[chip].noise_params[1] = (data >> 4) & 0x03;
		break;
	/* envelope generators parameters */
	case 0x18:	case 0x19:
		if (data & 0xbf)
		{
			logerror("%04x: (SAA1099 #%d) -reg 0x%02x- Envelope generators not supported (%02x)\n",cpu_get_pc(), chip, reg, data);
		}
		reg -= 0x18;
		chip_list[chip].env_params[reg] = data & 0x3f;
		chip_list[chip].env_gen_enable[reg] = data & 0x80;
		break;
	/* channels enable & reset generators */
	case 0x1c:
		chip_list[chip].all_ch_enable = data & 0x01;
		chip_list[chip].sync_state = data & 0x02;
		if (data & 0x02)
		{
			int i;

			/* Synch & Reset generators */
			logerror("%04x: (SAA1099 #%d) -reg 0x1c- Chip reset\n",cpu_get_pc(), chip);
			for (i = 0; i < 6; i++)
			{
				chip_list[chip].channels[i].incr = 0.0;
				chip_list[chip].channels[i].level = 0;
				chip_list[chip].channels[i].counter = 0.0;
			}
		}
		break;
	default:	/* Error! */
		logerror("%04x: (SAA1099 #%d) Unknown operation (reg:%02x, data:%02x)\n",cpu_get_pc(), chip, reg, data);
	}
}


/*******************************************
	SAA1099 interface functions
*******************************************/

WRITE_HANDLER( saa1099_control_port_0_w )
{
	saa1099_control_port_w(0, offset, data);
}

WRITE_HANDLER( saa1099_write_port_0_w )
{
	saa1099_write_port_w(0, offset, data);
}
