/*****************************************************************************
 *
 *	Programmable Interval Timer 8253/8254
 *
 *	three independent timers
 *	(gate, clock, out pins)
 *
 *  8254 has an additional readback feature
 *
 *****************************************************************************/

/*
 * PeT 1.July 2000
 * splittet from pc
 * changed to component
 * changed into 3 independand timer
 */

#include "driver.h"
#include "includes/pit8253.h"

#define VERBOSE 2

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

#define MAX_TIMER	3

typedef struct {
    PIT8253_CONFIG *config;

	struct {
		UINT8 access;	/* latch/access mode */
		UINT8 mode; 	/* counter mode */
		UINT8 bcd;		/* BCD counter flag */
		UINT8 msb;		/* flag: access MSB of counter/latch */
		int clock;		/* clock divisor */
		int latch;		/* latched count */
		int output; 	/* output signal */
		int reset;		/* reset output on next clock pulse */
		int gate;		/* gate input (0: disable, != 0: enable) */

        double time_access;
		void *timer;	/* MAME timer */
	} timer[MAX_TIMER];
} PIT8253;

static PIT8253 pit[MAX_PIT8253] = { {NULL,}, };

void pit8253_reset(int which)
{
    PIT8253 *this = pit + which;
	int i;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_reset",("which >= MAX_PIT8253 (%d)\n", which));
		return;
    }
	if (this->config == NULL)
	{
		LOG(1,"pit8253_reset",("PIT8253[%d] not initialized\n", which));
		return;
    }

    for (i = 0; i < 3; i++)
	{
		pit->timer[i].access = 0;
		pit->timer[i].mode = 0;
		pit->timer[i].bcd = 0;
		pit->timer[i].msb = 0;
		pit->timer[i].clock = 0;
		pit->timer[i].output = 0;
		pit->timer[i].reset = 0;
        pit->timer[i].gate = 1;
		pit->timer[i].time_access = 0.0;
		if (pit->timer[i].timer)
            timer_remove(pit->timer[i].timer);
        pit->timer[i].timer = NULL;
    }
}

void pit8253_config(int which, PIT8253_CONFIG *config)
{
	PIT8253 *this = pit + which;
    if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_config",("which >= MAX_PIT8253 (%d)\n", which));
		return;
    }

	this->config = config;
	pit8253_reset(which);
}

INLINE int pit8253_get_count(PIT8253 *this, int timer)
{
	if (this->timer[timer].bcd)
	{
		if (this->timer[timer].clock)
			return this->timer[timer].clock;
		return 10000;
	}
	else
	{
		if (this->timer[timer].clock)
			return this->timer[timer].clock;
		return 0x10000;
	}
}

int pit8253_get_frequency(int which, int timer)
{
	PIT8253 *this = pit + which;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_get_frequency",("which >= MAX_PIT8253 (%d)\n", which));
		return 0;
    }
	if (timer >= MAX_TIMER)
	{
		LOG(1,"pit8253_get_frequency",("timer >= MAX_TIMER (%d)\n", which));
		return 0;
    }
    if (this->config == NULL)
	{
		LOG(1,"pit8253_get_frequency",("PIT8253[%d] not initialized\n", which));
		return 0;
    }

    return this->config->timer[timer].clockin / pit8253_get_count(this,timer);
}

int pit8253_get_output(int which, int timer)
{
	PIT8253 *this = pit + which;
	int ticks;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_get_output",("which >= MAX_PIT8253 (%d)\n", which));
		return 0;
    }
	if (timer >= MAX_TIMER)
	{
		LOG(1,"pit8253_get_output",("timer >= MAX_TIMER (%d)\n", which));
		return 0;
    }
    if (this->config == NULL)
    {
		LOG(1,"pit8253_get_output",("PIT8253[%d] not initialized\n", which));
        return 0;
    }

    if (this->config->timer[timer].irq_callback)
		return pit[which].timer[timer].output;

    switch (this->timer[timer].mode)
	{
	case 0:
		ticks = pit8253_get_count(this, timer) -
			((int)(this->config->timer[timer].clockin
				   * (timer_get_time() - this->timer[timer].time_access)));
		return ticks < 0;
	}

    return 0;
}

static void pit8253_timer_callback(int which)
{
    PIT8253 *this = pit + (which & 0xf);
    int timer = (which >> 4);

	if (this->timer[timer].reset)
	{
		this->timer[timer].reset = 0;
		this->timer[timer].output = 0;
	}
	else
		this->timer[timer].output = 1;

    if (this->config->timer[timer].irq_callback)
        (*this->config->timer[timer].irq_callback)(timer);
}

static void pit8253_timer_pulse(int which, int timer)
{
	PIT8253 *this = pit + which;

    double rate = 0.0;

	if( this->timer[timer].timer )
		timer_remove( this->timer[timer].timer );

	this->timer[timer].timer = NULL;

	if (this->config->timer[timer].irq_callback == NULL)
		return;

    if( this->timer[timer].gate == 0 )
		return;

    if( this->timer[timer].clock )
		rate = this->timer[timer].clock / this->config->timer[timer].clockin;
	else
		rate = 65536.0 / this->config->timer[timer].clockin;

	if( rate > 0.0 )
		this->timer[timer].timer = timer_pulse(rate, which | (timer<<4), pit8253_timer_callback);
}

