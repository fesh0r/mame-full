/**********************************************************************

	8259 PIC interface and emulation

**********************************************************************/

#include "includes/pic8259.h"

/* NPW 22-Nov-2002 - fun with Microsoft... */
#ifdef x86
#undef x86
#endif

#define IRQ_COUNT	8

#define VERBOSE	0

#if VERBOSE
#define LOG(msg)	logerror msg
#else
#define LOG(msg)
#endif

/* note this structure is whacked out; the author of this code was fixated
 * on separating out every single signal */
struct pic8259
{
	mame_timer *timer;

	UINT8 enable;
	UINT8 in_service;
	UINT8 pending;
	UINT8 prio;

	UINT8 icw2;
	UINT8 icw3;
	UINT8 icw4;

	UINT8 special;
	UINT8 input;

	UINT8 level_trig_mode;
	UINT8 vector_size;
	UINT8 cascade;
	UINT8 base;
	UINT8 slave;

	UINT8 nested;
	UINT8 mode;
	UINT8 auto_eoi;
	UINT8 x86;
};

static struct pic8259 *pic;

static void pic8259_timerproc(int which);



/* initializer */
int pic8259_init(int count)
{
	int i;

	pic = auto_malloc(count * sizeof(struct pic8259));
	if (!pic)
		return 1;

	memset(pic, 0, count * sizeof(struct pic8259));

	for (i = 0; i < count; i++)
	{
		pic[i].timer = mame_timer_alloc(pic8259_timerproc);
		if (!pic[i].timer)
			return 1;
	}

	return 0;
}



static void pic8259_timerproc(int which)
{
	struct pic8259 *p = &pic[which];
	int irq;
	UINT8 mask;


	/* check the various IRQs */
	for (irq = 0; irq < IRQ_COUNT; irq++) 
	{
		mask = 1 << irq;

		/* is this IRQ in service? */
		if (p->in_service & mask)
		{
			LOG(("pic8259_timerproc(): PIC #%d IRQ #%d still in service\n", which, irq));
			return;
		}

		/* is this IRQ pending and enabled? */
		if ((p->pending & mask) && !(p->enable & mask))
		{
			p->pending &= ~mask;
			p->in_service |= mask;

			LOG(("pic8259_timerproc(): PIC #%d triggering IRQ #%d\n", which, irq));

			cpu_irq_line_vector_w(0, 0, irq + p->base);
			cpu_set_irq_line(0, 0, HOLD_LINE);
			return;
		}
	}
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}



static void pic8259_set_timer(int which)
{
	mame_timer_adjust(pic[which].timer, time_zero, which, time_zero);
}



static void pic8259_issue_irq(int which, int irq)
{
	assert(irq >= 0);
	assert(irq < IRQ_COUNT);

	LOG(("pic8259_issue_irq(): PIC #%d received IRQ #%d\n", which, irq));

	pic[which].pending |= 1 << irq;
	pic8259_set_timer(which);
}



static int pic8259_irq_pending(int which, int irq)
{
	UINT8 mask = 1 << irq;
	return (pic[which].pending & mask) ? 1 : 0;
}



static int pic8259_read(int which, offs_t offset)
{
	struct pic8259 *this = &pic[which];

	/* NPW 18-May-2003 - Changing 0xFF to 0x00 as per Ruslan */
	int data = 0x00;

	switch( offset ) {
	case 0: /* PIC acknowledge IRQ */
        if (this->special)
		{
            this->special = 0;
            data = this->input;
            LOG(("PIC_ack_r(): $%02x read special\n", data));
        }
		else
		{
            LOG(("PIC_ack_r(): $%02x\n", data));
        }
        break;

	case 1: /* PIC mask register */
        data = this->enable;
        LOG(("PIC_enable_r(): $%02x\n", data));
        break;
	}
	return data;
}



