/*****************************************************************************
 *
 *	Programmable Interval Timer 8253/8254
 *
 *	Three Independent Timers
 *	(gate, clock, out pins)
 *
 *  8254 has an additional readback feature
 *
 *	Revision History
 *		1-Mar-2004 - NPW:	Did an almost total rewrite and cleaned out much
 *							of the ugliness in the previous design.  Bug #430
 *							seems to be fixed
 *		1-Jul-2000 - PeT:	Split off from PC driver and componentized
 *
 *	Modes:
 *		Mode 0: (Interrupt on Terminal Count)
 *			1. Output low
 *			2. Counter is loaded
 *			3. Output low until counter to zero
 *			4. Output high until counter is reloaded
 *			Counter rewrite:	First byte stops counter, and second byte
 *								starts the new count
 *
 *		Mode 1: (Programmable One-Shot)
 *			1. Gate input goes low
 *			2. Output low until counter to zero
 *			3. Output high
 *			Counter rewrite:	No effect until repeated
 *
 *		Mode 2: (Rate Generator)
 *			1. Output low for one clock cycle
 *			2. Output high until counter to zero
 *			Counter rewrite:	No effect until repeated
 *			Gate change:		Low forces output high, leading edge resets
 *
 *		Mode 3: (Square Wave Generator)
 *			1. Output low until counter to zero (double speed; rounded up)
 *			2. Output high until counter to zero (double speed; rounded down)
 *			Counter rewrite:	No effect until repeated
 *			Gate change:		Low forces output high, leading edge resets
 *
 *		Mode 4: (Software Trigger Strobe)
 *			1. Output high
 *			2. Counter loaded; counter decrements until zero
 *			3. Output low during terminal count
 *			4. Output high
 *			Counter rewrite:	Loads new counter
 *			Gate change:		Counting inhibited while gate low
 *
 *		Mode 5: (Hardware Trigger Strobe)
 *			1. Output high
 *			2. Counter loaded; counter decrements until zero
 *			3. Output low during terminal count
 *			4. Output high
 *			Counter rewrite:	???
 *			Gate change:		Loads new counter
 *
 *****************************************************************************/

#include <math.h>
#include "driver.h"
#include "includes/pit8253.h"
#include "state.h"



/***************************************************************************

	Structures & macros

***************************************************************************/

#define MAX_TIMER		3
#define VERBOSE			0

#if (VERBOSE == 2)
#define LOG1(msg)		logerror msg
#define LOG2(msg)		logerror msg
#elif (VERBOSE == 1)
#define LOG1(msg)		logerror msg
#define LOG2(msg)		(void)(0)
#else
#define LOG1(msg)		(void)(0)
#define LOG2(msg)		(void)(0)
#endif


#define TIMER_TIME_NEVER ((UINT64) -1)


typedef enum
{
	CW_NONE,
	CW_FIRSTBYTE,
	CW_NEWCOUNTER
} counter_write_t;



typedef enum
{
	WAKE_IDLE,
	WAKE_NEXT,
	WAKE_BYCOUNTER,
	WAKE_TRANSITION
} wake_t;



struct pit8253
{
    struct pit8253_config *config;

	struct
	{
		mame_timer *timer;					/* MAME timer */

		double clockin;						/* clock value */
		double new_clockin;					/* new clock in */
		UINT8 control;						/* 6 bit control word */
		int msb;							/* flag: access MSB of counter/latch */
		UINT16 counter;						/* current counter value */
		UINT16 latch;						/* latched counter value */
		int gate;							/* gate input (0: disable, != 0: enable) */
		int phase;
		counter_write_t counter_write;

		unsigned int output : 1; 			/* output signal */
		unsigned int counter_adjustment : 2;
		unsigned int update_all : 1;

		UINT64 counter_time;		/* time for which value in 'latch' is accurate */
		UINT64 event_time;			/* time at which we are due for an event */

		double clk_frequency;
	} timer[MAX_TIMER];
};

#define CTRL_ACCESS(control)		(((control) >> 4) & 0x03)
#define CTRL_MODE(control)			(((control) >> 1) & (((control) & 0x04) ? 0x03 : 0x07))
#define CTRL_BCD(control)			(((control) >> 0) & 0x01)


static int pit_count;
static struct pit8253 *pit;



/***************************************************************************

	Prototypes

***************************************************************************/

