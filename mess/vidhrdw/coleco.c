/***************************************************************************

  vidhrdw/coleco.c

  Routines to control the Colecovision video hardware

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms9928a.h"
#include "includes/coleco.h"

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static void coleco_vdp_interrupt (int state) 
{
	static int last_state = 0;

	if (state && !last_state) cpu_set_nmi_line(0, PULSE_LINE);

	last_state = state;
}

int coleco_vh_start(void)
{
    if (TMS9928A_start(TMS99x8A, 0x4000)) return 1;
	TMS9928A_int_callback(coleco_vdp_interrupt);

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void coleco_vh_stop(void)
{
	TMS9928A_stop();
	return;
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

/* This routine is called at the start of vblank to refresh the screen */
void coleco_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	TMS9928A_refresh(bitmap, full_refresh);
	return;
}
