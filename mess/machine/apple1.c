/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Using a 6821 instead of the 6820.

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6502/m6502.h"
#include "inptport.h"
#include "includes/apple1.h"
#include "image.h"


/*****************************************************************************
**	Structures
*****************************************************************************/
struct pia6821_interface apple1_pia0 =
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


/*****************************************************************************
**	driver init
*****************************************************************************/

DRIVER_INIT( apple1 )
{
	/* install RAM handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, MWA8_BANK1);
	cpu_setbank(1, mess_ram);

	pia_config(0, PIA_8BIT | PIA_AUTOSENSE, &apple1_pia0);
}

/*****************************************************************************
**	apple1_verify_header
*****************************************************************************/
static int apple1_verify_header (UINT8 *data)
{
	/* Verify the format for the snapshot */
	if ((data[0] == 'L') &&
		(data[1] == 'O') &&
		(data[2] == 'A') &&
		(data[3] == 'D') &&
		(data[4] == ':') &&
		(data[7] == 'D') &&
		(data[8] == 'A') &&
		(data[9] == 'T') &&
		(data[10]== 'A') &&
		(data[11]== ':'))
	{
		return(IMAGE_VERIFY_PASS);
	}
	else
	{
		return(IMAGE_VERIFY_FAIL);
	}
}


/*****************************************************************************
**	apple1_load_snap
**	Format of the binary SnapShot Image is:
**	[ LOAD:xxyyDATA:zzzzzzzzzzzzz....]
**	Where xxyy is the hex value to load the Data zzzzzzz to
**
*****************************************************************************/
SNAPSHOT_LOAD(apple1)
{
	UINT8 *memptr;
	UINT8 snapdata[0x1000];
	UINT16 starting_offset = 0x0000;

	/* Load the specified Snapshot */

	/* Read the snapshot data into a temporary array */
	if (mame_fread(fp, snapdata, 0x1000) != 0x1000)
		return INIT_FAIL;

	/* Verify the snapshot header */
	if (apple1_verify_header(snapdata) == IMAGE_VERIFY_FAIL)
	{
		logerror("Apple1 - Snapshot Header is in incorrect format - needs to be LOAD:xxyyDATA:\n");
		return INIT_FAIL;
	}

	/* Extract the starting offset to load the snapshot to! */
	starting_offset = (snapdata[5] << 8) | (snapdata[6]);
	logerror("Apple1 - LoadAddress is 0x%04x\n", starting_offset);

	/* Point to the region where the snapshot will be loaded to */
	memptr = memory_region(REGION_CPU1) + starting_offset;

	/* Copy the Actual Data into Memory Space */
	memcpy(memptr, &snapdata[12], 0x1000);

	return INIT_PASS;
}


/*****************************************************************************
**	apple1_interrupt
*****************************************************************************/
void apple1_interrupt(void)
{
	int loop;

	/* Check for keypresses */
	apple1_kbd_data = 0;
	if (readinputport(3) & 0x0020)	/* clear screen */
	{
		apple1_vh_dsp_clr();
	}
	else
	{
		for (loop = 0; loop < 51; loop++)
		{
			if (readinputport(loop / 16) & (1 << (loop & 15)))
			{
				if (readinputport(3) & 0x0018)	/* shift keys */
				{
					apple1_kbd_data = apple1_kbd_conv[loop + 51];
				}
				else
				{
					apple1_kbd_data = apple1_kbd_conv[loop];
				}
				loop = 51;
			}
		}
	}
}


/*****************************************************************************
**	READ/WRITE HANDLERS
*****************************************************************************/
READ_HANDLER( apple1_pia0_kbdin )
{
	return (apple1_kbd_data | 0x80);
}
READ_HANDLER( apple1_pia0_dsprdy )
{
	return (0x00);		/* Screen always ready */
}
READ_HANDLER( apple1_pia0_kbdrdy )
{
	if (apple1_kbd_data)
	{
		return (1);		/* Key available */
	}
	return (0x00);
}
WRITE_HANDLER( apple1_pia0_dspout )
{
	apple1_vh_dsp_w(data);
}

