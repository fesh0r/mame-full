/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *gunsmoke_fgvideoram;

static int bg_on, sprites_on;
static int sprite3bank;
static struct tilemap *fg_tilemap, *bg_tilemap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Gunsmoke has three 256x4 palette PROMs (one per gun) and a lot ;-) of
  256x4 lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

void gunsmoke_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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

	/* characters use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 64;
	color_prom += 128;	/* skip the bottom half of the PROM - not used */

	/* background tiles use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = color_prom[0] + 16 * (color_prom[256] & 0x03);
		color_prom++;
	}
	color_prom += TOTAL_COLORS(1);

	/* sprites use colors 128-255 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = color_prom[0] + 16 * (color_prom[256] & 0x07) + 128;
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = gunsmoke_fgvideoram[tile_index];
	color = gunsmoke_fgvideoram[tile_index + 0x400];
	SET_TILE_INFO(0, code + ((color & 0xc0) << 2), color & 0x1f);
}

static void get_bg_tile_info(int tile_index)
{
	int code, color;

	code = memory_region(REGION_GFX4)[2*tile_index];
	color = memory_region(REGION_GFX4)[2*tile_index+1];
	SET_TILE_INFO(1, code + ((color & 0x01) << 8), (color & 0x3c) >> 2);
	tile_info.flags = TILE_FLIPYX((color & 0xc0) >> 6);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int gunsmoke_vh_start(void)
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR, 8, 8,  32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,           32,32,2048, 8);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	fg_tilemap->transparent_pen = 79;

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( gunsmoke_fgvideoram_w )
{
	gunsmoke_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}


WRITE_HANDLER( gunsmoke_scrollx_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( gunsmoke_scrolly_w )
{
	tilemap_set_scrolly(bg_tilemap,0,data);
}


WRITE_HANDLER( gunsmoke_c804_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* bits 0 and 1 are for coin counters */
	coin_counter_w(1,data & 1);
	coin_counter_w(0,data & 2);

	/* bits 2 and 3 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0c) * 0x1000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bit 5 resets the sound CPU */
	cpu_set_reset_line(1,(data & 0x20) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 6 flips screen */
	flip_screen_w(0,data & 0x40);

	/* bit 7 enables characters */
	tilemap_set_enable(fg_tilemap, data & 0x80);
}


WRITE_HANDLER( gunsmoke_d806_w )
{
	/* bits 0-2 select the sprite 3 bank */
	sprite3bank = data & 0x07;

	/* bit 4 enables bg 1 */
	bg_on = data & 0x10;
	tilemap_set_enable(bg_tilemap, bg_on);

	/* bit 5 enables sprites */
	sprites_on = data & 0x20;
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
		int sx,sy,bank,flipx,flipy;


		bank = (spriteram[offs + 1] & 0xc0) >> 6;
		if (bank == 3) bank += sprite3bank;

		sx = spriteram[offs + 3] - ((spriteram[offs + 1] & 0x20) << 3);
		sy = spriteram[offs + 2];
		flipx = 0;
		flipy = spriteram[offs + 1] & 0x10;
		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs] + 256 * bank,
				spriteram[offs + 1] & 0x0f,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void gunsmoke_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	if (bg_on)
		tilemap_draw(bitmap,bg_tilemap,0);
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	if (sprites_on)  draw_sprites(bitmap);
	tilemap_draw(bitmap,fg_tilemap,0);
}
