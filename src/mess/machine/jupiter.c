/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

int jupiter_load_ace(int id);
void jupiter_exit_ace(int id);
int jupiter_load_tap(int id);
void jupiter_exit_tap(int id);

#define	JUPITER_NONE	0
#define	JUPITER_ACE	1
#define	JUPITER_TAP	2

static struct
{
	UINT8 hdr_type;
	UINT8 hdr_name[10];
	UINT16 hdr_len;
	UINT16 hdr_addr;
	UINT8 hdr_vars[8];
	UINT8 hdr_3c4c;
	UINT8 hdr_3c4d;
	UINT16 dat_len;
}
jupiter_tape;

static UINT8 *jupiter_data = NULL;
static int jupiter_data_type = JUPITER_NONE;

/* only gets called at the start of a cpu time slice */

OPBASE_HANDLER( jupiter_opbaseoverride )
{
	int loop;
	unsigned short tmpword;

	if (address == 0x059d)
	{
		cpu_setOPbaseoverride(0, 0);
		if (jupiter_data_type == JUPITER_ACE)
		{
			for (loop = 0; loop < 0x6000; loop++)
				cpu_writemem16(loop + 0x2000, jupiter_data[loop]);
			jupiter_exit_ace(0);
		}
		else if (jupiter_data_type == JUPITER_TAP)
		{

			for (loop = 0; loop < jupiter_tape.dat_len; loop++)
				cpu_writemem16(loop + jupiter_tape.hdr_addr, jupiter_data[loop]);

			cpu_writemem16(0x3c27, 0x01);

			for (loop = 0; loop < 8; loop++)
				cpu_writemem16(loop + 0x3c31, jupiter_tape.hdr_vars[loop]);
			cpu_writemem16(0x3c39, 0x00);
			cpu_writemem16(0x3c3a, 0x00);

			tmpword = cpu_readmem16(0x3c3b) + cpu_readmem16(0x3c3c) * 256 + jupiter_tape.hdr_len;

			cpu_writemem16(0x3c3b, tmpword & 0xff);
			cpu_writemem16(0x3c3c, (tmpword >> 8) & 0xff);

			cpu_writemem16(0x3c45, 0x0c);	/* ? */

			cpu_writemem16(0x3c4c, jupiter_tape.hdr_3c4c);
			cpu_writemem16(0x3c4d, jupiter_tape.hdr_3c4d);

			if (!cpu_readmem16(0x3c57) && !cpu_readmem16(0x3c58))
			{
				cpu_writemem16(0x3c57, 0x49);
				cpu_writemem16(0x3c58, 0x3c);
			}
			jupiter_exit_tap(0);
		}
	}
	return (-1);
}

static	int	jupiter_ramsize = 2;

void jupiter_init_machine(void)
{

	logerror("jupiter_init\r\n");
	logerror("data: %08X\n", jupiter_data);

	if (readinputport(8) != jupiter_ramsize)
	{
		jupiter_ramsize = readinputport(8);
		switch (jupiter_ramsize)
		{
			case 03:
			case 02:
				install_mem_write_handler(0, 0x8800, 0xffff, MWA_RAM);
				install_mem_read_handler(0, 0x8800, 0xffff, MRA_RAM);
				install_mem_write_handler(0, 0x4800, 0x87ff, MWA_RAM);
				install_mem_read_handler(0, 0x4800, 0x87ff, MRA_RAM);
				break;
			case 01:
				install_mem_write_handler(0, 0x8800, 0xffff, MWA_NOP);
				install_mem_read_handler(0, 0x8800, 0xffff, MRA_NOP);
				install_mem_write_handler(0, 0x4800, 0x87ff, MWA_RAM);
				install_mem_read_handler(0, 0x4800, 0x87ff, MRA_RAM);
				break;
			case 00:
				install_mem_write_handler(0, 0x8800, 0xffff, MWA_NOP);
				install_mem_read_handler(0, 0x8800, 0xffff, MRA_NOP);
				install_mem_write_handler(0, 0x4800, 0x87ff, MWA_NOP);
				install_mem_read_handler(0, 0x4800, 0x87ff, MRA_NOP);
				break;
		}

	}
	if (jupiter_data)
	{
		logerror("data: %08X. type: %d.\n", jupiter_data,
											jupiter_data_type);
		cpu_setOPbaseoverride(0, jupiter_opbaseoverride);
	}
}

void jupiter_stop_machine(void)
{
	logerror("jupiter_stop_machine\n");
	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
		jupiter_data_type = JUPITER_NONE;
	}

}

