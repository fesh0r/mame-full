/**********************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrups,
  I/O ports)

**********************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static	int		nascom2_tape_size = 0;
static	UINT8	*nascom2_tape_image = NULL;
static	int		nascom2_tape_index = 0;

void nascom1_init_machine(void)
{

	logerror("nascom1_init\r\n");

}

void nascom1_stop_machine(void)
{

}

/* .cas files and ascii .nas are in the same format, for Basic and monitor
   loading respectively:

   <addr> <byte> x 8 ^H^H^J

   Note <addr> and <byte> are in hex.

   Binary .nas files are assumed to load at 0x1000
*/

int	nascom2_init_cassette(int id)
{
	void	*file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		nascom2_tape_size = osd_fsize(file);
		nascom2_tape_image = (UINT8 *)malloc(nascom2_tape_size);
		if (!nascom2_tape_image || (osd_fread(file, nascom2_tape_image, nascom2_tape_size) != nascom2_tape_size))
		{
			osd_fclose(file);
			return (1);
		}
		else
		{
			osd_fclose(file);
			nascom2_tape_index = 0;
			return (0);
		}
	}
	return (1);
}

void nascom2_exit_cassette(int id)
{
	if (nascom2_tape_image)
	{
		free(nascom2_tape_image);
		nascom2_tape_image = NULL;
		nascom2_tape_size = nascom2_tape_index = 0;
	}
}

int	nascom2_read_cassette(void)
{
	if (nascom2_tape_image && (nascom2_tape_index < nascom2_tape_size))
					return (nascom2_tape_image[nascom2_tape_index++]);
	return (0);
}
