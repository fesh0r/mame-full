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
#include "cbm.h"

#include "cia6526.h"

/* todin pin 50 or 60 hertz frequency */
#define TODIN_50HZ (this->cra&0x80)  /* else 60 Hz */

#define TIMER1_B6 (this->cra&2)
#define TIMER1_B6_TOGGLE (this->cra&4)	/* else single pulse */
#define TIMER1_ONESHOT (this->cra&8) /* else continuous */
#define TIMER1_STOP (!(this->cra&1))
#define TIMER1_RELOAD (this->cra&0x10)
#define TIMER1_COUNT_CNT (this->cra&0x20)	/* else clock 2 input */

#define TIMER2_ONESHOT (this->crb&8) /* else continuous */
#define TIMER2_STOP (!(this->crb&1))
#define TIMER2_RELOAD (this->crb&0x10)
#define TIMER2_COUNT_CLOCK ((this->crb&0x60)==0)
#define TIMER2_COUNT_CNT ((this->crb&0x60)==0x20)
#define TIMER2_COUNT_TIMER1 ((this->crb&0x60)==0x40)
#define TIMER2_COUNT_TIMER1_CNT ((this->crb&0x60)==0x60)

#define SERIAL_MODE_OUT ((this->cra&0x40))
#define TOD_ALARM (this->crb&0x80)   /* else write to tod clock */
#define BCD_INC(v) ( ((v)&0xf)==9?(v)+=0x10-9:(v)++)

