
/********************************
DAVE SOUND CHIP FOUND IN ENTERPRISE
*********************************/
#include "driver.h"
#include "mess/sndhrdw/dave.h"

static DAVE dave;
static DAVE_INTERFACE *dave_iface;

static unsigned char Dave_IntRegRead(void);
static void Dave_IntRegWrite(unsigned char);
static void     Dave_SetInterruptWanted(void);

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

WRITE_HANDLER (	Dave_reg_w )
{
	dave.Regs[offset & 0x01f] = data;

	if (offset == 0x014)
	{
		Dave_IntRegWrite(data);
	}

	if (dave_iface!=NULL)
	{
		dave_iface->reg_w(offset, data);
	}
}


WRITE_HANDLER ( Dave_setreg )
{
	dave.Regs[offset & 0x01f] = data;
}

READ_HANDLER (	Dave_reg_r )
{
	if (dave_iface!=NULL)
	{
		dave_iface->reg_r(offset);
	}

	if (offset==0x14)
	{

               dave.Regs[offset & 0x01f] = Dave_IntRegRead();
	}

	return dave.Regs[offset & 0x01f];
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

        Dave_SetInterruptWanted();
}

static void	Dave_SetInterruptWanted(void)
{
        int int_wanted;

        int_wanted = (((dave.int_enable<<1) & dave.int_latch)!=0);

        if (dave_iface->int_callback)
        {
            dave_iface->int_callback(int_wanted);
        }
}

void    Dave_UpdateInterruptLatches(int input_mask)
{
        if (((dave.previous_int_input^dave.int_input)&(input_mask))!=0)
        {
                /* it changed */

                if ((dave.int_input & (input_mask))==0)
                {
                        /* negative edge */
                        dave.int_latch |= (input_mask<<1);
                }
        }
}

void    Dave_Interrupt(void)
{
      dave.int_input ^=0x055;

      Dave_UpdateInterruptLatches(0x01);
      Dave_UpdateInterruptLatches(0x04);
      Dave_UpdateInterruptLatches(0x10);
      Dave_UpdateInterruptLatches(0x40);

      Dave_SetInterruptWanted();

      dave.previous_int_input = dave.int_input;

}
