/**************************************************************************************
* Gameboy sound emulation (c) Ben Bruscella (ben@mame.net) <--- what shit!
*
* Anyways, sound on the gameboy consists of 4 separate 'channels'
*   Sound1 = Quadrangular waves with SWEEP and ENVELOPE functions  (NR10,11,12,13,14)
*   Sound2 = Quadrangular waves with ENVELOPE functions
*   Sound3 = Wave patterns from WaveRAM
*   Sound4 = White noise with an envelope
*
* Each sound channel has 2 modes, namely ON and OFF...  whoa
*


***************************************************************************************/

#include <math.h>
#include "driver.h"
#include "includes/gb.h"

/*UINT8 *gb_ram;*/

static int channel = 1;
static int sound_register;
static int register_value;


struct SOUND1
{
	int on;
	int length;
};

struct SOUND2
{
	int on;
	int length;
};

struct SOUND3
{
	int on;
	int length;
};

struct SOUND4
{
	int on;
	int length;
};

struct SOUNDALL
{
	int on;
	int length;
};

static struct SOUND1   snd_1;
static struct SOUND2   snd_2;
static struct SOUND3   snd_3;
static struct SOUND4   snd_4;
static struct SOUNDALL snd_all;



void gameboy_sh_update(int param, INT16 *buffer, int length);

void gameboy_sound_w(int offset, int data)
{

    /* change in registers so update first */
    stream_update(channel,0);
	sound_register = offset ^ 0xFF00;
	register_value = data;

	/*
	logerror ("gameboy_sound_w - SOUND Register = %x Value = %x\n",sound_register,register_value);
	*/
	switch( sound_register )
	{
	/*MODE 1 */
	case 0x10: /* NR10 Mode 1 Register - Sweep Register (R/W) */
		break;
	case 0x11: /* NR11 Mode 1 Register - Sound length/Wave pattern duty (R/W) */
		break;
	case 0x12: /* NR12 Mode 1 Register - Envelope (R/W) */
		break;
	case 0x13: /* NR13 Mode 1 Register - Frequency lo (R/W) */
		break;
	case 0x14: /* NR14 Mode 1 Register - Frequency hi (R/W) */
		break;

	/*MODE 2 */
	case 0x16: /* NR21 Mode 2 Register - Sound length/Wave pattern duty (R/W) */
		break;
	case 0x17: /* NR22 Mode 2 Register - Envelope (R/W) */
		break;
	case 0x18: /* NR23 Mode 2 Register - Frequency lo (R/W) */
		break;
	case 0x19: /* NR24 Mode 2 Register - Frequency hi (R/W) */
		break;

	/*MODE 3 */
	case 0x1A: /* NR30 Mode 3 Register - Sound On/Off (R/W) */
		snd_3.on = (data & 0x80)>>7;
		/*logerror ("gameboy_sound_w - SOUND 3 ON/OFF is %x\n",snd_3->on);*/
		break;
	case 0x1B: /* NR31 Mode 3 Register - Sound Length (R/W) */
		snd_3.length = data;
		break;
	case 0x1C: /* NR32 Mode 3 Register - Select Output Level */
		break;
	case 0x1D: /* NR33 Mode 3 Register - Frequency lo (R/W) */
		break;
	case 0x1E: /* NR34 Mode 3 Register - Frequency hi (R/W) */
		break;

	/*MODE 4 */
	case 0x20: /* NR41 Mode 4 Register - Sound Length (R/W) */
		snd_4.length = data<<2;
		logerror("Sound4 Data length changed to %4x\n",snd_4.length);
		break;
	case 0x21: /* NR42 Mode 4 Register - Envelope */
		break;
	case 0x22: /* NR43 Mode 4 Register - Polynomial Counter */
		break;
	case 0x23: /* NR44 Mode 4 Register - Counter/Consecutive; Initial (R/W)  */
		break;

	/*GENERAL */
	case 0x24: /* NR50 Channel Control / On/Off / Volume (R/W)  */
		break;

	case 0x25: /* NR51 Selection of Sound Output Terminal */
		break;

	case 0x26: /* NR52 Sound On/Off (R/W) */
		logerror("NR52 - %x\n",data);
		snd_1.on = (data & 0x01);
		/* logerror("Sound 1 = %02x\n",snd_1->on); */
		snd_2.on = (data & 0x02)>>1;
		/* logerror("Sound 2 = %02x\n",snd_2->on); */
		snd_3.on = (data & 0x04)>>2;
		/* logerror("Sound 3 = %02x\n",snd_3->on); */
		snd_4.on = (data & 0x08)>>3;
		/* logerror("Sound 4 = %02x\n",snd_4->on); */
		snd_all.on = (data & 0x80)>>7;
		logerror("Sound ALL = %02x\n",snd_all.on);
		break;

 	/*   0xFF30 - 0xFF3F = Wave Pattern RAM for arbitrary sound data */

	}

}



int gameboy_sh_start(const struct MachineSound* driver)
{
    memset(&snd_1, 0, sizeof(snd_1));
    memset(&snd_2, 0, sizeof(snd_2));
    memset(&snd_3, 0, sizeof(snd_3));
    memset(&snd_4, 0, sizeof(snd_4));
    memset(&snd_all, 0, sizeof(snd_all));

	channel = stream_init("Gameboy out", 50, Machine->sample_rate, 0, gameboy_sh_update);



    return 0;

}



void gameboy_sh_update(int param, INT16 *buffer, int length)
{
	static INT16 max = 0x7FFF;
	int i;

/*
  These tend to be the two most important equations in
 converting between Hertz and GB frequency registers:
 (Sounds will have a 2.4% higher frequency on Super GB.)
     gb = 2048 - (131072 / Hz)
     Hz  = 131072 / (2048 - gb)
*/

	/* Need to create 4 channels of sound here */

	if (snd_all.on)    /* Produce Sound */
		max = 0x7FFF;
	else
		max = 0x0000;


	for (i = 0; i < length; i++)
	{

		buffer[i] = 0;
	}

}
