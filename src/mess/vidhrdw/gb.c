/***************************************************************************

  gb.c

  Video file to handle emulation of the Nintendo GameBoy.

  Original code                               Carsten Sorensen   1998
  Mess modifications, bug fixes and speedups  Hans de Goede      1998

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/machine/gb.h"

static UINT8 bg_zbuf[160];

INLINE void gb_update_sprites (void)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;
	UINT8 height, tilemask, line, *oam;
	int i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = CURLINE;
	line = CURLINE + 16;

	oam = &gb_ram[OAM + 39 * 4];
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data, *spal;
			UINT8 bit;
			int xindex;

//			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			spal = gb_bpal;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				data = *((UINT16 *) &gb_ram[VRAM + (oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2]);
			}
			else
			{
				data = *((UINT16 *) &gb_ram[VRAM + (oam[2] & tilemask) * 16 + (line - oam[0]) * 2]);
			}
#ifndef LSB_FIRST
			data = (data << 8) | (data >> 8);
#endif

			switch (oam[3] & 0xA0)
			{
			case 0xA0:
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, spal[colour]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
                    if (colour)
						plot_pixel(bitmap, xindex, yindex, spal[colour]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, spal[colour]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, spal[colour]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

struct layer_struct
{
	UINT8 enabled;
	UINT16 *bg_tiles;
	UINT8 *bg_map;
	UINT8 xindex;
	UINT8 xshift;
	UINT8 xend;
};

void gb_refresh_scanline (void)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = CURLINE;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		struct rectangle r = Machine->drv->visible_area;
		r.min_y = r.max_y = yindex;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && CURLINE >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + CURLINE) & 0xFF;

		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (CURLINE - WNDPOSY) & 0xFF;
		xpos = WNDPOSX - 7;

		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		if (xpos < 0)
			xpos = 0;
		layer[0].xend = xpos;
		layer[1].xend = 160 - xpos;
	}

	while (l < 2)
	{
		/* 
		 * BG display on
		 */
		UINT8 *map, xidx, bit, i;
		UINT16 *tiles, data;
		int xindex;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		tiles = layer[l].bg_tiles;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		data = (UINT16) (tiles[(map[xidx] ^ gb_tile_no_mod) * 8] << bit);
#ifndef LSB_FIRST
		data = (data << 8) | (data >> 8);
#endif
		xindex = 0;
		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
				plot_pixel(bitmap, xindex, yindex, gb_bpal[colour]);
				xindex++;
				*zbuf++ = colour;
				data <<= 1;
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			bit = 0;
			data = tiles[(map[xidx] ^ gb_tile_no_mod) * 8];
		}
		l++;
	}

	if (LCDCONT & 0x02)
		gb_update_sprites();
}

int gb_vh_start(void)
{
	if( generic_bitmapped_vh_start() )
		return 1;
	return 0;
}

void gb_vh_stop(void)
{
	generic_vh_stop();
}

void gb_vh_screen_refresh(struct osd_bitmap *bitmap, int full_refresh)
{

}