typedef struct {
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

static CIA6526 cia[MAX_CIA] =
{
	{0}
};

static void cia_timer1_timeout (int which);
static void cia_timer2_timeout (int which);
static void cia_tod_timeout (int which);

/******************* configuration *******************/

void cia6526_config (int which, const struct cia6526_interface *intf)
{
	if (which >= MAX_CIA)
		return;
	memset (cia + which, 0, sizeof (cia[which]));
	cia[which].number=which;
	cia[which].intf = intf;
}


/******************* reset *******************/

void cia6526_reset (void)
{
	int i;

	assert (((int) cia[0].intf & 3) == 0);

	/* zap each structure, preserving the interface and swizzle */
	for (i = 0; i < MAX_CIA; i++)
	{
		const struct cia6526_interface *intf = cia[i].intf;

		if (cia[i].timer1)
			timer_remove (cia[i].timer1);
		if (cia[i].timer2)
			timer_remove (cia[i].timer2);
		if (cia[i].todtimer)
			timer_remove (cia[i].todtimer);
		memset (&cia[i], 0, sizeof (cia[i]));
		cia[i].number = i;
		cia[i].intf = intf;
		cia[i].t1l = 0xffff;
		cia[i].t2l = 0xffff;
		if (cia[i].intf!=0) cia[i].todtimer=timer_set(0.1,i,cia_tod_timeout);
	}
}

/******************* external interrupt check *******************/

static void cia_set_interrupt (CIA6526 *this, int data)
{
	this->ifr |= data;
	if (this->ier & data)
	{
		if (!(this->ifr & 0x80))
		{
			DBG_LOG (3, "cia set interrupt", ("%d %.2x\n",
											  this->number, data));
			if (this->intf->irq_func)
				this->intf->irq_func (1);
			this->ifr |= 0x80;
		}
	}
}

static void cia_clear_interrupt (CIA6526 *this, int data)
{
	this->ifr &= ~data;
	if ((this->ifr & 0x9f) == 0x80)
	{
		this->ifr &= ~0x80;
		if (this->intf->irq_func)
			this->intf->irq_func (0);
	}
}

/******************* Timer timeouts *************************/
static void cia_tod_timeout (int which)
{
	CIA6526 *this = cia + which;

	this->tod10ths++;
	if (this->tod10ths > 9)
	{
		this->tod10ths = 0;
		BCD_INC (this->todsec);
		if (this->todsec > 0x59)
		{
			this->todsec = 0;
			BCD_INC (this->todmin);
			if (this->todmin > 0x59)
			{
				this->todmin = 0;
				if (this->todhour == 0x91)
					this->todhour = 0;
				else if (this->todhour == 0x89)
					this->todhour = 0x90;
				else if (this->todhour == 0x11)
					this->todhour = 0x80;
				else if (this->todhour == 0x09)
					this->todhour = 0x10;
				else
					this->todhour++;
			}
		}
	}
	if ((this->todhour == this->alarmhour)
		&& (this->todmin == this->alarmmin)
		&& (this->todsec == this->alarmsec)
		&& (this->tod10ths == this->alarm10ths))
		cia_set_interrupt (this, 4);
	if (TODIN_50HZ)
	{
		if (this->intf->todin50hz)
			timer_reset (this->todtimer, 0.1);
		else
			timer_reset (this->todtimer, 5.0 / 60);
	}
	else
	{
		if (this->intf->todin50hz)
			timer_reset (this->todtimer, 6.0 / 50);
		else
			timer_reset (this->todtimer, 0.1);
	}
}

static void cia_timer1_state (CIA6526 *this)
{

	DBG_LOG (1, "timer1 state", ("%d\n", this->timer1_state));
	switch (this->timer1_state)
	{
	case 0:						   /* timer stopped */
		if (TIMER1_RELOAD)
		{
			this->cra &= ~0x10;
			this->t1c = this->t1l;
		}
		if (!TIMER1_STOP)
		{
			if (TIMER1_COUNT_CNT)
			{
				this->timer1_state = 2;
			}
			else
			{
				this->timer1_state = 1;
				this->timer1 = timer_set (TIME_IN_CYCLES (this->t1c, 0),
										  this->number, cia_timer1_timeout);
			}
		}
		break;
	case 1:						   /* counting clock input */
		if (TIMER1_RELOAD)
		{
			this->cra &= ~0x10;
			this->t1c = this->t1l;
			if (!TIMER1_STOP)
				timer_reset (this->timer1, TIME_IN_CYCLES (this->t1c, 0));
		}
		if (TIMER1_STOP)
		{
			this->timer1_state = 0;
			timer_remove (this->timer1);
			this->timer1 = 0;
		}
		else if (TIMER1_COUNT_CNT)
		{
			this->timer1_state = 2;
			timer_remove (this->timer1);
			this->timer1 = 0;
		}
		break;
	case 2:						   /* counting cnt input */
		if (TIMER1_RELOAD)
		{
			this->cra &= ~0x10;
			this->t1c = this->t1l;
		}
		if (TIMER1_STOP)
		{
			this->timer1_state = 0;
		}
		else if (!TIMER1_COUNT_CNT)
		{
			this->timer1 = timer_set (TIME_IN_CYCLES (this->t1c, 0),
									  this->number, cia_timer1_timeout);
			this->timer1_state = 1;
		}
		break;
	}
	DBG_LOG (1, "timer1 state", ("%d\n", this->timer1_state));
}

static void cia_timer2_state (CIA6526 *this)
{
	switch (this->timer2_state)
	{
	case 0:						   /* timer stopped */
		if (TIMER2_RELOAD)
		{
			this->crb &= ~0x10;
			this->t2c = this->t2l;
		}
		if (!TIMER2_STOP)
		{
			if (TIMER2_COUNT_CLOCK)
			{
				this->timer2_state = 1;
				this->timer2 = timer_set (TIME_IN_CYCLES (this->t2c, 0),
										  this->number, cia_timer2_timeout);
			}
			else
			{
				this->timer2_state = 2;
			}
		}
		break;
	case 1:						   /* counting clock input */
		if (TIMER2_RELOAD)
		{
			this->crb &= ~0x10;
			this->t2c = this->t2l;
			timer_reset (this->timer2, TIME_IN_CYCLES (this->t2c, 0));
		}
		if (TIMER2_STOP )
		{
			this->timer2_state = 0;
			timer_remove (this->timer2);
			this->timer2 = 0;
		}
		else if (!TIMER2_COUNT_CLOCK)
		{
			this->timer2_state = 2;
			timer_remove (this->timer2);
			this->timer2 = 0;
		}
		break;
	case 2:						   /* counting cnt, timer1  input */
		if (this->t2c == 0)
		{
			cia_set_interrupt (this, 2);
			this->crb |= 0x10;
		}
		if (TIMER2_RELOAD)
		{
			this->crb &= ~0x10;
			this->t2c = this->t2l;
		}
		if (TIMER2_STOP)
		{
			this->timer2_state = 0;
		}
		else if (TIMER2_COUNT_CLOCK)
		{
			this->timer2 = timer_set (TIME_IN_CYCLES (this->t2c, 0),
									  this->number, cia_timer2_timeout);
			this->timer2_state = 1;
		}
		break;
	}
}

static void cia_timer1_timeout (int which)
{
	CIA6526 *this = cia + which;

	this->t1c = this->t1l;

	if (TIMER1_ONESHOT)
	{
		this->cra &= ~1;
		this->timer1_state = 0;
	}
	else
	{
		timer_reset (this->timer1, TIME_IN_CYCLES (this->t1c, 0));
	}
	cia_set_interrupt (this, 1);
	if (SERIAL_MODE_OUT)
	{
		if (this->shift || this->loaded)
		{
			if (this->cnt)
			{
				if (this->shift == 0)
				{
					this->loaded = 0;
					this->serial = this->sdr;
				}
				this->sp = (this->serial & 0x80) ? 1 : 0;
				this->shift++;
				this->serial <<= 1;
				this->cnt = 0;
			}
			else
			{
				this->cnt = 1;
				if (this->shift == 8)
				{
					cia_set_interrupt (this, 8);
					this->shift = 0;
				}
			}
		}
	}

	/*  cia_timer1_state(this); */

	if (TIMER2_COUNT_TIMER1 || ((TIMER2_COUNT_TIMER1_CNT ) && (this->cnt)))
	{
		this->t2c--;
		cia_timer2_state (this);
	}
}

static void cia_timer2_timeout (int which)
{
	CIA6526 *this = cia + which;

	this->t2c = this->t2l;

	if (TIMER2_ONESHOT)
	{
		this->crb &= ~1;
		this->timer2_state = 0;
	}
	else
	{
		timer_reset (this->timer2, TIME_IN_CYCLES (this->t2c, 0));
	}

	cia_set_interrupt (this, 2);
	/*  cia_timer2_state(this); */
}


/******************* CPU interface for VIA read *******************/

static int cia6526_read (CIA6526 *this, int offset)
{
	int val = 0;

	offset &= 0xf;
	switch (offset)
	{
	case 0:
		if (this->intf->in_a_func)
			this->in_a = this->intf->in_a_func (this->number);
		val = ((this->out_a & this->ddr_a)
			   | (this->intf->a_pullup & ~this->ddr_a)) & this->in_a;
		break;
	case 1:
		if (this->intf->in_b_func)
			this->in_b = this->intf->in_b_func (this->number);
		val = ((this->out_b & this->ddr_b)
			   | (this->intf->b_pullup & ~this->ddr_b)) & this->in_b;
		break;
	case 2:
		val = this->ddr_a;
		break;
	case 3:
		val = this->ddr_b;
		break;
	case 8:
		if (this->todlatched)
			val = this->latch10ths;
		else
			val = this->tod10ths;
		this->todlatched = 0;
		break;
	case 9:
		if (this->todlatched)
			val = this->latchsec;
		else
			val = this->todsec;
		break;
	case 0xa:
		if (this->todlatched)
			val = this->latchmin;
		else
			val = this->todmin;
		break;
	case 0xb:
		this->latch10ths = this->tod10ths;
		this->latchsec = this->todsec;
		this->latchmin = this->todmin;
		val = this->latchhour = this->todhour;
		this->todlatched = 1;
		break;
	case 0xd:
		val = this->ifr & ~0x60;
		cia_clear_interrupt (this, 0x1f);
		break;
	case 4:
		if (this->timer1)
			val = TIME_TO_CYCLES (0, timer_timeleft (this->timer1)) & 0xff;
		else
			val = this->t1c & 0xff;
		DBG_LOG (3, "cia timer 1 lo", ("%d %.2x\n", this->number, val));
		break;
	case 5:
		if (this->timer1)
			val = TIME_TO_CYCLES (0, timer_timeleft (this->timer1)) >> 8;
		else
			val = this->t1c >> 8;
		DBG_LOG (3, "cia timer 1 hi", ("%d %.2x\n", this->number, val));
		break;
	case 6:
		if (this->timer2)
			val = TIME_TO_CYCLES (0, timer_timeleft (this->timer2)) & 0xff;
		else
			val = this->t2c & 0xff;
		DBG_LOG (3, "cia timer 2 lo", ("%d %.2x\n", this->number, val));
		break;
	case 7:
		if (this->timer2)
			val = TIME_TO_CYCLES (0, timer_timeleft (this->timer2)) >> 8;
		else
			val = this->t2c >> 8;
		DBG_LOG (3, "cia timer 2 hi", ("%d %.2x\n", this->number, val));
		break;
	case 0xe:
		val = this->cra;
		break;
	case 0xf:
		val = this->crb;
		break;
	case 0xc:
		val = this->sdr;
		break;
	}
	DBG_LOG (1, "cia read", ("%d %.2x:%.2x\n", this->number, offset, val));
	return val;
}


/******************* CPU interface for VIA write *******************/

static void cia6526_write (CIA6526 *this, int offset, int data)
{
	DBG_LOG (1, "cia write", ("%d %.2x:%.2x\n", this->number, offset, data));
	offset &= 0xf;

	switch (offset)
	{
	case 0:
		this->out_a = data;
		if (this->intf->out_a_func)
			this->intf->out_a_func (this->number, (this->out_a & this->ddr_a)
								 | (~this->ddr_a & this->intf->a_pullup));
		break;
	case 1:
		this->out_b = data;
		if (this->intf->out_b_func)
			this->intf->out_b_func (this->number, (this->out_b & this->ddr_b)
								 | (~this->ddr_b & this->intf->b_pullup));
		break;
	case 2:
		this->ddr_a = data;
		if (this->intf->out_a_func)
			this->intf->out_a_func (this->number, (this->out_a & this->ddr_a)
								 | (~this->ddr_a & this->intf->a_pullup));
		break;
	case 3:
		this->ddr_b = data;
		if (this->intf->out_b_func)
			this->intf->out_b_func (this->number, (this->out_b & this->ddr_b)
								 | (~this->ddr_b & this->intf->b_pullup));
		break;
	case 8:
		if (TOD_ALARM)
			this->alarm10ths = data;
		else
		{
			this->tod10ths = data;
			if (this->todstopped)
			{
				if (TODIN_50HZ)
				{
					if (this->intf->todin50hz)
						this->todtimer = timer_set (0.1, this->number,
													cia_tod_timeout);
					else
						this->todtimer = timer_set (5.0 / 60, this->number,
													cia_tod_timeout);
				}
				else
				{
					if (this->intf->todin50hz)
						this->todtimer = timer_set (60 / 5.0, this->number,
													cia_tod_timeout);
					else
						this->todtimer = timer_set (0.1, this->number,
												 cia_tod_timeout);
				}
			}
			this->todstopped = 0;
		}
		break;
	case 9:
		if (TOD_ALARM)
			this->alarmsec = data;
		else
			this->todsec = data;
		break;
	case 0xa:
		if (TOD_ALARM)
			this->alarmmin = data;
		else
			this->todmin = data;
		break;
	case 0xb:
		if (TOD_ALARM)
			this->alarmhour = data;
		else
		{
			if (this->todtimer)
				timer_remove (this->todtimer);
			this->todtimer = 0;
			this->todstopped = 1;
			this->todhour = data;
		}
		break;
	case 0xd:
		DBG_LOG (1, "cia interrupt enable", ("%d %.2x\n", this->number, data));
		if (data & 0x80)
		{
			this->ier |= data;
			cia_set_interrupt (this, data & 0x1f);
		}
		else
		{
			this->ier &= ~data;
			cia_clear_interrupt (this, data & 0x1f);
		}
		break;
	case 4:
		this->t1l = (this->t1l & ~0xff) | data;
		if (this->t1l == 0)
			this->t1l = 0x10000;		   /*avoid hanging in timer_schedule */
		DBG_LOG (3, "cia timer 1 lo write", ("%d %.2x\n", this->number, data));
		break;
	case 5:
		this->t1l = (this->t1l & 0xff) | (data << 8);
		if (this->t1l == 0)
			this->t1l = 0x10000;		   /*avoid hanging in timer_schedule */
		if (TIMER1_STOP)
			this->t1c = this->t1l;
		DBG_LOG (3, "cia timer 1 hi write", ("%d %.2x\n", this->number, data));
		break;
	case 6:
		this->t2l = (this->t2l & ~0xff) | data;
		if (this->t2l == 0)
			this->t2l = 0x10000;		   /*avoid hanging in timer_schedule */
		DBG_LOG (3, "cia timer 2 lo write", ("%d %.2x\n", this->number, data));
		break;
	case 7:
		this->t2l = (this->t2l & 0xff) | (data << 8);
		if (this->t2l == 0)
			this->t2l = 0x10000;		   /*avoid hanging in timer_schedule */
		if (TIMER2_STOP)
			this->t2c = this->t2l;
		DBG_LOG (3, "cia timer 2 hi write", ("%d %.2x\n", this->number, data));
		break;
	case 0xe:
		DBG_LOG (3, "cia write cra", ("%d %.2x\n", this->number, data));
		if ((this->cra & 0x40) != (data & 0x40))
		{
			if (!(this->cra & 0x40))
			{
				this->loaded = 0;
				this->shift = 0;
				this->cnt = 1;
			}
		}
		this->cra = data;
		cia_timer1_state (this);
		break;
	case 0xf:
		DBG_LOG (3, "cia write crb", ("%d %.2x\n", this->number, data));
		this->crb = data;
		cia_timer2_state (this);
		break;
	case 0xc:
		this->sdr = data;
		if (SERIAL_MODE_OUT)
		{
			this->loaded = 1;
		}
		break;
	}
}

static void cia_set_input_a (CIA6526 *this, int data)
{
	this->in_a = data;
}

static void cia_set_input_b (CIA6526 *this, int data)
{
	this->in_b = data;
}

static void cia6526_set_input_flag (CIA6526 *this, int data)
{
	if (this->flag && !data)
		cia_set_interrupt (this, 0x10);
	this->flag = data;
}

static void cia6526_set_input_sp (CIA6526 *this, int data)
{
	this->sp = data;
}

static void cia6526_set_input_cnt (CIA6526 *this, int data)
{
	if (!this->cnt && data)
	{
		if (!SERIAL_MODE_OUT)
		{
			this->serial >>= 1;
			if (this->sp)
				this->serial |= 0x80;
			if (++this->shift == 8)
			{
				this->sdr = this->serial;
				this->serial = 0;
				this->shift = 0;
				cia_set_interrupt (this, 8);
			}
		}
	}
	this->cnt = data;
}

READ_HANDLER ( cia6526_0_port_r )
{
	return cia6526_read (cia, offset);
}
READ_HANDLER ( cia6526_1_port_r )
{
	return cia6526_read (cia+1, offset);
}
READ_HANDLER ( cia6526_2_port_r )
{
	return cia6526_read (cia+2, offset);
}
READ_HANDLER ( cia6526_3_port_r )
{
	return cia6526_read (cia+3, offset);
}
READ_HANDLER ( cia6526_4_port_r )
{
	return cia6526_read (cia+4, offset);
}
READ_HANDLER ( cia6526_5_port_r )
{
	return cia6526_read (cia+5, offset);
}
READ_HANDLER ( cia6526_6_port_r )
{
	return cia6526_read (cia+6, offset);
}
READ_HANDLER ( cia6526_7_port_r )
{
	return cia6526_read (cia+7, offset);
}

WRITE_HANDLER ( cia6526_0_port_w )
{
	cia6526_write (cia, offset, data);
}
WRITE_HANDLER ( cia6526_1_port_w )
{
	cia6526_write (cia+1, offset, data);
}
WRITE_HANDLER ( cia6526_2_port_w )
{
	cia6526_write (cia+2, offset, data);
}
WRITE_HANDLER ( cia6526_3_port_w )
{
	cia6526_write (cia+3, offset, data);
}
WRITE_HANDLER ( cia6526_4_port_w )
{
	cia6526_write (cia+4, offset, data);
}
WRITE_HANDLER ( cia6526_5_port_w )
{
	cia6526_write (cia+5, offset, data);
}
WRITE_HANDLER ( cia6526_6_port_w )
{
	cia6526_write (cia+6, offset, data);
}
WRITE_HANDLER ( cia6526_7_port_w )
{
	cia6526_write (cia+7, offset, data);
}

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER ( cia6526_0_porta_w )
{
	cia_set_input_a (cia, data);
}
WRITE_HANDLER ( cia6526_1_porta_w )
{
	cia_set_input_a (cia+1, data);
}
WRITE_HANDLER ( cia6526_2_porta_w )
{
	cia_set_input_a (cia+2, data);
}
WRITE_HANDLER ( cia6526_3_porta_w )
{
	cia_set_input_a (cia+3, data);
}
WRITE_HANDLER ( cia6526_4_porta_w )
{
	cia_set_input_a (cia+4, data);
}
WRITE_HANDLER ( cia6526_5_porta_w )
{
	cia_set_input_a (cia+5, data);
}
WRITE_HANDLER ( cia6526_6_porta_w )
{
	cia_set_input_a (cia+6, data);
}
WRITE_HANDLER ( cia6526_7_porta_w )
{
	cia_set_input_a (cia+7, data);
}

WRITE_HANDLER ( cia6526_0_portb_w )
{
	cia_set_input_b (cia, data);
}
WRITE_HANDLER ( cia6526_1_portb_w )
{
	cia_set_input_b (cia+1, data);
}
WRITE_HANDLER ( cia6526_2_portb_w )
{
	cia_set_input_b (cia+2, data);
}
WRITE_HANDLER ( cia6526_3_portb_w )
{
	cia_set_input_b (cia+3, data);
}
WRITE_HANDLER ( cia6526_4_portb_w )
{
	cia_set_input_b (cia+4, data);
}
WRITE_HANDLER ( cia6526_5_portb_w )
{
	cia_set_input_b (cia+5, data);
}
WRITE_HANDLER ( cia6526_6_portb_w )
{
	cia_set_input_b (cia+6, data);
}
WRITE_HANDLER ( cia6526_7_portb_w )
{
	cia_set_input_b (cia+7, data);
}

READ_HANDLER ( cia6526_0_porta_r )
{
	return cia[0].in_a;
}
READ_HANDLER ( cia6526_1_porta_r )
{
	return cia[1].in_a;
}
READ_HANDLER ( cia6526_2_porta_r )
{
	return cia[2].in_a;
}
READ_HANDLER ( cia6526_3_porta_r )
{
	return cia[3].in_a;
}
READ_HANDLER ( cia6526_4_porta_r )
{
	return cia[4].in_a;
}
READ_HANDLER ( cia6526_5_porta_r )
{
	return cia[5].in_a;
}
READ_HANDLER ( cia6526_6_porta_r )
{
	return cia[6].in_a;
}
READ_HANDLER ( cia6526_7_porta_r )
{
	return cia[7].in_a;
}
READ_HANDLER ( cia6526_0_portb_r )
{
	return cia[0].in_b;
}
READ_HANDLER ( cia6526_1_portb_r )
{
	return cia[1].in_b;
}
READ_HANDLER ( cia6526_2_portb_r )
{
	return cia[2].in_b;
}
READ_HANDLER ( cia6526_3_portb_r )
{
	return cia[3].in_b;
}
READ_HANDLER ( cia6526_4_portb_r )
{
	return cia[4].in_b;
}
READ_HANDLER ( cia6526_5_portb_r )
{
	return cia[5].in_b;
}
READ_HANDLER ( cia6526_6_portb_r )
{
	return cia[6].in_b;
}
READ_HANDLER ( cia6526_7_portb_r )
{
	return cia[7].in_b;
}

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
