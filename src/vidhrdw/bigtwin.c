#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *bigtwin_bgvideoram,*bigtwin_videoram1,*bigtwin_videoram2;
static struct osd_bitmap *bgbitmap;



int bigtwin_vh_start(void)
{
	if ((bgbitmap = bitmap_alloc(512,512)) == 0)
		return 1;

	return 0;
}

void bigtwin_vh_stop(void)
{
	bitmap_free(bgbitmap);
	bgbitmap = 0;
}



WRITE_HANDLER( bigtwin_paletteram_w )
{
	int r,g,b,val;


	COMBINE_WORD_MEM(&paletteram[offset],data);

	val = READ_WORD(&paletteram[offset]);
	r = (val >> 11) & 0x1e;
	g = (val >>  7) & 0x1e;
	b = (val >>  3) & 0x1e;

	r |= ((val & 0x08) >> 3);
	g |= ((val & 0x04) >> 2);
	b |= ((val & 0x02) >> 1);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset / 2,r,g,b);
}

WRITE_HANDLER( bigtwin_bgvideoram_w )
{
	int sx,sy,color;


	COMBINE_WORD_MEM(&bigtwin_bgvideoram[offset],data);

	sx = (offset/2) % 512;
	sy = (offset/2) / 512;

	color = READ_WORD(&bigtwin_bgvideoram[offset]) & 0xff;

	plot_pixel(bgbitmap,sx,sy,Machine->pens[256 + color]);
}

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = 8;offs < spriteram_size;offs += 8)
	{
		int sx,sy,code,color;

		sy = READ_WORD(&spriteram[offs+6-8]);	/* -8? what the... ??? */
		if (sy == 0x2000) return;	/* end of list marker */

		sx = (READ_WORD(&spriteram[offs+2]) & 0x01ff) - 16-6;
		sy = (256-32-8 - sy) & 0xff;
		code = READ_WORD(&spriteram[offs+4]) >> 4;
		color = (READ_WORD(&spriteram[offs+2]) & 0x1e00) >> 9;

		drawgfx(bitmap,Machine->gfx[2],
				code,
				color,
				0,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void bigtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	palette_used_colors[256] = PALETTE_COLOR_TRANSPARENT;	/* keep the background black */
	if (palette_recalc())
	{
		for (offs = 0;offs < 0x80000;offs += 2)
			bigtwin_bgvideoram_w(offs,READ_WORD(&bigtwin_bgvideoram[offs]));
	}

	copybitmap(bitmap,bgbitmap,0,0,-3,16,&Machine->visible_area,TRANSPARENCY_NONE,0);

	for (offs = 0;offs < 0x0800;offs += 4)
	{
		int sx,sy,code,color;


		sx = (offs/4) % 32;
		sy = (offs/4) / 32;

		code = READ_WORD(&bigtwin_videoram1[offs]);
		color = READ_WORD(&bigtwin_videoram1[offs+2]);

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				0,0,
				16*sx,16*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

	draw_sprites(bitmap);

	for (offs = 0;offs < 0x2000;offs += 4)
	{
		int sx,sy,code,color;


		sx = (offs/4) % 64;
		sy = (offs/4) / 64;

		code = READ_WORD(&bigtwin_videoram2[offs]);
		color = (READ_WORD(&bigtwin_videoram2[offs+2])) & 0x07;

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color + 8,
				0,0,
				8*sx+8,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
