/**********************************************************************
	Metal Oxid Semiconductor / Commodore Business Machines
        Complex Interface Adapter 6526

	based on 6522via emulation
**********************************************************************/
/*
 only few tested (for c64)

 state:
 port a,b
 handshake support, flag input, not pc output
 timer a,b
  not counting of external clocks
  not switching port b pins
 interrupt system
*/

#include "driver.h"
#include "assert.h"

#define VERBOSE_DBG 1				   /* general debug messages */
#include "includes/cbm.h"

#include "includes/cia6526.h"

/* todin pin 50 or 60 hertz frequency */
#define TODIN_50HZ (This->cra&0x80)  /* else 60 Hz */

#define TIMER1_B6 (This->cra&2)
#define TIMER1_B6_TOGGLE (This->cra&4)	/* else single pulse */
#define TIMER1_ONESHOT (This->cra&8) /* else continuous */
#define TIMER1_STOP (!(This->cra&1))
#define TIMER1_RELOAD (This->cra&0x10)
#define TIMER1_COUNT_CNT (This->cra&0x20)	/* else clock 2 input */

#define TIMER2_ONESHOT (This->crb&8) /* else continuous */
#define TIMER2_STOP (!(This->crb&1))
#define TIMER2_RELOAD (This->crb&0x10)
#define TIMER2_COUNT_CLOCK ((This->crb&0x60)==0)
#define TIMER2_COUNT_CNT ((This->crb&0x60)==0x20)
#define TIMER2_COUNT_TIMER1 ((This->crb&0x60)==0x40)
#define TIMER2_COUNT_TIMER1_CNT ((This->crb&0x60)==0x60)

#define SERIAL_MODE_OUT ((This->cra&0x40))
#define TOD_ALARM (This->crb&0x80)   /* else write to tod clock */
#define BCD_INC(v) ( ((v)&0xf)==9?(v)+=0x10-9:(v)++)

typedef struct
{
	int active;
    int number; /* number of cia, to allow callback generate address */
	const struct cia6526_interface *intf;

	UINT8 in_a;
	UINT8 out_a;
	UINT8 ddr_a;

	UINT8 in_b;
	UINT8 out_b;
	UINT8 ddr_b;

	UINT16 t1c;
	UINT32 t1l;
	void *timer1;
	int timer1_state;

	UINT16 t2c;
	UINT32 t2l;
	void *timer2;
	int timer2_state;

	UINT8 tod10ths, todsec, todmin, todhour;
	UINT8 alarm10ths, alarmsec, alarmmin, alarmhour;

	UINT8 latch10ths, latchsec, latchmin, latchhour;
	int todlatched;

	int todstopped;
	void *todtimer;

	int flag;						   /* input */

	UINT8 sdr;
	int cnt,						   /* input or output */
	 sp;							   /* input or output */
	int serial, shift, loaded;

	UINT8 cra, crb;

	UINT8 ier;
	UINT8 ifr;
} CIA6526;

static CIA6526 *cia;

static void cia_timer1_timeout (int which);
static void cia_timer2_timeout (int which);
static void cia_tod_timeout (int which);

/******************* configuration *******************/

void cia6526_init()
{
	cia = auto_malloc(sizeof(CIA6526) * MAX_CIA);
	if (!cia)
		return;
	memset(cia, 0, sizeof(CIA6526) * MAX_CIA);
}

void cia6526_config (int which, const struct cia6526_interface *intf)
{
	if (which >= MAX_CIA)
		return;
	memset (cia + which, 0, sizeof (cia[which]));
	cia[which].active = 1;
	cia[which].number = which;
	cia[which].intf = intf;
	cia[which].timer1 = timer_alloc(cia_timer1_timeout);
	cia[which].timer2 = timer_alloc(cia_timer2_timeout);
	cia[which].todtimer = timer_alloc(cia_tod_timeout);
}


/******************* reset *******************/

