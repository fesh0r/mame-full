/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/galaxy.h"

#define GALAXY_SNAPSHOT_SIZE	8268

int galaxy_interrupts_enabled = TRUE;

/***************************************************************************
  Keyboard
***************************************************************************/

 READ8_HANDLER( galaxy_kbd_r )
{
	int port = offset/8;
	int bit = offset%8;
	return readinputport(port)&0x01<<bit ? 0xfe : 0xff;
}

WRITE8_HANDLER( galaxy_kbd_w )
{
}

/***************************************************************************
  Interrupts
***************************************************************************/

INTERRUPT_GEN( galaxy_interrupt )
{
	cpunum_set_input_line(0, 0, PULSE_LINE);
}

static int galaxy_irq_callback (int cpu)
{
	galaxy_interrupts_enabled = TRUE;
	return 1;
}

/***************************************************************************
  Snapshot files (GAL)
***************************************************************************/

static void galaxy_setup_snapshot (const UINT8 * data)
{
	int i;
	unsigned char lo,hi;

	/* Set registers */
	lo = data[0] & 0x0ff;
	hi = data[1] & 0x0ff;
	cpunum_set_reg(0, Z80_AF, (hi << 8) | lo);
	lo = data[4] & 0x0ff;
	hi = data[5] & 0x0ff;
	cpunum_set_reg(0, Z80_BC, (hi << 8) | lo);
	lo = data[8] & 0x0ff;
	hi = data[9] & 0x0ff;
	cpunum_set_reg(0, Z80_DE, (hi << 8) | lo);
	lo = data[12] & 0x0ff;
	hi = data[13] & 0x0ff;
	cpunum_set_reg(0, Z80_HL, (hi << 8) | lo);
	lo = data[16] & 0x0ff;
	hi = data[17] & 0x0ff;
	cpunum_set_reg(0, Z80_IX, (hi << 8) | lo);
	lo = data[20] & 0x0ff;
	hi = data[21] & 0x0ff;
	cpunum_set_reg(0, Z80_IY, (hi << 8) | lo);
	lo = data[24] & 0x0ff;
	hi = data[25] & 0x0ff;
	cpunum_set_reg(0, Z80_PC, (hi << 8) | lo);
	lo = data[28] & 0x0ff;
	hi = data[29] & 0x0ff;
	cpunum_set_reg(0, Z80_SP, (hi << 8) | lo);
	lo = data[32] & 0x0ff;
	hi = data[33] & 0x0ff;
	cpunum_set_reg(0, Z80_AF2, (hi << 8) | lo);
	lo = data[36] & 0x0ff;
	hi = data[37] & 0x0ff;
	cpunum_set_reg(0, Z80_BC2, (hi << 8) | lo);
	lo = data[40] & 0x0ff;
	hi = data[41] & 0x0ff;
	cpunum_set_reg(0, Z80_DE2, (hi << 8) | lo);
	lo = data[44] & 0x0ff;
	hi = data[45] & 0x0ff;
	cpunum_set_reg(0, Z80_HL2, (hi << 8) | lo);
	cpunum_set_reg(0, Z80_IFF1, data[48]&0x0ff);
	cpunum_set_reg(0, Z80_IFF2, data[52]&0x0ff);
	cpunum_set_reg(0, Z80_HALT, data[56]&0x0ff);
	cpunum_set_reg(0, Z80_IM, data[60]&0x0ff);
	cpunum_set_reg(0, Z80_I, data[64]&0x0ff);
	cpunum_set_reg(0, Z80_R, (data[68]&0x7f) | (data[72]&0x80));
	activecpu_set_input_line(INPUT_LINE_NMI, 0);
	activecpu_set_input_line(0, 0);

	/* Memory dump */
	for (i = 0; i < GALAXY_SNAPSHOT_SIZE-76; i++)
		program_write_byte(i + 0x2000, data[i+76]);
}

SNAPSHOT_LOAD( galaxy )
{
	UINT8 *galaxy_snapshot_data;

	if (snapshot_size != GALAXY_SNAPSHOT_SIZE)
	{
		logerror ("Incomplete snapshot file\n");
		return INIT_FAIL;
	}

	galaxy_snapshot_data = malloc(GALAXY_SNAPSHOT_SIZE);
	if (!galaxy_snapshot_data)
	{
		logerror ("Unable to load snapshot file\n");
		return INIT_FAIL;
	}

	mame_fread(fp, galaxy_snapshot_data, GALAXY_SNAPSHOT_SIZE);

	galaxy_setup_snapshot(galaxy_snapshot_data);
	free(galaxy_snapshot_data);

	logerror("Snapshot file loaded\n");
	return INIT_PASS;
}


/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_INIT( galaxy )
{
	cpu_set_irq_callback(0, galaxy_irq_callback);
}
