/*
 *
 *	  Sound emulation hooks for Genesis
 *
 *   ***********************************
 *   ***    C h a n g e   L i s t    ***
 *   ***********************************
 *   Date       Name   Description
 *   ----       ----   -----------
 *   00-Jan-00  GSL    Started
 *	 03-Aug-98	GSL	   Tidied.. at last!
 *
 */
//#include "osd_cpu.h"
//#include "sndintrf.h"
#include "driver.h"
#include "sound/2612intf.h"
#include "machine/genesis.h"
//#include "sound/psgintf.h"

void genesis_s_interrupt(void)
{
	// logerror("Z80 interrupt ");
	cpu_set_irq_line(1, 0xff, HOLD_LINE);
}

WRITE16_HANDLER ( YM2612_68000_w )
{
	switch (offset)
	{
		case 0:
			if (ACCESSING_LSB) YM2612_data_port_0_A_w(offset, data 	   & 0xff);
			if (ACCESSING_MSB) YM2612_control_port_0_A_w(offset, (data >> 8) & 0xff);
			break;
		case 1:
			if (ACCESSING_LSB) YM2612_data_port_0_B_w(offset, data 		& 0xff);
			if (ACCESSING_MSB) YM2612_control_port_0_B_w(offset, (data >> 8) & 0xff);
	}
}

READ16_HANDLER ( YM2612_68000_r )
{
	switch (offset)
	{
		case 0:
			return ((YM2612_status_port_0_A_r(offset) << 8) + YM2612_status_port_0_B_r(offset) );
			break;
		case 1:
			return (YM2612_read_port_0_r(offset) << 8);
			break;
	}
	return 0;
}