/* Load in .ace files. These are memory images of 0x2000 to 0x7fff
   and compressed as follows:

   ED 00		: End marker
   ED 01 ED		: 0xED
   ED <cnt> <byt>	: repeat <byt> count <cnt:3-240> times
   <byt>		: <byt>
*/

void	jupiter_exit_ace(int id)
{
	logerror("jupiter_exit_ace\n");
	/*
	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
		jupiter_data_type = JUPITER_NONE;
	}
	*/
}

int jupiter_load_ace(int id)
{

	void *file;
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done, jupiter_index;

	if (jupiter_data_type != JUPITER_NONE)
		return (0);
	jupiter_exit_ace(id);

	done = 0;
	jupiter_index = 0;
	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
		if ((jupiter_data = malloc(0x6000)))
		{
			logerror("Loading file %s.\r\n", device_filename(IO_CARTSLOT,id));
			while (!done && (jupiter_index < 0x6001))
			{
				osd_fread(file, &jupiter_byte, 1);
				if (jupiter_byte == 0xed)
				{
					osd_fread(file, &jupiter_byte, 1);
					switch (jupiter_byte)
					{
					case 0x00:
						logerror("File loaded!\r\n");
						done = 1;
						break;
					case 0x01:
						osd_fread(file, &jupiter_byte, 1);
						jupiter_data[jupiter_index++] = jupiter_byte;
						break;
					case 0x02:
						logerror("Sequence 0xED 0x02 found in .ace file\r\n");
						break;
					default:
						osd_fread(file, &jupiter_repeat, 1);
						for (loop = 0; loop < jupiter_byte; loop++)
							jupiter_data[jupiter_index++] = jupiter_repeat;
						break;
					}
				}
				else
					jupiter_data[jupiter_index++] = jupiter_byte;
			}
		}
		osd_fclose(file);
	}
	if (!done)
	{
		jupiter_exit_ace(id);
		logerror("file not loaded\r\n");
		return (1);
	}

	logerror("Decoded %d bytes.\r\n", jupiter_index);
	jupiter_data_type = JUPITER_ACE;

	logerror("data: %08X\n", jupiter_data);
	return (0);
}

void	jupiter_exit_tap(int id)
{
	logerror("jupiter_exit_tap\n");
	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
		jupiter_data_type = JUPITER_NONE;
	}
}

int jupiter_load_tap(int id)
{

	void *file;
	UINT8 inpbyt;
	int loop;
	UINT16 hdr_len;

	if (jupiter_data_type != JUPITER_NONE)
		return (0);
	jupiter_exit_tap(id);
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
		logerror("Loading file %s.\r\n", device_filename(IO_CASSETTE,id));

        osd_fread(file, &inpbyt, 1);
		hdr_len = inpbyt;
		osd_fread(file, &inpbyt, 1);
		hdr_len += (inpbyt * 256);

		/* Read header block */

		osd_fread(file, &jupiter_tape.hdr_type, 1);
		osd_fread(file, jupiter_tape.hdr_name, 10);
		osd_fread(file, &inpbyt, 1);
		jupiter_tape.hdr_len = inpbyt;
		osd_fread(file, &inpbyt, 1);
		jupiter_tape.hdr_len += (inpbyt * 256);
		osd_fread(file, &inpbyt, 1);
		jupiter_tape.hdr_addr = inpbyt;
		osd_fread(file, &inpbyt, 1);
		jupiter_tape.hdr_addr += (inpbyt * 256);
		osd_fread(file, &jupiter_tape.hdr_3c4c, 1);
		osd_fread(file, &jupiter_tape.hdr_3c4d, 1);
		osd_fread(file, jupiter_tape.hdr_vars, 8);
		if (hdr_len > 0x19)
			for (loop = 0x19; loop < hdr_len; loop++)
				osd_fread(file, &inpbyt, 1);

		/* Read data block */

		osd_fread(file, &inpbyt, 1);
		jupiter_tape.dat_len = inpbyt;
		osd_fread(file, &inpbyt, 1);
		jupiter_tape.dat_len += (inpbyt * 256);

		if ((jupiter_data = malloc(jupiter_tape.dat_len)))
		{
			osd_fread(file, jupiter_data, jupiter_tape.dat_len);
			jupiter_data_type = JUPITER_TAP;
			logerror("File loaded\r\n");
		}
		osd_fclose(file);
	}

	if (!jupiter_data)
	{
		jupiter_exit_tap(id);
		logerror("file not loaded\r\n");
		return (1);
	}

	return (0);

}

