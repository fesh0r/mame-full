/***************************************************************************

  machine.c

  Functions to emulate general aspects of PK-01 Lviv (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cassette.h"
#include "cpu/i8085/i8085.h"
#include "includes/lviv.h"
#include "machine/8255ppi.h"
#include "formats/lviv_lvt.h"

unsigned char * lviv_ram;
unsigned char * lviv_video_ram;

static UINT8 lviv_ppi_port_outputs[2][3];

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
	UINT8 data = 0xff;
	if (!(device_input(IO_CASSETTE,0) > 255))
		data &= 0xef;
	return data;
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
	speaker_level_w(0, data&0x01);
	device_output(IO_CASSETTE, 0, (data & 0x01) ? -32768 : 32767);
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

int lviv_tape_init(int id)
{
	void *file;
	struct wave_args wa;

	if (device_filename(IO_CASSETTE, id)==NULL)
		return INIT_PASS;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
	if( file )
	{
		int lviv_lvt_size;

		lviv_lvt_size = osd_fsize(file);

		logerror("Lviv .lvt size: %04x\n",lviv_lvt_size);

		if (lviv_lvt_size!=0)
		{
			UINT8 *lviv_lvt_data;

			lviv_lvt_data = (UINT8 *)malloc(lviv_lvt_size);

			if (lviv_lvt_data!=NULL)
			{
				int size_in_samples;

				osd_fread(file, lviv_lvt_data, lviv_lvt_size);
				size_in_samples = lviv_cassette_calculate_size_in_samples(lviv_lvt_size, lviv_lvt_data);
				osd_fseek(file, 0, SEEK_SET);
				free(lviv_lvt_data);
				logerror("size in samples: %d\n",size_in_samples);

				memset(&wa, 0, sizeof(&wa));
				wa.file = file;
				wa.chunk_size = lviv_lvt_size;
				wa.chunk_samples = size_in_samples;
                                wa.smpfreq = 44100;
				wa.fill_wave = lviv_cassette_fill_wave;
				wa.header_samples = 0;
				wa.trailer_samples = 0;
				wa.display = 1;
				if( device_open(IO_CASSETTE,id,0,&wa) )
					return INIT_FAIL;

				return INIT_PASS;
			}

			return INIT_FAIL;
		}
	}

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW_CREATE);
	if( file )
	{
		memset(&wa, 0, sizeof(&wa));
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 44100;
		if( device_open(IO_CASSETTE,id,1,&wa) )
        		return INIT_FAIL;
		return INIT_PASS;
	}

	return INIT_FAIL;
}

void lviv_tape_exit(int id)
{
	device_close(IO_CASSETTE, id);
}
