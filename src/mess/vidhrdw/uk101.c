/***************************************************************************

  uk101.c

  Functions to emulate the video hardware of the Jupiter Ace.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern	struct	GfxLayout	uk101_charlayout;


int	uk101_vh_start (void)
{
	if (generic_vh_start ())
		return (1);

	return (0);
}

void	uk101_vh_stop (void)

{

generic_vh_stop ();

}

/* || */

void	uk101_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int sx, sy;

	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	for (sx = 0; sx < 64; sx++) {
		for (sy = 0; sy < 16; sy++) {
			if (dirtybuffer[sy * 64 + sx]) {
				drawgfx (bitmap, Machine->gfx[0], videoram[sy * 64 + sx], 1,
				0, 0, sx * 8, sy * 16, &Machine->drv->visible_area,
				TRANSPARENCY_NONE, 0);
				dirtybuffer[sy * 64 + sx] = 0;
			}
		}
	}
}

void	superbrd_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int sx, sy;

	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	for (sx = 0; sx < 32; sx++) {
		for (sy = 0; sy < 16; sy++) {
			if (dirtybuffer[sy * 32 + sx]) {
				drawgfx (bitmap, Machine->gfx[0], videoram[sy * 32 + sx], 1,
				0, 0, sx * 8, sy * 16, &Machine->drv->visible_area,
				TRANSPARENCY_NONE, 0);
				dirtybuffer[sy * 32 + sx] = 0;
			}
		}
	}
}
