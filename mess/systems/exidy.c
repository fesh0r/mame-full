/******************************************************************************

  Exidy Sorcerer system driver


	port fc:
	========
	input/output:
		hd6402 uart data
   
	port fd:
	========
	input: hd6402 uart status 
		bit 4: parity error
		bit 3: framing error
		bit 2: over-run
		bit 1: data available
		bit 0: transmit buffer empty

	output: 
		bit 4: no parity
		bit 3: parity type
		bit 2: number of stop bits
		bit 1: number of bits per char bit 2
		bit 0: number of bits per char bit 1

	port fe:
	========

	output:

		bit 7: rs232 enable (1=enable rs232, 0=disable rs232)
		bit 6: baud rate (1=1200, 0=600)
		bit 5: cassette motor 1
		bit 4: cassette motor 0
		bit 3..0: keyboard line select 

	input:
		bit 7..6: parallel control
				7: must be 1 to read data from parallel port
				6: must be 1 to send data out of parallel port
		bit 5: vsync 
		bit 4..0: keyboard line data 

	port ff:
	========
	  parallel port in/out

	When cassette is selected, it is connected to the uart data input via the cassette
	interface hardware.

	The cassette interface hardware converts square-wave pulses into bits which the uart receives.
	
	
	Sound:
	
	external speaker connected to the parallel port
	speaker is connected to all pins

	  
	Kevin Thacker [MESS driver]

 ******************************************************************************/
#include "driver.h"
#include "../includes/exidy.h"
#include "devices/printer.h"
#include "includes/centroni.h"
#include "includes/hd6402.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "devices/basicdsk.h"
#include "image.h"

static DEVICE_LOAD( exidy_floppy )
{
	if (device_load_basicdsk_floppy(image, file)==INIT_PASS)
	{
		/* not correct */
		basicdsk_set_geometry(image, 80, 2, 9, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}


//static unsigned char exidy_fc;
//static unsigned char exidy_fd;
static unsigned char exidy_fe;
//static unsigned char exidy_ff;

//static int exidy_parallel_control;
static int exidy_keyboard_line;
static unsigned long exidy_hd6402_state;

static WRITE_HANDLER(exidy_fe_port_w);

/* timer for exidy serial chip transmit and receive */
static void *serial_timer;

static void exidy_serial_timer_callback(int dummy)
{
	/* if rs232 is enabled, uart is connected to clock defined by bit6 of port fe.
	Transmit and receive clocks are connected to the same clock */

	/* if rs232 is disabled, receive clock is linked to cassette hardware */
	if (exidy_fe & 0x080)
	{
		/* trigger transmit clock on hd6402 */
		hd6402_transmit_clock();
		/* trigger receive clock on hd6402 */
		hd6402_receive_clock();
	}
}



/* timer to read cassette waveforms */
static void *cassette_timer;

/* cassette data is converted into bits which are clocked into serial chip */
static struct serial_connection cassette_serial_connection;

/* two flip-flops used to store state of bit comming from cassette */
static int cassette_input_ff[2];
/* state of clock, used to clock in bits */
static int cassette_clock_state;
/* a up-counter. when this reaches a fixed value, the cassette clock state is changed */
static int cassette_clock_counter;

/*	1. the cassette format: "frequency shift" is converted
	into the uart data format "non-return to zero" 
	
	2. on cassette a 1 data bit is stored as a high frequency
	and a 0 data bit as a low frequency 
	- At 1200hz, a logic is 1 cycle of 1200hz and a logic 0 is 1/2
	cycle of 600hz.
	- At 300hz, a 1 is 8 cycles of 2400hz and a logic 0 is 4 cycles
	of 1200hz.

	Attenuation is applied to the signal and the square
	wave edges are rounded.

	A manchester encoder is used. A flip-flop synchronises input
	data on the positive-edge of the clock pulse.

	Interestingly the data on cassette is stored in xmodem-checksum.


*/

/* only works for one cassette so far */
static void exidy_cassette_timer_callback(int dummy)
{
	cassette_clock_counter++;

	if (cassette_clock_counter==(4800/1200))
	{
		/* reset counter */
		cassette_clock_counter = 0;

		/* toggle state of clock */
		cassette_clock_state^=0x0ffffffff;

		/* previously was 0, now gone 1 */
		/* +ve edge detected */
		if (cassette_clock_state)
		{			
			int bit;

			/* clock bits into cassette flip flops */
			/* bit is inverted! */

			/* detect level */
			bit = 1;
			if (device_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 255)
				bit = 0;
			cassette_input_ff[0] = bit;
			/* detect level */
			bit = 1;
			if (device_input(image_from_devtype_and_index(IO_CASSETTE, 1)) > 255)
				bit = 0;

			cassette_input_ff[1] = bit;

			logerror("cassette bit: %0d\n",bit);

			/* set out data bit */
			set_out_data_bit(cassette_serial_connection.State, cassette_input_ff[0]);
			/* output through serial connection */
			serial_connection_out(&cassette_serial_connection);

			/* update hd6402 receive clock */
			hd6402_receive_clock();
		}
	}
}

static void exidy_hd6402_callback(int mask, int data)
{
	exidy_hd6402_state &=~mask;
	exidy_hd6402_state |= (data & mask);

	logerror("hd6402 state: %04x %04x\n",mask,data);
}

static void exidy_reset_timer_callback(int dummy)
{
	cpunum_set_pc(0,0x0e000);
}

#if 0
static OPBASE_HANDLER( exidy_opbaseoverride )
{
	memory_set_opbase_handler(0,0);

	cpunum_set_pc(0,0x0e000);

	return (cpunum_get_pc(0) & 0x0ffff);
}
#endif
static void exidy_printer_handshake_in(int number, int data, int mask)
{
	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
		}
	}
}

