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

#include "driver.h"
#include "includes/gb.h"

static int channel = 1;
static int sound_register;
static int register_value;

void gameboy_sh_update(int param, INT16 *buffer, int length);

void gameboy_sound_w(int offset, int data)
{

    /* change in registers so update first */
    stream_update(channel,0);
	sound_register = offset ^ 0xFF00;
	register_value = data;
	logerror ("gameboy_sound_w - SOUND Register = %x Value = %x\n",sound_register,register_value);

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
		break;
	case 0x1B: /* NR31 Mode 3 Register - Sound Length (R/W) */
		break;
	case 0x1C: /* NR32 Mode 3 Register - Select Output Level */
		break;
	case 0x1D: /* NR33 Mode 3 Register - Frequency lo (R/W) */
		break;
	case 0x1E: /* NR34 Mode 3 Register - Frequency hi (R/W) */
		break;

	case 0x20: /* NR41 Mode 4 Register - Sound Length (R/W) */
		break;
	case 0x21: /* NR42 Mode 4 Register - Envelope */
		break;
	case 0x22: /* NR43 Mode 4 Register - Polynomial Counter */
		break;
	case 0x23: /* NR44 Mode 4 Register - Counter/Consecutive; Initial (R/W)  */
		break;


	case 0x24: /* NR50 Channel Control / On/Off / Volume (R/W)  */
		break;

	case 0x25: /* NR51 Selection of Sound Output Terminal */
		break;

	case 0x26: /* NR52 Sound On/Off (R/W) */

		break;

 	/*   0xFF30 - 0xFF3F = Wave Pattern RAM for arbitrary sound data */

	}

}



int gameboy_sh_start(const struct MachineSound* driver)
{
	channel = stream_init("Gameboy out", 50, Machine->sample_rate, 0, gameboy_sh_update);
    return 0;

}



void gameboy_sh_update(int param, INT16 *buffer, int length)
{
	static INT16 max = 0x7fff;
	INT16 *sample = buffer;
	/* Need to create 3 channels of sound here */
    while( length-- > 0 )
	{
		*sample++ = max;
	}

}
