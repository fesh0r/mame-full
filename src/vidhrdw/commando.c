/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"



unsigned char *commando_foreground_videoram;
unsigned char *commando_foreground_colorram;
unsigned char *commando_background_videoram;
unsigned char *commando_background_colorram;
unsigned char *commando_spriteram;
size_t commando_spriteram_size;
unsigned char *commando_scrollx,*commando_scrolly;

static struct tilemap *foreground_tilemap, *background_tilemap;




/***************************************************************************

  Convert the color PROMs into a more useable format.

  Commando has three 256x4 palette PROMs (one per gun), connected to the
  RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void commando_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i+Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i+Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i+Machine->drv->total_colors] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i+2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i+2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i+2*Machine->drv->total_colors] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_foreground_tile_info(int tile_index)
{
	int code, color;

	code = commando_foreground_videoram[tile_index];
	color = commando_foreground_colorram[tile_index];
	SET_TILE_INFO(0, code + ((color & 0xc0) << 2), color & 0x0f);
	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
}

static void get_background_tile_info(int tile_index)
{
	int code, color;

	code = commando_background_videoram[tile_index];
	color = commando_background_colorram[tile_index];
	SET_TILE_INFO(1, code + ((color & 0xc0) << 2), color & 0x0f);
	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int commando_vh_start(void)
{
	foreground_tilemap = tilemap_create(get_foreground_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	background_tilemap = tilemap_create(get_background_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE     ,16,16,32,32);

	if (!foreground_tilemap || !background_tilemap)
		return 1;

	foreground_tilemap->transparent_pen = 3;

	/* is vertical scrolling really used in this game??? */
	tilemap_set_scrolldy( background_tilemap, 0, 512 );

	return 0;
}


WRITE_HANDLER( commando_foreground_videoram_w )
{
	commando_foreground_videoram[offset] = data;
	tilemap_mark_tile_dirty(foreground_tilemap,offset);
}

WRITE_HANDLER( commando_foreground_colorram_w )
{
	commando_foreground_colorram[offset] = data;
	tilemap_mark_tile_dirty(foreground_tilemap,offset);
}

WRITE_HANDLER( commando_background_videoram_w )
{
	commando_background_videoram[offset] = data;
	tilemap_mark_tile_dirty(background_tilemap,offset);
}

WRITE_HANDLER( commando_background_colorram_w )
{
	commando_background_colorram[offset] = data;
	tilemap_mark_tile_dirty(background_tilemap,offset);
}


WRITE_HANDLER( commando_spriteram_w )
{
	if (data != commando_spriteram[offset] && offset % 4 == 2)
		logerror("%04x: sprite %d X offset (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,commando_spriteram[offset],data,cpu_getscanline());
	if (data != commando_spriteram[offset] && offset % 4 == 3)
		logerror("%04x: sprite %d Y offset (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,commando_spriteram[offset],data,cpu_getscanline());
	if (data != commando_spriteram[offset] && offset % 4 == 0)
		logerror("%04x: sprite %d code (old = %d new = %d) scanline %d\n",
				cpu_get_pc(),offset/4,commando_spriteram[offset],data,cpu_getscanline());

	commando_spriteram[offset] = data;
}


WRITE_HANDLER( commando_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 4 resets the sound CPU */
	cpu_set_reset_line(1,(data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 7 flips screen */
	flip_screen_w(offset, ~data & 0x80);
}


static draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	/* Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = commando_spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy,bank;


		/* bit 1 of [offs+1] is not used */

		sx = commando_spriteram[offs + 3] - 0x100 * (commando_spriteram[offs + 1] & 0x01);
		sy = commando_spriteram[offs + 2];
		flipx = commando_spriteram[offs + 1] & 0x04;
		flipy = commando_spriteram[offs + 1] & 0x08;
		bank = (commando_spriteram[offs + 1] & 0xc0) >> 6;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[2],
					commando_spriteram[offs] + 256 * bank,
					(commando_spriteram[offs + 1] & 0x30) >> 4,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void commando_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(background_tilemap, 0, commando_scrollx[0] + 256 * commando_scrollx[1]);
	tilemap_set_scrolly(background_tilemap, 0, commando_scrolly[0] + 256 * commando_scrolly[1]);

	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,background_tilemap,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,foreground_tilemap,0);
}