static CENTRONICS_CONFIG exidy_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		exidy_printer_handshake_in
	},
};

static void cassette_serial_in(int id, unsigned long state)
{
//	int cassette_output;

	cassette_serial_connection.input_state = state;

//	cassette_output = 32768;
//	if (get_in_data_bit(cassette_serial_connection.input_state))
//	{
//		cassette_output = -cassette_output;
//	}
//
//	device_output(IO_CASSETTE, 0, cassette_output);
}

static MACHINE_INIT( exidy )
{
	hd6402_init();
	hd6402_set_callback(exidy_hd6402_callback);
	hd6402_reset();

	centronics_config(0, exidy_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);

	serial_timer = timer_alloc(exidy_serial_timer_callback);
	cassette_timer = timer_alloc(exidy_cassette_timer_callback);

	serial_connection_init(&cassette_serial_connection);
	serial_connection_set_in_callback(&cassette_serial_connection, cassette_serial_in);
	
	exidy_fe_port_w(0,0);

	timer_set(TIME_NOW, 0, exidy_reset_timer_callback);
	
	wd179x_init(WD_TYPE_179X,NULL);

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);

	/* this is temporary. Normally when a Z80 is reset, it will
	execute address 0. The exidy starts executing from 0x0e000 */
//	memory_set_opbase_handler(0, exidy_opbaseoverride);
	
//	cpunum_write_byte(0,0,0x0c3);
//	cpunum_write_byte(0,1,0x000);
//	cpunum_write_byte(0,2,0x0e0);

}

#if 0
static READ_HANDLER(exidy_unmapped_r)
{
	logerror("unmapped r: %04x\r\n",offset);
	return 0x0ff;
}

static WRITE_HANDLER(exidy_unmapped_w)
{
	logerror("unmapped r: %04x %d\r\n", offset, data);
}
#endif



static READ_HANDLER ( exidy_wd179x_r )
{
	switch (offset & 0x03)
	{
	case 0:
		return wd179x_status_r(offset);
	case 1:
		return wd179x_track_r(offset);
	case 2:
		return wd179x_sector_r(offset);
	case 3:
		return wd179x_data_r(offset);
	default:
		break;
	}

	return 0x0ff;
}

static WRITE_HANDLER ( exidy_wd179x_w )
{
	switch (offset & 0x03)
	{
	case 0:
		wd179x_command_w(offset, data);
		return;
	case 1:
		wd179x_track_w(offset, data);
		return;
	case 2:
		wd179x_sector_w(offset, data);
		return;
	case 3:
		wd179x_data_w(offset, data);
		return;
	default:
		break;
	}
}


MEMORY_READ_START( readmem_exidy )
	{0x00000, 0x07fff, MRA_RAM},		/* ram 32k machine */
//	{0x08000, 0x0bbff, exidy_unmapped_r},
	{0x0bc00,0x0bcff, MRA_ROM},
