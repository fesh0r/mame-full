/* Video functions for the a2600 */

#include "vidhrdw/generic.h"


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int a2600_vh_start(void)
{
    return 0;
}

void a2600_vh_stop(void)
{

}


int a2600_interrupt(void)
{
	return 0;
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/
/* This routine is called at the start of vblank to refresh the screen */
void a2600_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{

}