static void pic8259_write(int which, offs_t offset, data8_t data )
{
	struct pic8259 *this = &pic[which];
	switch( offset )
	{
    case 0:    /* PIC acknowledge IRQ */
		if( data & 0x10 )	/* write ICW1 ? */
		{
            this->icw2 = 1;
            this->icw3 = 1;
            this->level_trig_mode = (data >> 3) & 1;
            this->vector_size = (data >> 2) & 1;
			this->cascade = ((data >> 1) & 1) ^ 1;
			if( this->cascade == 0 )
				this->icw3 = 0;
            this->icw4 = data & 1;
			LOG(("PIC_ack_w(): $%02x: ICW1, icw4 %d, cascade %d, vec size %d, ltim %d\n",
                data, this->icw4, this->cascade, this->vector_size, this->level_trig_mode));
		}
		else if (data & 0x08)
		{
            LOG(("PIC_ack_w(): $%02x: OCW3", data));
			switch (data & 0x60)
			{
                case 0x00:
                case 0x20:
                    break;
                case 0x40:
                    LOG((", reset special mask"));
                    break;
                case 0x60:
                    LOG((", set special mask"));
                    break;
            }
			switch (data & 0x03)
			{
                case 0x00:
				case 0x01:
					LOG((", no operation"));
                    break;
                case 0x02:
                    LOG((", read request register"));
                    this->special = 1;
					this->input = this->pending;
                    break;
                case 0x03:
                    LOG((", read in-service register"));
                    this->special = 1;
					this->input = this->in_service & ~this->enable;
                    break;
            }
            LOG(("\n"));
		}
		else
		{
			int n = data & 7;
            UINT8 mask = 1 << n;
            LOG(("PIC_ack_w(): $%02x: OCW2", data));
			switch (data & 0xe0)
			{
                case 0x00:
                    LOG((" rotate auto EOI clear\n"));
					this->prio = 0;
                    break;
                case 0x20:
                    LOG((" nonspecific EOI\n"));
					for( n = 0, mask = 1<<this->prio; n < 8; n++, mask = (mask<<1) | (mask>>7) )
					{
						if( this->in_service & mask )
						{
							LOG(("pic8259_write(): PIC #%d clearing IRQ %d\n", which, n));
                            this->in_service &= ~mask;
							this->pending &= ~mask;
                            break;
                        }
                    }
                    break;
                case 0x40:
                    LOG((" OCW2 NOP\n"));
                    break;
                case 0x60:
                    LOG((" OCW2 specific EOI%d\n", n));
					if( this->in_service & mask )
                    {
						this->in_service &= ~mask;
						this->pending &= ~mask;
					}
                    break;
                case 0x80:
					LOG((" OCW2 rotate auto EOI set\n"));
					this->prio = ++this->prio & 7;
                    break;
                case 0xa0:
                    LOG((" OCW2 rotate on nonspecific EOI\n"));
					for( n = 0, mask = 1<<this->prio; n < 8; n++, mask = (mask<<1) | (mask>>7) )
					{
						if( this->in_service & mask )
						{
                            this->in_service &= ~mask;
							this->pending &= ~mask;
							this->prio = ++this->prio & 7;
                            break;
                        }
                    }
					break;
                case 0xc0:
                    LOG((" OCW2 set priority\n"));
					this->prio = n & 7;
                    break;
                case 0xe0:
                    LOG((" OCW2 rotate on specific EOI%d\n", n));
					if( this->in_service & mask )
					{
						this->in_service &= ~mask;
						this->pending &= ~mask;
						this->prio = ++this->prio & 7;
					}
                    break;
            }
        }
        break;
    case 1:    /* PIC ICW2,3,4 or OCW1 */
		if( this->icw2 )
		{
            this->base = data & 0xf8;
            LOG(("PIC_enable_w(): $%02x: ICW2 (base)\n", this->base));
            this->icw2 = 0;
		}
		else if( this->icw3 )
		{
            this->slave = data;
            LOG(("PIC_enable_w(): $%02x: ICW3 (slave)\n", this->slave));
            this->icw3 = 0;
		}
		else if( this->icw4 )
		{
            this->nested = (data >> 4) & 1;
            this->mode = (data >> 2) & 3;
            this->auto_eoi = (data >> 1) & 1;
            this->x86 = data & 1;
            LOG(("PIC_enable_w(): $%02x: ICW4 x86 mode %d, auto EOI %d, mode %d, nested %d\n",
                data, this->x86, this->auto_eoi, this->mode, this->nested));
            this->icw4 = 0;
		}
		else
		{
            LOG(("PIC_enable_w(): $%02x: OCW1 enable\n", data));
            this->enable = data;
			this->in_service &= data;
			this->pending &= data;
        }
        break;
    }

	pic8259_set_timer(which);
}



/* ----------------------------------------------------------------------- */

READ_HANDLER ( pic8259_0_r )	{ return pic8259_read(0, offset); }
READ_HANDLER ( pic8259_1_r )	{ return pic8259_read(1, offset); }
WRITE_HANDLER ( pic8259_0_w )	{ pic8259_write(0, offset, data); }
WRITE_HANDLER ( pic8259_1_w )	{ pic8259_write(1, offset, data); }

void pic8259_0_issue_irq(int irq)
{
	pic8259_issue_irq(0, irq);
}

void pic8259_1_issue_irq(int irq)
{
	pic8259_issue_irq(1, irq);
}

int pic8259_0_irq_pending(int irq)
{
	return pic8259_irq_pending(0, irq);
}

int pic8259_1_irq_pending(int irq)
{
	return pic8259_irq_pending(1, irq);
}
