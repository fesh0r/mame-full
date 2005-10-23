/******************************************************************************
 MC-10

  TODO
	RS232
	Printer
	Disk
 *****************************************************************************/

#include "driver.h"
#include "vidhrdw/m6847.h"
#include "includes/mc10.h"
#include "devices/cassette.h"
#include "sound/dac.h"

static int mc10_keyboard_strobe;

void mc10_init_machine(void)
{
	mc10_keyboard_strobe = 0xff;

	/* NPW: Taken from Juergen's MC-10 attempt that I just noticed... */
	if( readinputport(7) & 0x80 )
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MRA8_RAM);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MWA8_RAM);
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0xbffe, 0, 0, MWA8_NOP);
	}
	/* Install DOS ROM ? */
	if( readinputport(7) & 0x40 )
	{
		mame_file *rom = mame_fopen(Machine->gamedrv->name, "mc10ext.rom", FILETYPE_IMAGE, 0);
		if( rom )
			mame_fread(rom, memory_region(REGION_CPU1) + 0xc000, 0x2000);
	}
	else
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MWA8_NOP);
    }
}

 READ8_HANDLER ( mc10_bfff_r )
{
	/*   BIT 0 KEYBOARD ROW 1
	 *   BIT 1 KEYBOARD ROW 2
	 *   BIT 2 KEYBOARD ROW 3
	 *   BIT 3 KEYBOARD ROW 4
	 *   BIT 4 KEYBOARD ROW 5
	 *   BIT 5 KEYBOARD ROW 6
	 */

	int val = 0x40;

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

 READ8_HANDLER ( mc10_port1_r )
{
	return mc10_keyboard_strobe;
}

WRITE8_HANDLER ( mc10_port1_w )
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

 READ8_HANDLER ( mc10_port2_r )
{
	/*   BIT 1 KEYBOARD SHIFT/CONTROL KEYS INPUT
	 * ! BIT 2 PRINTER OTS INPUT
	 * ! BIT 3 RS232 INPUT DATA
	 *   BIT 4 CASSETTE TAPE INPUT
	 */

	mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);
	int val = 0xed;

	if ((input_port_6_r(0) | mc10_keyboard_strobe) == 0xff)
		val |= 0x02;

	if (cassette_input(img) >= 0)
		val |= 0x10;

	return val;
}

WRITE8_HANDLER ( mc10_port2_w )
{
	mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);

	/*   BIT 0 PRINTER OUTFUT & CASS OUTPUT
	 */

	cassette_output(img, (data & 0x01) ? +1.0 : -1.0);
}

/* --------------------------------------------------
 * Video hardware
 * -------------------------------------------------- */

VIDEO_START( mc10 )
{
	extern void dragon_charproc(UINT8 c);
	struct m6847_init_params p;

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL_NTSC;
	p.artifactdipswitch = 7;
	p.ram = mess_ram;
	p.ramsize = mess_ram_size;
	p.charproc = dragon_charproc;
	p.initial_video_offset = 0;

	return video_start_m6847(&p);
}

WRITE8_HANDLER ( mc10_bfff_w )
{
	/*   BIT 2 GM2 6847 CONTROL & INT/EXT CONTROL
	 *   BIT 3 GM1 6847 CONTROL
	 *   BIT 4 GM0 6847 CONTROL
	 *   BIT 5 A/G 684? CONTROL
	 *   BIT 6 CSS 6847 CONTROL
	 *   BIT 7 SOUND OUTPUT BIT
	 */

	m6847_gm2_w(0,	data & 0x04);
	m6847_gm1_w(0,	data & 0x08);
	m6847_gm0_w(0,	data & 0x10);
	m6847_ag_w(0,	data & 0x20);
	m6847_css_w(0,	data & 0x40);
	DAC_data_w(0, data & 0x80);

	m6847_set_cannonical_row_height();
	schedule_full_refresh();
}



static WRITE8_HANDLER ( mc10_ram_w )
{
	if (mess_ram[offset] != data)
	{
		m6847_touch_vram(offset);
		mess_ram[offset] = data;
	}
}



DRIVER_INIT( mc10 )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1,
		0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4000 + mess_ram_size - 1,
		0, 0, mc10_ram_w);
	memory_set_bankptr(1, mess_ram);
}

