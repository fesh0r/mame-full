#include "driver.h"
#include "includes/gb.h"

static int channel;
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
	case 0x10: /* Mode 1 Register - Sweep Register (R/W) */
		break;
	case 0x11: /* Mode 1 Register - Sound length/Wave pattern duty (R/W) */
		break;
	case 0x12: /* Mode 1 Register - Envelope (R/W) */
		break;
	case 0x13: /* Mode 1 Register - Frequency lo (R/W) */
		break;
	case 0x14: /* Mode 1 Register - Frequency hi (R/W) */
		break;

	/*MODE 2 */
	case 0x16: /* Mode 2 Register - Sound length/Wave pattern duty (R/W) */
		break;
	case 0x17: /* Mode 2 Register - Envelope (R/W) */
		break;
	case 0x18: /* Mode 2 Register - Frequency lo (R/W) */
		break;
	case 0x19: /* Mode 2 Register - Frequency hi (R/W) */
		break;

	/*MODE 3 */
	case 0x1A: /* Mode 3 Register - Sound On/Off (R/W) */
		break;
	case 0x1B: /* Mode 3 Register - Sound Length (R/W) */
		break;
	case 0x1C: /* Mode 3 Register - Select Output Level */
		break;
	case 0x1D: /* Mode 3 Register - Frequency lo (R/W) */
		break;
	case 0x1E: /* Mode 3 Register - Frequency hi (R/W) */
		break;

	case 0x20: /* Mode 4 Register - Sound Length (R/W) */
		break;
	case 0x21: /* Mode 4 Register - Envelope */
		break;
	case 0x22: /* Mode 3 Register - Polynomial Counter */
		break;
	case 0x23: /* Mode 2 Register - Counter/Consecutive; Initial (R/W)  */
		break;


	case 0x24: /* Channel Control / On/Off / Volume (R/W)  */
		break;

	case 0x25: /* Selection of Sound Output Terminal */
		break;

	case 0x26: /* Sound On/Off (R/W) */
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
