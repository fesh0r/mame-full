/***********************************************************************

	atom.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"
#include "includes/i8271.h"
#include "includes/basicdsk.h"
#include "includes/flopdrv.h"

static	ppi8255_interface	atom_8255_int =
{
	1,
	atom_8255_porta_r,
	atom_8255_portb_r,
	atom_8255_portc_r,
	atom_8255_porta_w,
	atom_8255_portb_w,
	atom_8255_portc_w,
};

static	struct
{
	UINT8	atom_8255_porta;
	UINT8	atom_8255_portb;
	UINT8	atom_8255_portc;
} atom_8255;



static int previous_i8271_int_state;

static void atom_8271_interrupt_callback(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */
	
	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}

static struct i8271_interface atom_8271_interface=
{
	atom_8271_interrupt_callback,
	NULL
};

void atom_init_machine(void)
{
	ppi8255_init (&atom_8255_int);
	atom_8255.atom_8255_porta = 0xff;
	atom_8255.atom_8255_portb = 0xff;
	atom_8255.atom_8255_portc = 0xff;

	floppy_drives_init();
	i8271_init(&atom_8271_interface);
}

void atom_stop_machine(void) 
{ 
	i8271_stop();
}

/* start 2900. exec C2B2.
   start xxxx. exec C2B2.
   start xxxx. exec xxxx.
*/

int atom_init_atm (int id)
{
	int		loop;
	void	*file;
	UINT8	*mem = memory_region (REGION_CPU1);

	struct
	{
		UINT8	atm_name[16];
		UINT16	atm_start;
		UINT16	atm_exec;
		UINT16	atm_size;
	} atm;

	return (1);

	/* This code not endian safe */

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		if (osd_fread(file, &atm, sizeof (atm)) != sizeof (atm))
		{
			logerror("atom: Could not read atm header\n");
			osd_fclose(file);
			return (1);
		}
		else
		{
			*mem += atm.atm_start;
			if (osd_fread(file, mem, atm.atm_size) != atm.atm_size)
			{
				logerror("atom: Could not read atm data\n");
				osd_fclose(file);
				return (1);
			}
			else
			{

				{
				    logerror("atom: Atm: %s, %04X, %04X, %04X\n", atm.atm_name, atm.atm_start, atm.atm_exec, atm.atm_size);
					for (loop = 0; loop < 16; loop++)
					{
						logerror("%02X ", mem[loop]);
					}
					logerror("\n");
				}
				osd_fclose(file);
				return (0);
			}
		}
	}
	return (1);
}


/* load floppy */
int atom_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_OK)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(id,80,1,10,256,12,3,0);

		return INIT_OK;
	}

	return INIT_FAILED;
}


READ_HANDLER ( atom_8255_porta_r )
{
	/* logerror("8255: Read port a\n"); */
	return (atom_8255.atom_8255_porta);
}

READ_HANDLER ( atom_8255_portb_r )
{
	/* ilogerror("8255: Read port b: %02X %02X\n",
			readinputport ((atom_8255.atom_8255_porta & 0x0f) + 1),
			readinputport (11) & 0xc0); */
	return ((readinputport ((atom_8255.atom_8255_porta & 0x0f) + 1) & 0x3f) |
											(readinputport (11) & 0xc0));
}

READ_HANDLER ( atom_8255_portc_r )
{

/* | */
	atom_8255.atom_8255_portc &= 0x3f;
	atom_8255.atom_8255_portc |= (readinputport (0) & 0x80);
	atom_8255.atom_8255_portc |= (readinputport (12) & 0x40);
	/* logerror("8255: Read port c (%02X)\n",atom_8255.atom_8255_portc); */
	return (atom_8255.atom_8255_portc);
}

/* Atom 6847 modes:

0000xxxx	Text
0001xxxx	64x64	4
0011xxxx	128x64	2
0101xxxx	128x64	4
0111xxxx	128x96	2
1001xxxx	128x96	4
1011xxxx	128x192	2
1101xxxx	128x192	4
1111xxxx	256x192	2

*/

static int atom_mode_trans[] =
{
	M6847_MODE_G1C,
	M6847_MODE_G1R,
	M6847_MODE_G2C,
	M6847_MODE_G2R,
	M6847_MODE_G3C,
	M6847_MODE_G3R,
	M6847_MODE_G4C,
	M6847_MODE_G4R
};

WRITE_HANDLER ( atom_8255_porta_w )
{
	if ((data & 0xf0) != (atom_8255.atom_8255_porta & 0xf0))
	{
		if (!(data & 0x10))
		{
			m6847_set_mode (M6847_MODE_TEXT);
		}
		else
		{
			m6847_set_mode (atom_mode_trans[data >> 5]);
		}
	}
	atom_8255.atom_8255_porta = data;
	logerror("8255: Write port a, %02x\n", data);
}

WRITE_HANDLER ( atom_8255_portb_w )
{
	atom_8255.atom_8255_portb = data;
	logerror("8255: Write port b, %02x\n", data);
}

WRITE_HANDLER ( atom_8255_portc_w )
{
	atom_8255.atom_8255_portc = data;
	speaker_level_w(0, (data & 0x04) >> 2);
	logerror("8255: Write port c, %02x\n", data);
}


/* KT- I've assumed that the atom 8271 is linked in exactly the same way as on the bbc */
READ_HANDLER(atom_8271_r)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			return i8271_r(offset);
		case 4:
			return i8271_data_r(offset);
		default:
			break;
	}

	return 0x0ff;
}

WRITE_HANDLER(atom_8271_w)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(offset, data);
			return;
		case 4:
			i8271_data_w(offset, data);
			return;
		default:
			break;
	}
}

