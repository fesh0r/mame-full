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
	UINT8 serial_rate;		//05
	UINT8 serial_output_buffer;	//06
	UINT8 keyboard_scanner_mask;	//07
	UINT8 interrupt_mask;		//08
	UINT8 timer_counter[5];		//09-0d

	UINT8 int7;			//'0' - Timer 5
					//'1' - IN7 of DCE-bus

	UINT8 serial_break;		//'1' - serial output is high impedance

	UINT8 int_ack;			//'1' - enables to accept a INTA signal from CPU

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
	tms5501[which].serial_rate = 0;
	tms5501[which].serial_output_buffer = 0;
	tms5501[which].keyboard_scanner_mask = 0;
	tms5501[which].interrupt_mask = 0;

	for (i=0; i<5; i++)
		tms5501[which].timer_counter[i] = 0;

	tms5501[which].serial_break = 0;
	tms5501[which].int7 = 0;
	tms5501[which].int_ack = 0;

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

UINT8 tms5501_read (int which, UINT16 offset)
{
	UINT8 data = 0x00;
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
			data = 0x10;
			break;
		case 0x04:	// Command register
			data |= (tms5501[which].serial_break << 1);
			data |= (tms5501[which].int7 << 2 );
			data |= (tms5501[which].int_ack << 3);
			LOG_TMS5501(which, "Command register read", data);	 
			break;
		case 0x05:	// Serial rate register
			data = tms5501[which].serial_rate;
			LOG_TMS5501(which, "Serial rate read", data);
			break;
		case 0x06:	// Serial output buffer
			break;
		case 0x07:	// Keyboard scanner output
			break;
		case 0x08:	// Interrupt mask register
			data = tms5501[which].interrupt_mask;
			LOG_TMS5501(which, "Interrupt mask read", data);	 
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
		case 0x04:
			// Command register
			//	bit 0: Reset
			//	bit 1: Send break
			//	       '1' - serial output is high impedance
			//	bit 2: Interrupt 7 select
			//	       '0' - timer 5
			//	       '1' - IN7 of the DCE-bus
			//	bit 3: Interrupt acknowledge enable
			//	       '0' - disables to accept INTA
			//	       '1' - enables to accept INTA
			//	bits 4-7: always '0'

			LOG_TMS5501(which, "Command register write", data);

			if (data&0x01) tms5501_reset(which);

			tms5501[which].serial_break = (data&0x02) >> 1;
			LOG_TMS5501(which, "Send BREAK", tms5501[which].serial_break);	 

			tms5501[which].int7 = (data&0x04) >> 2;
			LOG_TMS5501(which, "INT7", tms5501[which].int7);	 

			tms5501[which].int_ack = (data&0x08) >> 3;
			LOG_TMS5501(which, "Interrupt acknowledge", tms5501[which].int_ack);	 

			break;
		case 0x05:
			// Serial rate register
			//	bit 0: 110 baud
			//	bit 1: 150 baud
			//	bit 2: 300 baud
			//	bit 3: 1200 baud
			//	bit 4: 2400 baud
			//	bit 5: 4800 baud
			//	bit 6: 9600 baud
			//	bit 7: '0' - two stop bits
			//	       '1' - one stop bit
			tms5501[which].serial_rate = data;
			LOG_TMS5501(which, "Serial rate write", data);
			break;
		case 0x06:	// Serial output buffer
			tms5501_serial_output(which, data);
			break;
		case 0x07:	// Keyboard scanner mask
			tms5501_keyboard_scanner (which, data);
			break;
		case 0x08:
			// Interrupt mask register
			//	bit 0: Timer 1 has expired (UTIM)
			//	bit 1: Timer 2 has expired
			//	bit 2: External interrupt (STKIM)
			//	bit 3: Timer 3 has expired (SNDIM)
			//	bit 4: Serial receiver loaded
			//	bit 5: Serial transmitter empty
			//	bit 6: Timer 4 has expired (KBIM)
			//	bit 7: Timer 5 has expired or IN7 (CLKIM)

			tms5501[which].interrupt_mask = data;
			LOG_TMS5501(which, "Interrupt mask write", data);
			break;
		case 0x09:	// Timer 1 counter
		case 0x0a:	// Timer 2 counter
		case 0x0b:	// Timer 3 counter
		case 0x0c:	// Timer 4 counter
		case 0x0d:	// Timer 5 counter 
			// 0.064 ms resolution
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x08);
			LOG_TMS5501(which, "Timer counter set", data);
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
