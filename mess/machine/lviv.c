/***************************************************************************

  machine.c

  Functions to emulate general aspects of PK-01 Lviv (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/lviv.h"
#include "machine/8255ppi.h"

unsigned char * lviv_ram;
unsigned char * lviv_video_ram;

static UINT8 lviv_ppi_port_outputs[2][3];

static UINT8 *lviv_data = NULL;
static unsigned long lviv_data_size = 0;
static OPBASE_HANDLER(lviv_opbaseoverride);

void lviv_update_memory (void)
{
	if (lviv_ppi_port_outputs[0][2] & 0x02)
	{
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);

		memory_set_bankhandler_w(5, 0, MWA_BANK5);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);

		cpu_setbank(1, lviv_ram);
		cpu_setbank(2, lviv_ram + 0x4000);

		cpu_setbank(5, lviv_ram);
		cpu_setbank(6, lviv_ram + 0x4000);
	}
	else
	{
		memory_set_bankhandler_r(1, 0, MRA_NOP);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);

		memory_set_bankhandler_w(5, 0, MWA_NOP);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);

		cpu_setbank(2, lviv_video_ram);
		cpu_setbank(6, lviv_video_ram);
	}
}

READ_HANDLER ( lviv_ppi_0_porta_r )
{
	return 0xff;
}

READ_HANDLER ( lviv_ppi_0_portb_r )
{
	return 0xff;
}

READ_HANDLER ( lviv_ppi_0_portc_r )
{
	return 0xff;
}

WRITE_HANDLER ( lviv_ppi_0_porta_w )
{
	lviv_ppi_port_outputs[0][0] = data;
}

WRITE_HANDLER ( lviv_ppi_0_portb_w )
{
	lviv_ppi_port_outputs[0][1] = data;
}

WRITE_HANDLER ( lviv_ppi_0_portc_w )	/* tape in/out, video memory on/off */
{
	lviv_ppi_port_outputs[0][2] = data;
	lviv_update_memory();
}

READ_HANDLER ( lviv_ppi_1_porta_r )
{
	return 0xff;
}

READ_HANDLER ( lviv_ppi_1_portb_r )	/* keyboard reading */
{
	return	((lviv_ppi_port_outputs[1][0]&0x01) ? 0xff : readinputport(0)) &
		((lviv_ppi_port_outputs[1][0]&0x02) ? 0xff : readinputport(1)) &
		((lviv_ppi_port_outputs[1][0]&0x04) ? 0xff : readinputport(2)) &
		((lviv_ppi_port_outputs[1][0]&0x08) ? 0xff : readinputport(3)) &
		((lviv_ppi_port_outputs[1][0]&0x10) ? 0xff : readinputport(4)) &
		((lviv_ppi_port_outputs[1][0]&0x20) ? 0xff : readinputport(5)) &
		((lviv_ppi_port_outputs[1][0]&0x40) ? 0xff : readinputport(6)) &
		((lviv_ppi_port_outputs[1][0]&0x80) ? 0xff : readinputport(7));
}

READ_HANDLER ( lviv_ppi_1_portc_r )     /* keyboard reading */
{
	return	((lviv_ppi_port_outputs[1][2]&0x01) ? 0xff : readinputport(8))  &
		((lviv_ppi_port_outputs[1][2]&0x02) ? 0xff : readinputport(9))  &
		((lviv_ppi_port_outputs[1][2]&0x04) ? 0xff : readinputport(10)) &
		((lviv_ppi_port_outputs[1][2]&0x08) ? 0xff : readinputport(11));
}

WRITE_HANDLER ( lviv_ppi_1_porta_w )	/* kayboard scaning */
{
	lviv_ppi_port_outputs[1][0] = data;
}

WRITE_HANDLER ( lviv_ppi_1_portb_w )
{
	lviv_ppi_port_outputs[1][1] = data;
}

WRITE_HANDLER ( lviv_ppi_1_portc_w )	/* kayboard scaning */
{
	lviv_ppi_port_outputs[1][2] = data;
}

