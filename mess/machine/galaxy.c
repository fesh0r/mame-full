/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/galaxy.h"
#include "cassette.h"

#define	galaxy_NONE	0
#define	galaxy_GAL	1

static UINT8 *galaxy_data = NULL;
static unsigned long galaxy_data_size = 0;
static int galaxy_data_type = galaxy_NONE;

static void galaxy_setup_gal(UINT8*, unsigned long);
static OPBASE_HANDLER(galaxy_opbaseoverride);

int galaxy_interrupts_enabled = TRUE;


static int galaxy_irq_callback (int cpu)
{
	galaxy_interrupts_enabled = TRUE;
	return 1;
}

MACHINE_INIT( galaxy )
{
	logerror("galaxy_init\r\n");
	if (galaxy_data)
	{
		logerror("data: %08X. type: %d.\n", galaxy_data,galaxy_data_type);
		memory_set_opbase_handler(0, galaxy_opbaseoverride);
	}
	cpu_set_irq_callback(0, galaxy_irq_callback);

}

READ_HANDLER( galaxy_kbd_r )
{
	int port = offset/8;
	int bit = offset%8;
	return readinputport(port)&0x01<<bit ? 0xfe : 0xff;
}

WRITE_HANDLER( galaxy_kbd_w )
{
}

int galaxy_init_wav(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 11025;
	return cassette_init(id, &args);
}

int galaxy_load_snap(int id)
{
	void *file;

	logerror("galaxy_load_gal\n");
	file = image_fopen_new(IO_SNAPSHOT, id, NULL);

	if (file)
	{

		galaxy_data_size = osd_fsize(file);

		if (galaxy_data_size != 0)
		{
			galaxy_data = malloc(galaxy_data_size);

			if (galaxy_data != NULL)
			{
				osd_fread(file, galaxy_data, galaxy_data_size);
				osd_fclose(file);

				galaxy_data_type = galaxy_GAL;

				logerror("data_size %d\n", galaxy_data_size);

				logerror("File loaded!\n");
				return 0;
			}
			osd_fclose(file);
		}
		return 1;
	}
	return 0;
}

void	galaxy_exit_snap(int id)
{
	logerror("galaxy_exit_snap\n");
	if (galaxy_data)
	{
		free(galaxy_data);
		galaxy_data = NULL;
		galaxy_data_size = 0;
		memory_set_opbase_handler(0, 0);
	}
	galaxy_data_type = galaxy_NONE;
}

static OPBASE_HANDLER( galaxy_opbaseoverride )
{
	logerror("galaxy_opbaseoverride\n");

	/* clear op base override */
	memory_set_opbase_handler(0, 0);

	if (galaxy_data_type == galaxy_GAL)
		galaxy_setup_gal(galaxy_data,galaxy_data_size);
	logerror("Snapshot loaded - new PC = %04x\n", activecpu_get_reg(Z80_PC) & 0x0ffff);

	return (activecpu_get_reg(Z80_PC) & 0x0ffff);
}


static void galaxy_setup_gal(unsigned char *data, unsigned long data_size)
{
	int i;
	unsigned char lo,hi;

	/* Set registers */
	lo = data[0] & 0x0ff;
	hi = data[1] & 0x0ff;
	activecpu_set_reg(Z80_AF, (hi << 8) | lo);
	lo = data[4] & 0x0ff;
	hi = data[5] & 0x0ff;
	activecpu_set_reg(Z80_BC, (hi << 8) | lo);
	lo = data[8] & 0x0ff;
	hi = data[9] & 0x0ff;
	activecpu_set_reg(Z80_DE, (hi << 8) | lo);
	lo = data[12] & 0x0ff;
	hi = data[13] & 0x0ff;
	activecpu_set_reg(Z80_HL, (hi << 8) | lo);
	lo = data[16] & 0x0ff;
	hi = data[17] & 0x0ff;
	activecpu_set_reg(Z80_IX, (hi << 8) | lo);
	lo = data[20] & 0x0ff;
	hi = data[21] & 0x0ff;
	activecpu_set_reg(Z80_IY, (hi << 8) | lo);
	lo = data[24] & 0x0ff;
	hi = data[25] & 0x0ff;
	activecpu_set_reg(Z80_PC, (hi << 8) | lo);
	lo = data[28] & 0x0ff;
	hi = data[29] & 0x0ff;
	activecpu_set_reg(Z80_SP, (hi << 8) | lo);
	lo = data[32] & 0x0ff;
	hi = data[33] & 0x0ff;
	activecpu_set_reg(Z80_AF2, (hi << 8) | lo);
	lo = data[36] & 0x0ff;
	hi = data[37] & 0x0ff;
	activecpu_set_reg(Z80_BC2, (hi << 8) | lo);
	lo = data[40] & 0x0ff;
	hi = data[41] & 0x0ff;
	activecpu_set_reg(Z80_DE2, (hi << 8) | lo);
	lo = data[44] & 0x0ff;
	hi = data[45] & 0x0ff;
	activecpu_set_reg(Z80_HL2, (hi << 8) | lo);
	activecpu_set_reg(Z80_IFF1, data[48]&0x0ff);
	activecpu_set_reg(Z80_IFF2, data[52]&0x0ff);
	activecpu_set_reg(Z80_HALT, data[56]&0x0ff);
	activecpu_set_reg(Z80_IM, data[60]&0x0ff);
	activecpu_set_reg(Z80_I, data[64]&0x0ff);

	activecpu_set_reg(Z80_R, (data[68]&0x7f) | (data[72]&0x80));

	activecpu_set_reg(Z80_NMI_STATE, 0);
	activecpu_set_reg(Z80_IRQ_STATE, 0);

	/* Memory dump */

	for (i = 0; i < data_size-76; i++)
	   cpu_writemem16(i + 0x2000, data[i+76]);

}

