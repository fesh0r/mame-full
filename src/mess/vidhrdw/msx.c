/***************************************************************************

  vidhrdw/msx.c

  Routines to control the MSX1 video hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h" 
#include "tms9928a.h"

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int msx_vh_start(void)
{
	return TMS9928A_start(0x4000);
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void msx_vh_stop(void)
{
	TMS9928A_stop();
	return;
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

/* This routine is called at the start of vblank to refresh the screen */
void msx_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	TMS9928A_refresh(bitmap, full_refresh);
	return;
}
