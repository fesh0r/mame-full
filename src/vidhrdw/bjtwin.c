#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *nmk_bgvideoram,*nmk_fgvideoram,*nmk_txvideoram;
static int redraw_bitmap;

static unsigned char *spriteram_old,*spriteram_old2;
static int bgbank;
static int videoshift;
static int bioship_background_bank;
static int bioship_scroll[4];

static struct tilemap *bg_tilemap,*fg_tilemap,*tx_tilemap;
static struct osd_bitmap *background_bitmap;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static void macross_get_bg_tile_info(int tile_index)
{
	int code = READ_WORD(&nmk_bgvideoram[2*tile_index]);
	SET_TILE_INFO(1,(code & 0xfff) + (bgbank << 12),code >> 12)
}

static void strahl_get_fg_tile_info(int tile_index)
{
	int code = READ_WORD(&nmk_fgvideoram[2*tile_index]);
	SET_TILE_INFO(3,(code & 0xfff),code >> 12)
}

static void macross_get_tx_tile_info(int tile_index)
{
	int code = READ_WORD(&nmk_txvideoram[2*tile_index]);
	SET_TILE_INFO(0,code & 0xfff,code >> 12)
}

static void bjtwin_get_bg_tile_info(int tile_index)
{
	int code = READ_WORD(&nmk_bgvideoram[2*tile_index]);
	int bank = (code & 0x800) ? 1 : 0;
	SET_TILE_INFO(bank,(code & 0x7ff) + ((bank) ? (bgbank << 11) : 0),code >> 12)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void nmk_vh_stop(void)
{
	free(spriteram_old);
	free(spriteram_old2);
	spriteram_old = NULL;
	spriteram_old2 = NULL;
	if (background_bitmap) bitmap_free(background_bitmap);
}
 
int bioship_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);
	background_bitmap = bitmap_alloc(4096,256);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2 || !background_bitmap)
		return 1;

	bg_tilemap->transparent_pen = 15;
	tx_tilemap->transparent_pen = 15;
	bioship_background_bank=0;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */

	return 0;
}

int strahl_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	fg_tilemap = tilemap_create(strahl_get_fg_tile_info, bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !fg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	fg_tilemap->transparent_pen = 15;
	tx_tilemap->transparent_pen = 15;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;
	return 0;
}

int macross_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tx_tilemap->transparent_pen = 15;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;

	return 0;
}

int macross2_vh_start(void)
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,128);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tx_tilemap->transparent_pen = 15;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}

int bjtwin_vh_start(void)
{
	bg_tilemap = tilemap_create(bjtwin_get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,64,32);
	spriteram_old = malloc(spriteram_size);
	spriteram_old2 = malloc(spriteram_size);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	memset(spriteram_old,0,spriteram_size);
	memset(spriteram_old2,0,spriteram_size);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( nmk_bgvideoram_r )
{
	return READ_WORD(&nmk_bgvideoram[offset]);
}

WRITE_HANDLER( nmk_bgvideoram_w )
{
	int oldword = READ_WORD(&nmk_bgvideoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&nmk_bgvideoram[offset],newword);
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
	}
}

READ_HANDLER( nmk_fgvideoram_r )
{
	return READ_WORD(&nmk_fgvideoram[offset]);
}

WRITE_HANDLER( nmk_fgvideoram_w )
{
	int oldword = READ_WORD(&nmk_fgvideoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&nmk_fgvideoram[offset],newword);
		tilemap_mark_tile_dirty(fg_tilemap,offset/2);
	}
}

READ_HANDLER( nmk_txvideoram_r )
{
	return READ_WORD(&nmk_txvideoram[offset]);
}

WRITE_HANDLER( nmk_txvideoram_w )
{
	int oldword = READ_WORD(&nmk_txvideoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&nmk_txvideoram[offset],newword);
		tilemap_mark_tile_dirty(tx_tilemap,offset/2);
	}
}

WRITE_HANDLER( nmk_paletteram_w )
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
 
	/* This is very bad, ensure Bioships tilemap is redrawn if palette changes */
    if (offset<512) redraw_bitmap=1; 
}

WRITE_HANDLER( mustang_scroll_w )
{
	static UINT8 scroll[4];

	scroll[offset/2] = (data >> 8) & 0xff;

	if (offset & 4)
		tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
	else
		tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
}

