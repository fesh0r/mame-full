
/********************************
DAVE SOUND CHIP FOUND IN ENTERPRISE
*********************************/
#include "driver.h"
#include "mess/sndhrdw/dave.h"

static DAVE dave;
static DAVE_INTERFACE *dave_iface;

static unsigned char Dave_IntRegRead(void);
static void Dave_IntRegWrite(unsigned char);

int Dave_sh_start(void)
{
	int i;

	for (i=0; i<32; i++)
	{
		dave.Regs[i] = 0;
	}

	return 0;
}

void	Dave_sh_stop(void)
{
}

void	Dave_sh_update(void)
{
}

void	Dave_reg_w(int RegIndex, int Data)
{
	dave.Regs[RegIndex & 0x01f] = Data;

	if (RegIndex == 0x014)
	{
		Dave_IntRegWrite(Data);
	}

	if (dave_iface!=NULL)
	{
		dave_iface->reg_w(RegIndex,Data);
	}
}


void Dave_setreg(int RegIndex, int Data)
{
	dave.Regs[RegIndex & 0x01f] = Data;
}

int	Dave_reg_r(int RegIndex)
{
	if (dave_iface!=NULL)
	{
		dave_iface->reg_r(RegIndex);
	}

	if (RegIndex==0x14)
	{
              dave.int_input ^=0x055;

               dave.Regs[RegIndex & 0x01f] = Dave_IntRegRead();
	}

	return dave.Regs[RegIndex & 0x01f];
}

int	Dave_getreg(int RegIndex)
{
	return dave.Regs[RegIndex & 0x01f];
}

void	Dave_SetIFace(struct DAVE_INTERFACE *newInterface)
{
	dave_iface = newInterface;
}

unsigned char Dave_IntRegRead(void)
{
	return (dave.int_input & 0x055) | (dave.int_latch & 0x0aa);
}

/*
Reg 4 READ:

b7 = 1: INT2 latch set
b6 = INT2 input pin
b5 = 1: INT1 latch set
b4 = INT1 input pin
b3 = 1: 1Hz latch set
b2 = 1hz input pin
b1 = 1: 1khz/50hz/TG latch set
b0 = 1khz/50hz/TG input

Reg 4 WRITE:

b7 = 1: Reset INT2 latch
b6 = 1: Enable INT2
b5 = 1: Reset INT1 latch
b4 = 1: Enable INT1
b3 = 1: Reset 1hz interrupt latch
b2 = 1: Enable 1hz interrupt
b1 = 1: Reset 1khz/50hz/TG latch
b0 = 1: Enable 1khz/50Hz/TG latch
*/

static void	Dave_IntRegWrite(unsigned char Data)
{
        /* reset latch */
        dave.int_latch &= ~(Data & (0x080|0x020|0x08|0x02));
        /* int enables */
	dave.int_enable = Data & 0x055;
}




static void	Dave_SetInterruptWanted(void)
{
	if (dave.int_latch & 0x0aa)
	{
		dave.int_wanted = 1;
	}
}

/* change int state*/

/* interrupt inputs are negative edge triggered.
If new state is 0, then previous state is 1, and a negative edge
has occured. */
void	Dave_SetInt(int IntID)
{
	switch (IntID)
	{
                case DAVE_INT_SELECTABLE:
                {
                        /* change state */
                        dave.int_input ^=0x01;

                        if ((dave.int_input & 0x01)==0)
                        {
                                dave.int_latch |= 0x02;
                        }
                }
                break;

		case DAVE_INT_1HZ:
		{
                        /* change state */
                        dave.int_input^=0x04;

                        if ((dave.int_input & 0x04)==0)
                        {
                          /* negative edge - set int latch */
                          dave.int_latch |= 0x08;
                        }
		}
		break;

		case DAVE_INT_INT1:
		{
                        /* change state */
                        dave.int_input^=0x010;

                        /* negative edge? */
                        if ((dave.int_input & 0x010)==0)
                        {
                                /* set latch */
                                dave.int_latch |= 0x020;
                        }
		}
		break;

		case DAVE_INT_INT2:
		{
                        /* change state */
                        dave.int_input^=0x040;

                        /* negative edge? */
                        if ((dave.int_input & 0x040)==0)
			{
                                /* set latch */
				dave.int_latch |= 0x80;
			}

			dave.int_input^=0x040;
		}
		break;

	}

	Dave_SetInterruptWanted();
}

int	Dave_Interrupt(void)
{
	int IntWanted = dave.int_wanted;

	dave.int_wanted = 0;

	return IntWanted;
}

void	Dave_UpdateTimers(void)
{
	dave.fiftyhertz++;

	if (dave.fiftyhertz == (1000/50))
	{
		dave.fiftyhertz = 0;

		dave.onehz++;

		if (dave.onehz == 50)
		{
			dave.onehz = 0;

			Dave_SetInt(DAVE_INT_1HZ);
		}
	}
}