static ppi8255_interface lviv_ppi8255_interface =
{
	2,
	{lviv_ppi_0_porta_r, lviv_ppi_1_porta_r},
	{lviv_ppi_0_portb_r, lviv_ppi_1_portb_r},
	{lviv_ppi_0_portc_r, lviv_ppi_1_portc_r},
	{lviv_ppi_0_porta_w, lviv_ppi_1_porta_w},
	{lviv_ppi_0_portb_w, lviv_ppi_1_portb_w},
	{lviv_ppi_0_portc_w, lviv_ppi_1_portc_w}
};

void lviv_init_machine(void)
{
	ppi8255_init(&lviv_ppi8255_interface);

	lviv_ram = (unsigned char *)malloc(0xffff); /* 48kB RAM and 16 kB Video RAM */
	lviv_video_ram = lviv_ram + 0xc000;

	if (lviv_ram)
	{
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);

		memory_set_bankhandler_w(5, 0, MWA_BANK5);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);
		memory_set_bankhandler_w(7, 0, MWA_BANK7);
		memory_set_bankhandler_w(8, 0, MWA_ROM);

		cpu_setbank(1, lviv_ram);
		cpu_setbank(2, lviv_ram + 0x4000);
		cpu_setbank(3, lviv_ram + 0x8000);
		cpu_setbank(4, memory_region(REGION_CPU1) + 0x010000);

		cpu_setbank(5, lviv_ram);
		cpu_setbank(6, lviv_ram + 0x4000);
		cpu_setbank(7, lviv_ram + 0x8000);
		cpu_setbank(8, memory_region(REGION_CPU1) + 0x010000);

		memset(lviv_ram, 0, sizeof(unsigned char)*0xffff);

		lviv_ram[0x0000] = 0xc3;		
		lviv_ram[0x0001] = 0x00;		
		lviv_ram[0x0002] = 0xc0;		
	}
}

void lviv_stop_machine(void)
{
	if(lviv_ram)
	{
		free(lviv_ram);
		lviv_ram = NULL;
	}
}

int lviv_snap_load (int id)
{
	FILE *fp;

	if (device_filename(IO_QUICKLOAD, id) == NULL)
		return INIT_PASS;

	fp = image_fopen(IO_QUICKLOAD, id, OSD_FILETYPE_IMAGE, 0);
	if (!fp) return INIT_FAIL;

	lviv_data_size = osd_fsize(fp);

	if (lviv_data_size == 0) return INIT_FAIL;

	if ((lviv_data = (UINT8 *) malloc(lviv_data_size)) == NULL)
	{
		osd_fclose(fp);
		return INIT_FAIL;
	}


	osd_fread(fp, lviv_data, lviv_data_size);
	osd_fclose(fp);

	if (lviv_data[9] != 0xd0)
	{
		free (lviv_data);
		return INIT_FAIL;
	}

	logerror("File loaded!\n");
	memory_set_opbase_handler(0, lviv_opbaseoverride);

	return INIT_PASS;
}

void lviv_snap_exit(int id)
{
	if (lviv_data != NULL)
		free(lviv_data);
}


static void lviv_snap_open (unsigned char *data)
{
	int i;

	UINT8 hi, lo;

	UINT16 lviv_begin;
	UINT16 lviv_end;
	UINT16 lviv_start;

	lo = data[16] & 0x0ff;
	hi = data[17] & 0x0ff;
	lviv_begin = (hi << 8) | lo;

	lo = data[18] & 0x0ff;
	hi = data[19] & 0x0ff;
	lviv_end = (hi << 8) | lo;

	lo = data[20] & 0x0ff;
	hi = data[21] & 0x0ff;
	lviv_start = (hi << 8) | lo;

	for (i = 0; i < lviv_end-lviv_start+1; i++)
		cpu_writemem16(i + lviv_begin, data[i+22]);

	activecpu_set_reg(REG_PC, lviv_start);
}

static OPBASE_HANDLER( lviv_opbaseoverride )
{
	/* clear op base override */
	memory_set_opbase_handler(0, 0);

	lviv_snap_open (lviv_data);

	return (activecpu_get_reg(REG_PC) & 0x0ffff);
}

