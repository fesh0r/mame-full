/* Video functions for the a2600 */

#include "vidhrdw/generic.h"


static struct osd_bitmap *stella_bitmap;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int a2600_vh_start(void)
{	if ((stella_bitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
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
    copybitmap(bitmap,stella_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}


