/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/mc6850.h"

static	int		uk101_tape_size = 0;
static	UINT8	*uk101_tape_image = 0;
static	int		uk101_tape_index = 0;

READ_HANDLER( uk101_acia0_casin );
READ_HANDLER( uk101_acia0_statin );

struct acia6850_interface uk101_acia0 =
{
	uk101_acia0_statin,
	uk101_acia0_casin,
	0,
	0
};

static	int	uk101_ramsize = 2;	/* 40Kb */

void uk101_init_machine(void)
{
	logerror("uk101_init\r\n");

	acia6850_config (0, &uk101_acia0);

	if (readinputport(8) != uk101_ramsize)
	{
		uk101_ramsize = readinputport(8);
		switch (uk101_ramsize)
		{
			case 2:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_RAM);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_RAM);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_RAM);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_RAM);
				break;
			case 1:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_NOP);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_NOP);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_RAM);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_RAM);
				break;
			case 0:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_NOP);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_NOP);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_NOP);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_NOP);
				break;
		}
	}
}

void uk101_stop_machine(void)
{

}

READ_HANDLER( uk101_acia0_casin )
{
	if (uk101_tape_image && (uk101_tape_index < uk101_tape_size))
							return (uk101_tape_image[uk101_tape_index++]);
	return (0);
}

READ_HANDLER (uk101_acia0_statin )
{
	if (uk101_tape_image && (uk101_tape_index < uk101_tape_size))
							return (ACIA_6850_RDRF);
	return (0);
}

/* || */

int	uk101_init_cassette(int id)
{
	void	*file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		uk101_tape_size = osd_fsize(file);
		uk101_tape_image = (UINT8 *)malloc(uk101_tape_size);
		if (!uk101_tape_image || (osd_fread(file, uk101_tape_image, uk101_tape_size) != uk101_tape_size))
		{
			osd_fclose(file);
			return (1);
		}
		else
		{
			osd_fclose(file);
			uk101_tape_index = 0;
			return (0);
		}
	}
	return (1);
}

void uk101_exit_cassette(int id)
{
	if (uk101_tape_image)
	{
		free(uk101_tape_image);
		uk101_tape_image = NULL;
		uk101_tape_size = uk101_tape_index = 0;
	}
}

