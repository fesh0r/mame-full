#include "includes/6551.h"

struct acia6551
{
	unsigned char transmit_data_register;
	unsigned char receive_data_register;
	unsigned char status_register;
	unsigned char command_register;
	unsigned char control_register;

	/* internal baud rate timer */
	void	*timer;
	/* callback for internal baud rate timer */
	void (*acia_updated_callback)(int id, unsigned long State);

	void	(*irq_callback)(int);

};


static void	acia_6551_timer_callback(int id)
{






}

static struct acia6551 acia;

/*
A1 A0     Write                    Read
 0 0  Transmit Data Register  Receiver Data Register
 0 1  Programmed Reset        Status Register
 1 0              Command Register
 1 1              Control Register
*/


void acia_6551_init(void)
{
	memset(&acia, 0, sizeof(struct acia6551));
	/* transmit data reg is empty */
	acia.status_register |= (1<<4);
}

void	acia_6551_stop(void)
{
	if (acia.timer!=NULL)
	{
		timer_remove(acia.timer);
        acia.timer = NULL;
	}
}

void acia_6551_set_irq_callback(void (*callback)(int))
{
	acia.irq_callback = callback;
}

static void acia_6551_recieve_char(char ch)
{
	acia.receive_data_register = ch;
	/* receive register is full */
	acia.status_register |= (1<<3);
}

static void acia_6551_refresh_ints(void)
{
	int interrupt_state;

	interrupt_state = 0;

	/* receive data register full? */ 
	if (acia.status_register & (1<<3))
	{
		/* receiver interrupt enable? */
		if ((acia.command_register & (1<<1))==0)
		{
			/* trigger a interrupt */
			interrupt_state = 1;
		}
	}

	/* set state of irq bit in status register */
	acia.status_register &= ~(1<<7);

	if (interrupt_state)
	{
		acia.status_register |=(1<<7);
	}

	if (acia.irq_callback)
        acia.irq_callback(interrupt_state);
}

READ_HANDLER(acia_6551_r)
{
	unsigned char data;

	data = 0x0ff;
	switch (offset & 0x03)
	{
		case 0:
		{
			/*  clear parity error, framing error and overrun error */
			/* when read of data reigster is done */
			acia.status_register &=~((1<<0) | (1<<1) | (1<<2));
			/* clear receive data register full flag */
			acia.status_register &=~(1<<3);
			/* return data */
			data = acia.receive_data_register;
		}
		break;

        case 1:
        {
            data = acia.status_register;
			/* clear interrupt */
			data &= ~(1<<7);
		}
        break;

        case 2:
			data = acia.command_register;
			break;

		case 3:
			data = acia.control_register;
			break;

		default:
			break;
	}

	logerror("6551 R %04x %02x\n",offset & 0x03,data);

	return data;
}

WRITE_HANDLER(acia_6551_w)
{
	logerror("6551 W %04x %02x\n",offset & 0x03, data);

	switch (offset & 0x03)
	{
		case 0:
		{
			/* clear transmit data register empty */
			acia.status_register &= ~(1<<4);
			/* store byte */
			acia.transmit_data_register = data;
		}
		break;

		case 1:
		{
			/* telstrat writes 0x07f! */		
		}
		break;

		case 2:
		{
			acia.command_register = data;

		}
		break;

		case 3:
		{
			unsigned char previous_control_register;

            previous_control_register = acia.control_register;

            if (((previous_control_register^data) & 0x07)!=0)
			{
				int rate;

                rate = data & 0x07;

				/* baud rate changed? */
				if (acia.timer!=NULL)
				{
					timer_remove(acia.timer);
                    acia.timer = NULL;
				}

				if (rate==0)
				{
					/* 16x external clock */
					logerror("6551: external clock not supported!\n");
				}
				else
				{

					

				}
			}

			acia.control_register = data;

		}
		break;

		default:
			break;
	}

}