static void pit8253_prepare_update(int which, int timer, int cycles);
static void pit8253_prepare_update_basetime(int which, int timer, int cycles, UINT64 base_time);
static void pit8253_update(int which);
static void pit8253_state_postload(void);



/***************************************************************************

	Functions

***************************************************************************/

int pit8253_init(int count)
{
	int i, timer;

	pit_count = 0;
	pit = auto_malloc(count * sizeof(struct pit8253));
	if (!pit)
		goto error;

	memset(pit, 0, count * sizeof(struct pit8253));

	for (i = 0; i < count; i++)
	{
		for (timer = 0; timer < MAX_TIMER; timer++)
		{
			pit[i].timer[timer].timer = mame_timer_alloc(pit8253_update);
			if (!pit[i].timer[timer].timer)
				goto error;
			pit[i].timer[timer].update_all = 1;
			pit[i].timer[timer].event_time = TIMER_TIME_NEVER;
			pit[i].timer[timer].clk_frequency = 0;

			/* set up state save values */
			state_save_register_double("pit8253", i * MAX_TIMER + timer,
				"clockin",				&pit[i].timer[timer].new_clockin, 1);
			state_save_register_UINT8("pit8253", i * MAX_TIMER + timer,
				"control",				&pit[i].timer[timer].control, 1);
			state_save_register_UINT16("pit8253", i * MAX_TIMER + timer,
				"latch",				&pit[i].timer[timer].latch, 1);
			state_save_register_int("pit8253", i * MAX_TIMER + timer,
				"msb",					&pit[i].timer[timer].msb);
			state_save_register_int("pit8253", i * MAX_TIMER + timer,
				"gate",					&pit[i].timer[timer].gate);
			state_save_register_int("pit8253", i * MAX_TIMER + timer,
				"phase",				&pit[i].timer[timer].phase);
		}
	}

	state_save_register_func_postload(pit8253_state_postload);

	pit_count = count;
	return 0;

error:
	pit_count = 0;
	pit = NULL;
	return 1;
}



static void pit8253_state_postload(void)
{
	int i, timer;

	for (i = 0; i < pit_count; i++)
	{
		for (timer = 0; timer < MAX_TIMER; timer++)
		{
			pit[i].timer[timer].update_all = 1;
			pit[i].timer[timer].output = -1;
			pit[i].timer[timer].event_time = TIMER_TIME_NEVER;
			pit[i].timer[timer].clk_frequency = -1;
			pit8253_prepare_update(i, timer, 0);
		}
	}
}



void pit8253_config(int which, struct pit8253_config *config)
{
	int i;

	pit[which].config = config;

	for (i = 0; i < MAX_TIMER; i++)
	{
		pit[which].timer[i].clockin		= config->timer[i].clockin;
		pit[which].timer[i].new_clockin	= config->timer[i].clockin;
	}

	pit8253_reset(which);
}



static struct pit8253 *get_pit(int which)
{
	assert(which >= 0);
	assert(which < pit_count);
	assert(pit[which].config);
	return &pit[which];
}



void pit8253_reset(int which)
{
    struct pit8253 *p = get_pit(which);
	int i;

    for (i = 0; i < MAX_TIMER; i++)
	{
		p->timer[i].control = 0;
		p->timer[i].msb = 0;
		p->timer[i].counter = 0;
        p->timer[i].gate = 1;
		pit8253_prepare_update(which, i, 0);
    }
}



static UINT32 decimal_from_bcd(UINT32 val)
{
	return
		(val & 0xF0000) * 10000 +
		(val & 0x0F000) *  1000 +
		(val & 0x00F00) *   100 +
		(val & 0x000F0) *    10 +
		(val & 0x0000F) *     1;
}



static UINT32 bcd_from_decimal(UINT32 val)
{
	return
		((val / 10000) % 10) * 0x10000 +
		((val /  1000) % 10) *  0x1000 +
		((val /   100) % 10) *   0x100 +
		((val /    10) % 10) *    0x10 +
		((val /     1) % 10) *     0x1;
}



/* returns the current time, local to this timer */
static UINT64 get_timer_time(int which, int timer)
{
    struct pit8253 *p = get_pit(which);
	double current_time = timer_get_time();
	UINT64 val = (UINT64) (current_time * p->timer[timer].clockin);
	return val;
}