void cia6526_reset (void)
{
	int i;

	assert (((int) cia[0].intf & 3) == 0);

	/* zap each structure, preserving the interface and swizzle */
	for (i = 0; i < MAX_CIA; i++)
	{
		if (cia[i].active)
		{
			const struct cia6526_interface *intf = cia[i].intf;
			void *timer1 = cia[i].timer1;
			void *timer2 = cia[i].timer2;
			void *todtimer = cia[i].todtimer;

			memset (&cia[i], 0, sizeof (cia[i]));
			cia[i].active = 1;
			cia[i].number = i;
			cia[i].intf = intf;
			cia[i].t1l = 0xffff;
			cia[i].t2l = 0xffff;
			cia[i].timer1 = timer1;
			cia[i].timer2 = timer2;
			cia[i].todtimer = todtimer;

			timer_reset(cia[i].timer1, TIME_NEVER);
			timer_reset(cia[i].timer2, TIME_NEVER);
			if (cia[i].intf)
				timer_adjust(cia[i].todtimer, 0.1, i, 0);
			else
				timer_reset(cia[i].todtimer, TIME_NEVER);
		}
	}
}

/******************* external interrupt check *******************/

static void cia_set_interrupt (CIA6526 *This, int data)
{
	This->ifr |= data;
	if (This->ier & data)
	{
		if (!(This->ifr & 0x80))
		{
			DBG_LOG (3, "cia set interrupt", ("%d %.2x\n",
											  This->number, data));
			if (This->intf->irq_func)
				This->intf->irq_func (1);
			This->ifr |= 0x80;
		}
	}
}

static void cia_clear_interrupt (CIA6526 *This, int data)
{
	This->ifr &= ~data;
	if ((This->ifr & 0x9f) == 0x80)
	{
		This->ifr &= ~0x80;
		if (This->intf->irq_func)
			This->intf->irq_func (0);
	}
}

/******************* Timer timeouts *************************/
static void cia_tod_timeout (int which)
{
	CIA6526 *This = cia + which;

	This->tod10ths++;
	if (This->tod10ths > 9)
	{
		This->tod10ths = 0;
		BCD_INC (This->todsec);
		if (This->todsec > 0x59)
		{
			This->todsec = 0;
			BCD_INC (This->todmin);
			if (This->todmin > 0x59)
			{
				This->todmin = 0;
				if (This->todhour == 0x91)
					This->todhour = 0;
				else if (This->todhour == 0x89)
					This->todhour = 0x90;
				else if (This->todhour == 0x11)
					This->todhour = 0x80;
				else if (This->todhour == 0x09)
					This->todhour = 0x10;
				else
					This->todhour++;
			}
		}
	}
	if ((This->todhour == This->alarmhour)
		&& (This->todmin == This->alarmmin)
		&& (This->todsec == This->alarmsec)
		&& (This->tod10ths == This->alarm10ths))
		cia_set_interrupt (This, 4);
	if (TODIN_50HZ)
	{
		if (This->intf->todin50hz)
			timer_reset (This->todtimer, 0.1);
		else
			timer_reset (This->todtimer, 5.0 / 60);
	}
	else
	{
		if (This->intf->todin50hz)
			timer_reset (This->todtimer, 6.0 / 50);
		else
			timer_reset (This->todtimer, 0.1);
	}
}

static void cia_timer1_state (CIA6526 *This)
{

	DBG_LOG (1, "timer1 state", ("%d\n", This->timer1_state));
	switch (This->timer1_state)
	{
	case 0:						   /* timer stopped */
		if (TIMER1_RELOAD)
		{
			This->cra &= ~0x10;
			This->t1c = This->t1l;
		}
		if (!TIMER1_STOP)
		{
			if (TIMER1_COUNT_CNT)
			{
				This->timer1_state = 2;
			}
			else
			{
				This->timer1_state = 1;
				timer_adjust(This->timer1, TIME_IN_CYCLES (This->t1c, 0), This->number, 0);
			}
		}
		break;
	case 1:						   /* counting clock input */
		if (TIMER1_RELOAD)
		{
			This->cra &= ~0x10;
			This->t1c = This->t1l;
			if (!TIMER1_STOP)
				timer_reset (This->timer1, TIME_IN_CYCLES (This->t1c, 0));
		}
		if (TIMER1_STOP)
		{
			This->timer1_state = 0;
			timer_reset(This->timer1, TIME_NEVER);
		}
		else if (TIMER1_COUNT_CNT)
		{
			This->timer1_state = 2;
			timer_reset(This->timer1, TIME_NEVER);
		}
		break;
	case 2:						   /* counting cnt input */
		if (TIMER1_RELOAD)
		{
			This->cra &= ~0x10;
			This->t1c = This->t1l;
		}
		if (TIMER1_STOP)
		{
			This->timer1_state = 0;
		}
		else if (!TIMER1_COUNT_CNT)
		{
			timer_adjust(This->timer1, TIME_IN_CYCLES (This->t1c, 0), This->number, 0);
			This->timer1_state = 1;
		}
		break;
	}
	DBG_LOG (1, "timer1 state", ("%d\n", This->timer1_state));
}