//	{0x0bd00, 0x0bfff, exidy_unmapped_r},
	{0x0be00,0x0be03, exidy_wd179x_r},

	{0x0c000, 0x0efff, MRA_ROM},		/* rom pac */
	{0x0f000, 0x0f7ff, MRA_RAM},		/* screen ram */	
	{0x0f800, 0x0fbff, MRA_ROM},		/* char rom */
	{0x0fc00, 0x0ffff, MRA_RAM},		/* programmable chars */
MEMORY_END


MEMORY_WRITE_START( writemem_exidy )
	{0x00000, 0x07fff, MWA_RAM},		/* ram 32k machine */
//	{0x08000, 0x0bbff, exidy_unmapped_w},
//	{0x0bd00, 0x0bfff, exidy_unmapped_w},
{0x0be00, 0x0be03, exidy_wd179x_w},	
{0x0c000, 0x0efff, MWA_ROM},			/* rom pac */	
	{0x0f000, 0x0f7ff, MWA_RAM},		/* screen ram */
	{0x0f800, 0x0fbff, MWA_ROM},		/* char rom */
	{0x0fc00, 0x0ffff, MWA_RAM},		/* programmable chars */
MEMORY_END

static WRITE_HANDLER(exidy_fc_port_w)
{
	logerror("exidy fc w: %04x %02x\n",offset,data);

	hd6402_set_input(HD6402_INPUT_TBRL, HD6402_INPUT_TBRL);
	hd6402_data_w(offset,data);
}


static WRITE_HANDLER(exidy_fd_port_w)
{
	logerror("exidy fd w: %04x %02x\n",offset,data);

	/* bit 0,1: char length select */
	if (data & (1<<0))
	{
		hd6402_set_input(HD6402_INPUT_CLS1, HD6402_INPUT_CLS1);
	}
	else
	{
		hd6402_set_input(HD6402_INPUT_CLS1, 0);
	}

	if (data & (1<<1))
	{
		hd6402_set_input(HD6402_INPUT_CLS2, HD6402_INPUT_CLS2);
	}
	else
	{
		hd6402_set_input(HD6402_INPUT_CLS2, 0);
	}

	/* bit 2: stop bit count */
	if (data & (1<<2))
	{
		hd6402_set_input(HD6402_INPUT_SBS, HD6402_INPUT_SBS);
	}
	else
	{
		hd6402_set_input(HD6402_INPUT_SBS, 0);
	}

	/* bit 3: parity type select */
	if (data & (1<<3))
	{
		hd6402_set_input(HD6402_INPUT_EPE, HD6402_INPUT_EPE);
	}
	else
	{
		hd6402_set_input(HD6402_INPUT_EPE, 0);
	}

	/* bit 4: inhibit parity (no parity) */
	if (data & (1<<4))
	{
		hd6402_set_input(HD6402_INPUT_PI, HD6402_INPUT_PI);
	}
	else
	{
		hd6402_set_input(HD6402_INPUT_PI, 0);
	}
}

#define EXIDY_CASSETTE_MOTOR_MASK ((1<<4)|(1<<5))


static WRITE_HANDLER(exidy_fe_port_w)
{
	int changed_bits;

	exidy_keyboard_line = data & 0x0f;

	changed_bits = exidy_fe^data;

	/* either cassette motor state changed */
	if ((changed_bits & EXIDY_CASSETTE_MOTOR_MASK)!=0)
	{
		/* cassette 1 motor */
		device_status(image_from_devtype_and_index(IO_CASSETTE, 0), ((data>>4) & 0x01));
		/* cassette 2 motor */
		device_status(image_from_devtype_and_index(IO_CASSETTE, 1), ((data>>5) & 0x01));

		if ((data & EXIDY_CASSETTE_MOTOR_MASK)==0)
		{
			/* both are now off */

			/* stop timer */
			timer_adjust(cassette_timer, 0, 0, 0);
		}
		else
		{
			/* if both motors were off previously, at least one motor
			has been switched on */
			if ((exidy_fe & EXIDY_CASSETTE_MOTOR_MASK)==0)
			{
				cassette_clock_counter = 0;
				cassette_clock_state = 0;
				/* start timer */
				timer_adjust(cassette_timer, 0, 0, TIME_IN_HZ(4800));
			}
		}
	}

	if (data & 0x080)
	{
		/* connect to serial device */
	}
	else
	{
		/* connect to tape */
		hd6402_connect(&cassette_serial_connection);
	}

	if ((changed_bits & (1<<6))!=0)
	{
		int baud_rate;

		baud_rate = 300;
		if (data & (1<<6))
		{
			baud_rate = 1200;
		}

		timer_adjust(serial_timer, 0, 0, TIME_IN_HZ(baud_rate));
	}

	exidy_fe = data;
}