static mame_time get_time_until_timer_time(int which, int timer, UINT64 timer_time)
{
    struct pit8253 *p = get_pit(which);
	int cmp;

	mame_time target_time = double_to_mame_time(timer_time / p->timer[timer].clockin);
	mame_time current_time = mame_timer_get_time();

	cmp = compare_mame_times(current_time, target_time);

	/* special hack for certain rounding errors */
	if ((cmp == 0) && (timer_time > get_timer_time(which, timer)))
		return double_to_mame_time(1 / p->timer[timer].clockin);

	return cmp >= 0 ? time_zero : sub_mame_times(target_time, current_time); 
}



/* computes the value in a given counter, at a given time */
static UINT16 pit8253_get_counter(int which, int timer, UINT64 current_time)
{
    struct pit8253 *p = get_pit(which);
	UINT32 counter;
	int elapsed_cycles;

	counter = p->timer[timer].counter;

	if (p->timer[timer].counter_adjustment != 0)
	{
		if (counter == 0)
			counter = 0x10000;

		if (CTRL_BCD(p->timer[timer].control))
			counter = decimal_from_bcd(counter);

		elapsed_cycles = (int) (current_time - p->timer[timer].counter_time);
		elapsed_cycles *= p->timer[timer].counter_adjustment;

		if (elapsed_cycles >= counter)
			counter = 0;
		else
			counter -= elapsed_cycles;

		if (CTRL_BCD(p->timer[timer].control))
			counter = bcd_from_decimal(counter);
	}
	return (UINT16) counter;
}



/* causes pit8253_update() to be called in a certain amount of cycles */
static void pit8253_prepare_update_basetime(int which, int timer, int cycles, UINT64 basetime)
{
    struct pit8253 *p = get_pit(which);
	UINT64 current_time;
	UINT64 new_event_time;
	mame_time delay;

	current_time = get_timer_time(which, timer);

	if (cycles >= 0)
		new_event_time = basetime + cycles;
	else
		new_event_time = TIMER_TIME_NEVER;

	if (new_event_time < p->timer[timer].event_time)
	{
		if (new_event_time != TIMER_TIME_NEVER)
		{
			delay = get_time_until_timer_time(which, timer, new_event_time);
			mame_timer_adjust(p->timer[timer].timer, delay, which | (timer<<4), time_zero);
		}
		else
		{
			mame_timer_reset(p->timer[timer].timer, time_never);
		}
		p->timer[timer].event_time = new_event_time;
	}
}



static void pit8253_prepare_update(int which, int timer, int cycles)
{
	pit8253_prepare_update_basetime(which, timer, cycles, get_timer_time(which, timer));
}



static UINT32 adjust_latch(UINT16 latch, UINT8 control)
{
	UINT32 adjusted_latch = latch ? latch : 0x10000;
	if (CTRL_BCD(control))
		adjusted_latch = decimal_from_bcd(adjusted_latch);
	return adjusted_latch;
}



/* this function is used to appease the compiler with respect to
 * uninitialized variables */
static void invalid(int *output, int *counter_adjustment)
{
	if (output)
		*output = 1;
	if (counter_adjustment)
		*counter_adjustment = 0;
	osd_die("Invalid PIT8253 state");
}



