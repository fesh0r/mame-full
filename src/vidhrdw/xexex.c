#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


static int sprite_colorbase;
unsigned char *xexex_paletteram, *xexex_spriteram;
static int layer_colorbase[4], bg_colorbase, layerpri[4];

static void xexex_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 4;	/* ??????? */
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase | (*color & 0x001f);
}

static void xexex_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);
	*color = layer_colorbase[layer] | ((*color & 0xf0) >> 4);
}

static int xexex_scrolld[2][4][2] = {
	{{ 53, 16 }, {53, 16}, {53, 16}, {53, 16}},
	{{ 42, 16 }, {42-4, 16}, {42-2, 16}, {42, 16}}
};

int xexex_vh_start(void)
{
	K053157_vh_start(2, 6, REGION_GFX1, xexex_scrolld, NORMAL_PLANE_ORDER, xexex_tile_callback);
	if (K053247_vh_start(REGION_GFX2, -92, 32, NORMAL_PLANE_ORDER, xexex_sprite_callback))
	{
		K053157_vh_stop();
		return 1;
	}
	return 0;
}

void xexex_vh_stop(void)
{
	K053157_vh_stop(); 
	K053247_vh_stop();
}


WRITE_HANDLER( xexex_palette_w )
{
	int r, g, b;
	int data0, data1;

	COMBINE_WORD_MEM(xexex_paletteram+offset, data);

	offset &= ~3;

	data0 = READ_WORD(xexex_paletteram + offset);
	data1 = READ_WORD(xexex_paletteram + offset + 2);

	r = data0 & 0xff;
	g = data1 >> 8;
	b = data1 & 0xff;

	palette_change_color(offset>>2, r, g, b);
}


/* useful function to sort the four tile layers by priority order */
/* suboptimal, but for such a size who cares ? */
static void sortlayers(int *layer, int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0, 1)
	SWAP(0, 2)
	SWAP(0, 3)
	SWAP(1, 2)
	SWAP(1, 3)
	SWAP(2, 3)
}

WRITE_HANDLER( xexex_sprite_w )
{
	COMBINE_WORD_MEM(xexex_spriteram+offset, data);
	if(!(offset & 2) && !(offset & 0x60))
		K053247_word_w(((offset >> 3) & 0x1ff0) | ((offset >> 1) & 0xe), data);
}

void xexex_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int layer[4];

	bg_colorbase       = K053251_get_palette_index(K053251_CI1);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI0);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);
	layer_colorbase[3] = 0x70;

	K053157_tilemap_update();

	palette_init_used_colors();
	K053247_mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);
	layer[3] = -1;
	layerpri[3] = -1 /*K053251_get_priority(K053251_CI1)*/;

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
	K053157_tilemap_draw(bitmap, layer[0], 1<<16);
	K053157_tilemap_draw(bitmap, layer[1], 2<<16);
	K053157_tilemap_draw(bitmap, layer[2], 4<<16);

	K053247_sprites_draw(bitmap);

	K053157_tilemap_draw(bitmap, 3, 0);
}