static void cia_timer2_state (CIA6526 *This)
{
	switch (This->timer2_state)
	{
	case 0:						   /* timer stopped */
		if (TIMER2_RELOAD)
		{
			This->crb &= ~0x10;
			This->t2c = This->t2l;
		}
		if (!TIMER2_STOP)
		{
			if (TIMER2_COUNT_CLOCK)
			{
				This->timer2_state = 1;
				timer_adjust(This->timer2, TIME_IN_CYCLES(This->t2c, 0), This->number, 0);
			}
			else
			{
				This->timer2_state = 2;
			}
		}
		break;
	case 1:						   /* counting clock input */
		if (TIMER2_RELOAD)
		{
			This->crb &= ~0x10;
			This->t2c = This->t2l;
			timer_reset (This->timer2, TIME_IN_CYCLES (This->t2c, 0));
		}
		if (TIMER2_STOP )
		{
			This->timer2_state = 0;
			timer_reset(This->timer2, TIME_NEVER);
		}
		else if (!TIMER2_COUNT_CLOCK)
		{
			This->timer2_state = 2;
			timer_reset(This->timer2, TIME_NEVER);
		}
		break;
	case 2:						   /* counting cnt, timer1  input */
		if (This->t2c == 0)
		{
			cia_set_interrupt (This, 2);
			This->crb |= 0x10;
		}
		if (TIMER2_RELOAD)
		{
			This->crb &= ~0x10;
			This->t2c = This->t2l;
		}
		if (TIMER2_STOP)
		{
			This->timer2_state = 0;
		}
		else if (TIMER2_COUNT_CLOCK)
		{
			timer_adjust(This->timer2, TIME_IN_CYCLES(This->t2c, 0), This->number, 0);
			This->timer2_state = 1;
		}
		break;
	}
}

static void cia_timer1_timeout (int which)
{
	CIA6526 *This = cia + which;

	This->t1c = This->t1l;

	if (TIMER1_ONESHOT)
	{
		This->cra &= ~1;
		This->timer1_state = 0;
	}
	else
	{
		timer_reset (This->timer1, TIME_IN_CYCLES (This->t1c, 0));
	}
	cia_set_interrupt (This, 1);
	if (SERIAL_MODE_OUT)
	{
		if (This->shift || This->loaded)
		{
			if (This->cnt)
			{
				if (This->shift == 0)
				{
					This->loaded = 0;
					This->serial = This->sdr;
				}
				This->sp = (This->serial & 0x80) ? 1 : 0;
				This->shift++;
				This->serial <<= 1;
				This->cnt = 0;
			}
			else
			{
				This->cnt = 1;
				if (This->shift == 8)
				{
					cia_set_interrupt (This, 8);
					This->shift = 0;
				}
			}
		}
	}

	/*  cia_timer1_state(This); */

	if (TIMER2_COUNT_TIMER1 || ((TIMER2_COUNT_TIMER1_CNT ) && (This->cnt)))
	{
		This->t2c--;
		cia_timer2_state (This);
	}
}