static void pit8253_timer_clock(int which, int timer)
{
	PIT8253 *this = pit + which;

	double rate = 0.0;

	if( this->timer[timer].timer )
		timer_remove( this->timer[timer].timer );

	this->timer[timer].timer = NULL;

	if( this->timer[timer].clock )
		rate = this->timer[timer].clock / this->config->timer[timer].clockin;
	else
		rate = 65536.0 / this->config->timer[timer].clockin;

	if( this->timer[timer].gate == 0 )
		rate = 0;

	if (this->config->timer[timer].clk_callback)
		(*this->config->timer[timer].clk_callback)(rate ? 1.0 / rate : 0);
}

void pit8253_set_clockin(int which, int timer, double new_clockin)
{
    PIT8253 *this = pit + which;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_set_clockin",("which >= MAX_PIT8253 (%d)\n", which));
		return;
    }
	if (timer >= MAX_TIMER)
	{
		LOG(1,"pit8253_set_clockin",("timer >= MAX_TIMER (%d)\n", timer));
		return;
    }
    if (this->config == NULL)
    {
		LOG(1,"pit8253_set_clockin",("PIT8253[%d] not initialized\n", which));
		return;
    }

	LOG(2,"pit8253_set_clockin",("timer #%d new clocki %f\n", timer, new_clockin));

    this->config->timer[timer].clockin = new_clockin;

	pit8253_timer_clock(which,timer);
    pit8253_timer_pulse(which,timer);
}

static void pit8253_w(int which, offs_t offset, data8_t data)
{
	PIT8253 *this = pit + which;
    int timer, count;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_w",("which >= MAX_PIT8253 (%d)\n", which));
		return;
    }
    if (this->config == NULL)
    {
		LOG(1,"pit8253_w",("PIT8253[%d] not initialized\n", which));
		return;
    }

    switch( offset )
	{
	case 0:
	case 1:
	case 2:
		LOG(2,"pit8253_w",("counter #%d $%02x: ", offset, data));
		switch (this->timer[offset].access)
		{
            case 0: /* counter latch command */
				LOG(2,0,("*latch command* "));
				this->timer[offset].msb ^= 1;
				if( !this->timer[offset].msb )
                    this->timer[offset].access = 3;
                break;
            case 1: /* read/write counter bits 0-7 only */
				LOG(2,0,("LSB only "));
                this->timer[offset].clock = data & 0xff;
                break;
            case 2: /* read/write counter bits 8-15 only */
				LOG(2,0,("MSB only "));
                this->timer[offset].clock = (data & 0xff) << 8;
                break;
            case 3: /* read/write bits 0-7 first, then 8-15 */
				if (this->timer[offset].msb)
				{
                    this->timer[offset].clock = (this->timer[offset].clock & 0x00ff)
						| ((data & 0xff) << 8);
					LOG(2,0,("MSB "));
				}
				else
				{
                    this->timer[offset].clock = (this->timer[offset].clock & 0xff00) | (data & 0xff);
					LOG(2,0,("LSB "));
				}
                this->timer[offset].msb ^= 1;
                break;
        }
		if (this->timer[offset].gate)
			this->timer[offset].time_access = timer_get_time();

		LOG(2,0,("timer #%d clock $%04x (%d)\n", offset, this->timer[offset].clock, this->timer[offset].clock));

		pit8253_timer_clock(which, offset);
		pit8253_timer_pulse(which, offset);

        break;

    case 3: /* PIT mode port */
        timer = (data >> 6) & 3;
        this->timer[timer].access = (data >> 4) & 3;
        this->timer[timer].mode = (data >> 1) & 7;
        this->timer[timer].bcd = data & 1;
		this->timer[timer].msb = (this->timer[timer].access == 2) ? 1 : 0;
		LOG(2,"pit8253_w",("mode $%02x: idx %d, access %d, mode %d, BCD %d\n",
								data, timer, this->timer[timer].access,
								this->timer[timer].mode, this->timer[timer].bcd));
		if (this->timer[timer].access == 0)
		{
            count = pit8253_get_count(this, timer);
#if 0
            this->timer[timer].latch = count -
                ((int)(this->config->timer[timer].clockin
					   * (timer_get_time() - this->timer[timer].time_access))) % count;
#else
			/* PeT pc1512 expects in mode 4 latch values bigger than the start value! */
            this->timer[timer].latch = count -
                ((int)(this->config->timer[timer].clockin
					   * (timer_get_time() - this->timer[timer].time_access)));
			while (this->timer[timer].latch < 0)
				this->timer[timer].latch += 0x10000;
#endif
			LOG(1,"pit8253_w",("latch #%d $%04x\n", timer, this->timer[timer].latch));
        }
		switch (this->timer[timer].mode)
		{
		case 0:
			this->timer[timer].output = 0;
			break;
		}
        break;
	default:
		LOG(1,"pit8253_w",("offset > 3 (%d)\n", offset));
    }
}