/* main timer proc for timers */
static void pit8253_update(int param)
{
	int which = (param >> 0) & 0x0F;
	int timer = (param >> 4) & 0x0F;
    struct pit8253 *p = get_pit(which);
	int counter_adjustment;
	int phase;
	int output;
	int mode;
	int gate;
	int cycles_to_wake = -1;
	counter_write_t counter_write;
	wake_t wake = WAKE_IDLE;
	UINT16 counter;
	UINT16 latch;
	int output_changed, clock_changed;
	UINT64 current_time;
	UINT64 event_time;
	int clock_active = 0;
	double clk_frequency;

	mode				= CTRL_MODE(p->timer[timer].control);
	phase				= p->timer[timer].phase;
	counter_write		= p->timer[timer].counter_write;
	latch				= p->timer[timer].latch;
	gate				= p->timer[timer].gate;

	/* get current time, in timer cycles */
	current_time = get_timer_time(which, timer);

	/* this finite state machine relies on having accurate timing.  Since the
	 * MAME timing system is indeterminate, we may need to pretend that the
	 * actual time is somewhat different than what mame_timer_get_time()
	 * returns */
	event_time = current_time > p->timer[timer].event_time
		? p->timer[timer].event_time : current_time;
	mame_timer_reset(p->timer[timer].timer, time_never);
	p->timer[timer].event_time = TIMER_TIME_NEVER;

	/* update the latch value */
	counter = pit8253_get_counter(which, timer, event_time);

	LOG2(("pit8253_update(): PIT #%d timer %d: [%10d] mode=%d phase=%d counter=0x%04x\n", which,
		timer, (int) current_time, mode, phase, (unsigned) counter));

	/* here is where we readjust the clock */
	if (p->timer[timer].clockin != p->timer[timer].new_clockin)
	{
		event_time = (UINT64) (event_time / p->timer[timer].clockin / p->timer[timer].new_clockin);
		p->timer[timer].clockin = p->timer[timer].new_clockin;
	}

	do
	{
		switch(mode) {
		case 0:
			/* Mode 0: Interrupt on Terminal Count */
			if (counter_write == CW_NEWCOUNTER)
			{
				phase = 1;
				counter = latch;
			}

			switch(phase) {
			case 0:
				output				= 0;
				counter_adjustment	= counter ? 1 : 0;
				phase				= counter ? 1 : 0;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_IDLE;
				break;

			case 1:
				output				= 0;
				counter_adjustment	= 1;
				phase				= 2;
				wake				= WAKE_BYCOUNTER;
				break;

			case 2:
				output				= counter ? 0 : 1;
				counter_adjustment	= counter ? 1 : 0;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_IDLE;
				break;

			default:
				invalid(&output, &counter_adjustment);
				break;
			}

			if (counter_write == CW_FIRSTBYTE)
				counter_adjustment = 0;
			break;

		case 1:
			/* Mode 1: Programmable One-Shot */
			switch(phase) {
			case 0:
				output				= gate ? 1 : 0;
				counter_adjustment	= gate ? 0 : 1;
				phase				= gate ? 0 : 1;
				wake				= gate ? WAKE_IDLE : WAKE_BYCOUNTER;
				counter				= latch;
				break;

			case 1:
				output				= counter ? 0 : 1;
				counter_adjustment	= counter ? 1 : 0;
				phase				= counter ? 1 : 0;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_IDLE;
				break;

			default:
				invalid(&output, &counter_adjustment);
				break;
			}
			break;

		case 2:
			/* Mode 2: Rate Generator */
			clock_active = 1;
			switch(phase) {
			case 0:
				output				= 0;
				counter_adjustment	= 1;
				phase				= 1;
				wake				= WAKE_NEXT;
				counter				= latch;
				break;

			case 1:
				output				= 1;
				counter_adjustment	= 1;
				phase				= counter ? 1 : 0;
				wake				= WAKE_BYCOUNTER;
				break;

			default:
				invalid(&output, &counter_adjustment);
				break;
			}
			break;

		case 3:
			/* Mode 3: Square Wave Generator */
			clock_active = 1;
			switch(phase) {
			case 0:
				output				= 0;
				counter_adjustment	= (latch % 2) ? 1 : 2;
				phase				= 1;
				wake				= (latch % 2) ? WAKE_NEXT : WAKE_BYCOUNTER;
				counter				= latch;
				break;

			case 1:
				output				= 0;
				counter_adjustment	= counter ? 2 : 0;
				phase				= counter ? 1 : 2;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_TRANSITION;
				break;

			case 2:
				output				= 1;
				counter_adjustment	= (latch % 2) ? 3 : 2;
				phase				= 3;
				wake				= (latch % 2) ? WAKE_NEXT : WAKE_BYCOUNTER;
				counter				= latch;
				break;

			case 3:
				output				= 1;
				counter_adjustment	= counter ? 2 : 0;
				phase				= counter ? 3 : 0;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_TRANSITION;
				break;

			default:
				invalid(&output, &counter_adjustment);
				break;
			}

			if (!gate)
				output = 1;
			break;

		case 4:
		case 5:
			/* Mode 4: Software Trigger Strobe */
			/* Mode 5: Hardware Trigger Strobe */
			switch(phase) {
			case 0:
				output				= 1;
				counter_adjustment	= 1;
				phase				= 1;
				wake				= WAKE_BYCOUNTER;
				counter				= latch;
				break;

			case 1:
				output				= counter ? 1 : 0;
				counter_adjustment	= counter ? 1 : 0;
				phase				= counter ? 1 : 2;
				wake				= counter ? WAKE_BYCOUNTER : WAKE_IDLE;
				break;

			case 2:
				output				= 1;
				counter_adjustment	= 0;
				wake				= WAKE_IDLE;
				break;

			default:
				invalid(&output, &counter_adjustment);
				break;
			}

			if (mode == 4)
			{
				/* mode 4 */
				if (counter_write == CW_NEWCOUNTER)
					counter = latch;
				if (!gate)
					counter_adjustment = 0;
			}
			else
			{
				/* mode 5 */
				if (!gate)
					counter = latch;
			}
			break;

		default:
			invalid(&output, &counter_adjustment);
			break;
		}
	}
	while(wake == WAKE_TRANSITION);

	/* if counter_adjustment is non-zero and greater than the counter, then
	 * reduce it */
	if (counter && counter_adjustment && (counter_adjustment > counter))
		counter_adjustment = counter;

	/* update clock frequency */
	if (clock_active)
		clk_frequency = p->timer[timer].clockin / adjust_latch(latch, p->timer[timer].control);
	else
		clk_frequency = 0;

	/* mark changed variables */
	output_changed = (output ^ p->timer[timer].output)
		|| p->timer[timer].update_all;
	clock_changed = clk_frequency != p->timer[timer].output
		|| p->timer[timer].update_all;

	/* write back values */
	p->timer[timer].counter				= counter;
	p->timer[timer].counter_adjustment	= counter_adjustment;
	p->timer[timer].phase				= phase;
	p->timer[timer].output				= output;
	p->timer[timer].counter_write		= CW_NONE;
	p->timer[timer].counter_time		= event_time;
	p->timer[timer].update_all			= 0;
	p->timer[timer].clk_frequency		= clk_frequency;

	if ((wake == WAKE_BYCOUNTER) && (counter_adjustment == 0))
		wake = WAKE_IDLE;

	switch(wake) {
	case WAKE_IDLE:
		cycles_to_wake = -1;
		break;

	case WAKE_BYCOUNTER:
		cycles_to_wake = adjust_latch(counter, p->timer[timer].control);
		cycles_to_wake /= counter_adjustment;
		break;

	case WAKE_NEXT:
		cycles_to_wake = 1;
		break;

	default:
		invalid(NULL, NULL);
		break;
	}
	pit8253_prepare_update_basetime(which, timer, cycles_to_wake, event_time);

	/* issue callbacks, if appropriate */
	if (output_changed && p->config->timer[timer].irq_callback)
		p->config->timer[timer].irq_callback(output);
	if (clock_changed && p->config->timer[timer].clock_callback)
		p->config->timer[timer].clock_callback(clk_frequency);
}