static void cia_timer2_timeout (int which)
{
	CIA6526 *This = cia + which;

	This->t2c = This->t2l;

	if (TIMER2_ONESHOT)
	{
		This->crb &= ~1;
		This->timer2_state = 0;
	}
	else
	{
		timer_reset (This->timer2, TIME_IN_CYCLES (This->t2c, 0));
	}

	cia_set_interrupt (This, 2);
	/*  cia_timer2_state(This); */
}


/******************* CPU interface for VIA read *******************/

static int cia6526_read (CIA6526 *This, int offset)
{
	int val = 0;

	offset &= 0xf;
	switch (offset)
	{
	case 0:
		if (This->intf->in_a_func)
			This->in_a = This->intf->in_a_func (This->number);
		val = ((This->out_a & This->ddr_a)
			   | (This->intf->a_pullup & ~This->ddr_a)) & This->in_a;
		break;
	case 1:
		if (This->intf->in_b_func)
			This->in_b = This->intf->in_b_func (This->number);
		val = ((This->out_b & This->ddr_b)
			   | (This->intf->b_pullup & ~This->ddr_b)) & This->in_b;
		break;
	case 2:
		val = This->ddr_a;
		break;
	case 3:
		val = This->ddr_b;
		break;
	case 8:
		if (This->todlatched)
			val = This->latch10ths;
		else
			val = This->tod10ths;
		This->todlatched = 0;
		break;
	case 9:
		if (This->todlatched)
			val = This->latchsec;
		else
			val = This->todsec;
		break;
	case 0xa:
		if (This->todlatched)
			val = This->latchmin;
		else
			val = This->todmin;
		break;
	case 0xb:
		This->latch10ths = This->tod10ths;
		This->latchsec = This->todsec;
		This->latchmin = This->todmin;
		val = This->latchhour = This->todhour;
		This->todlatched = 1;
		break;
	case 0xd:
		val = This->ifr & ~0x60;
		cia_clear_interrupt (This, 0x1f);
		break;
	case 4:
		if (This->timer1)
			val = TIME_TO_CYCLES (0, timer_timeleft (This->timer1)) & 0xff;
		else
			val = This->t1c & 0xff;
		DBG_LOG (3, "cia timer 1 lo", ("%d %.2x\n", This->number, val));
		break;
	case 5:
		if (This->timer1)
			val = TIME_TO_CYCLES (0, timer_timeleft (This->timer1)) >> 8;
		else
			val = This->t1c >> 8;
		DBG_LOG (3, "cia timer 1 hi", ("%d %.2x\n", This->number, val));
		break;
	case 6:
		if (This->timer2)
			val = TIME_TO_CYCLES (0, timer_timeleft (This->timer2)) & 0xff;
		else
			val = This->t2c & 0xff;
		DBG_LOG (3, "cia timer 2 lo", ("%d %.2x\n", This->number, val));
		break;
	case 7:
		if (This->timer2)
			val = TIME_TO_CYCLES (0, timer_timeleft (This->timer2)) >> 8;
		else
			val = This->t2c >> 8;
		DBG_LOG (3, "cia timer 2 hi", ("%d %.2x\n", This->number, val));
		break;
	case 0xe:
		val = This->cra;
		break;
	case 0xf:
		val = This->crb;
		break;
	case 0xc:
		val = This->sdr;
		break;
	}
	DBG_LOG (1, "cia read", ("%d %.2x:%.2x\n", This->number, offset, val));
	return val;
}


/******************* CPU interface for VIA write *******************/

