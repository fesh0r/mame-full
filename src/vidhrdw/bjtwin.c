#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *bjtwin_txvideoram;
size_t bjtwin_txvideoram_size;

static int flipscreen = 0;
static int bgbank;


int bjtwin_vh_start(void)
{
	dirtybuffer = malloc(bjtwin_txvideoram_size/2);
	tmpbitmap = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if (!dirtybuffer || !tmpbitmap)
	{
		if (tmpbitmap) bitmap_free(tmpbitmap);
		if (dirtybuffer) free(dirtybuffer);
		return 1;
	}

	return 0;
}

void bjtwin_vh_stop(void)
{
	bitmap_free(tmpbitmap);
	free(dirtybuffer);

	dirtybuffer = 0;
	tmpbitmap = 0;
}


READ_HANDLER( bjtwin_txvideoram_r )
{
	return READ_WORD(&bjtwin_txvideoram[offset]);
}

WRITE_HANDLER( bjtwin_txvideoram_w )
{
	int oldword = READ_WORD(&bjtwin_txvideoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&bjtwin_txvideoram[offset],newword);
		dirtybuffer[offset/2] = 1;
	}
}


WRITE_HANDLER( bjtwin_paletteram_w )
{
	int r,g,b;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	WRITE_WORD(&paletteram[offset],newword);

	r = ((newword >> 11) & 0x1e) | ((newword >> 3) & 0x01);
	g = ((newword >>  7) & 0x1e) | ((newword >> 2) & 0x01);
	b = ((newword >>  3) & 0x1e) | ((newword >> 1) & 0x01);

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset / 2,r,g,b);
}


WRITE_HANDLER( bjtwin_flipscreen_w )
{
	if ((data & 1) != flipscreen)
	{
		flipscreen = data & 1;
		memset(dirtybuffer, 1, bjtwin_txvideoram_size/2);
	}
}

WRITE_HANDLER( bjtwin_videocontrol_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		if (bgbank != (data & 0xff))
		{
			bgbank = data & 0xff;
			memset(dirtybuffer, 1, bjtwin_txvideoram_size/2);
		}
	}
}


static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		if (READ_WORD(&spriteram[offs]) != 0)
		{
			int sx = (READ_WORD(&spriteram[offs+8]) & 0x1ff) + 64;
			int sy = (READ_WORD(&spriteram[offs+12]) & 0x1ff);
			int tilecode = READ_WORD(&spriteram[offs+6]);
			int xx = (READ_WORD(&spriteram[offs+2]) & 0x0f) + 1;
			int yy = (READ_WORD(&spriteram[offs+2]) >> 4) + 1;
			int width = xx;
			int delta = 16;
			int startx = sx;

			if (flipscreen)
			{
				sx = 367 - sx;
				sy = 239 - sy;
				delta = -16;
				startx = sx;
			}

			do
			{
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							tilecode,
							READ_WORD(&spriteram[offs+14]),
							flipscreen, flipscreen,
							((sx + 16) & 0x1ff) - 16,sy & 0x1ff,
							&Machine->visible_area,TRANSPARENCY_PEN,15);

					tilecode++;
					sx += delta;
				} while (--xx);

				sy += delta;
				sx = startx;
				xx = width;
			} while (--yy);
		}
	}
}

static void mark_sprites_colors(void)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		if (READ_WORD(&spriteram[offs]) != 0)
			memset(&palette_used_colors[256 + 16*READ_WORD(&spriteram[offs+14])],PALETTE_COLOR_USED,16);
	}
}


void bjtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	palette_init_used_colors();

	for (offs = (bjtwin_txvideoram_size/2)-1; offs >= 0; offs--)
	{
		int color = (READ_WORD(&bjtwin_txvideoram[offs*2]) >> 12);
		memset(&palette_used_colors[16 * color],PALETTE_COLOR_USED,16);
	}
	mark_sprites_colors();

	if (palette_recalc())
		memset(dirtybuffer, 1, bjtwin_txvideoram_size/2);

	for (offs = (bjtwin_txvideoram_size/2)-1; offs >= 0; offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx = offs / 32;
			int sy = offs % 32;

			int tilecode = READ_WORD(&bjtwin_txvideoram[offs*2]);
			int bank = (tilecode & 0x800) ? 1 : 0;

			if (flipscreen)
			{
				sx = 31-sx;
				sy = 31-sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[bank],
					(tilecode & 0x7ff) + ((bank) ? (bgbank << 11) : 0),
					tilecode >> 12,
					flipscreen, flipscreen,
					(8*sx + 64) & 0x1ff,8*sy,
					0,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	draw_sprites(bitmap);
}