/* ----------------------------------------------------------------------- */

static data8_t pit8253_read(int which, offs_t offset)
{
    struct pit8253 *p = get_pit(which);
	data8_t data = 0xff;
	int timer = offset % 4;
	UINT16 counter;

	switch(timer) {
	case 0:
	case 1:
	case 2:
		counter = p->timer[timer].latch;

		switch(CTRL_ACCESS(p->timer[timer].control)) {
		case 0:
			/* counter latch command */
			if (p->timer[timer].msb)
			{
				data = (p->timer[timer].latch >> 8) & 0xff;
				LOG2(("pit8253_read(): latch #%d MSB $%02x\n", timer, data));
			}
			else
			{
				data = (p->timer[timer].latch >> 0) & 0xff;
				LOG2(("pit8253_read(): latch #%d LSB $%02x\n", timer, data));
			}
			p->timer[timer].msb ^= 1;
			break;

		case 1:
			/* read/write counter bits 0-7 only */
			data = (counter >> 0) & 0xff;
			LOG2(("pit8253_read(): counter #%d LSB only $%02x\n", timer, data));
			break;

		case 2:
			/* read/write counter bits 8-15 only */
			data = (counter >> 8) & 0xff;
			LOG2(("pit8253_read(): counter #%d MSB only $%02x\n", timer, data));
			break;

		case 3:
			/* read/write bits 0-7 first, then 8-15 */
			data = (counter >> (p->timer[timer].msb ? 8 : 0)) & 0xff;
			p->timer[timer].msb ^= 1;
			break;
        }
        break;
    }

	LOG2(("pit8253_read(): PIT #%d offset=%d data=0x%02x\n", which, (int) offset, (unsigned) data));
	return data;
}