static void cia6526_write (CIA6526 *This, int offset, int data)
{
	DBG_LOG (1, "cia write", ("%d %.2x:%.2x\n", This->number, offset, data));
	offset &= 0xf;

	switch (offset)
	{
	case 0:
		This->out_a = data;
		if (This->intf->out_a_func)
			This->intf->out_a_func (This->number, (This->out_a & This->ddr_a)
								 | (~This->ddr_a & This->intf->a_pullup));
		break;
	case 1:
		This->out_b = data;
		if (This->intf->out_b_func)
			This->intf->out_b_func (This->number, (This->out_b & This->ddr_b)
								 | (~This->ddr_b & This->intf->b_pullup));
		break;
	case 2:
		This->ddr_a = data;
		if (This->intf->out_a_func)
			This->intf->out_a_func (This->number, (This->out_a & This->ddr_a)
								 | (~This->ddr_a & This->intf->a_pullup));
		break;
	case 3:
		This->ddr_b = data;
		if (This->intf->out_b_func)
			This->intf->out_b_func (This->number, (This->out_b & This->ddr_b)
								 | (~This->ddr_b & This->intf->b_pullup));
		break;
	case 8:
		if (TOD_ALARM)
			This->alarm10ths = data;
		else
		{
			This->tod10ths = data;
			if (This->todstopped)
			{
				if (TODIN_50HZ)
				{
					timer_adjust(This->todtimer, (This->intf->todin50hz) ? 0.1 : 5.0/60, This->number, 0);
				}
				else
				{
					timer_adjust(This->todtimer, (This->intf->todin50hz) ? 60 / 5.0 : 0.1, This->number, 0);
				}
			}
			This->todstopped = 0;
		}
		break;
	case 9:
		if (TOD_ALARM)
			This->alarmsec = data;
		else
			This->todsec = data;
		break;
	case 0xa:
		if (TOD_ALARM)
			This->alarmmin = data;
		else
			This->todmin = data;
		break;
	case 0xb:
		if (TOD_ALARM)
			This->alarmhour = data;
		else
		{
			timer_reset(This->todtimer, TIME_NEVER);
			This->todstopped = 1;
			This->todhour = data;
		}
		break;
	case 0xd:
		DBG_LOG (1, "cia interrupt enable", ("%d %.2x\n", This->number, data));
		if (data & 0x80)
		{
			This->ier |= data;
			cia_set_interrupt (This, 0);
		}
		else
		{
			This->ier &= ~data;
			cia_clear_interrupt (This, data & 0x1f);
		}
		break;
	case 4:
		This->t1l = (This->t1l & ~0xff) | data;
		if (This->t1l == 0)
			This->t1l = 0x10000;		   /*avoid hanging in timer_schedule */
		DBG_LOG (3, "cia timer 1 lo write", ("%d %.2x\n", This->number, data));
		break;
	case 5:
		This->t1l = (This->t1l & 0xff) | (data << 8);
		if (This->t1l == 0)
			This->t1l = 0x10000;		   /*avoid hanging in timer_schedule */
		if (TIMER1_STOP)
			This->t1c = This->t1l;
		DBG_LOG (3, "cia timer 1 hi write", ("%d %.2x\n", This->number, data));
		break;
	case 6:
		This->t2l = (This->t2l & ~0xff) | data;
		if (This->t2l == 0)
			This->t2l = 0x10000;		   /*avoid hanging in timer_schedule */
		DBG_LOG (3, "cia timer 2 lo write", ("%d %.2x\n", This->number, data));
		break;
	case 7:
		This->t2l = (This->t2l & 0xff) | (data << 8);
		if (This->t2l == 0)
			This->t2l = 0x10000;		   /*avoid hanging in timer_schedule */
		if (TIMER2_STOP)
			This->t2c = This->t2l;
		DBG_LOG (3, "cia timer 2 hi write", ("%d %.2x\n", This->number, data));
		break;
	case 0xe:
		DBG_LOG (3, "cia write cra", ("%d %.2x\n", This->number, data));
		if ((This->cra & 0x40) != (data & 0x40))
		{
			if (!(This->cra & 0x40))
			{
				This->loaded = 0;
				This->shift = 0;
				This->cnt = 1;
			}
		}
		This->cra = data;
		cia_timer1_state (This);
		break;
	case 0xf:
		DBG_LOG (3, "cia write crb", ("%d %.2x\n", This->number, data));
		This->crb = data;
		cia_timer2_state (This);
		break;
	case 0xc:
		This->sdr = data;
		if (SERIAL_MODE_OUT)
		{
			This->loaded = 1;
		}
		break;
	}
}

static void cia_set_input_a (CIA6526 *This, int data)
{
	This->in_a = data;
}

static void cia_set_input_b (CIA6526 *This, int data)
{
	This->in_b = data;
}

