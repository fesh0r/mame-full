/***************************************************************************

  coleco.c

  Machine file to handle emulation of the Colecovision.

  TODO:
	- Extra controller support
***************************************************************************/

#include "driver.h"
#include "vidhrdw/tms9928a.h"
#include "includes/coleco.h"
#include "devices/cartslot.h"
#include "image.h"

static int JoyMode=0;

int coleco_cart_verify(const UINT8 *cartdata, size_t size)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is in Colecovision format */
	if ((cartdata[0] == 0xAA) && (cartdata[1] == 0x55))
		retval = IMAGE_VERIFY_PASS;
	if ((cartdata[0] == 0x55) && (cartdata[1] == 0xAA))
		retval = IMAGE_VERIFY_PASS;

	return retval;
}

DEVICE_LOAD( coleco_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0x8000, 0x0001, 0x8000, 0);
}


READ_HANDLER ( coleco_paddle_r )
{

	/* Player 1 */
	if ((offset & 0x02)==0)
	{
		/* Keypad and fire 1 */
		if (JoyMode==0)
		{
			int inport0,inport1,data;

			inport0 = input_port_0_r(0);
			inport1 = input_port_1_r(0);

			if		((inport0 & 0x01) == 0)		/* 0 */
				data = 0x0A;
			else if	((inport0 & 0x02) == 0)		/* 1 */
				data = 0x0D;
			else if ((inport0 & 0x04) == 0)		/* 2 */
				data = 0x07;
			else if ((inport0 & 0x08) == 0)		/* 3 */
				data = 0x0C;
			else if ((inport0 & 0x10) == 0)		/* 4 */
				data = 0x02;
			else if ((inport0 & 0x20) == 0)		/* 5 */
				data = 0x03;
			else if ((inport0 & 0x40) == 0)		/* 6 */
				data = 0x0E;
			else if ((inport0 & 0x80) == 0)		/* 7 */
				data = 0x05;
			else if ((inport1 & 0x01) == 0)		/* 8 */
				data = 0x01;
			else if ((inport1 & 0x02) == 0)		/* 9 */
				data = 0x0B;
			else if ((inport1 & 0x04) == 0)		/* # */
				data = 0x06;
			else if ((inport1 & 0x08) == 0)		/* . */
				data = 0x09;
			else
				data = 0x0F;

			return (inport1 & 0x70) | (data);

		}
		/* Joystick and fire 2*/
		else
			return input_port_2_r(0);
	}
	/* Player 2 */
	else
	{
		/* Keypad and fire 1 */
		if (JoyMode==0)
		{
			int inport3,inport4,data;

			inport3 = input_port_3_r(0);
			inport4 = input_port_4_r(0);

			if		((inport3 & 0x01) == 0)		/* 0 */
				data = 0x0A;
			else if	((inport3 & 0x02) == 0)		/* 1 */
				data = 0x0D;
			else if ((inport3 & 0x04) == 0)		/* 2 */
				data = 0x07;
			else if ((inport3 & 0x08) == 0)		/* 3 */
				data = 0x0C;
			else if ((inport3 & 0x10) == 0)		/* 4 */
				data = 0x02;
			else if ((inport3 & 0x20) == 0)		/* 5 */
				data = 0x03;
			else if ((inport3 & 0x40) == 0)		/* 6 */
				data = 0x0E;
			else if ((inport3 & 0x80) == 0)		/* 7 */
				data = 0x05;
			else if ((inport4 & 0x01) == 0)		/* 8 */
				data = 0x01;
			else if ((inport4 & 0x02) == 0)		/* 9 */
				data = 0x0B;
			else if ((inport4 & 0x04) == 0)		/* # */
				data = 0x06;
			else if ((inport4 & 0x08) == 0)		/* . */
				data = 0x09;
			else
				data = 0x0F;

			return (inport4 & 0x70) | (data);

		}
		/* Joystick and fire 2*/
		else
			return input_port_5_r(0);
	}

}


WRITE_HANDLER ( coleco_paddle_toggle_off )
{
	JoyMode=0;
    return;
}

WRITE_HANDLER ( coleco_paddle_toggle_on )
{
	JoyMode=1;
    return;
}

