#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *lastday_fgvideoram,*lastday_bgscroll,*lastday_fgscroll;

WRITE_HANDLER( lastday_ctrl_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 3 is used but unknown */

	/* bit 4 is used but unknown */

	/* bit 6 is flip screen */
	flip_screen_w(0,data & 0x40);
}



void lastday_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scroll;


	palette_recalc();

	/* BG */
	scroll = lastday_bgscroll[0] + (lastday_bgscroll[1] << 8);

	for (offs = 0;offs < 0x80;offs++)
	{
		int sx,sy,code,attr,flipx,flipy;

		attr = memory_region(REGION_GFX5)[offs+0x10000+((scroll&~0x1f)>>2)],
		code = memory_region(REGION_GFX5)[offs+((scroll&~0x1f)>>2)] | ((attr & 0x01) << 8)
				| ((attr & 0x80) << 2),
		sx = offs / 8;
		sy = offs % 8;
		flipx = attr & 0x02;
		flipy = attr & 0x04;
		if (flip_screen)
		{
			sx = 15 - sx;
			sy = 7 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				(attr & 0x78) >> 3,
				flipx,flipy,
				32*sx-(scroll&0x1f),32*sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	/* FG */
	scroll = lastday_fgscroll[0] + (lastday_fgscroll[1] << 8);

	for (offs = 0;offs < 0x80;offs++)
	{
		int sx,sy,code,attr,flipx,flipy;

		attr = memory_region(REGION_GFX6)[offs+0x10000+((scroll&~0x1f)>>2)],
		code = memory_region(REGION_GFX6)[offs+((scroll&~0x1f)>>2)] | ((attr & 0x01) << 8);
		sx = offs / 8;
		sy = offs % 8;
		flipx = attr & 0x02;
		flipy = attr & 0x04;
		if (flip_screen)
		{
			sx = 15 - sx;
			sy = 7 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				code,
				(attr & 0x78) >> 3,
				flipx,flipy,
				32*sx-(scroll&0x1f),32*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}

	/* sprites */
	for (offs = spriteram_size-32;offs >= 0;offs -= 32)
	{
		int sx,sy,code;

		sx = spriteram[offs+3] + ((spriteram[offs+1] & 0x10) << 4);
		sy = ((spriteram[offs+2] + 8) & 0xff) - 8;
		code = spriteram[offs] + ((spriteram[offs+1] & 0xe0) << 3);
		if (flip_screen)
		{
			sx = 498 - sx;
			sy = 240 - sy;
		}

		drawgfx(bitmap,Machine->gfx[3],
				code,
				spriteram[offs+1] & 0x0f,
				flip_screen,flip_screen,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}

	/* TX */
	for (offs = 0;offs < 0x800;offs++)
	{
		int sx,sy,attr;

		attr = lastday_fgvideoram[offs+0x800];
		sx = offs / 32;
		sy = offs % 32;
		if (flip_screen)
		{
			sx = 63 - sx;
			sy = 31 - sy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				lastday_fgvideoram[offs] | ((attr & 0x07) << 8),
				(attr & 0xf0) >> 4,
				flip_screen,flip_screen,
				8*sx,8*(sy-1),
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}
}
