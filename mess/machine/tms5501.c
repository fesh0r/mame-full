/* TMS5501 timer and interrupt controler */

#include "driver.h"
#include "tms5501.h"

#define DEBUG_TMS5501	1

#define MAX_TMS5501	1


#if DEBUG_TMS5501
	#define LOG_TMS5501(n, message, data) logerror ("TMS5501 %d: %s %02x\n", n, message, data)
#else
	#define LOG_TMS5501(n, message, data)
#endif

typedef struct tms5501_t
{
	/* i/o registers */
	UINT8 status;			//03
	UINT8 command;			//04
	UINT8 serial_rate;		//05
	UINT8 serial_output_buffer;	//06
	UINT8 keyboard_scanner_mask;	//07
	UINT8 interrupt_mask;		//08
	UINT8 timer_counter[5];		//09-0d

	/* handlers & callbacks */
	UINT8 (*keyboard_read_handler)(UINT8);

	/* internal timers */
	void * timer[5];
} tms5501_t;

static tms5501_t tms5501[MAX_TMS5501];

static void tms5501_reset (int which)
{
	int i;

	tms5501[which].status = 0;
	tms5501[which].command = 0;
	tms5501[which].serial_rate = 0;
	tms5501[which].serial_output_buffer = 0;
	tms5501[which].keyboard_scanner_mask = 0;
	tms5501[which].interrupt_mask = 0;
	for (i=0; i<5; i++)
		tms5501[which].timer_counter[i] = 0;

	LOG_TMS5501(which, "Reset", 0);
}

static void decrementer_callback(int which)
{
}

void tms5501_init (int which, const tms5501_init_param *param)
{
	int i;

	for (i=0; i<5; i++)
		tms5501[which].timer[i] = timer_alloc(decrementer_callback);

	tms5501[which].keyboard_read_handler = param->keyboard_read_handler;
	tms5501_reset(which);
	LOG_TMS5501(which, "Init", 0);
}

void tms5501_cleanup (int which)
{
	int i;

	for (i=0; i<5; i++)
	{
		if (tms5501[which].timer[i])
		{
			timer_remove(tms5501[which].timer[i]);
			tms5501[which].timer[i] = NULL;
		}
	}
	LOG_TMS5501(which, "Cleanup", 0);
}

static void tms5501_send_break (int which)
{
	LOG_TMS5501(which, "Send BREAK", 0);	 
}

static void tms5501_int7_select (int which, UINT8 data)
{
	tms5501[which].command |= data&0x04;
	LOG_TMS5501(which, "INT 7", data&0x04);
}

static void tms5501_int_ack (int which, UINT8 data)
{
	tms5501[which].command |= data&0x08;
	LOG_TMS5501(which, "ACK", data&0x08);
}

static void tms5501_serial_rate_select (int which, UINT8 data)
{
	tms5501[which].serial_rate = data;
	LOG_TMS5501(which, "Serial rate", data);
}

static void tms5501_serial_output (int which, UINT8 data)
{
	tms5501[which].serial_output_buffer = data;
	LOG_TMS5501(which, "Serial output data", data);
}

static void tms5501_keyboard_scanner (int which, UINT8 data)
{
	tms5501[which].keyboard_scanner_mask = data;
	LOG_TMS5501(which, "Keyboard scanner mask:", data);
}

static void tms5501_interrupt_mask (int which, UINT8 data)
{
	tms5501[which].interrupt_mask = data;
	LOG_TMS5501(which, "Interrupt mask:", data);
}

static void tms5501_set_timer (int which, UINT8 offset, UINT8 data)
{
	// 0.064 ms resolution
	tms5501[which].timer_counter[offset-0x09] = data;
	LOG_TMS5501(which, "Timer counter", data);
}


UINT8 tms5501_read (int which, UINT16 offset)
{
	UINT8 data = 0xff;
	switch (offset&0x000f)
	{
		case 0x00:	// Serial input buffer
			LOG_TMS5501(which, "Reading from serial input buffer", data);
			break;
		case 0x01:	// Keyboard input port, Page blanking signal
			data = tms5501[which].keyboard_read_handler(tms5501[which].keyboard_scanner_mask);
			LOG_TMS5501(which, "Reading from keyboard.", data);
			break;
		case 0x02:	// Interrupt address register
			break;
		case 0x03:	// Status register
			break;
		case 0x04:	// Command register
			break;
		case 0x05:	// Serial rate register
			break;
		case 0x06:	// Serial output buffer
			break;
		case 0x07:	// Keyboard scanner output
			break;
		case 0x08:	// Interrupt mask register
			break;
		case 0x09:	// Timer 1 address (UT)
		case 0x0a:	// Timer 2 address
		case 0x0b:	// Timer 3 address (sound)
		case 0x0c:	// Timer 4 address (keyboard)
		case 0x0d:	// Timer 5 address 
			break;
	}
	return data;
}

void tms5501_write (int which, UINT16 offset, UINT8 data)
{
	switch (offset&0x000f)
	{
		case 0x00:	// Serial input buffer
			LOG_TMS5501(which, "ERROR: Writing to serial input buffer", data);
			break;
		case 0x01:	// Keyboard input port, Page blanking signal
			LOG_TMS5501(which, "ERROR: Writing to keyboard input port", data);
			break;
		case 0x02:	// Interrupt address register
			LOG_TMS5501(which, "Writing to interrupt address register", data);
			break;
		case 0x03:	// Status register
			LOG_TMS5501(which, "Writing to status register", data);
			break;
		case 0x04:	// Command register
			if (data&0x01)
				tms5501_reset(which);
			if (data&0x02)
				tms5501_send_break(which);
			tms5501_int7_select(which, data&0x04);
			tms5501_int_ack(which, data&0x08);
			break;
		case 0x05:	// Serial rate register
			tms5501_serial_rate_select(which, data);
			break;
		case 0x06:	// Serial output buffer
			tms5501_serial_output(which, data);
			break;
		case 0x07:	// Keyboard scanner mask
			tms5501_keyboard_scanner (which, data);
			break;
		case 0x08:	// Interrupt mask register
			tms5501_interrupt_mask(which, data);
			break;
		case 0x09:	// Timer 1 counter(UT)
		case 0x0a:	// Timer 2 counter
		case 0x0b:	// Timer 3 counter (sound)
		case 0x0c:	// Timer 4 counter (keyboard)
		case 0x0d:	// Timer 5 counter 
			LOG_TMS5501(which, "Timer", offset-0x09);
			tms5501_set_timer (which, offset, data);
			break;
	}
}

READ_HANDLER( tms5501_0_r )
{
	return tms5501_read(0, offset);
}

WRITE_HANDLER( tms5501_0_w )
{
	tms5501_write(0, offset, data);
}