static WRITE_HANDLER(exidy_ff_port_w)
{
	logerror("exidy ff w: %04x %02x\n",offset,data);

	switch ((readinputport(0)>>1) & 0x01)
	{
		case 0:
		{
			int level;

			level = 0;
			if (data)
			{
				level = 0x0ff;
			}

			speaker_level_w(0, level);
		}
		break;

		case 1:
		{
			/* printer */
			/* bit 7 = strobe, bit 6..0 = data */
			centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
			centronics_write_handshake(0, (data>>7) & 0x01, CENTRONICS_STROBE);
			centronics_write_data(0,data & 0x07f);

		}
		break;
	}

}

static READ_HANDLER(exidy_fc_port_r)
{
	int data;

	hd6402_set_input(HD6402_INPUT_DRR, HD6402_INPUT_DRR);
	data = hd6402_data_r(offset);

	logerror("exidy fc r: %04x %02x\n",offset,data);

	return data;
}

static READ_HANDLER(exidy_fd_port_r)
{
	int data;

	data = 0;

	/* bit 4: parity error */
	if (exidy_hd6402_state & HD6402_OUTPUT_PE)
	{
		data |= (1<<4);
	}

	/* bit 3: framing error */
	if (exidy_hd6402_state & HD6402_OUTPUT_FE)
	{
		data |= (1<<3);
	}

	/* bit 2: over-run error */
	if (exidy_hd6402_state & HD6402_OUTPUT_OE)
	{
		data |= (1<<2);
	}

	/* bit 1: data receive ready - data ready in receive reg */
	if (exidy_hd6402_state & HD6402_OUTPUT_DR)
	{
		data |= (1<<1);
	}

	/* bit 0: transmitter buffer receive empty */
	if (exidy_hd6402_state & HD6402_OUTPUT_TBRE)
	{
		data |= (1<<0);
	}


	logerror("exidy fd r: %04x %02x\n",offset,data);

	return data;
}

static READ_HANDLER(exidy_fe_port_r)
{
	unsigned char data;

	data = 0;
	/* vsync */
	data |= (readinputport(0) & 0x01)<<5;
	/* keyboard data */
	data |= (readinputport(exidy_keyboard_line+1) & 0x01f);
	return data;
}

static READ_HANDLER(exidy_ff_port_r)
{
	int data;

	data = 0;
	/* bit 7 = printer busy */
	/* 0 = printer is not busy */

	if (device_status(image_from_devtype_and_index(IO_PRINTER, 0), 0)==0 )
		data |= 0x080;
	
	logerror("exidy ff r: %04x %02x\n",offset,data);

	return data;
}


PORT_READ_START( readport_exidy )
//	{0x000, 0x0fb, exidy_unmapped_r},
	{0x0fc, 0x0fc, exidy_fc_port_r},
	{0x0fd, 0x0fd, exidy_fd_port_r},
	{0x0fe, 0x0fe, exidy_fe_port_r},
	{0x0ff, 0x0ff, exidy_ff_port_r},
PORT_END

PORT_WRITE_START( writeport_exidy )
	{0x0fc, 0x0fc, exidy_fc_port_w},
	{0x0fd, 0x0fd, exidy_fd_port_w},
	{0x0fe, 0x0fe, exidy_fe_port_w},
	{0x0ff, 0x0ff, exidy_ff_port_w},
PORT_END


