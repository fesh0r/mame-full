/*
	TMS5501 input/output controller
	Krzysztof Strzecha, Nathan Woods, 2003
	Based on TMS9901 emulator by Raphael Nabet

	TODO:
	- SIO
	- INTA
	- status register
*/

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
	UINT8 status;			//status register

	UINT8 serial_rate;		//SIO configuration register
	UINT8 serial_output_buffer;	//SIO output buffer

	UINT8 pio_input_buffer;		//PIO input buffer
	UINT8 pio_output_buffer;	//PIO output buffer

	UINT8 sensor;			//Sensor input

	UINT8 interrupt_mask;		//interrupt mask register
	UINT8 pending_interrupts;	//pending interrupts register
	UINT8 interrupt_address;	//interrupt vector register

	UINT8 int7;			//'0' - Timer 5
					//'1' - IN7 of DCE-bus

	UINT8 serial_break;		//'1' - serial output is high impedance

	UINT8 int_ack;			//'1' - enables to accept a INTA signal from CPU

	/* PIO callbacks */
	UINT8 (*pio_read_callback)(void);
	void (*pio_write_callback)(UINT8);

	/* SIO handlers */

	/* interrupt callback to notify driver about interruot and its vector */
	void (*interrupt_callback)(int intreq, UINT8 vector);

	/* internal timers */
	UINT8 timer_counter[5];
	void * timer[5];
	double clock_rate;
} tms5501_t;

static tms5501_t tms5501[MAX_TMS5501];

static int find_first_bit(int value)
{
	int bit = 0;

	if (! value)
		return -1;

	while (! (value & 1))
	{
		value >>= 1;	/* try next bit */
		bit++;
	}
	return bit;
}

static void tms5501_field_interrupts(int which)
{
	UINT8 int_vectors[] = { 0xc7, 0xcf, 0xd7, 0xdf, 0xe7, 0xef, 0xf7, 0xff };
	UINT8 current_ints = tms5501[which].pending_interrupts;

	/* disabling masked interrupts */
	current_ints &= tms5501[which].interrupt_mask;

	LOG_TMS5501(which, "Pending interrupts", tms5501[which].pending_interrupts);
	LOG_TMS5501(which, "Interrupt mask", tms5501[which].interrupt_mask);
	LOG_TMS5501(which, "Current interrupts", current_ints);

	if (current_ints)
	{
		/* selecting interrupt with highest priority */
		int level = find_first_bit(current_ints);	
		LOG_TMS5501(which, "Interrupt level", level);

		/* reseting proper bit in pending interrupts register */
		tms5501[which].pending_interrupts &= ~(1<<level);

		/* selecting  interrupt vector */
		tms5501[which].interrupt_address = int_vectors[level];
		LOG_TMS5501(which, "Interrupt vector", int_vectors[level]);

		if (tms5501[which].interrupt_callback)
			(*tms5501[which].interrupt_callback)(1, int_vectors[level]);
	}
	else
	{
		if (tms5501[which].interrupt_callback)
			(*tms5501[which].interrupt_callback)(0, 0);
	}
}

static void decrementer_callback_0(int which)
{
	tms5501[which].pending_interrupts |= 0x01;
	tms5501_field_interrupts(which);
}

static void decrementer_callback_1(int which)
{
	tms5501[which].pending_interrupts |= 0x02;
	tms5501_field_interrupts(which);
}

static void decrementer_callback_2(int which)
{
	tms5501[which].pending_interrupts |= 0x08;
	tms5501_field_interrupts(which);
}

static void decrementer_callback_3(int which)
{
	tms5501[which].pending_interrupts |= 0x40;
	tms5501_field_interrupts(which);
}

static void decrementer_callback_4(int which)
{
	if (!tms5501[which].int7)
		tms5501[which].pending_interrupts |= 0x80;
	tms5501_field_interrupts(which);
}

static void tms5501_timer_reload_0(int which)
{
	if (tms5501[which].timer_counter[0])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[0], (double) tms5501[which].timer_counter[0] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[0] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		decrementer_callback_0(which);
		timer_enable(tms5501[which].timer[0], 0);
	}
}

static void tms5501_timer_reload_1(int which)
{
	if (tms5501[which].timer_counter[1])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[1], (double) tms5501[which].timer_counter[1] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[1] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		decrementer_callback_1(which);
		timer_enable(tms5501[which].timer[1], 0);
	}
}

static void tms5501_timer_reload_2(int which)
{
	if (tms5501[which].timer_counter[2])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[2], (double) tms5501[which].timer_counter[2] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[2] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		decrementer_callback_2(which);
		timer_enable(tms5501[which].timer[2], 0);
	}
}

