/***************************************************************************

  primo.c

  Functions to emulate general aspects of Primo (RAM, ROM, interrupts,
  I/O ports)

  Krzysztof Strzecha

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "includes/cbmserb.h"
#include "includes/primo.h"
#include "sound/speaker.h"

static int primo_nmi = 0;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

INTERRUPT_GEN( primo_vblank_interrupt )
{
	if (primo_nmi)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

READ8_HANDLER( primo_be_1_r )
{
	UINT8 data = 0x00;

	// bit 7, 6 - not used

	// bit 5 - VBLANK
	data |= (cpu_getvblank()) ? 0x20 : 0x00;

	// bit 4 - I4 (external bus)

	// bit 3 - I3 (external bus)

	// bit 2 - cassette
	data |= (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) < 0.1) ? 0x04 : 0x00;

	// bit 1 - reset button
	data |= (readinputport(4)) ? 0x02 : 0x00;

	// bit 0 - keyboard
	data |= (readinputport((offset&0x0030)>>4) >> (offset&0x000f))&0x0001 ? 0x01 : 0x00;

	return data;
}

READ8_HANDLER( primo_be_2_r )
{
	UINT8 data = 0xff;

	// bit 7, 6 - not used

	// bit 5 - SCLK
	if (!serial_clock || !cbm_serial_clock_read ())
		data &= ~0x20; 

	// bit 4 - SDATA
	if (!serial_data || !cbm_serial_data_read ())
		data &= ~0x10;

	// bit 3 - SRQ
//	data &= (!cbm_serial_request_read ()) ? ~0x08 : ~0x00;

	// bit 2 - joystic 2 (not implemeted yet)

	// bit 1 - ATN
	if (!serial_atn || !cbm_serial_atn_read ())
		data &= ~0x02;

	// bit 0 - joystic 1 (not implemeted yet)

	logerror ("IOR BE-2 data:%02x\n", data);
	return data;
}

WRITE8_HANDLER( primo_ki_1_w )
{
	// bit 7 - NMI generator enable/disable
	primo_nmi = (data & 0x80) ? 1 : 0;

	// bit 6 - joystick register shift (not emulated)

	// bit 5 - V.24 (2) / tape control (not emulated)

	// bit 4 - speaker
	speaker_level_w(0, (data&0x10)>>4);
	
	// bit 3 - display buffer
	if (data & 0x08)
		primo_video_memory_base |= 0x4000;
	else
		primo_video_memory_base &= 0xdfff;

	// bit 2 - V.24 (1) / tape control (not emulated)

	// bit 1, 0 - cassette output
	switch (data & 0x03)
	{
		case 0:
			cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), -1.0);
			break;
		case 1:
		case 2:
			cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), 0.0);
			break;
		case 3:
			cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), 1.0);
			break;
	}
}

WRITE8_HANDLER( primo_ki_2_w )
{
	// bit 7, 6 - not used

	// bit 5 - SCLK
	cbm_serial_clock_write (serial_clock = !(data & 0x20));
	logerror ("W - SCLK: %d ", serial_clock ? 1 : 0);

	// bit 4 - SDATA
	cbm_serial_data_write (serial_data = !(data & 0x10));
	logerror ("SDATA: %d ", serial_data ? 1 : 0);

	// bit 3 - not used

	// bit 2 - SRQ
	cbm_serial_request_write (!(data & 0x04));

	// bit 1 - ATN
	cbm_serial_atn_write (serial_atn = !(data & 0x02));
	logerror ("ATN: %d\n", serial_atn ? 1 : 0);

	// bit 0 - not used

	if (data & 0x04)
		logerror ("SRQ WRITE\n");
//	logerror ("IOW KI-2 data:%02x\n", data);
}

DRIVER_INIT( primo32 )
{
	primo_video_memory_base = 0x6800;
}

DRIVER_INIT( primo48 )
{
	primo_video_memory_base = 0xa800;
}

DRIVER_INIT( primo64 )
{
	primo_video_memory_base = 0xe800;
}

MACHINE_INIT( primoa )
{
	cpunum_set_clockscale(0, readinputport(5) ? 1.5 : 1.0);
}

MACHINE_INIT( primob )
{
	cpunum_set_clockscale(0, readinputport(5) ? 1.5 : 1.0);

	cbm_serial_reset_write (0);
	cbm_drive_0_config (SERIAL, 8);
	cbm_drive_1_config (SERIAL, 9);
}

static void primo_setup_pp (UINT8* snapshot_data, UINT32 snapshot_size)
{
	int i;

	UINT16 load_addr;
	UINT16 start_addr;

	load_addr = snapshot_data[0] + snapshot_data[1]*256;
	start_addr = snapshot_data[2] + snapshot_data[3]*256;

	for (i=4; i<snapshot_size; i++)
		program_write_byte(start_addr+i-4, snapshot_data[i]);

	cpunum_set_reg(0, Z80_PC, start_addr);

	logerror ("Snapshot .pp l: %04x r: %04x s: %04x\n", load_addr, start_addr, snapshot_size-4);
}

static void primo_setup_pss (UINT8* snapshot_data, UINT32 snapshot_size)
{
	int i;

	/* Z80 registers */

	cpunum_set_reg(0, Z80_BC, snapshot_data[4] + snapshot_data[5]*256);
	cpunum_set_reg(0, Z80_DE, snapshot_data[6] + snapshot_data[7]*256);
	cpunum_set_reg(0, Z80_HL, snapshot_data[8] + snapshot_data[9]*256);
	cpunum_set_reg(0, Z80_AF, snapshot_data[10] + snapshot_data[11]*256);
	cpunum_set_reg(0, Z80_BC2, snapshot_data[12] + snapshot_data[13]*256);
	cpunum_set_reg(0, Z80_DE2, snapshot_data[14] + snapshot_data[15]*256);
	cpunum_set_reg(0, Z80_HL2, snapshot_data[16] + snapshot_data[17]*256);
	cpunum_set_reg(0, Z80_AF2, snapshot_data[18] + snapshot_data[19]*256);
	cpunum_set_reg(0, Z80_PC, snapshot_data[20] + snapshot_data[21]*256);
	cpunum_set_reg(0, Z80_SP, snapshot_data[22] + snapshot_data[23]*256);
	cpunum_set_reg(0, Z80_I, snapshot_data[24]);
	cpunum_set_reg(0, Z80_R, snapshot_data[25]);
	cpunum_set_reg(0, Z80_IX, snapshot_data[26] + snapshot_data[27]*256);
	cpunum_set_reg(0, Z80_IY, snapshot_data[28] + snapshot_data[29]*256);
	

	/* IO ports */

	// KI-1 bit 7 - NMI generator enable/disable
	primo_nmi = (snapshot_data[30] & 0x80) ? 1 : 0;

	// KI-1 bit 4 - speaker
	speaker_level_w(0, (snapshot_data[30]&0x10)>>4);


	/* memory */

	for (i=0; i<0xc000; i++)
		program_write_byte(i+0x4000, snapshot_data[i+38]);
}

SNAPSHOT_LOAD( primo )
{
	UINT8 *snapshot_data;

	if (!(snapshot_data = (UINT8*) malloc(snapshot_size)))
		return INIT_FAIL;

	if (mame_fread(fp, snapshot_data, snapshot_size) != snapshot_size)
	{
		free(snapshot_data);
		return INIT_FAIL;
	}
	
	if (!strncmp((char *)snapshot_data, "PS01", 4))
		primo_setup_pss(snapshot_data, snapshot_size);
	else
		primo_setup_pp(snapshot_data, snapshot_size);

	free(snapshot_data);
	return INIT_PASS;
}
