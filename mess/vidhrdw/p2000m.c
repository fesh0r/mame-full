/**********************************************************************

	p2000m.c

	Functions to emulate video hardware of the p2000m

**********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/p2000t.h"

static	INT8	frame_count;

int p2000m_vh_start (void)
{
	frame_count = 0;
	if (generic_vh_start ())
		return (1);
	return (0);
}

void p2000m_vh_stop (void)
{
	generic_vh_stop ();
}

void p2000m_vh_callback (void)
{

	int	offs;

	if (frame_count++ > 49) frame_count = 0;
	if ((frame_count == 24) | !frame_count)
	{
		for (offs = 0; offs < 2047; offs++)
		{
			if (videoram[offs + 2048] & 0x40)
				dirtybuffer[offs] = 1;
		}
	}
}

void p2000m_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{
	int offs, sx, sy, code, loop;

	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	for (offs = 0; offs < 80 * 24; offs++)
	{
		if (dirtybuffer[offs] | dirtybuffer[offs + 2048])
		{
			sy = (offs / 80) * 10;
			sx = (offs % 80) * 6;
			if ((frame_count > 25) && (videoram[offs + 2048] & 0x40)) code = 32;
			else
			{
				code = videoram[offs];
				if ((videoram[offs + 2048] & 0x01) && (code & 0x20))
				{
					code += (code & 0x40) ? 64 : 96;
				} else {
					code &= 0x7f;
				}
				if (code < 32) code = 32;
			}

			drawgfx (bitmap, Machine->gfx[0], code,
				videoram[offs + 2048] & 0x08 ? 0 : 1, 0, 0, sx, sy,
				&Machine->visible_area, TRANSPARENCY_NONE, 0);
			if (videoram[offs] & 0x80)
			{
				for (loop = 0; loop < 6; loop++)
				{
					plot_pixel (bitmap, sx + loop, sy + 9, 1);
				}
			}
			dirtybuffer[offs] = 0;
		}
	}
}