INPUT_PORTS_START(exidy)
	PORT_START
	/* vblank */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* hardware connected to printer port */
	PORT_DIPNAME( 0x02, 0x02, "Hardware connected to printer/parallel port" )
	PORT_DIPSETTING(    0x00, "Speaker" )
	PORT_DIPSETTING(    0x01, "Printer" )
	/* line 0 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Graphic", KEYCODE_F1, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Stop", KEYCODE_ESC, IP_JOY_NONE)
	/* line 1 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Sel)", KEYCODE_F2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Skip", KEYCODE_F3, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Repeat", KEYCODE_F4, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Clear", KEYCODE_BACKSPACE, IP_JOY_NONE)
	/* line 2 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	/* line 3 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	/* line 4 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	/* line 5 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	/* line 6 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	/* line 7 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	/* line 8 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	/* line 9 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	/* line 10 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	/* line 11 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "^", KEYCODE_F7, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "LINE FEED", KEYCODE_F8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", KEYCODE_F9, IP_JOY_NONE)
	/* line 12 */
	PORT_START
	PORT_BIT (0x10, 0x10, IPT_UNUSED)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "- (PAD)", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ (PAD)", KEYCODE_F10, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "* (PAD)", KEYCODE_ASTERISK, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "+ (PAD)", KEYCODE_PLUS_PAD, IP_JOY_NONE)
	/* line 13 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 (PAD)", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 (PAD)", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 (PAD)", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 (PAD)", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 (PAD)", KEYCODE_0_PAD, IP_JOY_NONE)
	/* line 14 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 (PAD)", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 (PAD)", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 (PAD)", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 (PAD)", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". (PAD)", KEYCODE_DEL_PAD, IP_JOY_NONE)
	/* line 15 */
	PORT_START
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 (PAD)", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "? (PAD)", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BIT (0x04, 0x04, IPT_UNUSED)
	PORT_BIT (0x02, 0x02, IPT_UNUSED)
	PORT_BIT (0x01, 0x01, IPT_UNUSED)
INPUT_PORTS_END


/**********************************************************************************************************/

static struct Speaker_interface exidy_speaker_interface=
{
 1,
 {50},
};

	
static MACHINE_DRIVER_START( exidy )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2000000)        /* clock - not correct */
	MDRV_CPU_MEMORY(readmem_exidy,writemem_exidy)
	MDRV_CPU_PORTS(readport_exidy,writeport_exidy)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(200)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( exidy )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(EXIDY_SCREEN_WIDTH, EXIDY_SCREEN_HEIGHT)
	MDRV_VISIBLE_AREA(0, EXIDY_SCREEN_WIDTH-1, 0, EXIDY_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(EXIDY_NUM_COLOURS)
	MDRV_COLORTABLE_LENGTH(EXIDY_NUM_COLOURS)
	MDRV_PALETTE_INIT( exidy )

	MDRV_VIDEO_START( exidy )
	MDRV_VIDEO_UPDATE( exidy )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, exidy_speaker_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(exidy)
	/* these are common to all because they are inside the machine */
	ROM_REGION(64*1024+32, REGION_CPU1,0)

	ROM_LOAD_OPTIONAL("diskboot.dat",0x0bc00, 0x0100, BAD_DUMP)

	/* char rom */
	ROM_LOAD("exchr-1.dat",0x0f800, 1024, CRC(4a7e1cdd))

	/* video prom */
	ROM_LOAD("bruce.dat", 0x010000, 32, CRC(fae922cb))

	ROM_LOAD("exmo1-1.dat", 0x0e000, 0x0800, CRC(ac924f67))
	ROM_LOAD("exmo1-2.dat", 0x0e800, 0x0800, CRC(ead1d0f6))

	ROM_LOAD_OPTIONAL("exsb1-1.dat", 0x0c000, 0x0800, CRC(1dd20d80))
	ROM_LOAD_OPTIONAL("exsb1-2.dat", 0x0c800, 0x0800, CRC(1068a3f8))
	ROM_LOAD_OPTIONAL("exsb1-3.dat", 0x0d000, 0x0800, CRC(e6332518))
	ROM_LOAD_OPTIONAL("exsb1-4.dat", 0x0d800, 0x0800, CRC(a370cb19))	
ROM_END

SYSTEM_CONFIG_START(exidy)
	CONFIG_DEVICE_PRINTER			(1)
	CONFIG_DEVICE_FLOPPY_BASICDSK	(4,	"dsk\0",	device_load_exidy_floppy)
	//CONFIG_DEVICE_CASSETTE		(2,	"",			device_load_exidy_cassette)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY        FULLNAME */
COMPX(1979,	exidy,	0,		0,		exidy,	exidy,	0,		exidy,	"Exidy Inc", "Sorcerer", GAME_NOT_WORKING | GAME_NO_SOUND)
