/*
  TMS9901 Programmable system interface

  Note that this is still WIP : much work is still needed to make a versatile and complete
  tms9901 emulator

  Raphael Nabet, 2000
*/

#include <math.h>
#include "driver.h"

#include "tms9901.h"

/*================================================================
  TMS9901 emulation.

  TMS9901 handles interrupts, provides several I/O pins, and a timer (a.k.a. clock : it is
  merely a register which decrements regularily and can generate an interrupt when it reaches
  0).

  It communicates with the TMS9900 with a CRU interface, and with the rest of the world with
  a number of parallel I/O pins.

  TODO :
  * complete TMS9901 emulation (e.g. other interrupts pins, pin mirroring...)
  * make emulation more general (most TMS9900-based systems used a TMS9901, so it could be
    interesting for other drivers)
  * support for tape input, possibly improved tape output.

  KNOWN PROBLEMS :
  * in a real TI99/4A, TMS9901 is mirrored in the whole first half of CRU address space
    (i.e. CRU bit 0 = CRU bit 32 = bit 64 = ... = bit 504 ).  This will be emulated later...
  * a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is :
    on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
    timer mode even thought the program did not explicitely ask... and THIS is impossible
    to emulate efficiently (we'd have to check each memory operation).
================================================================*/

/*
  TMS9901 interrupt handling on a TI99/4x.

  Three interrupts are used by the system (INT1, INT2, and timer), out of 15/16 possible
  interrupt.  Keyboard pins can be used as interrupt pins, too, but this is not emulated
  (it's a trick, anyway, and I don't know of any program which uses it).

  When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
  a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
  the interrupt pin and the revelant bit in the interrupt mask are set.

  TIMER interrupts are kind of an exception, since they are not associated with an external
  interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
  the pending interrupt (or am I wrong once again ?).

nota :
  All interrupt routines notify (by software) the TMS9901 of interrupt recognition ("SBO n").
  However, AFAIK, this has strictly no consequence in the TMS9901, and interrupt routines
  would work fine without this, (except probably TIMER interrupt).  All this is quite weird.
  Maybe the interrupt recognition notification is needed on TMS9985, or any other weird
  variant of TMS9900 (does any TI990/10 owner wants to test this :-) ?).
*/

/* mask :  bit #n is set if pin #n is supported as an interrupt pin,
i.e. the driver sends a notification every time the pin state changes */
static int supported_int_mask = TMS9901_INT1 | TMS9901_INT2;

static int int_state;				/* state of the int1-int15 lines */
static int timer_int_pending = 0;	/* timer int pending (overrides int3 pin if timer enabled) */
static int enabled_ints = 0;		/* interrupt enable mask */

static int int_pending = 0;			/* status of int* pin (connected to TMS9900) */

/* direction register for PIO */
static int pio_direction;
/* current PIO output (to be masked with pio_direction) */
static int pio_output;
/* mirrors used for INT7*-INT15* */
static int pio_direction_mirror;
static int pio_output_mirror;

#if 0
/*
  reverse the bit order of a 9-bit operand
*/
static int reverse_9bits(int value)
{
	value = ((value << 15) & 0x8000) | ((value >> 1) & 0x0FF);
	value = ((value << 4) & 0xF0F0) | ((value >> 4) & 0x0F0F);
	value = ((value << 2) & 0xCCCC) | ((value >> 2) & 0x3333);
	value = ((value << 1) & 0xAAAA) | ((value >> 1) & 0x5555);

	return value;
}

/*
  return the number of the first non-zero bit among the 16 first bits
*/
static int find_first_bit(int value)
{
	int bit = 0;

	if (! value)
		return -1;

#if 0
	if (! (value & 0x00FF))
	{
		value >> 8;
		bit = 8;
	}
	if (! (value & 0x000F))
	{
		value >> 4;
		bit += 4;
	}
	if (! (value & 0x0003))
	{
		value >> 2;
		bit += 2;
	}
	if (! (value & 0x0001))
		bit++;
#else
	while (! (value & 1))
	{
		value >>= 1;	/* try next bit */
		bit++;
	}
#endif

	return bit;
}
#endif

/*
  clock registers

  frequency : CPUCLK/64 = 46875Hz
*/

/* MESS timer, used to emulate the decrementer register */
static void *tms9901_timer = NULL;

