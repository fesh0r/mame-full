/***************************************************************************

  nascom1.c

  Functions to emulate the video hardware of the nascom1.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from mess/systems/nascom1.c */
extern struct GfxLayout nascom1_charlayout;

int	nascom1_vh_start (void)
{

	if( generic_vh_start() )
		return 1;
    return 0;
}

void nascom1_vh_stop (void)
{
	generic_vh_stop();
}

void nascom1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{

int	sy, sx;

if (full_refresh) memset (dirtybuffer, 1, videoram_size);

for (sx = 0; sx < 48; sx++) {
  if (dirtybuffer[sx + 0x03ca]) {
	drawgfx (bitmap, Machine->gfx[0], videoram[0x03ca + sx],
				  1, 0, 0, sx * 8, 0, &Machine->drv->visible_area,
				  TRANSPARENCY_NONE, 0);
    dirtybuffer[0x03ca + sx] = 0;
  }
}

for (sy = 0; sy < 15; sy++) {
  for (sx = 0; sx < 48; sx++) {
    if (dirtybuffer[0x000a + (sy * 64) + sx]) {
	  drawgfx (bitmap, Machine->gfx[0], videoram[0x000a + (sy * 64) + sx],
				  1, 0, 0, sx * 8, (sy + 1) * 10, &Machine->drv->visible_area,
				  TRANSPARENCY_NONE, 0);
	  dirtybuffer[0x000a + (sy * 64) + sx] = 0;
	}
  }
}


}
