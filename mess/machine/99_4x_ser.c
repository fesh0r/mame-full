/*
	TI99/4(a) RS232 card emulation

	This card features two RS232C ports (which share one single connector) and
	one centronics-like fully bi-directional parallel port.  Each TI99 system
	can accomodate two such RS232 cards (provided a resistor is moved in one of
	the cards to change its CRU base address from >1300 to >1500), for a total
	of four RS232 ports and two parallel ports.

	The card is fairly simple: two 9902 chips handle one serial port each, two
	unidirectional TTL buffers handle the PIO data bus, there is extra TTL for
	address decoding and CRU interface, and a DSR ROM.

	References:
	<http://www.nouspikel.com/ti99/rs232c.htm>: great description of the
		hardware and software aspects.
	<ftp://ftp.whtech.com//datasheets/TMS9902A.max>: tms9902a datasheet
	<ftp://ftp.whtech.com//datasheets/Hardware manuals/ti rs232 card schematic.max>

	Raphael Nabet, 2003.
*/


#include "ti99_4x.h"
#include "tms9902.h"
#include "99_4x_ser.h"

/* prototypes */
static void int_callback(int which, int INT);
static int rs232_cru_r(int offset);
static void rs232_cru_w(int offset, int data);
static READ_HANDLER(rs232_mem_r);
static WRITE_HANDLER(rs232_mem_w);

/* pointer to the rs232 ROM data */
static UINT8 *rs232_DSR;

static int pio_direction;
static int flag0;	// spare
static int cts1;	// a.k.a. flag1
static int cts2;	// a.k.a. flag2
static int led;		// a.k.a. flag3

static const ti99_exp_card_handlers_t rs232_handlers =
{
	rs232_cru_r,
	rs232_cru_w,
	rs232_mem_r,
	rs232_mem_w
};

static const tms9902reset_param tms9902_params =
{
	3000000.,				/* clock rate (3MHz) */
	int_callback			/* called when interrupt pin state changes */
	//rts_callback,			/* called when Request To Send pin state changes */
	//brk_callback,			/* called when BReaK state changes */
	//xmit_callback			/* called when a character is transmitted */
};

/*
	Reset rs232 card, set up handlers
*/
void ti99_rs232_init(void)
{
	rs232_DSR = memory_region(region_dsr) + offset_rs232_dsr;

	ti99_exp_set_card_handlers(0x1300, & rs232_handlers);

	pio_direction = 0;

	tms9902_init(0, &tms9902_params);
	tms9902_init(1, &tms9902_params);
}

/*
	rs232 card clean-up
*/
void ti99_rs232_cleanup(void)
{
	tms9902_cleanup(0);
	tms9902_cleanup(1);
}

/*
	rs232 interrupt handler
*/
static void int_callback(int which, int INT)
{
	switch (which)
	{
	case 0:
		ti99_exp_set_ila_bit(inta_rs232_1_bit, INT);
		break;

	case 1:
		ti99_exp_set_ila_bit(inta_rs232_2_bit, INT);
		break;

	case 2:
		ti99_exp_set_ila_bit(inta_rs232_3_bit, INT);
		break;

	case 3:
		ti99_exp_set_ila_bit(inta_rs232_4_bit, INT);
		break;
	}
}

/*
	Read rs232 card CRU interface

	bit 0: always 0
	...
*/
static int rs232_cru_r(int offset)
{
	int reply;

	switch (offset >> 5)
	{
	case 0:
		/* custom buffer */
		switch (offset)
		{
		case 0:
			reply = 0x00;
			if (pio_direction)
				reply |= 0x02;
			if (flag0)
				reply |= 0x10;
			if (cts1)
				reply |= 0x20;
			if (cts2)
				reply |= 0x40;
			if (led)
				reply |= 0x80;
			break;

		default:
			reply = 0;
			break;
		}
		break;

	case 1:
		/* first 9902 */
		reply = tms9902_CRU_read(0, offset);
		break;

	case 2:
		/* second 9902 */
		reply = tms9902_CRU_read(1, offset);
		break;

	default:
		/* NOP */
		reply = 0;	/* ??? */
		break;
	}

	return reply;
}

/*
	Write rs232 card CRU interface
*/
static void rs232_cru_w(int offset, int data)
{
	switch (offset >> 5)
	{
	case 0:
		/* custom buffer */
		switch (offset)
		{
		case 0:
			/* WRITE to rs232 card enable bit (bit 0) */
			/* handled in ti99_expansion_CRU_w() */
			break;

		case 1:
			pio_direction = data;
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			flag0 = data;
			break;

		case 5:
			cts1 = data;
			break;

		case 6:
			cts2 = data;
			break;

		case 7:
			led = data;
			break;
		}
		break;

	case 1:
		/* first 9902 */
		tms9902_CRU_write(0, offset, data);
		break;

	case 2:
		/* second 9902 */
		tms9902_CRU_write(1, offset, data);
		break;

	default:
		/* NOP */
		break;
	}
}


/*
	read a byte in rs232 DSR space
*/
static READ_HANDLER(rs232_mem_r)
{
	int reply;

	if (offset < 0x1000)
	{
		/* DSR ROM */
		reply = rs232_DSR[offset];
	}
	else
	{
		/* PIO */
		/*...*/
		reply = 0;
	}

	return reply;
}

/*
	write a byte in rs232 DSR space
*/
static WRITE_HANDLER(rs232_mem_w)
{
	if (offset >= 0x1000)
	{
		/* PIO */
		/*...*/
	}
	else
	{
		/* DSR ROM: do nothing */
	}
}