static void pit8253_write(int which, offs_t offset, int data)
{
	struct pit8253 *p = get_pit(which);
	int timer;
	int clock_val;
	int clock_mask;
	counter_write_t counter_write;
	UINT64 current_time;

	offset %= 4;

	LOG2(("pit8253_write(): PIT #%d offset=%d data=0x%02x\n", which, (int) offset, (unsigned) data));

	switch( offset ) {
	case 0:
	case 1:
	case 2:
		timer = offset % 4;
		clock_val = 0;
		clock_mask = 0;
		counter_write = p->timer[timer].counter_write;

		switch(CTRL_ACCESS(p->timer[timer].control)) {
        case 0:
			/* counter latch command */
			p->timer[timer].msb ^= 1;
			if (!p->timer[timer].msb)
			{
				/* NPW 29-Feb-2004 - no clue what p is for */
				p->timer[timer].control |= 0x30;
			}
            break;

        case 1:
			/* read/write counter bits 0-7 only */
			clock_val = data << 0;
			clock_mask = 0xFFFF;
			counter_write = CW_NEWCOUNTER;
            break;

        case 2:
			/* read/write counter bits 8-15 only */
			clock_val = data << 8;
			clock_mask = 0xFFFF;
			counter_write = CW_NEWCOUNTER;
            break;

        case 3:
			/* read/write bits 0-7 first, then 8-15 */
			if (p->timer[timer].msb)
			{
				clock_val = data << 8;
				clock_mask = 0xFF00;
				counter_write = CW_NEWCOUNTER;
			}
			else
			{
				clock_val = data << 0;
				clock_mask = 0x00FF;
				counter_write = CW_FIRSTBYTE;
			}
            p->timer[timer].msb ^= 1;
            break;
        }

		p->timer[timer].latch &= ~clock_mask;
		p->timer[timer].latch |= clock_val;
		p->timer[timer].counter_write = counter_write;
		pit8253_prepare_update(which, timer, 0);
        break;

    case 3:
		/* PIT mode port */
		timer = (data >> 6) & 3;

		if (timer < MAX_TIMER)
		{
			/* pc1512 seems to expect the timer to relatch itself */
			current_time = get_timer_time(which, timer);
			p->timer[timer].latch = pit8253_get_counter(which, timer, current_time);

			/* prepare an update */
			p->timer[timer].control = data & 0x3F;
			p->timer[timer].phase = 0;
			p->timer[timer].msb = (CTRL_ACCESS(p->timer[timer].control) == 2) ? 1 : 0;
			pit8253_prepare_update(which, timer, 0);
			LOG1(("pit8253_write(): PIT #%d timer=%d control=0x%02x\n", which, timer, p->timer[timer].control));
		}
		break;
    }
}



static void pit8253_gate_write(int which, int timer, int gate)
{
    struct pit8253 *p = get_pit(which);

	/* normalize data */
	gate = gate ? 1 : 0;

	if (gate != p->timer[timer].gate)
	{
		p->timer[timer].gate = gate;
		pit8253_prepare_update(which, timer, 0);
	}
}



/* ----------------------------------------------------------------------- */

int pit8253_get_frequency(int which, int timer)
{
    struct pit8253 *p = get_pit(which);
	return (int) p->timer[timer].clk_frequency;
}



int pit8253_get_output(int which, int timer)
{
    struct pit8253 *p = get_pit(which);
	int result = (int) p->timer[timer].output;
	LOG2(("pit8253_get_output(): PIT #%d timer=%d result=%d\n", which, timer, result));
	return result;
}



void pit8253_set_clockin(int which, int timer, double new_clockin)
{
    struct pit8253 *p = get_pit(which);
	p->timer[timer].new_clockin = new_clockin;
	pit8253_prepare_update(which, timer, 0);
}



/* ----------------------------------------------------------------------- */

READ_HANDLER ( pit8253_0_r ) { return pit8253_read(0, offset); }
READ_HANDLER ( pit8253_1_r ) { return pit8253_read(1, offset); }

WRITE_HANDLER ( pit8253_0_w ) { pit8253_write(0, offset, data); }
WRITE_HANDLER ( pit8253_1_w ) { pit8253_write(1, offset, data); }

WRITE_HANDLER ( pit8253_0_gate_w ) { pit8253_gate_write(0, offset, data); }
WRITE_HANDLER ( pit8253_1_gate_w ) { pit8253_gate_write(1, offset, data); }

