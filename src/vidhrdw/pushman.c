#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *background_layer,*text_layer;
static int control[4];

WRITE_HANDLER( pushman_scroll_w )
{
	WRITE_WORD(&control[offset],data);
}

static UINT32 background_scan_rows(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return ((col & 0x7)) + ((7-(row & 0x7)) << 3) + ((col & 0x78) <<3) + ((0x38-(row&0x38))<<7);
}

READ_HANDLER( pushman_videoram_r )
{
	return READ_WORD(&videoram[offset]);
}

WRITE_HANDLER( pushman_videoram_w )
{
	COMBINE_WORD_MEM(&videoram[offset],data);
	tilemap_mark_tile_dirty( text_layer, offset/2 );
}

static void get_back_tile_info( int tile_index )
{
	unsigned char *bgMap = memory_region(REGION_USER1);
	int tile;

	tile=bgMap[tile_index<<1]+(bgMap[(tile_index<<1)+1]<<8);
	SET_TILE_INFO(2,(tile&0xff)|((tile&0x4000)>>6),(tile>>8)&0xf)
	tile_info.flags = ((tile&0x2000) >> 13);  /* flip x*/
}

static void get_text_tile_info( int tile_index )
{
	int tile=READ_WORD(&videoram[tile_index<<1]);
	SET_TILE_INFO(0,(tile&0xff)|((tile&0x8000)>>6)|((tile&0x4000)>>6)|((tile&0x2000)>>4),(tile>>8)&0xf)
}

int pushman_vh_start(void)
{
	background_layer = tilemap_create(get_back_tile_info,background_scan_rows,TILEMAP_OPAQUE,32,32,128,64);
	text_layer = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!text_layer || !background_layer)
		return 1;

	text_layer->transparent_pen = 3;

	return 0;
}

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs,x,y,color,sprite;

	for (offs = 0x1000-8;offs >=0;offs -= 8)
	{
		/* Don't draw empty sprite table entries */
		x = READ_WORD(&spriteram[offs+6])&0x1ff;
		if (x==0x180) continue;
		if (x>0xff) x=0-(0x200-x);
		y = 240-READ_WORD(&spriteram[offs+4]);
		color = ((READ_WORD(&spriteram[offs+2])>>2)&0xf);
		sprite = READ_WORD(&spriteram[offs])&0x7ff;

		drawgfx(bitmap,Machine->gfx[1],
				sprite,
				color,0,0,x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}
}

void pushman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int color,offs,sprite;
	int colmask[16],i,pal_base;

	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer,0, READ_WORD(&control[0]) );
	tilemap_set_scrolly( background_layer,0, 0xf00-READ_WORD(&control[2]) );
	tilemap_update(ALL_TILEMAPS);

	/* Build the dynamic palette */
	palette_init_used_colors();

	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0;offs <0x1000;offs += 8)
	{
		color = ((READ_WORD(&spriteram[offs+2])>>2)&0xf);
		sprite = READ_WORD(&spriteram[offs])&0x7ff;
		colmask[color] |= Machine->gfx[1]->pen_usage[sprite];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);
	tilemap_draw(bitmap,background_layer,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,text_layer,0);
}
