/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


unsigned char *vulgus_foreground_videoram;
unsigned char *vulgus_foreground_colorram;
unsigned char *vulgus_background_videoram;
unsigned char *vulgus_background_colorram;
unsigned char *vulgus_spriteram;
size_t vulgus_spriteram_size;
unsigned char *vulgus_scroll_low,*vulgus_scroll_high;

static data_t vulgus_palette_bank;
static struct tilemap *foreground_tilemap, *background_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void vulgus_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* characters use colors 32-47 (?) */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 32;

	/* sprites use colors 16-31 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = *(color_prom++) + 16;
	}

	/* background tiles use colors 0-15, 64-79, 128-143, 192-207 in four banks */
	for (i = 0;i < TOTAL_COLORS(1)/4;i++)
	{
		COLOR(1,i) = *color_prom;
		COLOR(1,i+32*8) = *color_prom + 64;
		COLOR(1,i+2*32*8) = *color_prom + 128;
		COLOR(1,i+3*32*8) = *color_prom + 192;
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_foreground_tile_info(int tile_index)
{
	int code, color;

	code = vulgus_foreground_videoram[tile_index];
	color = vulgus_foreground_colorram[tile_index];
	SET_TILE_INFO(0, code + ((color & 0x80) << 1), color & 0x3f);
}

static void get_background_tile_info(int tile_index)
{
	int code, color;

	code = vulgus_background_videoram[tile_index];
	color = vulgus_background_colorram[tile_index];
	SET_TILE_INFO(1, code + ((color & 0x80) << 1), (color & 0x1f) + (0x20 * vulgus_palette_bank));
	tile_info.flags = TILE_FLIPYX((color & 0x60) >> 5);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int vulgus_vh_start(void)
{
	foreground_tilemap = tilemap_create(get_foreground_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR, 8, 8,32,32);
	background_tilemap = tilemap_create(get_background_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE           ,16,16,32,32);

	if (!foreground_tilemap || !background_tilemap)
		return 1;

	foreground_tilemap->transparent_pen = 47;

	return 0;
}



WRITE_HANDLER( vulgus_foreground_videoram_w )
{
	vulgus_foreground_videoram[offset] = data;
	tilemap_mark_tile_dirty(foreground_tilemap,offset);
}

WRITE_HANDLER( vulgus_foreground_colorram_w )
{
	vulgus_foreground_colorram[offset] = data;
	tilemap_mark_tile_dirty(foreground_tilemap,offset);
}

WRITE_HANDLER( vulgus_background_videoram_w )
{
	vulgus_background_videoram[offset] = data;
	tilemap_mark_tile_dirty(background_tilemap,offset);
}

WRITE_HANDLER( vulgus_background_colorram_w )
{
	vulgus_background_colorram[offset] = data;
	tilemap_mark_tile_dirty(background_tilemap,offset);
}


extern struct GameDriver driver_vulgus;

WRITE_HANDLER( vulgus_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 7 flips screen, but it's reversed in set 1 */
	if (Machine->gamedrv == &driver_vulgus)
		data ^= 0x80;

	flip_screen_w(offset, data & 0x80);
}


WRITE_HANDLER( vulgus_palette_bank_w )
{
	if (vulgus_palette_bank != data)
		tilemap_mark_all_tiles_dirty(background_tilemap);

	vulgus_palette_bank = data;
}


static draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = vulgus_spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int code,i,col,sx,sy,dir;


		code = vulgus_spriteram[offs];
		col = vulgus_spriteram[offs + 1] & 0x0f;
		sx = vulgus_spriteram[offs + 3];
		sy = vulgus_spriteram[offs + 2];
		dir = 1;
		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			dir = -1;
		}

		i = (vulgus_spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

		do
		{
			drawgfx(bitmap,Machine->gfx[2],
					code + i,
					col,
					flip_screen,flip_screen,
					sx, sy + 16 * i * dir,
					&Machine->visible_area,TRANSPARENCY_PEN,15);

			/* draw again with wraparound */
			drawgfx(bitmap,Machine->gfx[2],
					code + i,
					col,
					flip_screen,flip_screen,
					sx, sy + 16 * i * dir -  dir * 256,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
			i--;
		} while (i >= 0);
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void vulgus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(background_tilemap, 0, vulgus_scroll_low[1] + 256 * vulgus_scroll_high[1]);
	tilemap_set_scrolly(background_tilemap, 0, vulgus_scroll_low[0] + 256 * vulgus_scroll_high[0]);

	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,background_tilemap,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,foreground_tilemap,0);
}
