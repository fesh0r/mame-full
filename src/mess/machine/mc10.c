/******************************************************************************
 MC-10

  TODO
	RS232
	Cassette Ouput
	Printer
	Disk
 *****************************************************************************/

#include "driver.h"
#include "mess/vidhrdw/m6847.h"

static int mc10_keyboard_strobe;

void mc10_init_machine(void)
{
	mc10_keyboard_strobe = 0xff;

	/* NPW: Taken from Juergen's MC-10 attempt that I just noticed... */
    if( readinputport(7) & 0x80 )
    {
		install_mem_read_handler(0, 0x5000, 0xbffe, MRA_RAM);
		install_mem_write_handler(0, 0x5000, 0xbffe, MWA_RAM);
    }
    else
    {
		install_mem_read_handler(0, 0x5000, 0xbffe, MRA_NOP);
		install_mem_write_handler(0, 0x5000, 0xbffe, MWA_NOP);
    }
	/* Install DOS ROM ? */
	if( readinputport(7) & 0x40 )
	{
		void *rom = osd_fopen(Machine->gamedrv->name, "mc10ext.rom", OSD_FILETYPE_IMAGE_R, 0);
		if( rom )
		{
			osd_fread(rom, memory_region(REGION_CPU1) + 0xc000, 0x2000);
			osd_fclose(rom);
        }
	}
	else
	{
		install_mem_read_handler(0, 0xc000, 0xdfff, MRA_NOP);
		install_mem_write_handler(0, 0xc000, 0xdfff, MWA_NOP);
    }
}

void mc10_stop_machine(void)
{
}

int mc10_interrupt(void)
{
	return ignore_interrupt();
}

int mc10_bfff_r(int offset)
{
	/*   BIT 0 KEYBOARD ROW 1
	 *   BIT 1 KEYBOARD ROW 2
	 *   BIT 2 KEYBOARD ROW 3
	 *   BIT 3 KEYBOARD ROW 4
	 *   BIT 4 KEYBOARD ROW 5
	 *   BIT 5 KEYBOARD ROW 6
	 */

    int val = 0x60;

	if ((input_port_0_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x01;
	if ((input_port_1_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;
	if ((input_port_2_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x04;
	if ((input_port_3_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x08;
	if ((input_port_4_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x10;
	if ((input_port_5_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x20;

	return val;
}

int mc10_port1_r(int offset)
{
	return mc10_keyboard_strobe;
}

void mc10_port1_w(int offset, int data)
{
	/*   BIT 0  KEYBOARD COLUMN 1 STROBE
	 *   BIT 1  KEYBOARD COLUMN 2 STROBE
	 *   BIT 2  KEYBOARD COLUMN 3 STROBE
	 *   BIT 3  KEYBOARD COLUMN 4 STROBE
	 *   BIT 4  KEYBOARD COLUMN 5 STROBE
	 *   BIT 5  KEYBOARD COLUMN 6 STROBE
	 *   BIT 6  KEYBOARD COLUMN 7 STROBE
	 *   BIT 7  KEYBOARD COLUMN 8 STROBE
	 */
	mc10_keyboard_strobe = data;
}

int mc10_port2_r(int offset)
{
	/*   BIT 1 KEYBOARD SHIFT/CONTROL KEYS INPUT
	 * ! BIT 2 PRINTER OTS INPUT
	 * ! BIT 3 RS232 INPUT DATA
	 *   BIT 4 CASSETTE TAPE INPUT
	 */

	int val = 0xed;

	if ((input_port_6_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;

	if (device_input(IO_CASSETTE, 0) >= 0)
		val |= 0x10;

	return val;
}

void mc10_port2_w(int offset, int data)
{
	/* ! BIT 0 PRINTER OUTFUT & CASS OUTPUT
	 */

	/* Nothing here implemented yet */
}

/* --------------------------------------------------
 * Video hardware
 * -------------------------------------------------- */

int mc10_vh_start(void)
{
	if (m6847_vh_start())
		return 1;

	m6847_set_vram(memory_region(REGION_CPU1), 0x7fff);
	m6847_set_video_offset(0x4000);
	m6847_set_artifact_dipswitch(7);
	return 0;
}

void mc10_bfff_w(int offset, int data)
{
	/*   BIT 2 GM2 6847 CONTROL & INT/EXT CONTROL
	 *   BIT 3 GM1 6847 CONTROL
	 *   BIT 4 GM0 6847 CONTROL
	 *   BIT 5 A/G 684? CONTROL
	 *   BIT 6 CSS 6847 CONTROL
	 *   BIT 7 SOUND OUTPUT BIT
	 */

	int video_mode = 0;

	/* The following code merely maps the MC-10 video mode registers
	 * onto the CoCo/Dragon registers
	 *
	 * GM2 --> video_mode bit 3
	 * GM1 --> video_mode bit 2
	 * GM0 --> video_mode bit 1
	 * A/G --> video_mode bit 4
	 * CSS --> video_mode bit 0
	 */

	if (data & 0x04)
		video_mode |= 0x08;
	if (data & 0x08)
		video_mode |= 0x04;
	if (data & 0x10)
		video_mode |= 0x02;
	if (data & 0x20)
		video_mode |= 0x10;
	if (data & 0x40)
		video_mode |= 0x01;

	m6847_set_mode(video_mode);
	DAC_data_w(0, data & 0x80);
}

void mc10_ram_w (int offset, int data)
{
	/* from vidhrdw/dragon.c */
	extern void coco_ram_w (int offset_loc, int data_loc);
	coco_ram_w(offset + 0x4000, data);
}
