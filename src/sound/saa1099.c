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
	0x00 | ---- xxxx | Amplitude channel 0 (left)
	0x00 | xxxx ---- | Amplitude channel 0 (right)
	0x01 | ---- xxxx | Amplitude channel 1 (left)
	0x01 | xxxx ---- | Amplitude channel 1 (right)
	0x02 | ---- xxxx | Amplitude channel 2 (left)
	0x02 | xxxx ---- | Amplitude channel 2 (right)
	0x03 | ---- xxxx | Amplitude channel 3 (left)
	0x03 | xxxx ---- | Amplitude channel 3 (right)
	0x04 | ---- xxxx | Amplitude channel 4 (left)
	0x04 | xxxx ---- | Amplitude channel 4 (right)
	0x05 | ---- xxxx | Amplitude channel 5 (left)
	0x05 | xxxx ---- | Amplitude channel 5 (right)
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
#include "saa1099.h"
#include <math.h>


#define LEFT	0x00
#define RIGHT	0x01

/* this structure defines a channel */
struct saa1099_channel
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
};

/* this structure defines a noise channel */
struct saa1099_noise
{
	/* vars to simulate the noise generator output */
	double counter;
	double incr;
	int level;						/* noise polynomal shifter */
};

/* this structure defines a SAA1099 chip */
struct SAA1099
{
	int stream;						/* our stream */
	int noise_params[2];			/* noise generators parameters */
	int env_gen_enable[2];			/* envelope generators enable */
	int env_params[2];				/* envelope generators parameters */
	int all_ch_enable;				/* all channels enable */
	int sync_state;					/* sync all channels */
	int selected_reg;				/* selected register */
	struct saa1099_channel channels[6];    /* channels */
	struct saa1099_noise noise[2];	/* noise generators */
};

/* saa1099 chips */
static struct SAA1099 saa1099[MAX_SAA1099];

/* global parameters */
static int sample_rate;



static void saa1099_update(int chip, INT16 **buffer, int length)
{
	struct SAA1099 *saa = &saa1099[chip];
    int j, ch;

	/* if the channels are disabled we're done */
	if (!saa->all_ch_enable)
	{
		/* init output data */
		memset(buffer[LEFT],0,length*sizeof(INT16));
		memset(buffer[RIGHT],0,length*sizeof(INT16));
        return;
	}

	/* update incr based on the actual frequency and octave */
	for (ch = 0; ch < 6; ch++)
	{
		saa->channels[ch].incr = (double)(15625 << saa->channels[ch].octave) /
			(511.0 - (double)saa->channels[ch].frequency) /
			(double)sample_rate;
	}

	for (ch = 0; ch < 2; ch++)
	{
		switch (saa->noise_params[ch])
		{
		case 0: saa->noise[ch].incr += 31250.0 / (double)sample_rate; break;
		case 1: saa->noise[ch].incr += 15625.0 / (double)sample_rate; break;
		case 2: saa->noise[ch].incr +=	7812.5 / (double)sample_rate; break;
		case 3: saa->noise[ch].incr = saa->channels[ch * 3].incr; break;
		}
	}

    /* fill all data needed */
	for( j = 0; j < length; j++ )
	{
		int output_l = 0, output_r = 0, active = 6;

		for (ch = 0; ch < 2; ch++)
		{
			if (!saa->sync_state)
			{
				/* check the actual position in noise generator */
				saa->noise[ch].counter += saa->noise[ch].incr;
				while (saa->noise[ch].counter > 1.0)
                {
					saa->noise[ch].counter -= 1.0;
					if( ((saa->noise[ch].level & 0x4000) == 0) == ((saa->noise[ch].level & 0x0040) == 0) )
						saa->noise[ch].level = (saa->noise[ch].level << 1) | 1;
					else
						saa->noise[ch].level <<= 1;
                }
            }
		}

        /* for each channel */
		for (ch = 0; ch < 6; ch++)
		{
			if (!saa->sync_state)
			{
				/* check the actual position in the square wave */
				saa->channels[ch].counter += saa->channels[ch].incr;
				while (saa->channels[ch].counter > 1.0)
				{
					saa->channels[ch].counter -= 1.0;
					saa->channels[ch].level ^= 1;
				}

				/* if the noise is enabled */
                if (saa->channels[ch].noise_enable)
				{
					active++;
					/* if the noise level is high (noise 0: chan 0-2, noise 1: chan 3-5) */
					if (saa->noise[ch/3].level & 1)
					{
						output_l += saa->channels[ch].amplitude[LEFT];
                        output_r += saa->channels[ch].amplitude[RIGHT];
                    }
                }

				/* if the square wave is enabled */
				if (saa->channels[ch].freq_enable)
				{
					active++;
                    /* if the channel level is high */
					if (saa->channels[ch].level & 1)
					{
						output_l += saa->channels[ch].amplitude[LEFT];
						output_r += saa->channels[ch].amplitude[RIGHT];
					}
				}
			}
		}

		/* write sound data to the buffer */
		buffer[LEFT][j] = output_l / active;
		buffer[RIGHT][j] = output_r / active;
	}
}



