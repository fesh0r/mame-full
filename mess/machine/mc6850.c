/**********************************************************************

	Motorola 6850 ACIA interface and emulation

	This function is a simple emulation of up to 4 MC6850
	Asynchronous Communications Interface Adapter.

	Todo:
		Handle interrupts.
		Handle state changes.
**********************************************************************/

#include "driver.h"
#include "mc6850.h"

struct acia6850
{
	const struct acia6850_interface *intf;
};

static struct acia6850 acia[ACIA_6850_MAX];

void acia6850_unconfig (void)
{
	memset (&acia, 0, sizeof (acia));
}

void acia6850_config (int which, const struct acia6850_interface *intf)
{
	if (which >= ACIA_6850_MAX) return;
	acia[which].intf = intf;
}

void acia6850_reset (void)
{
}

int acia6850_read (int which, int offset)
{
	struct acia6850 *currptr = acia + which;
	int	val = 0;

	switch (offset)
	{
		case ACIA_6850_CTRL:
			if ((*(*currptr).intf).in_status_func)
							val = (*(*currptr).intf).in_status_func(0);
			break;
		case ACIA_6850_DATA:
			if ((*(*currptr).intf).in_recv_func)
							val = (*(*currptr).intf).in_recv_func(0);
			break;
	}
	return (val);
}

void acia6850_write (int which, int offset, int data)
{
	struct acia6850 *currptr = acia + which;

	switch (offset)
	{
		case ACIA_6850_CTRL:
			if ((*(*currptr).intf).out_status_func)
							(*(*currptr).intf).out_status_func(0, data);
			break;
		case ACIA_6850_DATA:
			if ((*(*currptr).intf).out_tran_func)
							(*(*currptr).intf).out_tran_func(0, data);
			break;
	}
}

 READ8_HANDLER( acia6850_0_r ) { return acia6850_read (0, offset); }
 READ8_HANDLER( acia6850_1_r ) { return acia6850_read (1, offset); }
 READ8_HANDLER( acia6850_2_r ) { return acia6850_read (2, offset); }
 READ8_HANDLER( acia6850_3_r ) { return acia6850_read (3, offset); }

WRITE8_HANDLER( acia6850_0_w ) { acia6850_write (0, offset, data); }
WRITE8_HANDLER( acia6850_1_w ) { acia6850_write (1, offset, data); }
WRITE8_HANDLER( acia6850_2_w ) { acia6850_write (2, offset, data); }
WRITE8_HANDLER( acia6850_3_w ) { acia6850_write (3, offset, data); }
