#include "includes/6551.h"

struct acia6551
{
	unsigned char transmit_data_register;
	unsigned char receive_data_register;
	unsigned char status_register;
	unsigned char command_register;
	unsigned char control_register;

	void	(*irq_callback)(int);

};

static struct acia6551 acia;

/*
A1 A0     Write                    Read
 0 0  Transmit Data Register  Receiver Data Register
 0 1  Programmed Reset        Status Register
 1 0              Command Register
 1 1              Control Register
*/


void acia_6551_reset(void)
{
	memset(&acia, 0, sizeof(struct acia6551));
}

void acia_6551_set_irq_callback(void (*callback)(int))
{
	acia.irq_callback = callback;
}

READ_HANDLER(acia_6551_r)
{
	logerror("6551 R %04x\n",offset & 0x03);

	switch (offset & 0x03)
	{
		case 0:
			return acia.receive_data_register;
		case 1:
			return acia.status_register;
		case 2:
			return acia.command_register;
		case 3:
			return acia.control_register;
		default:
			break;
	}

	return 0x0ff;
}

WRITE_HANDLER(acia_6551_w)
{
	logerror("6551 W %04x %02x\n",offset & 0x03, data);

	switch (offset & 0x03)
	{
		case 0:
		{
			acia.transmit_data_register = data;
		}
		break;

		case 1:
		{
			
		}
		break;

		case 2:
		{
			acia.command_register = data;
		}
		break;

		case 3:
		{
			acia.control_register = data;
		}
		break;

		default:
			break;
	}

}
