/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Using a 6821 instead of the 6820.

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6821pia.h"
#include "inptport.h"

int apple1_pia0_kbdin(UINT32);
int apple1_pia0_dsprdy(UINT32);
int apple1_pia0_kbdrdy(UINT32);
void apple1_pia0_dspout(UINT32, UINT32);
void apple1_vh_dsp_w(int);
void apple1_vh_dsp_clr(void);

struct pia6821_interface pia0 =
{
	apple1_pia0_kbdin,				   /* returns key input */
	apple1_pia0_dsprdy,				   /* Bit 7 low when display ready */
	apple1_pia0_kbdrdy,				   /* Bit 7 high means key pressed */
	0,								   /* CA2 not used */
	0,								   /* CB1 not used */
	0,								   /* CB2 not used */
	0,								   /* CRA has no output */
	apple1_pia0_dspout,				   /* Send character to screen */
	0,								   /* CA2 not written to */
	0,								   /* CB2 not written to */
	0,								   /* Interrupts not connected */
	0								   /* Interrupts not connected */
};

static UINT8 apple1_kbd_conv[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', '-', '=', '[', ']', ';', '\'',
	'\\', ',', '.', '/', '#', 'A', 'B', 'C',
	'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x0d,
	0x5f, ' ', 0x1b,
/* shifted */
	')', '!', '"', '3', '$', '%', '^', '&',
	'*', '(', '-', '+', '[', ']', ':', '@',
	'\\', '<', '>', '?', '#', 'A', 'B', 'C',
	'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
	'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x0d,
	0x5f, ' ', 0x1b
};

static int apple1_kbd_data;

void apple1_init_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "apple1_init\r\n");

}

void apple1_stop_machine(void)
{

}

int apple1_interrupt(void)
{

	int loop;

/* Check for keypresses */

	apple1_kbd_data = 0;
	if (readinputport(3) & 0x0020)
	{
		for (loop = 0; loop < 0x1fff; loop++)
			cpu_writemem16(loop, 0);
		apple1_vh_dsp_clr();
		pia_reset();
		m6502_reset(NULL);
		m6502_set_pc(0xff00);
	}
	else if (readinputport(3) & 0x0040)
	{
		apple1_vh_dsp_clr();
	}
	else
	{
		for (loop = 0; loop < 51; loop++)
		{
			if (readinputport(loop / 16) & (1 << (loop & 15)))
			{
				if (readinputport(3) & 0x0018)
				{
					apple1_kbd_data = apple1_kbd_conv[loop + 51];
				}
				else
				{
					apple1_kbd_data = apple1_kbd_conv[loop];
				}
				loop = 51;
				//if (errorlog) fprintf (errorlog, "key: %c\r\n", apple1_kbd_data);
			}
		}
	}

	return (0);

}

void init_apple1(void)
{

	pia_config(0, PIA_8BIT | PIA_AUTOSENSE, &pia0);

}

/* || */

int apple1_pia0_kbdin(UINT32 offset)
{

	return (apple1_kbd_data | 0x80);

}

int apple1_pia0_dsprdy(UINT32 offset)
{

	return (0x00);					   /* Screen always ready */

}

int apple1_pia0_kbdrdy(UINT32 offset)
{

	if (apple1_kbd_data)
		return (1);

	return (0x00);

}

void apple1_pia0_dspout(UINT32 offset, UINT32 val)
{

	apple1_vh_dsp_w(val);

}

int apple1_rom_load(void)
{

	return (0);

}

int apple1_rom_id(int id)
{

	return (1);

}
