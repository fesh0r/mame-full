/***************************************************************************

	Sega 16-bit common hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/res_net.h"



/*************************************
 *
 *	Globals
 *
 *************************************/



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT16 multiply_regs[2][4];
static UINT16 divide_regs[2][4];
static UINT16 compare_timer_regs[2][16];



/*************************************
 *
 *	Multiply chip
 *
 *************************************/

static data16_t multiply_r(int which, offs_t offset, data16_t mem_mask)
{
	offset &= 3;
	switch (offset)
	{
		case 0:	return multiply_regs[which][0];
		case 1:	return multiply_regs[which][1];
		case 2:	return ((INT16)multiply_regs[which][0] * (INT16)multiply_regs[which][1]) >> 16;
		case 3:	return ((INT16)multiply_regs[which][0] * (INT16)multiply_regs[which][1]) & 0xffff;
	}
	return 0xffff;
}


static void multiply_w(int which, offs_t offset, data16_t data, data16_t mem_mask)
{
	offset &= 3;
	switch (offset)
	{
		case 0:	COMBINE_DATA(&multiply_regs[which][0]);	break;
		case 1:	COMBINE_DATA(&multiply_regs[which][1]);	break;
		case 2:	COMBINE_DATA(&multiply_regs[which][0]);	break;
		case 3:	COMBINE_DATA(&multiply_regs[which][1]);	break;
	}
}


READ16_HANDLER( segaic16_multiply_0_r )  { return multiply_r(0, offset, mem_mask); }
READ16_HANDLER( segaic16_multiply_1_r )  { return multiply_r(1, offset, mem_mask); }
WRITE16_HANDLER( segaic16_multiply_0_w ) { multiply_w(0, offset, data, mem_mask); }
WRITE16_HANDLER( segaic16_multiply_1_w ) { multiply_w(1, offset, data, mem_mask); }



/*************************************
 *
 *	Divide chip
 *
 *************************************/

static data16_t divide_r(int which, offs_t offset, data16_t mem_mask)
{
	offset &= 3;
	switch (offset)
	{
		case 0:	return divide_regs[which][0];
		case 1:	return divide_regs[which][1];
		case 2:	return ((INT16)divide_regs[which][0] * (INT16)divide_regs[which][1]) >> 16;
		case 3:	return ((INT16)divide_regs[which][0] * (INT16)divide_regs[which][1]) & 0xffff;
	}
	return 0xffff;
}


static void divide_w(int which, offs_t offset, data16_t data, data16_t mem_mask)
{
	offset &= 3;
	switch (offset)
	{
		case 0:	COMBINE_DATA(&divide_regs[which][0]);	break;
		case 1:	COMBINE_DATA(&divide_regs[which][1]);	break;
		case 2:	COMBINE_DATA(&divide_regs[which][0]);	break;
		case 3:	COMBINE_DATA(&divide_regs[which][1]);	break;
	}
}


READ16_HANDLER( segaic16_divide_0_r )  { return divide_r(0, offset, mem_mask); }
READ16_HANDLER( segaic16_divide_1_r )  { return divide_r(1, offset, mem_mask); }
WRITE16_HANDLER( segaic16_divide_0_w ) { divide_w(0, offset, data, mem_mask); }
WRITE16_HANDLER( segaic16_divide_1_w ) { divide_w(1, offset, data, mem_mask); }



/*************************************
 *
 *	Compare/timer chip
 *
 *************************************/

static void update_compare(int which, int update_history)
{
	int bound1 = (INT16)compare_timer_regs[which][0];
	int bound2 = (INT16)compare_timer_regs[which][1];
	int value = (INT16)compare_timer_regs[which][2];
	int min = (bound1 < bound2) ? bound1 : bound2;
	int max = (bound1 > bound2) ? bound1 : bound2;

	if (value < min)
	{
		compare_timer_regs[which][7] = min;
		compare_timer_regs[which][3] = 0x8000;
	}
	else if (value > max)
	{
		compare_timer_regs[which][7] = max;
		compare_timer_regs[which][3] = 0x4000;
	}
	else
	{
		compare_timer_regs[which][7] = value;
		compare_timer_regs[which][3] = 0x0000;
	}

	if (update_history)
		compare_timer_regs[which][4] = ((compare_timer_regs[which][4] << 1) & 3) | (compare_timer_regs[which][3] == 0);
}


static void update_timer(int which)
{
}


static void timer_interrupt_ack(int which)
{
}


static data16_t compare_timer_r(int which, offs_t offset, data16_t mem_mask)
{
	offset &= 0xf;
	logerror("%06X:compare%d_r(%X) = %04X\n", activecpu_get_pc(), which, offset, compare_timer_regs[which][offset]);
	switch (offset)
	{
		case 0x0:	return compare_timer_regs[which][0];
		case 0x1:	return compare_timer_regs[which][1];
		case 0x2:	return compare_timer_regs[which][2];
		case 0x3:	return compare_timer_regs[which][3];
		case 0x4:	return compare_timer_regs[which][4];
		case 0x5:	return compare_timer_regs[which][1];
		case 0x6:	return compare_timer_regs[which][2];
		case 0x7:	return compare_timer_regs[which][7];
		case 0x9:
		case 0xd:	timer_interrupt_ack(which); break;
	}
	return 0xffff;
}


static void compare_timer_w(int which, offs_t offset, data16_t data, data16_t mem_mask)
{
	offset &= 0xf;
	logerror("%06X:compare%d_w(%X) = %04X\n", activecpu_get_pc(), which, offset, data);
	switch (offset)
	{
		case 0x0:	COMBINE_DATA(&compare_timer_regs[which][0]); update_compare(which, 0); break;
		case 0x1:	COMBINE_DATA(&compare_timer_regs[which][1]); update_compare(which, 0); break;
		case 0x2:	COMBINE_DATA(&compare_timer_regs[which][2]); update_compare(which, 1); break;
		case 0x4:	compare_timer_regs[which][4] = 0; break;
		case 0x6:	COMBINE_DATA(&compare_timer_regs[which][2]); update_compare(which, 0); break;
		case 0x8:	
		case 0xc:	COMBINE_DATA(&compare_timer_regs[which][8]); update_timer(which); break;
		case 0x9:
		case 0xd:	timer_interrupt_ack(which); break;
		case 0xa:	
		case 0xe:	COMBINE_DATA(&compare_timer_regs[which][10]); update_timer(which); break;
		case 0xb:	
		case 0xf:	COMBINE_DATA(&compare_timer_regs[which][11]); /* write sound command */ break;
	}
}


READ16_HANDLER( segaic16_compare_timer_0_r )  { return compare_timer_r(0, offset, mem_mask); }
READ16_HANDLER( segaic16_compare_timer_1_r )  { return compare_timer_r(1, offset, mem_mask); }
WRITE16_HANDLER( segaic16_compare_timer_0_w ) { compare_timer_w(0, offset, data, mem_mask); }
WRITE16_HANDLER( segaic16_compare_timer_1_w ) { compare_timer_w(1, offset, data, mem_mask); }
