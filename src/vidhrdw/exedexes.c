/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *exedexes_fgvideoram;

static struct tilemap *fg_tilemap, *bg1_tilemap, *bg2_tilemap;
static int bg1_on, sprites_on;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Exed Exes has three 256x4 palette PROMs (one per gun), three 256x4 lookup
  table PROMs (one for characters, one for sprites, one for background tiles)
  and one 256x4 sprite palette bank selector PROM.

  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

void exedexes_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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

	/* characters use colors 192-207 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = (*color_prom++) + 192;

	/* 32x32 tiles use colors 0-15 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = (*color_prom++);

	/* 16x16 tiles use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
		COLOR(2,i) = (*color_prom++) + 64;

	/* sprites use colors 128-191 in four banks */
	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		COLOR(3,i) = color_prom[0] + 128 + 16 * color_prom[256];
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 get_bg1_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (col & 0x07) | (row << 3) | ((col & 0x1f8) << 4);
}

static UINT32 get_bg2_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (col & 0x0f) | ((row & 0x0f) << 4) | ((col & 0x70) << 4) | ((row & 0x70) << 7);
}

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = exedexes_fgvideoram[tile_index];
	color = exedexes_fgvideoram[tile_index + 0x400];
	SET_TILE_INFO(0, code + ((color & 0x80) << 1), color & 0x3f);
}

static void get_bg1_tile_info(int tile_index)
{
	int code, color;

	code = memory_region(REGION_GFX5)[tile_index + 0x4000];
	color = memory_region(REGION_GFX5)[tile_index + 0x4000 + 0x40];
	SET_TILE_INFO(1, code & 0x3f, color);
	tile_info.flags = TILE_FLIPYX((code & 0xc0) >> 6);
}

static void get_bg2_tile_info(int tile_index)
{
	SET_TILE_INFO(2, memory_region(REGION_GFX5)[tile_index], 0);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int exedexes_vh_start(void)
{
	fg_tilemap  = tilemap_create(get_fg_tile_info, tilemap_scan_rows,    TILEMAP_TRANSPARENT_COLOR, 8, 8, 32, 32);
	bg1_tilemap = tilemap_create(get_bg1_tile_info,get_bg1_memory_offset,TILEMAP_OPAQUE,           32,32,512,  8);
	bg2_tilemap = tilemap_create(get_bg2_tile_info,get_bg2_memory_offset,TILEMAP_TRANSPARENT,      16,16,128,128);

	if (!fg_tilemap || !bg1_tilemap || !bg2_tilemap)
		return 1;

	fg_tilemap->transparent_pen = 207;
	bg2_tilemap->transparent_pen = 0;

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( exedexes_fgvideoram_w )
{
	exedexes_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}


WRITE_HANDLER( exedexes_bg1_scrollx_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg1_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( exedexes_bg2_scrollx_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg2_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( exedexes_bg2_scrolly_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrolly(bg2_tilemap,0,scroll[0] | (scroll[1] << 8));
}


WRITE_HANDLER( exedexes_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 7 is text enable */
	tilemap_set_enable(fg_tilemap, data & 0x80);

	/* other bits seem to be unused */
}


WRITE_HANDLER( exedexes_gfxctrl_w )
{
	/* bit 4 is far background enable */
	bg1_on = data & 0x10;
	tilemap_set_enable(bg1_tilemap, bg1_on);

	/* bit 5 is near background enable */
	tilemap_set_enable(bg2_tilemap, data & 0x20);

	/* bit 6 is sprite enable */
	sprites_on = data & 0x40;

	/* other bits seem to be unused */
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap,int priority)
{
	int offs;


	if (!sprites_on)
		return;

	priority = priority ? 0x40 : 0x00;

	for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
		if ((buffered_spriteram[offs + 1] & 0x40) == priority)
		{
			int code,color,flipx,flipy,sx,sy;

			code = buffered_spriteram[offs];
			color = buffered_spriteram[offs + 1] & 0x0f;
			flipx = buffered_spriteram[offs + 1] & 0x10;
			flipy = buffered_spriteram[offs + 1] & 0x20;
			sx = buffered_spriteram[offs + 3] - ((buffered_spriteram[offs + 1] & 0x80) << 1);
			sy = buffered_spriteram[offs + 2];

			drawgfx(bitmap,Machine->gfx[3],
					code,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

void exedexes_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	if (bg1_on)
		tilemap_draw(bitmap,bg1_tilemap,0);
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	draw_sprites(bitmap, 1);
	tilemap_draw(bitmap,bg2_tilemap,0);
	draw_sprites(bitmap, 0);
	tilemap_draw(bitmap,fg_tilemap,0);
}

void exedexes_eof_callback(void)
{
	buffer_spriteram_w(0,0);
}
