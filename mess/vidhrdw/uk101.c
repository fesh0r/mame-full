/***************************************************************************

  uk101.c

  Functions to emulate the video hardware of the UK101.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/uk101.h"

VIDEO_UPDATE( uk101 )
{
	int sx, sy;

	for (sy = 0; sy < 25; sy++)
	{
		for (sx = 0; sx < 32; sx++)
		{
			drawgfx (bitmap, Machine->gfx[0], videoram[0x84 + sy * 32 + sx], 1,
				0, 0, sx * 8, sy * 16, &Machine->visible_area,
				TRANSPARENCY_NONE, 0);
		}
	}
}

VIDEO_UPDATE( superbrd )
{
	int sx, sy;

	for (sy = 0; sy < 32; sy++)
	{
		for (sx = 0; sx < 32; sx++)
		{
			drawgfx (bitmap, Machine->gfx[0], videoram[sy * 32 + sx], 1,
				0, 0, sx * 8, sy * 8, &Machine->visible_area,
				TRANSPARENCY_NONE, 0);
		}
	}
}