static void tms5501_timer_reload_3(int which)
{
	if (tms5501[which].timer_counter[3])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[3], (double) tms5501[which].timer_counter[3] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[3] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		decrementer_callback_3(which);
		timer_enable(tms5501[which].timer[3], 0);
	}
}

static void tms5501_timer_reload_4(int which)
{
	if (tms5501[which].timer_counter[4])
	{	/* reset clock interval */
		timer_adjust(tms5501[which].timer[4], (double) tms5501[which].timer_counter[4] / (tms5501[which].clock_rate / 128.), which, (double) tms5501[which].timer_counter[4] / (tms5501[which].clock_rate / 128.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		decrementer_callback_4(which);
		timer_enable(tms5501[which].timer[4], 0);
	}
}

static void tms5501_reset (int which)
{
	int i;

	tms5501[which].status = 0;
	tms5501[which].serial_rate = 0;
	tms5501[which].serial_output_buffer = 0;
	tms5501[which].pio_input_buffer = 0;
	tms5501[which].pio_output_buffer = 0;
        tms5501[which].pending_interrupts = 0;

	for (i=0; i<5; i++)
	{
		tms5501[which].timer_counter[i] = 0;
		timer_enable(tms5501[which].timer[i], 0);
	}

	LOG_TMS5501(which, "Reset", 0);
}

void tms5501_init (int which, const tms5501_init_param *param)
{
	tms5501[which].timer[0] = timer_alloc(decrementer_callback_0);
	tms5501[which].timer[1] = timer_alloc(decrementer_callback_1);
	tms5501[which].timer[2] = timer_alloc(decrementer_callback_2);
	tms5501[which].timer[3] = timer_alloc(decrementer_callback_3);
	tms5501[which].timer[4] = timer_alloc(decrementer_callback_4);

	tms5501[which].pio_read_callback = param->pio_read_callback;
	tms5501[which].pio_write_callback = param->pio_write_callback;

	tms5501[which].clock_rate = param->clock_rate;
	tms5501[which].interrupt_callback = param->interrupt_callback;

	tms5501[which].serial_break = 0;
	tms5501[which].int7 = 0;
	tms5501[which].int_ack = 0;
	tms5501[which].interrupt_mask = 0;

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
//	LOG_TMS5501(which, "Serial output data", data);
}

void tms5501_set_pio_bit_7 (int which, UINT8 data)
{
	if (!(tms5501[which].pio_input_buffer & 0x80) && data && tms5501[which].int7)
		tms5501[which].pending_interrupts |= 0x80;

	tms5501[which].pio_input_buffer &= 0x7f;
	if (data)
		tms5501[which].pio_input_buffer |= 0x80;
	tms5501_field_interrupts(which);
}

void tms5501_sensor (int which, UINT8 data)
{
	if (!(tms5501[which].sensor) && data)
		tms5501[which].pending_interrupts |= 0x04;

	tms5501[which].sensor = data;

	tms5501_field_interrupts(which);
}

UINT8 tms5501_read (int which, UINT16 offset)
{
	UINT8 data = 0x00;
	switch (offset&0x000f)
	{
		case 0x00:	// Serial input buffer
			LOG_TMS5501(which, "Reading from serial input buffer", data);
			break;
		case 0x01:	// PIO input port
			if (tms5501[which].pio_read_callback)
				data = tms5501[which].pio_read_callback();
			LOG_TMS5501(which, "Reading from PIO", data);
			break;
		case 0x02:	// Interrupt address register
			data = tms5501[which].interrupt_address;
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
		case 0x07:	// PIO output
			tms5501[which].pio_output_buffer = data;
			if (tms5501[which].pio_write_callback)
				tms5501[which].pio_write_callback(tms5501[which].pio_output_buffer);
			LOG_TMS5501(which, "Writing to PIO", data);
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
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload_0(which);
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x08);
			LOG_TMS5501(which, "Timer counter set", data);
			break;
		case 0x0a:	// Timer 2 counter
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload_1(which);
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x08);
			LOG_TMS5501(which, "Timer counter set", data);
			break;
		case 0x0b:	// Timer 3 counter
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload_2(which);
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x08);
			LOG_TMS5501(which, "Timer counter set", data);
			break;
		case 0x0c:	// Timer 4 counter
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload_3(which);
			LOG_TMS5501(which, "Write timer", (offset&0x0f)-0x08);
			LOG_TMS5501(which, "Timer counter set", data);
			break;
		case 0x0d:	// Timer 5 counter 
			tms5501[which].timer_counter[(offset&0x0f)-0x09] = data;
			tms5501_timer_reload_4(which);
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