static void cia6526_set_input_flag (CIA6526 *This, int data)
{
	if (This->flag && !data)
		cia_set_interrupt (This, 0x10);
	This->flag = data;
}

static void cia6526_set_input_sp (CIA6526 *This, int data)
{
	This->sp = data;
}

static void cia6526_set_input_cnt (CIA6526 *This, int data)
{
	if (!This->cnt && data)
	{
		if (!SERIAL_MODE_OUT)
		{
			This->serial >>= 1;
			if (This->sp)
				This->serial |= 0x80;
			if (++This->shift == 8)
			{
				This->sdr = This->serial;
				This->serial = 0;
				This->shift = 0;
				cia_set_interrupt (This, 8);
			}
		}
	}
	This->cnt = data;
}

 READ8_HANDLER ( cia6526_0_port_r )
{
	return cia6526_read (cia, offset);
}
 READ8_HANDLER ( cia6526_1_port_r )
{
	return cia6526_read (cia+1, offset);
}
 READ8_HANDLER ( cia6526_2_port_r )
{
	return cia6526_read (cia+2, offset);
}
 READ8_HANDLER ( cia6526_3_port_r )
{
	return cia6526_read (cia+3, offset);
}
 READ8_HANDLER ( cia6526_4_port_r )
{
	return cia6526_read (cia+4, offset);
}
 READ8_HANDLER ( cia6526_5_port_r )
{
	return cia6526_read (cia+5, offset);
}
 READ8_HANDLER ( cia6526_6_port_r )
{
	return cia6526_read (cia+6, offset);
}
 READ8_HANDLER ( cia6526_7_port_r )
{
	return cia6526_read (cia+7, offset);
}

WRITE8_HANDLER ( cia6526_0_port_w )
{
	cia6526_write (cia, offset, data);
}
WRITE8_HANDLER ( cia6526_1_port_w )
{
	cia6526_write (cia+1, offset, data);
}
WRITE8_HANDLER ( cia6526_2_port_w )
{
	cia6526_write (cia+2, offset, data);
}
WRITE8_HANDLER ( cia6526_3_port_w )
{
	cia6526_write (cia+3, offset, data);
}
WRITE8_HANDLER ( cia6526_4_port_w )
{
	cia6526_write (cia+4, offset, data);
}
WRITE8_HANDLER ( cia6526_5_port_w )
{
	cia6526_write (cia+5, offset, data);
}
WRITE8_HANDLER ( cia6526_6_port_w )
{
	cia6526_write (cia+6, offset, data);
}
WRITE8_HANDLER ( cia6526_7_port_w )
{
	cia6526_write (cia+7, offset, data);
}

/******************* 8-bit A/B port interfaces *******************/

WRITE8_HANDLER ( cia6526_0_porta_w )
{
	cia_set_input_a (cia, data);
}
WRITE8_HANDLER ( cia6526_1_porta_w )
{
	cia_set_input_a (cia+1, data);
}
WRITE8_HANDLER ( cia6526_2_porta_w )
{
	cia_set_input_a (cia+2, data);
}
WRITE8_HANDLER ( cia6526_3_porta_w )
{
	cia_set_input_a (cia+3, data);
}
WRITE8_HANDLER ( cia6526_4_porta_w )
{
	cia_set_input_a (cia+4, data);
}
WRITE8_HANDLER ( cia6526_5_porta_w )
{
	cia_set_input_a (cia+5, data);
}
WRITE8_HANDLER ( cia6526_6_porta_w )
{
	cia_set_input_a (cia+6, data);
}
WRITE8_HANDLER ( cia6526_7_porta_w )
{
	cia_set_input_a (cia+7, data);
}

