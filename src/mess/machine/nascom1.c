/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include <stdio.h>
#include "driver.h"
#include "cpu/z80/z80.h"

static	int		nascom1_tape_size = 0;
static	UINT8	*nascom1_tape_image = NULL;
static	int		nascom1_tape_index = 0;

static	int	nascom1_ramsize = 3;

void nascom1_init_machine(void)
{
	logerror("nascom1_init\r\n");
	if (readinputport(9) != nascom1_ramsize)
	{
		nascom1_ramsize = readinputport(9);
		switch (nascom1_ramsize)
		{
			case 03:
				install_mem_write_handler(0, 0x9000, 0xafff, MWA_RAM);
				install_mem_read_handler(0, 0x9000, 0xafff, MRA_RAM);
				install_mem_write_handler(0, 0x5000, 0x8fff, MWA_RAM);
				install_mem_read_handler(0, 0x5000, 0x8fff, MRA_RAM);
				install_mem_write_handler(0, 0x1400, 0x4fff, MWA_RAM);
				install_mem_read_handler(0, 0x1400, 0x4fff, MRA_RAM);
				break;
			case 02:
				install_mem_write_handler(0, 0x9000, 0xafff, MWA_NOP);
				install_mem_read_handler(0, 0x9000, 0xafff, MRA_NOP);
				install_mem_write_handler(0, 0x5000, 0x8fff, MWA_RAM);
				install_mem_read_handler(0, 0x5000, 0x8fff, MRA_RAM);
				install_mem_write_handler(0, 0x1400, 0x4fff, MWA_RAM);
				install_mem_read_handler(0, 0x1400, 0x4fff, MRA_RAM);
				break;
			case 01:
				install_mem_write_handler(0, 0x9000, 0xafff, MWA_NOP);
				install_mem_read_handler(0, 0x9000, 0xafff, MRA_NOP);
				install_mem_write_handler(0, 0x5000, 0x8fff, MWA_NOP);
				install_mem_read_handler(0, 0x5000, 0x8fff, MRA_NOP);
				install_mem_write_handler(0, 0x1400, 0x4fff, MWA_RAM);
				install_mem_read_handler(0, 0x1400, 0x4fff, MRA_RAM);
				break;
			case 00:
				install_mem_write_handler(0, 0x9000, 0xafff, MWA_NOP);
				install_mem_read_handler(0, 0x9000, 0xafff, MRA_NOP);
				install_mem_write_handler(0, 0x5000, 0x8fff, MWA_NOP);
				install_mem_read_handler(0, 0x5000, 0x8fff, MRA_NOP);
				install_mem_write_handler(0, 0x1400, 0x4fff, MWA_NOP);
				install_mem_read_handler(0, 0x1400, 0x4fff, MRA_NOP);
				break;
		}
	}
}

void nascom1_stop_machine(void)
{
}

int	nascom1_init_cassette(int id)
{
	void	*file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		nascom1_tape_size = osd_fsize(file);
		nascom1_tape_image = (UINT8 *)malloc(nascom1_tape_size);
		if (!nascom1_tape_image || (osd_fread(file, nascom1_tape_image, nascom1_tape_size) != nascom1_tape_size))
		{
			osd_fclose(file);
			return (1);
		}
		else
		{
			osd_fclose(file);
			nascom1_tape_index = 0;
			return (0);
		}
	}
	return (1);
}

void nascom1_exit_cassette(int id)
{
	if (nascom1_tape_image)
	{
		free(nascom1_tape_image);
		nascom1_tape_image = NULL;
		nascom1_tape_size = nascom1_tape_index = 0;
	}
}

int	nascom1_read_cassette(void)
{
	if (nascom1_tape_image && (nascom1_tape_index < nascom1_tape_size))
					return (nascom1_tape_image[nascom1_tape_index++]);
	return (0);
}

/* Ascii .nas format

   <addr> <byte> x 8 ^H^H^J
   .

   Note <addr> and <byte> are in hex.
*/

int	nascom1_init_cartridge(int id)
{
	void	*file;
	int		done;
	char	fileaddr[5];
	/* int	filebyt1, filebyt2, filebyt3, filebyt4;
	int	filebyt5, filebyt6, filebyt7, filebyt8; */
	int	addr;

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		done = 0;
		fileaddr[4] = 0;
		while (!done)
		{
			osd_fread(file, (void *)fileaddr, 4);
			printf ("%4.4s\n", fileaddr);
			if (fileaddr[0] == '.')
			{
				done = 1;
			}
			else
			{
				vsscanf (fileaddr, "%4X", &addr);
			    /* printf ("%04X: %02X %02X %02X %02X %02X %02X %02X %02X\n",
							  addr, filebyt1, filebyt2, filebyt3, filebyt4,
									filebyt5, filebyt6, filebyt7, filebyt8); */
								
			}
		}
	}
	else
	{
		return (1);
	}

	return (0);
}
