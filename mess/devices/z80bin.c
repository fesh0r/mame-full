/*********************************************************************

	z80bin.c

	A binary quickload format used by the Microbee, the Exidy Sorcerer
	VZ200/300 and the Super 80

*********************************************************************/

#include "mess.h"
#include "snapquik.h"
#include "z80bin.h"


static QUICKLOAD_LOAD( z80bin )
{
	int ch;
	UINT16 args[3];
	UINT16 start_addr, end_addr, exec_addr, i;
	UINT8 data;

	mame_fseek(fp, 7, SEEK_SET);

	while((ch = mame_fgetc(fp)) != 0x1A)
	{
		if (ch == EOF)
			return INIT_FAIL;
	}

	if (mame_fread(fp, args, sizeof(args)) != sizeof(args))
		return INIT_FAIL;

	exec_addr	= LITTLE_ENDIANIZE_INT16(args[0]);
	start_addr	= LITTLE_ENDIANIZE_INT16(args[1]);
	end_addr	= LITTLE_ENDIANIZE_INT16(args[2]);

	for (i = start_addr; i <= end_addr; i++)
	{
		if (mame_fread(fp, &data, 1) != 1)
			return INIT_FAIL;
		program_write_byte(i, data);
	}
	cpunum_set_reg(0, REG_PC, exec_addr);
	return INIT_PASS;
}



void z80bin_quickload_getinfo(struct IODevice *dev)
{
	/* quickload */
	quickload_device_getinfo(dev, quickload_load_z80bin, 0.0);
	dev->file_extensions = "bin\0";
}