int saa1099_sh_start(const struct MachineSound *msound)
{
	int i, j;
	const struct SAA1099_interface *intf = msound->sound_interface;

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
		struct SAA1099 *saa = &saa1099[i];

		memset(saa, 0, sizeof(struct SAA1099));

		for (j = 0; j < 2; j++)
		{
			sprintf(buf[j], "SAA1099 #%d", i);
			name[j] = buf[j];
			vol[j] = MIXER(intf->volume[i][j], j ? MIXER_PAN_RIGHT : MIXER_PAN_LEFT);
		}
		saa->stream = stream_init_multi(2, name, vol, sample_rate, i, saa1099_update);
	}

	return 0;
}

void saa1099_sh_stop(void)
{
}

static void saa1099_control_port_w( int chip, int reg, int data )
{
	struct SAA1099 *saa = &saa1099[chip];

    if ((data & 0xff) > 0x1c)
	{
		/* Error! */
		logerror("%04x: (SAA1099 #%d) Unknown register selected\n",cpu_get_pc(), chip);
	}
	
	saa->selected_reg = data & 0x1f;
}


static void saa1099_write_port_w( int chip, int offset, int data )
{
	struct SAA1099 *saa = &saa1099[chip];
	int reg = saa->selected_reg;

	/* first update the stream to this point in time */
	stream_update(saa->stream, 0);

	switch (reg)
	{
	/* channel i amplitude */
	case 0x00:	case 0x01:	case 0x02:	case 0x03:	case 0x04:	case 0x05:
		saa->channels[reg & 0x07].amplitude[LEFT] = data & 0x0f;
		saa->channels[reg & 0x07].amplitude[RIGHT] = (data >> 4) & 0x0f;
		break;
	/* channel i frequency */
	case 0x08:	case 0x09:	case 0x0a:	case 0x0b:	case 0x0c:	case 0x0d:
		reg -= 0x08;
		if (saa->channels[reg & 0x07].frequency != (data & 0xff))
		{
			saa->channels[reg & 0x07].frequency = data & 0xff;
			saa->channels[reg & 0x07].counter = 0.0;
		}
		break;
	/* channel i octave */
	case 0x10:	case 0x11:	case 0x12:
		reg -= 0x10;
		if (saa->channels[(reg << 1) + 0].octave != (data & 0x07))
		{
			saa->channels[(reg << 1) + 0].octave = data & 0x07;
			saa->channels[(reg << 1) + 0].counter = 0.0;
		}
		if (saa->channels[(reg << 1) + 1].octave != ((data >> 4) & 0x07))
		{
			saa->channels[(reg << 1) + 1].octave = (data >> 4) & 0x07;
			saa->channels[(reg << 1) + 1].counter = 0.0;
		}
		break;
	/* channel i frequency enable */
	case 0x14:
		saa->channels[0].freq_enable = data & 0x01;
		saa->channels[1].freq_enable = data & 0x02;
		saa->channels[2].freq_enable = data & 0x04;
		saa->channels[3].freq_enable = data & 0x08;
		saa->channels[4].freq_enable = data & 0x10;
		saa->channels[5].freq_enable = data & 0x20;
		break;
	/* channel i noise enable */
	case 0x15:
		if (data & 0xff)
		{
			logerror("%04x: (SAA1099 #%d) -reg 0x15- Noise not supported (%02x)\n",cpu_get_pc(), chip, data);
		}
		saa->channels[0].noise_enable = data & 0x01;
		saa->channels[1].noise_enable = data & 0x02;
		saa->channels[2].noise_enable = data & 0x04;
		saa->channels[3].noise_enable = data & 0x08;
		saa->channels[4].noise_enable = data & 0x10;
		saa->channels[5].noise_enable = data & 0x20;
		break;
	/* noise generators parameters */
	case 0x16:
		logerror("%04x: (SAA1099 #%d) -reg 0x16- Noise not supported (%02x)\n",cpu_get_pc(), chip, data);
		saa->noise_params[0] = data & 0x03;
		saa->noise_params[1] = (data >> 4) & 0x03;
		break;
	/* envelope generators parameters */
	case 0x18:	case 0x19:
		if (data & 0xbf)
		{
			logerror("%04x: (SAA1099 #%d) -reg 0x%02x- Envelope generators not supported (%02x)\n",cpu_get_pc(), chip, reg, data);
		}
		reg -= 0x18;
		saa->env_params[reg] = data & 0x3f;
		saa->env_gen_enable[reg] = data & 0x80;
		break;
	/* channels enable & reset generators */
	case 0x1c:
		saa->all_ch_enable = data & 0x01;
		saa->sync_state = data & 0x02;
		if (data & 0x02)
		{
			int i;

			/* Synch & Reset generators */
			logerror("%04x: (SAA1099 #%d) -reg 0x1c- Chip reset\n",cpu_get_pc(), chip);
			for (i = 0; i < 6; i++)
			{
				saa->channels[i].incr = 0.0;
				saa->channels[i].level = 0;
				saa->channels[i].counter = 0.0;
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
