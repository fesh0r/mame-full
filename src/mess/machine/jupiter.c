/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"

int jupiter_load_ace(int id, const char *name);
int jupiter_load_tap(int id, const char *name);

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

int jupiter_opbaseoverride(int pc)
{

	int loop;
	unsigned short tmpword;

	if (pc == 0x059d)
	{
		cpu_setOPbaseoverride(0, 0);
		if (jupiter_data_type == JUPITER_ACE)
		{
			for (loop = 0; loop < 0x6000; loop++)
				cpu_writemem16(loop + 0x2000, jupiter_data[loop]);
		}
		else
		if (jupiter_data_type == JUPITER_TAP)
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
		}
	}

	return (-1);

}

void jupiter_init_machine(void)
{

	if (errorlog)
		fprintf(errorlog, "jupiter_init\r\n");

	if (jupiter_data)
	{
		cpu_setOPbaseoverride(0, jupiter_opbaseoverride);
	}

}

void jupiter_stop_machine(void)
{

	if (jupiter_data)
	{
		free(jupiter_data);
		jupiter_data = NULL;
	}

}

/* Load in .ace files. These are memory images of 0x2000 to 0x7fff
   and compressed as follows:

   ED 00		: End marker
   ED 01 ED		: 0xED
   ED <cnt> <byt>	: repeat <byt> count <cnt:3-240> times
   <byt>		: <byt>
*/

int jupiter_load_ace(int id, const char *name)
{

	void *file;
	unsigned char jupiter_repeat, jupiter_byte, loop;
	int done, jupiter_index;

	done = 0;
	jupiter_index = 0;
	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
		if ((jupiter_data = malloc(0x6000)))
		{
			if (errorlog)
				fprintf(errorlog, "Loading file %s.\r\n", name);
			while (!done && (jupiter_index < 0x6001))
			{
				osd_fread(file, &jupiter_byte, 1);
				if (jupiter_byte == 0xed)
				{
					osd_fread(file, &jupiter_byte, 1);
					switch (jupiter_byte)
					{
					case 0x00:
						if (errorlog)
							fprintf(errorlog, "File loaded!\r\n");
						done = 1;
						break;
					case 0x01:
						osd_fread(file, &jupiter_byte, 1);
						jupiter_data[jupiter_index++] = jupiter_byte;
						break;
					case 0x02:
						if (errorlog)
							fprintf(errorlog, "Sequence 0xED 0x02 found in .ace file\r\n");
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
		if (errorlog)
			fprintf(errorlog, "file not loaded\r\n");
		if (jupiter_data)
		{
			free(jupiter_data);
			jupiter_data = NULL;
		}
		return (1);
	}

	if (errorlog)
		fprintf(errorlog, "Decoded %d bytes.\r\n", jupiter_index);
	jupiter_data_type = JUPITER_ACE;

	return (0);

}

int jupiter_load_tap(int id, const char *name)
{

	void *file;
	UINT8 inpbyt;
	int loop;
	UINT16 hdr_len;

	if (errorlog)
		fprintf(errorlog, "Loading file %s.\r\n", name);

	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, 0);
	if (file)
	{
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
			if (errorlog)
				fprintf(errorlog, "File loaded\r\n");
		}
		osd_fclose(file);
	}

	if (!jupiter_data)
	{
		if (errorlog)
			fprintf(errorlog, "file not loaded\r\n");
		return (1);
	}

	return (0);

}