/* clock interval, loaded in decrementer when it reaches 0.  0 means decrementer off */
static int clockinvl = 0;

/* when we go to timer mode, the decrementer is copied there to allow to read it reliably */
static int latchedtimer;

/* set to true when clockinvl has been written to, which means the decrementer must be reloaded
when we quit timer mode (??) */
static int clockinvlchanged;

/*
  TMS9901 mode bit.

  0 = I/O mode (allow interrupts ???),
  1 = Timer mode (we're programming the clock).
*/
static int mode9901 = 0;

static void tms9901_field_interrupts(void);
static void reset_tms9901_timer(void);

void tms9901_init(void)
{
	int_state = 0;
	timer_int_pending = 0;
	enabled_ints = 0;

	pio_direction = 0;
	pio_direction_mirror = 0;

	tms9901_field_interrupts();

	mode9901 = 0;

	clockinvl = 0;
	reset_tms9901_timer();
}


void tms9901_cleanup(void)
{
	if (tms9901_timer)
	{
		timer_remove(tms9901_timer);
		tms9901_timer = NULL;
	}
}

/*
  should be called after any change to int_state or enabled_ints.
*/
static void tms9901_field_interrupts(void)
{
	int current_ints;

	/* int_state : state of lines int1-int15 */
	/* enabled_ints : enabled interrupts */
	current_ints = int_state;
	if (clockinvl != 0)
	{	/* if timer is enabled, INT3 pin is overriden by timer */
		if (timer_int_pending)
			current_ints |= TMS9901_INT3;
		else
			current_ints &= ~ TMS9901_INT3;
	}

	current_ints &= enabled_ints & (~ pio_direction_mirror);

	if (current_ints)
	{
		int level;

		/* find which interrupt tripped us :
		 * we simply look for the first bit set to 1 in current_ints... */
		/*level = find_first_bit(current_ints);*/

		/* On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		 * but hard-wired to force level 1 interrupts */
		level = 1;

		int_pending = TRUE;

		cpu_0_irq_line_vector_w(0, level);
		cpu_set_irq_line(0, 0, ASSERT_LINE);	/* interrupt it, baby */
	}
	else
	{
		int_pending = FALSE;

		cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

/*
  callback function which is called when the state of INT2* change

  state == 0 : INT2* is inactive (high)
  state != 0 : INT2* is active (low)
*/
void tms9901_set_int2(int state)
{
	if (state)
		int_state |= TMS9901_INT2;		/* enable INT2* state interrupt requested */
	else
		int_state &= ~ TMS9901_INT2;

	tms9901_field_interrupts();
}

/*
  this call-back is called by MESS timer system when the decrementer reaches 0.
*/
static void decrementer_callback(int ignored)
{
	timer_int_pending = TRUE;			/* decrementer interrupt requested */

	tms9901_field_interrupts();
}

/*
  load the content of clockinvl into the decrementer
*/
static void reset_tms9901_timer(void)
{
	if (tms9901_timer)
	{
		timer_remove(tms9901_timer);
		tms9901_timer = NULL;
	}

	/* clock interval == 0 -> no timer */
	if (clockinvl)
	{
		tms9901_timer = timer_pulse(clockinvl / 46875., 0, decrementer_callback);
	}
}



/*----------------------------------------------------------------
  TMS9901 CRU interface.
----------------------------------------------------------------*/

/*
  Read a 8 bit chunk from tms9901.

  signification :
  bit 0 : mode9901
  if (mode9901 == 0)
   bit 1-15 : current status of the INT1*-INT15* pins
  else
   bit 1-14 : current timer value
   bit 15 : value of the INTREQ* (interrupt request to TMS9900) pin.

  bit 16-31 : current status of the P0-P15 pins
*/
static int ti99_R9901_0(int offset);
static int ti99_R9901_1(int offset);
static int ti99_R9901_3(int offset);

static int (*read_handlers[4])(int offset) =
{
	ti99_R9901_0,
	ti99_R9901_1,
	NULL,
	ti99_R9901_3
};


int tms9901_CRU_read(int offset)
{
	int answer=0xFF;

	offset &= 0x003;

	switch (offset)
	{
	case 0:
		if (mode9901)
		{	/* timer mode */
			return ((latchedtimer & 0x7F) << 1) | 0x01;
		}
		else
		{	/* I/O mode */
			answer = ((~ int_state) & supported_int_mask) & 0xFF;

			if (read_handlers[0])
				answer |= (* read_handlers[0])(0);

			answer &= ~ pio_direction_mirror;
			answer |= (pio_output_mirror & pio_direction_mirror) & 0xFF;
		}
		break;
	case 1:
		if (mode9901)
		{	/* timer mode */

			answer = (latchedtimer & 0x3F80) >> 7;
			if (! int_pending)
				answer |= 0x80;
		}
		else
		{	/* I/O mode */
			answer = ((~ int_state) & supported_int_mask) >> 8;

			if (read_handlers[1])
				answer |= (* read_handlers[1])(1);

			answer &= ~ (pio_direction_mirror >> 8);
			answer |= (pio_output_mirror & pio_direction_mirror) >> 8;
		}
		break;
	case 2:
		if (mode9901)
		{	/* exit timer mode */
			mode9901 = 0;

			if (clockinvlchanged)
				reset_tms9901_timer();
		}

		answer = (read_handlers[2]) ? (* read_handlers[2])(2) : 0;

		answer &= ~ pio_direction;
		answer |= (pio_output & pio_direction) & 0xFF;

		break;
	case 3:
		if (mode9901)
		{	/* exit timer mode */
			mode9901 = 0;

			if (clockinvlchanged)
				reset_tms9901_timer();
		}

		answer = (read_handlers[3]) ? (* read_handlers[3])(3) : 0;

		answer &= ~ (pio_direction >> 8);
		answer |= (pio_output & pio_direction) >> 8;

		break;
	}

	return answer;
}



static void ti99_KeyC2(int offset, int data);
static void ti99_KeyC1(int offset, int data);
static void ti99_KeyC0(int offset, int data);
static void ti99_AlphaW(int offset, int data);
static void ti99_CS1_motor(int offset, int data);
static void ti99_CS2_motor(int offset, int data);
static void ti99_audio_gate(int offset, int data);
static void ti99_CS_output(int offset, int data);

static void (*write_handlers[16])(int offset, int data) =
{
	NULL,
	NULL,
	ti99_KeyC2,
	ti99_KeyC1,
	ti99_KeyC0,
	ti99_AlphaW,
	ti99_CS1_motor,
	ti99_CS2_motor,
	ti99_audio_gate,
	ti99_CS_output,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

void tms9901_CRU_write(int offset, int data)
{
	data &= 1;	/* clear extra bits */
	offset &= 0x01F;

	switch (offset)
	{
	case 0x00:	/* write to mode bit */
		if (mode9901 != data)
		{
			mode9901 = data;

			if (mode9901)
			{
				if (tms9901_timer)
					latchedtimer = ceil(timer_timeleft(tms9901_timer) * 46875.);
				else
					latchedtimer = 0;		/* timer inactive... */
				clockinvlchanged = FALSE;
			}
			else
			{
				if (clockinvlchanged)
					reset_tms9901_timer();
			}
		}
		break;
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
		/*
		  write one bit to 9901 (bits 1-14)

		  mode9901==0 ?  Disable/Enable an interrupt
		              :  Bit in clock interval
		*/
		/* offset is the index of the modified bit of register (-> interrupt number -1) */
		if (mode9901)
		{	/* modify clock interval */
			int mask = 1 << ((offset & 0x0F) - 1);	/* corresponding mask */

			if (data)
				clockinvl |= mask;			/* set bit */
			else
				clockinvl &= ~ mask;		/* unset bit */
			clockinvlchanged = TRUE;
		}
		else
		{	/* modify interrupt mask */
			int mask = 1 << (offset & 0x0F);	/* corresponding mask */

			if (data)
				enabled_ints |= mask;		/* set bit */
			else
				enabled_ints &= ~ mask;		/* unset bit */

			if (offset == 3)
				timer_int_pending = FALSE;	/* SBO 3 clears pending timer interrupt (??) */

			tms9901_field_interrupts();		/* changed interrupt state */
		}
		break;
	case 0x0F:
		if (mode9901)
		{	/* clock mode */
			if (! data)
			{	/* TMS9901 soft reset */
				/* all output pins are input pins again */
				pio_direction = 0;
				pio_direction_mirror = 0;
			}
		}
		else
		{	/* modify interrupt mask */
			if (data)
				enabled_ints |= 0x4000;		/* set bit */
			else
				enabled_ints &= ~0x4000;	/* unset bit */

			tms9901_field_interrupts();		/* changed interrupt state */
		}
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		{
			int pin = offset & 0x0F;
			int mask = (1 << pin);


			if (mode9901)
			{	/* exit timer mode */
				mode9901 = 0;

				if (clockinvlchanged)
					reset_tms9901_timer();
			}

			pio_direction |= mask;			/* set up as output pin */

			if (data)
				pio_output |= mask;
			else
				pio_output &= ~ mask;

			if (pin >= 7)
			{	/* pins P7-P15 are mirrored as INT15*-INT7* */
				int pin2 = 22 - pin;
				int mask2 = (1 << pin2);

				pio_direction_mirror |= mask2;	/* set up as output pin */

				if (data)
					pio_output_mirror |= mask2;
				else
					pio_output_mirror &= ~ mask2;
			}

			if (write_handlers[pin] != NULL)
				(* write_handlers[pin])(pin, data);
		}

		break;
	}
}

/*
	TI99 functions
*/

/*
  TI99 uses :
  INT1 : external interrupt (used by RS232 controller, for instance)
  INT2 : VDP interrupt
  timer interrupt (overrides INT3)
*/

/* keyboard interface */
static int KeyCol = 0;
static int AlphaLockLine = 0;

/*
  Read pins INT3*-INT7* of TI99's 9901.

  signification :
   (bit 1 : INT1 status)
   (bit 2 : INT2 status)
   bit 3-7 : keyboard status bits 0 to 4
*/
static int ti99_R9901_0(int offset)
{
	int answer;

	answer = (readinputport(KeyCol) << 3) & 0xF8;

	if (! AlphaLockLine)
		answer &= ~ (input_port_8_r(0) << 3);

	return (answer);
}

/*
  Read pins INT8*-INT15* of TI99's 9901.

  signification :
   bit 0-2 : keyboard status bits 5 to 7
   bit 3-7 : weird, not emulated
*/
static int ti99_R9901_1(int offset)
{
	return (readinputport(KeyCol) >> 5) & 0x07;
}

/*
  Read pins P0-P7 of TI99's 9901.
  Nothing is connected !
*/
/*static int ti99_R9901_2(int offset)
{
	return 0;
}*/

/*
  Read pins P8-P15 of TI99's 9901.
*/
static int ti99_R9901_3(int offset)
{
	/*only important bit : bit 26 : tape input */

	/*return 8; */
	return 0;
}





/*
  WRITE column number bit 2
*/
static void ti99_KeyC2(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	if (data & 1)
		KeyCol |= 1;
	else
		KeyCol &= (~1);
}

/*
  WRITE column number bit 1
*/
static void ti99_KeyC1(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	if (data & 1)
		KeyCol |= 2;
	else
		KeyCol &= (~2);
}

/*
  WRITE column number bit 0
*/
static void ti99_KeyC0(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	if (data & 1)
		KeyCol |= 4;
	else
		KeyCol &= (~4);
}

/*
  WRITE alpha lock line
*/
static void ti99_AlphaW(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	AlphaLockLine = (data & 1);
}

/*
  command CS1 tape unit motor - not emulated
*/
static void ti99_CS1_motor(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}


}

/*
  command CS2 tape unit motor - not emulated
*/
static void ti99_CS2_motor(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}


}

/*
  audio gate

  connected to the AUDIO IN pin of TMS9919

  set to 1 before using tape (in order not to burn the TMS9901 ??)

  I am not sure about polarity.
*/
static void ti99_audio_gate(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	if (data & 1)
		DAC_data_w(1, 0xFF);
	else
		DAC_data_w(1, 0);
}

/*
  tape output
  I am not sure about polarity.
*/
static void ti99_CS_output(int offset, int data)
{
	if (mode9901)
	{									/* exit timer mode */
		mode9901 = 0;

		if (clockinvlchanged)
			reset_tms9901_timer();
	}

	if (data & 1)
		DAC_data_w(0, 0xFF);
	else
		DAC_data_w(0, 0);
}