static data8_t pit8253_r(int which, offs_t offset)
{
	PIT8253 *this = pit + which;
	data8_t data = 0xff;
	int timer = offset;

	if (which >= MAX_PIT8253)
	{
		LOG(1,"pit8253_r",("which >= MAX_PIT8253 (%d)\n", which));
		return data;
    }
    if (this->config == NULL)
    {
		LOG(1,"pit8253_r",("PIT8253[%d] not initialized\n", which));
		return data;
    }

	switch( timer )
	{
	case 0:
	case 1:
	case 2:
		switch (this->timer[timer].access)
		{
		case 0: /* counter latch command */
			if( this->timer[timer].msb )
			{
				data = (this->timer[timer].latch >> 8) & 0xff;
				LOG(2,"pit8253_r",("latch #%d MSB $%02x\n", timer, data));
			}
			else
			{
				data = this->timer[timer].latch & 0xff;
				LOG(2,"pit8253_r",("latch #%d LSB $%02x\n", timer, data));
			}
			this->timer[timer].msb ^= 1;
			if( !this->timer[timer].msb )
				this->timer[timer].access = 3; /* switch back to clock access */
			break;
		case 1: /* read/write counter bits 0-7 only */
			data = this->timer[timer].clock & 0xff;
			LOG(2,"pit8253_r",("counter #%d LSB only $%02x\n", timer, data));
			break;
		case 2: /* read/write counter bits 8-15 only */
			data = (this->timer[timer].clock >> 8) & 0xff;
			LOG(2,"pit8253_r",("counter #%d MSB only $%02x\n", timer, data));
			break;
		case 3: /* read/write bits 0-7 first, then 8-15 */
			if (this->timer[timer].msb)
			{
				data = (this->timer[timer].clock >> 8) & 0xff;
				LOG(2,"pit8253_r",("counter #%d MSB $%02x\n", timer, data));
			}
			else
			{
				data = this->timer[timer].clock & 0xff;
				LOG(2,"pit8253_r",("counter #%d LSB $%02x\n", timer, data));
			}
			this->timer[timer].msb ^= 1;
			break;
        }
        break;
	case 3:
		break;
	default:
		LOG(1,"pit8253_r",("offset > 3 (%d)\n", offset));
    }
	return data;
}

void pit8253_gate_w(int which, offs_t offset, data8_t data)
{
    PIT8253 *this = pit + which;
	int timer = offset;
	int raising;

    if (which >= MAX_PIT8253)
    {
		LOG(1,"pit8253_gate_w",("which >= MAX_PIT8253 (%d)\n", which));
        return;
    }
	if (timer >= MAX_TIMER)
    {
		LOG(1,"pit8253_gate_w",("timer >= MAX_TIMER (%d)\n", timer));
        return;
    }
    if (this->config == NULL)
    {
		LOG(1,"pit8253_gate_w",("PIT8253[%d] not initialized\n", which));
        return;
    }

	/* no change? */
    if (this->timer[timer].gate == data)
		return;

    raising = (!this->timer[timer].gate && data) ? 1 : 0;
    this->timer[timer].gate = data;
	LOG(2,"pit8253_gate_w",("timer #%d gate %c%d\n", timer, raising ? '^' : ' ', data));

	switch( this->timer[timer].mode )
	{
	case 0:
		/* disable/enable counting only */
		pit8253_timer_clock(which,timer);
		pit8253_timer_pulse(which,timer);
        break;
	case 1: /* on raising edge: reset output after next clock */
		if (raising)
			this->timer[timer].reset = 1;
		break;
	case 2:
	case 6:
		/* raising edge: reload counter */
		if (raising)
		{
			this->timer[timer].time_access = timer_get_time();
			pit8253_timer_clock(which,timer);
			pit8253_timer_pulse(which,timer);
        }
		else
		/* low: disable + set output hi */
		if (data == 0)
			this->timer[timer].output = 1;
		/* hi: enabled */
        break;
	case 3:
	case 7:
		if (raising)
		{
			/* initiates counting !? */
        }
		else
		/* low: disable + set output hi */
		if (data == 0)
			this->timer[timer].output = 1;
		/* hi: enable */
		if (this->config->timer[timer].clk_callback)
			pit8253_timer_clock(which,timer);
        break;
	case 4:
		/* disable/enable counting only */
		pit8253_timer_clock(which,timer);
		pit8253_timer_pulse(which,timer);
        break;
    }
}

READ_HANDLER ( pit8253_0_r ) { return pit8253_r(0, offset); }
READ_HANDLER ( pit8253_1_r ) { return pit8253_r(1, offset); }

WRITE_HANDLER ( pit8253_0_w ) { pit8253_w(0, offset, data); }
WRITE_HANDLER ( pit8253_1_w ) { pit8253_w(1, offset, data); }

WRITE_HANDLER ( pit8253_0_gate_w ) { pit8253_gate_w(0, offset, data); }
WRITE_HANDLER ( pit8253_1_gate_w ) { pit8253_gate_w(1, offset, data); }

