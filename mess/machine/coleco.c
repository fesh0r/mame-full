/***************************************************************************

  coleco.c

  Machine file to handle emulation of the Colecovision.

  TODO:
	- Clean up the code
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms9928a.h"
#include "includes/coleco.h"

/* local */
unsigned char *coleco_ram;
unsigned char *coleco_cartridge_rom;

static int JoyMode=0;

//static unsigned char *ROM;


int coleco_id_rom (int id)
{
	FILE *romfile;
	unsigned char magic[2];
	int retval = ID_FAILED;

	logerror("---------coleco_id_rom-----\n");
	logerror("Gamename is %s\n",device_filename(IO_CARTSLOT,id));
	logerror("filetype is %d\n",OSD_FILETYPE_IMAGE_R);

	/* If no file was specified, don't bother */
	if (!device_filename(IO_CARTSLOT,id) || !strlen(device_filename(IO_CARTSLOT,id)) )
		return ID_OK;

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
		return ID_FAILED;

	retval = 0;
	/* Verify the file is in Colecovision format */
	osd_fread (romfile, magic, 2);
	if ((magic[0] == 0xAA) && (magic[1] == 0x55))
		retval = ID_OK;
	if ((magic[0] == 0x55) && (magic[1] == 0xAA))
		retval = ID_OK;

	osd_fclose (romfile);
	return retval;
}

int coleco_load_rom (int id)
{
    FILE *cartfile;

	UINT8 *ROM = memory_region(REGION_CPU1);

	logerror("---------coleco_load_rom-----\n");
	logerror("filetype is %d  \n",OSD_FILETYPE_IMAGE_R);
	logerror("Machine->game->name is %s  \n",Machine->gamedrv->name);
	logerror("romname[0] is %s  \n",device_filename(IO_CARTSLOT,id));

	/* A cartridge isn't strictly mandatory, but it's recommended */
	cartfile = NULL;
	if (!device_filename(IO_CARTSLOT,id) || !strlen(device_filename(IO_CARTSLOT,id) ))
	{
		logerror("Coleco - warning: no cartridge specified!\n");
	}
	else if (!(cartfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("Coleco - Unable to locate cartridge: %s\n",device_filename(IO_CARTSLOT,id) );
		return 1;
	}


	coleco_cartridge_rom = &(ROM[0x8000]);

	if (cartfile!=NULL)
	{
		osd_fread (cartfile, coleco_cartridge_rom, 0x8000);
		osd_fclose (cartfile);
	}

	return 0;
}


READ_HANDLER ( coleco_ram_r )
{
    return coleco_ram[offset];
}

WRITE_HANDLER ( coleco_ram_w )
{
    coleco_ram[offset]=data;
}

READ_HANDLER ( coleco_paddle_r )
{
	/* Player 1 */
	if ((offset & 0x02)==0)
	{
		/* Keypad and fire 1 */
		if (JoyMode==0)
		{
			int inport0,inport1,data;

			inport0 = input_port_0_r(0);
			inport1 = input_port_1_r(0);

			if		((inport0 & 0x01) == 0)		/* 0 */
				data = 0x0A;
			else if	((inport0 & 0x02) == 0)		/* 1 */
				data = 0x0D;
			else if ((inport0 & 0x04) == 0)		/* 2 */
				data = 0x07;
			else if ((inport0 & 0x08) == 0)		/* 3 */
				data = 0x0C;
			else if ((inport0 & 0x10) == 0)		/* 4 */
				data = 0x02;
			else if ((inport0 & 0x20) == 0)		/* 5 */
				data = 0x03;
			else if ((inport0 & 0x40) == 0)		/* 6 */
				data = 0x0E;
			else if ((inport0 & 0x80) == 0)		/* 7 */
				data = 0x05;
			else if ((inport1 & 0x01) == 0)		/* 8 */
				data = 0x01;
			else if ((inport1 & 0x02) == 0)		/* 9 */
				data = 0x0B;
			else if ((inport1 & 0x04) == 0)		/* # */
				data = 0x06;
			else if ((inport1 & 0x08) == 0)		/* . */
				data = 0x09;
			else
				data = 0x0F;

			return (inport1 & 0xF0) | (data);

		}
		/* Joystick and fire 2*/
		else
			return input_port_2_r(0);
	}
	/* Player 2 */
	else
	{
		/* Keypad and fire 1 */
		if (JoyMode==0)
		{
			int inport3,inport4,data;

			inport3 = input_port_3_r(0);
			inport4 = input_port_4_r(0);

			if		((inport3 & 0x01) == 0)		/* 0 */
				data = 0x0A;
			else if	((inport3 & 0x02) == 0)		/* 1 */
				data = 0x0D;
			else if ((inport3 & 0x04) == 0)		/* 2 */
				data = 0x07;
			else if ((inport3 & 0x08) == 0)		/* 3 */
				data = 0x0C;
			else if ((inport3 & 0x10) == 0)		/* 4 */
				data = 0x02;
			else if ((inport3 & 0x20) == 0)		/* 5 */
				data = 0x03;
			else if ((inport3 & 0x40) == 0)		/* 6 */
				data = 0x0E;
			else if ((inport3 & 0x80) == 0)		/* 7 */
				data = 0x05;
			else if ((inport4 & 0x01) == 0)		/* 8 */
				data = 0x01;
			else if ((inport4 & 0x02) == 0)		/* 9 */
				data = 0x0B;
			else if ((inport4 & 0x04) == 0)		/* # */
				data = 0x06;
			else if ((inport4 & 0x08) == 0)		/* . */
				data = 0x09;
			else
				data = 0x0F;

			return (inport4 & 0xF0) | (data);

		}
		/* Joystick and fire 2*/
		else
			return input_port_5_r(0);
	}

	return 0x00;
}

WRITE_HANDLER ( coleco_paddle_toggle_1_w )
{
	JoyMode=0;
    return;
}

WRITE_HANDLER ( coleco_paddle_toggle_2_w )
{
	JoyMode=1;
    return;
}

READ_HANDLER ( coleco_VDP_r )
{
	if (offset & 0x01)
		return TMS9928A_register_r();
	else
		return TMS9928A_vram_r();
}

WRITE_HANDLER ( coleco_VDP_w )
{
	if (offset & 0x01)
		TMS9928A_register_w(data);
	else
		TMS9928A_vram_w(data);

    return;
}


