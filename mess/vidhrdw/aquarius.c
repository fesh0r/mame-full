/***************************************************************************

  aquarius.c

  Functions to emulate the video hardware of the aquarius.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/aquarius.h"

VIDEO_START( aquarius )
{
	return video_start_generic();
}

VIDEO_UPDATE( aquarius )
{
	int	sy, sx;
	int full_refresh = 1;

	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	for (sy = 0; sy < 24; sy++)
	{
		for (sx = 0; sx < 40; sx++)
		{
			if (dirtybuffer[sy * 40 + sx + 40] || dirtybuffer[sy * 40 + sx + 0x400])
			{
				drawgfx (bitmap, Machine->gfx[0], videoram[sy * 40 + sx + 40],
						videoram[sy * 40 + sx + 0x400], 0, 0, sx * 8,
						sy * 8, &Machine->visible_area, TRANSPARENCY_NONE, 0);
				dirtybuffer[sy * 40 + sx + 40] = 0;
				dirtybuffer[sy * 40 + sx + 0x400] = 0;
			}
		}
	}
}