WRITE_HANDLER( nmk_scroll_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		static UINT8 scroll[4];

		scroll[offset/2] = data & 0xff;

		if (offset & 4)
			tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE_HANDLER( nmk_scroll_2_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		static UINT8 scroll[4];

		scroll[offset/2] = data & 0xff;

		if (offset & 4)
			tilemap_set_scrolly(fg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(fg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE_HANDLER( nmk_flipscreen_w )
{
	flip_screen_w(0,data & 0x01);
}

WRITE_HANDLER( nmk_tilebank_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		if (bgbank != (data & 0xff))
		{
			bgbank = data & 0xff;
			tilemap_mark_all_tiles_dirty(bg_tilemap);
		}
	}
}

WRITE_HANDLER( bioship_scroll_w )
{
	bioship_scroll[offset/2]=data>>8;
}

WRITE_HANDLER( bioship_bank_w )
{
	if (bioship_background_bank!=((data&7)*0x2000))
		redraw_bitmap=1;
	bioship_background_bank=(data&7)*0x2000;
}

/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap, int priority, int pri_mask)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		if (READ_WORD(&spriteram_old2[offs]))
		{
			int sx = (READ_WORD(&spriteram_old2[offs+8]) & 0x1ff) + videoshift;
			int sy = (READ_WORD(&spriteram_old2[offs+12]) & 0x1ff);
			int code = READ_WORD(&spriteram_old2[offs+6]);
			int color = READ_WORD(&spriteram_old2[offs+14]);
			int w = (READ_WORD(&spriteram_old2[offs+2]) & 0x0f);
			int h = ((READ_WORD(&spriteram_old2[offs+2]) & 0xf0) >> 4);
			int pri = READ_WORD( &spriteram_old2[offs+14] )>>8;
			int xx,yy,x;
			int delta = 16;

			if ((priority&pri_mask)!=pri) continue;

			if (flip_screen)
			{
				sx = 368 - sx;
				sy = 240 - sy;
				delta = -16;
			}

			yy = h;
			do
			{
				x = sx;
				xx = w;
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							code,
							color,
							flip_screen, flip_screen,
							((x + 16) & 0x1ff) - 16,sy & 0x1ff,
							&Machine->visible_area,TRANSPARENCY_PEN,15);

					code++;
					x += delta;
				} while (--xx >= 0);

				sy += delta;
			} while (--yy >= 0);
		}
	}
}

static void mark_sprites_colors(void)
{
	int offs,pal_base,color_mask;

	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	color_mask = Machine->gfx[2]->total_colors - 1;

	for (offs = 0;offs < spriteram_size;offs += 16)
	{
		if (READ_WORD(&spriteram_old2[offs]))
		{
			int color = READ_WORD(&spriteram_old2[offs+14]) & color_mask;
			memset(&palette_used_colors[pal_base + 16*color],PALETTE_COLOR_USED,16);
		}
	}
}

static void mark_user_tilemap_colors(void)
{
	int offs,pal_base,colmask[16],color,code,i;
	unsigned char *tilerom = memory_region(REGION_USER1);

	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs = 0; offs < 0x2000;offs += 2)
	{
		code = READ_WORD(&tilerom[offs+bioship_background_bank]);
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= Machine->gfx[3]->pen_usage[code];
	}
	
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}
}

void macross_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0);
	draw_sprites(bitmap,0,0);
	tilemap_draw(bitmap,tx_tilemap,0);
}

void bioship_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char *tilerom = memory_region(REGION_USER1);
	int scrollx=-((bioship_scroll[1]&0xff) + (bioship_scroll[0]&0xff)*256);
	int scrolly=-((bioship_scroll[3]&0xff) + (bioship_scroll[2]&0xff)*256);

	tilemap_set_scrollx(tx_tilemap,0,-videoshift);
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();
	mark_user_tilemap_colors();

	if (palette_recalc()) {
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
		redraw_bitmap=1;
	}

	if (redraw_bitmap) {
		int sx=0, sy=-1, offs;
		redraw_bitmap=0;

        /* Draw background from tile rom */
        for (offs = 0;offs <0x2000;offs+=2) {
                unsigned short data = READ_WORD( &tilerom[offs+bioship_background_bank] );
                int numtile = data&0xfff;
                int color = (data&0xf000)>>12;
                sy++;
				if (sy==16) {sy=0; sx++;}
 
                drawgfx(background_bitmap,Machine->gfx[3],
                        numtile,
                        color,
                        0,0,   /* no flip */
                        16*sx,16*sy,
                        0,TRANSPARENCY_NONE,0);
				
				data = READ_WORD( &tilerom[offs+0x1000+bioship_background_bank] );
				numtile = data&0xfff; 
				color = (data&0xf000)>>12;
		        drawgfx(background_bitmap,Machine->gfx[3],
                      	numtile,
                        color,
                        0,0,   /* no flip */
                        16*sx,(16*sy)+256,
                        0,TRANSPARENCY_NONE,0);
        }
	}

	tilemap_render(ALL_TILEMAPS);
	copyscrollbitmap(bitmap,background_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	draw_sprites(bitmap,5,~5); /* Is this right? */
	tilemap_draw(bitmap,bg_tilemap,0);
	draw_sprites(bitmap,5,5);
	tilemap_draw(bitmap,tx_tilemap,0);
}

void strahl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0);
	tilemap_draw(bitmap,fg_tilemap,0);
	draw_sprites(bitmap,0,0);
	tilemap_draw(bitmap,tx_tilemap,0);
}

void bjtwin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_set_scrollx(bg_tilemap,0,-videoshift);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	mark_sprites_colors();

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,bg_tilemap,0);
	draw_sprites(bitmap,0,0);
}

void nmk_eof_callback(void)
{
	/* looks like sprites are *two* frames ahead */
	memcpy(spriteram_old2,spriteram_old,spriteram_size);
	memcpy(spriteram_old,spriteram,spriteram_size);
}