WRITE8_HANDLER ( cia6526_0_portb_w )
{
	cia_set_input_b (cia, data);
}
WRITE8_HANDLER ( cia6526_1_portb_w )
{
	cia_set_input_b (cia+1, data);
}
WRITE8_HANDLER ( cia6526_2_portb_w )
{
	cia_set_input_b (cia+2, data);
}
WRITE8_HANDLER ( cia6526_3_portb_w )
{
	cia_set_input_b (cia+3, data);
}
WRITE8_HANDLER ( cia6526_4_portb_w )
{
	cia_set_input_b (cia+4, data);
}
WRITE8_HANDLER ( cia6526_5_portb_w )
{
	cia_set_input_b (cia+5, data);
}
WRITE8_HANDLER ( cia6526_6_portb_w )
{
	cia_set_input_b (cia+6, data);
}
WRITE8_HANDLER ( cia6526_7_portb_w )
{
	cia_set_input_b (cia+7, data);
}

 READ8_HANDLER ( cia6526_0_porta_r )
{
	return cia[0].in_a;
}
 READ8_HANDLER ( cia6526_1_porta_r )
{
	return cia[1].in_a;
}
 READ8_HANDLER ( cia6526_2_porta_r )
{
	return cia[2].in_a;
}
 READ8_HANDLER ( cia6526_3_porta_r )
{
	return cia[3].in_a;
}
 READ8_HANDLER ( cia6526_4_porta_r )
{
	return cia[4].in_a;
}
 READ8_HANDLER ( cia6526_5_porta_r )
{
	return cia[5].in_a;
}
 READ8_HANDLER ( cia6526_6_porta_r )
{
	return cia[6].in_a;
}
 READ8_HANDLER ( cia6526_7_porta_r )
{
	return cia[7].in_a;
}
#if 0
 READ8_HANDLER ( cia6526_0_portb_r )
{
	return cia[0].in_b;
}
 READ8_HANDLER ( cia6526_1_portb_r )
{
	return cia[1].in_b;
}
 READ8_HANDLER ( cia6526_2_portb_r )
{
	return cia[2].in_b;
}
 READ8_HANDLER ( cia6526_3_portb_r )
{
	return cia[3].in_b;
}
 READ8_HANDLER ( cia6526_4_portb_r )
{
	return cia[4].in_b;
}
 READ8_HANDLER ( cia6526_5_portb_r )
{
	return cia[5].in_b;
}
 READ8_HANDLER ( cia6526_6_portb_r )
{
	return cia[6].in_b;
}
 READ8_HANDLER ( cia6526_7_portb_r )
{
	return cia[7].in_b;
}
#endif

void cia6526_0_set_input_flag (int data)
{
	cia6526_set_input_flag (cia, data);
}
void cia6526_1_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+1, data);
}
void cia6526_2_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+2, data);
}
void cia6526_3_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+3, data);
}
void cia6526_4_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+4, data);
}
void cia6526_5_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+5, data);
}
void cia6526_6_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+6, data);
}
void cia6526_7_set_input_flag (int data)
{
	cia6526_set_input_flag (cia+7, data);
}

void cia6526_0_set_input_sp (int data)
{
	cia6526_set_input_sp (cia, data);
}
void cia6526_1_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+1, data);
}
void cia6526_2_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+2, data);
}
void cia6526_3_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+3, data);
}
void cia6526_4_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+4, data);
}
void cia6526_5_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+5, data);
}
void cia6526_6_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+6, data);
}
void cia6526_7_set_input_sp (int data)
{
	cia6526_set_input_sp (cia+7, data);
}

void cia6526_0_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia, data);
}
void cia6526_1_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+1, data);
}
void cia6526_2_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+2, data);
}
void cia6526_3_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+3, data);
}
void cia6526_4_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+4, data);
}
void cia6526_5_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+5, data);
}
void cia6526_6_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+6, data);
}
void cia6526_7_set_input_cnt (int data)
{
	cia6526_set_input_cnt (cia+7, data);
}

void cia6526_status (char *text, int size)
{
	text[0] = 0;
#if VERBOSE_DBG
#if 0
	snprintf (text, size, "cia ier:%.2x ifr:%.2x %d", cia[1].ier, cia[1].ifr,
			  cia[1].flag);
#endif
#if 0
	snprintf (text, size, "cia 1 %.2x %.2x %.2x %.2x",
			  cia[1].tod10ths, cia[1].todsec,
			  cia[1].todmin, cia[1].todhour);

#endif
#endif
}
