/*************************************************************************

  i8255.c

  Functions to emulate a Intel 8255 PPI interface chip

  It doesn't emulate the different modes of operation. (it emulates
	mode 0 only)

  8255 is a general purpose input/output chip. It has 3 ports and
  a control port.

  Port C upper 4 bits and lower 4 bits can be controlled independantly.

  Writing to the control port sets the I/O status of ports A,B and C,
  and the mode of operation.

*************************************************************************/

#include "driver.h"
#include "i8255.h"

static I8255 ppi;
static I8255_INTERFACE *ppi_iface;

void	i8255_init(I8255_INTERFACE *iface)
{
	ppi.porta_data = 0;
	ppi.portb_data = 0;
	ppi.portc_data = 0;

	ppi_iface = iface;
}

void	i8255_porta_w(int data)
{
	ppi.porta_data = data;

	if (ppi_iface!=NULL)
	{
		if (ppi_iface->porta_w!=NULL)
		{
			ppi_iface->porta_w(data);
		}
	}	
}

void	i8255_portb_w(int data)
{
	ppi.portb_data = data;

	if (ppi_iface!=NULL)
	{
		if (ppi_iface->portb_w!=NULL)
		{
			ppi_iface->portb_w(data);
		}
	}
}

void	i8255_portc_w(int data)
{
	ppi.portc_data = data;

	if (ppi_iface!=NULL)
	{
		if (ppi_iface->portc_w!=NULL)
		{
			ppi_iface->portc_w(data);
		}
	}
}


int	i8255_porta_r()
{

	if (ppi_iface!=NULL)
	{
		if (ppi_iface->porta_r!=NULL)
		{
                        /*ppi.porta_data =*/ ppi_iface->porta_r();
		}
	}

	return ppi.porta_data;			
}
	
int	i8255_portb_r()
{

	if (ppi_iface!=NULL)
	{
		if (ppi_iface->portb_r!=NULL)
		{
                        /*ppi.portb_data =*/ ppi_iface->portb_r();
		}
	}

        return ppi.portb_data;           
}

int	i8255_portc_r()
{
	if (ppi_iface!=NULL)
	{
		if (ppi_iface->portc_r!=NULL)
		{
                        /*ppi.portc_data = */ppi_iface->portc_r();
		}
	}

	return ppi.portc_data;			
}

/* write to 8255 control port */
void	i8255_control_port_w(int data)
{
	/* get function */
	if (data & 0x080)
	{
		/* mode set */

		/* clear port data */
		ppi.porta_data = 0;
		ppi.portb_data = 0;
		ppi.portc_data = 0;
		ppi.control_port_data = data;
	}
	else
	{
		/* port c bit set/reset */

		int BitIndex = (data>>1) & 0x07;
		
		if (data & 1)
		{
			/* set bit */
			ppi.portc_data = (1<<BitIndex);
		}
		else
		{	
			/* reset bit */
			ppi.portc_data &= ~(1<<BitIndex);
		}

		if (ppi_iface!=NULL)
		{
			if (ppi_iface->portc_w!=NULL)
			{
				ppi_iface->portc_w(ppi.portc_data);
			}
		}

	}
}

void	i8255_set_porta(int Data)
{
	ppi.porta_data = Data;
}

void	i8255_set_portb(int Data)
{
	ppi.portb_data = Data;
}

void	i8255_set_portc(int Data)
{
	ppi.portc_data = Data;
}


/*void	I8255_Init(struct I8255_INTERFACE *newInterface)
{
	ppi_iface = newInterface;
}
*/
