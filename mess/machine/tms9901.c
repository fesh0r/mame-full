/*
	TMS9901 Programmable system interface

	Note that this is still WIP : much work is still needed to make a versatile and complete
	tms9901 emulator

	Raphael Nabet, 2000
*/

#include <math.h>
#include "driver.h"

#include "tms9901.h"

/*
	TMS9901 emulation.

Overview :
	TMS9901 is a support chip for TMS9900.  It handles interrupts, provides several I/O pins, and
	a timer (a.k.a. clock : it is merely a register which decrements regularily and can generate
	an interrupt when it reaches 0).

	It communicates with the TMS9900 with the CRU bus, and with the rest of the world with
	a number of parallel I/O pins.

	I/O and timer functions should work with any other 990/99xx/99xxx CPU.  Since interrupt bus is
	different on each CPU, I guess interrupt handling was only used on tms9900-based systems.


TODO :
	* complete TMS9901 emulation (e.g. other interrupts pins...)

Pins :
	Reset
	INT1-INT7 : used as interrupt/input pins.
	P0-P7 : used as input/output pins.
	INT8/P15-INT15/P8 : used as both interrupt/input or input/output pins.  Note you can
	simulteanously use a pin as output and interrupt.  (Mostly obvious - but it imply you cannot
	raise an interrupt with a write, which is not SO obvious.)

Interrupt handling :
	After each clock cycle, TMS9901 reads the state of INT1*-INT15* (except pins which are set as
	output pins).  If the clock is enabled, it replaces INT3* with an internal timer int pending
	register.  Then it inverts the value and performs a bit-wise AND with the interrupt mask.

	If some bits are set, TMS9901 set the INTREQ* pin and present the number of the first bit set
	on the IC0-IC3 pins.  If the tms9900 is connected to these pins, the result is that asserting
	an INTn* on tms9901 causes a level-n interrupt request on the tms9900, provided the interrupt
	pin is enabled in tms9901 and no higher priority pin is set.

	This interrupt request lasts as long as the interrupt pin and the revelant bit in the interrupt
	mask are set (level-triggered interrupts).

	TIMER interrupts are kind of an exception, since they are not associated with an external
	interrupt pin.  I think the request is set by the decrementer reaching 0, and is cleared by a
	write to the 9901 int3 enable bit ("SBO 3") (or am I wrong once again ?).

nota :
	On TI99/4a, interrupt routines notify (by software) the TMS9901 of interrupt recognition
	("SBO n").  However, AFAIK, this has strictly no consequence in the TMS9901, and interrupt
	routines would work fine without this (except probably TIMER interrupt).  All this is quite
	weird.  Maybe the interrupt recognition notification is needed on TMS9985, or any other weird
	variant of TMS9900 (does any TI990/10 owner wants to test this :-) ?).
*/


/*
	Interrupt variables
*/

/* mask :  bit #n is set if pin #n is supported as an interrupt pin,
i.e. the driver sends a notification every time the pin state changes */
static int supported_int_mask;

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

/*
	clock registers

	frequency : CPUCLK/64 = 46875Hz for a 3MHz clock

	(warning : 3MHz on a tms9900 = 12MHz on a tms9995/99xxx)
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
	TMS9901 current mode

	0 = I/O mode (allow interrupts ???),
	1 = Timer mode (we're programming the clock).
*/
static int mode9901 = 0;

/*
	driver-dependent read and write handlers
*/

static int (*read_handlers[4])(int offset) =
{	/* defaults to all NULL */
	NULL,
};

static void (*write_handlers[16])(int offset, int data) =
{	/* defaults to all NULL */
	NULL,
};


/*
	prototypes
*/

static void tms9901_field_interrupts(void);
static void reset_tms9901_timer(void);



/*
	utilities
*/


#if 0
/*
	Reverse the bit order of bits 15-7 (i.e. 9 MSB in a 16-bit word) of an operand.
	Other bits are masked out.

Why :

	Since not enough pins were available, pins P7-P15 are the same as INT15*-INT7*, i.e.
	the same pins can be used both as input/interrupt pins and as normal I/O pins.

	The problem is bit order is reversed, i.e. P7 = INT15*, P8 = INT14*,... P15 = INT7*.
	Since the order is determined by the conventions used by the CRU read routines,
	we MUST somehow reverse the bit order.
*/
static int reverse_9MSBits(int value)
{
	value = ((value << 1) & 0xFF00) | ((value >> 15) & 0x0001);
	value = ((value << 4) & 0xF0F0) | ((value >> 4) & 0x0F0F);
	value = ((value << 2) & 0xCCCC) | ((value >> 2) & 0x3333);
	value = ((value << 1) & 0xAAAA) | ((value >> 1) & 0x5555);

	return value;
}

/*
	return the number of the first (i.e. least significant) non-zero bit among the 16 first bits
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
	initialize the tms9901 core
*/
void tms9901_init(tms9901reset_param *param)
{
	int i;

	supported_int_mask = param->supported_int_mask;

	for (i=0; i<4; i++)
		read_handlers[i] = param->read_handlers[i];

	for (i=0; i<16; i++)
		write_handlers[i] = param->write_handlers[i];

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

	/* mask out all int pins currently set as output */
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

	state == 0 : INTn* is inactive (high)
	state != 0 : INTn* is active (low)

	0<=pin_number<=15
*/
void tms9901_set_single_int(int pin_number, int state)
{
	if (state)
		int_state |= 1 << pin_number;		/* raise INTn* pin state mirror */
	else
		int_state &= ~ (1 << pin_number);

	/* we do not need to always call this function - time for an optimization */
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
		tms9901_timer = timer_pulse(clockinvl / 46875.L, 0, decrementer_callback);
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

	bit 16-31 : current status of the P0-P15 pins (quits timer mode, too...)
*/
READ16_HANDLER ( tms9901_CRU_read )
{
	int answer;

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
	default:
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

/*
	Read 1 bit to tms9901.

	signification :
	bit 0 : write mode9901
	if (mode9901 == 0)
	 bit 1-15 : write interrupt mask register
	else
	 bit 1-14 : write timer period
	 bit 15 : if written value == 0, soft reset (just resets all I/O pins as input)

	bit 16-31 : set output state of P0-P15 (and set them as output pin) (quits timer mode, too...)
*/
WRITE16_HANDLER ( tms9901_CRU_write )
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

