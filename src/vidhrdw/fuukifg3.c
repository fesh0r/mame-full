/***************************************************************************

						  -= Fuuki 32 Bit Games (FG-3) =-

				driver by Paul Priest and David Haywood
				based on fuuki16 by Luca Elia

	[ 4 Scrolling Layers ]

							[ Layer 0 ]		[ Layer 1 ]		[ Layers 2&3 ]

	Tile Size:				16 x 16 x 8		16 x 16 x 8		8 x 8 x 4
	Layer Size (tiles):		64 x 32			64 x 32			64 x 32

Per-line raster effects used on many stages
Sprites buffered by two frames

***************************************************************************/

#include "vidhrdw/generic.h"

/* Variables that driver has access to: */

data32_t *fuuki32_vregs, *fuuki32_priority, *spr_tilebank;
data32_t *fuuki32_tilemap_ram, *fuuki32_tilemap_2_ram, *fuuki32_tilemap_bg_ram, *fuuki32_tilemap_bg2_ram;

static struct tilemap *fuuki32_tilemap;
static struct tilemap *fuuki32_tilemap_2;
static struct tilemap *fuuki32_tilemap_bg;
static struct tilemap *fuuki32_tilemap_bg2;

static UINT32 spr_buffered_tilebank, spr_buffered_tilebank_2;

/***************************************************************************


								Sprites Drawing

	Offset: 	Bits:					Value:

		0.w		fedc ---- ---- ----		Number Of Tiles Along X - 1
				---- b--- ---- ----		Flip X
				---- -a-- ---- ----		1 = Don't Draw This Sprite
				---- --98 7654 3210		X (Signed)

		2.w		fedc ---- ---- ----		Number Of Tiles Along Y - 1
				---- b--- ---- ----		Flip Y
				---- -a-- ---- ----
				---- --98 7654 3210		Y (Signed)

		4.w		fedc ---- ---- ----		Zoom X ($0 = Full Size, $F = Half Size)
				---- ba98 ---- ----		Zoom Y ""
				---- ---- 76-- ----		Priority
				---- ---- --54 3210		Color

		6.w		fe-- ---- ---- ----		Tile Bank
				--dc ba98 7654 3210		Tile Code


***************************************************************************/

static void fuuki32_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	int max_x		=	Machine->visible_area.max_x+1;
	int max_y		=	Machine->visible_area.max_y+1;

	data32_t *src = buffered_spriteram32_2; /* Use spriteram buffered by 2 frames, need palette buffered by one frame? */

	/* Draw them backwards, for pdrawgfx */
	for ( offs = (0x2000-8)/4; offs >=0; offs -= 8/4 )
	{
		int x, y, xstart, ystart, xend, yend, xinc, yinc;
		int xnum, ynum, xzoom, yzoom, flipx, flipy;
		int pri_mask;

		int sx			=		(src[offs + 0]& 0xffff0000)>>16;
		int sy			=		(src[offs + 0]& 0x0000ffff);
		int attr		=		(src[offs + 1]& 0xffff0000)>>16;
		int code		=		(src[offs + 1]& 0x0000ffff);

		int bank = (code & 0xc000) >> 14;
		int bank_lookedup;

		bank_lookedup = ((spr_buffered_tilebank_2 & 0xffff0000)>>(16+bank*4))&0xf;
		code &= 0x3fff;
		code += bank_lookedup * 0x4000;

		if (sx & 0x400)		continue;

		flipx		=		sx & 0x0800;
		flipy		=		sy & 0x0800;

		xnum		=		((sx >> 12) & 0xf) + 1;
		ynum		=		((sy >> 12) & 0xf) + 1;

		xzoom		=		16*8 - (8 * ((attr >> 12) & 0xf))/2;
		yzoom		=		16*8 - (8 * ((attr >>  8) & 0xf))/2;

		switch( (attr >> 6) & 3 )
		{
			case 3:		pri_mask = 0xf0|0xcc|0xaa;	break;	// behind all layers
			case 2:		pri_mask = 0xf0|0xcc;		break;	// behind fg + middle layer
			case 1:		pri_mask = 0xf0;			break;	// behind fg layer
			case 0:
			default:	pri_mask = 0;						// above all
		}

		sx = (sx & 0x1ff) - (sx & 0x200);
		sy = (sy & 0x1ff) - (sy & 0x200);

		if (flip_screen)
		{	flipx = !flipx;		sx = max_x - sx - xnum * 16;
			flipy = !flipy;		sy = max_y - sy - ynum * 16;	}

		if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1; }
		else		{ xstart = 0;       xend = xnum;  xinc = +1; }

		if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1; }
		else		{ ystart = 0;       yend = ynum;  yinc = +1; }

#if 0
		if(!( (keyboard_pressed(KEYCODE_V) && (((attr >> 6)&3) == 0))
		   || (keyboard_pressed(KEYCODE_B) && (((attr >> 6)&3) == 1))
		   || (keyboard_pressed(KEYCODE_N) && (((attr >> 6)&3) == 2))
		   || (keyboard_pressed(KEYCODE_M) && (((attr >> 6)&3) == 3))
		   ))
#endif

		for (y = ystart; y != yend; y += yinc)
		{
			for (x = xstart; x != xend; x += xinc)
			{
				if (xzoom == (16*8) && yzoom == (16*8))
					pdrawgfx(		bitmap,Machine->gfx[0],
									code++,
									attr & 0x3f,
									flipx, flipy,
									sx + x * 16, sy + y * 16,
									cliprect,TRANSPARENCY_PEN,15,
									pri_mask	);
				else
					pdrawgfxzoom(	bitmap,Machine->gfx[0],
									code++,
									attr & 0x3f,
									flipx, flipy,
									sx + (x * xzoom) / 8, sy + (y * yzoom) / 8,
									cliprect,TRANSPARENCY_PEN,15,
									(0x10000/0x10/8) * (xzoom + 8),(0x10000/0x10/8) * (yzoom + 8),	// nearest greater integer value to avoid holes
									pri_mask	);
			}
		}
	}
}


/***************************************************************************


									Tilemaps

	Offset: 	Bits:					Value:

		0.w								Code

		2.w		fedc ba98 ---- ----
				---- ---- 7--- ----		Flip Y
				---- ---- -6-- ----		Flip X
				---- ---- --54 3210		Color (Only top two  bits on 8bpp?)


***************************************************************************/

static void get_fuuki32_tile_info(int tile_index)
{
	int tileno,attr;
	tileno = (fuuki32_tilemap_ram[tile_index]&0xffff0000)>>16;
	attr = (fuuki32_tilemap_ram[tile_index]&0x0000ffff); //?
	SET_TILE_INFO(1,tileno, (attr>>4) & 3 ,TILE_FLIPYX( (attr >> 6) & 3) )
}

WRITE32_HANDLER( fuuki32_tilemap_w )
{
	if (fuuki32_tilemap_ram[offset] != data)
	{
		COMBINE_DATA(&fuuki32_tilemap_ram[offset]);
		tilemap_mark_tile_dirty(fuuki32_tilemap,offset);
	}
}

static void get_fuuki32_tile_2_info(int tile_index)
{
	int tileno,attr;
	tileno = (fuuki32_tilemap_2_ram[tile_index]&0xffff0000)>>16;
	attr = (fuuki32_tilemap_2_ram[tile_index]&0x0000ffff); //?
	SET_TILE_INFO(2,tileno, ((attr>>4) & 3), TILE_FLIPYX( (attr >> 6) & 3)  )
}

WRITE32_HANDLER( fuuki32_tilemap_2_w )
{
	if (fuuki32_tilemap_2_ram[offset] != data)
	{
		COMBINE_DATA(&fuuki32_tilemap_2_ram[offset]);
		tilemap_mark_tile_dirty(fuuki32_tilemap_2,offset);
	}
}

static void get_fuuki32_tile_bg_info(int tile_index)
{
	int tileno,attr;
	tileno = (fuuki32_tilemap_bg_ram[tile_index]&0xffff0000)>>16;
	attr = (fuuki32_tilemap_bg_ram[tile_index]&0x0000ffff); //?
	SET_TILE_INFO(3,tileno, (attr & 0x3f), TILE_FLIPYX( (attr >> 6) & 3)   )
}

WRITE32_HANDLER( fuuki32_tilemap_bg_w )
{
	if (fuuki32_tilemap_bg_ram[offset] != data)
	{
		COMBINE_DATA(&fuuki32_tilemap_bg_ram[offset]);
		tilemap_mark_tile_dirty(fuuki32_tilemap_bg,offset);
	}
}

static void get_fuuki32_tile_bg2_info(int tile_index)
{
	int tileno,attr;
	tileno = (fuuki32_tilemap_bg2_ram[tile_index]&0xffff0000)>>16;
	attr = (fuuki32_tilemap_bg2_ram[tile_index]&0x0000ffff); //?
	SET_TILE_INFO(3,tileno, (attr & 0x3f), TILE_FLIPYX( (attr >> 6) & 3)   )
}

WRITE32_HANDLER( fuuki32_tilemap_bg2_w )
{
	if (fuuki32_tilemap_bg2_ram[offset] != data)
	{
		COMBINE_DATA(&fuuki32_tilemap_bg2_ram[offset]);
		tilemap_mark_tile_dirty(fuuki32_tilemap_bg2,offset);
	}
}


/***************************************************************************


							Video Hardware Init


***************************************************************************/

VIDEO_START( fuuki32 )
{
	buffered_spriteram32 = auto_malloc(spriteram_size);
	buffered_spriteram32_2 = auto_malloc(spriteram_size);

	fuuki32_tilemap     = tilemap_create(get_fuuki32_tile_info,    tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64,32);
	fuuki32_tilemap_2   = tilemap_create(get_fuuki32_tile_2_info,  tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 64,32);
	fuuki32_tilemap_bg  = tilemap_create(get_fuuki32_tile_bg_info, tilemap_scan_rows, TILEMAP_TRANSPARENT,  8,  8, 64,32);
	fuuki32_tilemap_bg2 = tilemap_create(get_fuuki32_tile_bg2_info,tilemap_scan_rows, TILEMAP_TRANSPARENT,  8,  8, 64,32);

	tilemap_set_transparent_pen(fuuki32_tilemap ,  0xff);
	tilemap_set_transparent_pen(fuuki32_tilemap_2, 0xff);
	tilemap_set_transparent_pen(fuuki32_tilemap_bg, 0xf);
	tilemap_set_transparent_pen(fuuki32_tilemap_bg2,0xf);

	return 0;
}


/***************************************************************************


								Screen Drawing

	Video Registers (fuuki32_vregs):

		00.w		Layer 1 Scroll Y
		02.w		Layer 1 Scroll X
		04.w		Layer 0 Scroll Y
		06.w		Layer 0 Scroll X
		08.w		Layer 2 Scroll Y
		0a.w		Layer 2 Scroll X
		0c.w		Layers Y Offset
		0e.w		Layers X Offset

		10-1a.w		? 0
		1c.w		Trigger a level 5 irq on this raster line
		1e.w		? $3390/$3393 (Flip Screen Off/On)

	Priority Register (fuuki32_priority):

		fedc ba98 7654 3---
		---- ---- ---- -210		Layer Order


	Unknown Registers ($de0000.l):

		00.w		? $0200/$0201	(Flip Screen Off/On)
		02.w		? $f300/$0330

***************************************************************************/

/* Wrapper to handle bg and bg2 ttogether */
static void fuuki32_draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int i, int flag, int pri)
{
	switch( i )
	{
		case 2:	tilemap_draw(bitmap,cliprect,fuuki32_tilemap_bg2,flag,pri);
				tilemap_draw(bitmap,cliprect,fuuki32_tilemap_bg,flag,pri);
				return;
		case 1:	tilemap_draw(bitmap,cliprect,fuuki32_tilemap_2,flag,pri);
				return;
		case 0:	tilemap_draw(bitmap,cliprect,fuuki32_tilemap,flag,pri);
				return;
	}
}

VIDEO_UPDATE( fuuki32 )
{
	/*
	It's not independant bits causing layers to switch, that wouldn't make sense with 3 bits.
	*/

	int tm_back, tm_middle, tm_front;
	int pri_table[6][3] = {
		{ 0, 1, 2 }, // Not used?
		{ 0, 2, 1 }, // Two Levels - 0>1 (0,1,2 or 0,2,1 or 2,0,1)
		{ 1, 0, 2 }, // Most Levels - 2>1 1>0 2>0 (1,0,2)
		{ 1, 2, 0 }, // Not used?
		{ 2, 0, 1 }, // Title etc. - 0>1 (0,1,2 or 0,2,1 or 2,0,1)
		{ 2, 1, 0 }}; // Char Select, prison stage 1>0 (leaves 1,2,0 or 2,1,0)

	tm_front  = pri_table[ (fuuki32_priority[0]>>16) & 0x0f ][0];
	tm_middle = pri_table[ (fuuki32_priority[0]>>16) & 0x0f ][1];
	tm_back   = pri_table[ (fuuki32_priority[0]>>16) & 0x0f ][2];

	fillbitmap(bitmap,get_black_pen(),cliprect);
	fillbitmap(priority_bitmap,0,cliprect);

#if 0
	usrintf_showmessage	("vidregs %08x %08x %08x %08x", fuuki32_vregs[0], fuuki32_vregs[1], fuuki32_vregs[2], fuuki32_vregs[3]);
	usrintf_showmessage	("pri %08x", fuuki32_priority[0] );
#endif

	tilemap_set_scrolly(fuuki32_tilemap,0,    (fuuki32_vregs[0]&0xffff0000)>>16);
	tilemap_set_scrollx(fuuki32_tilemap,0,    (fuuki32_vregs[0]&0x0000ffff));
	tilemap_set_scrolly(fuuki32_tilemap_2,0,  (fuuki32_vregs[1]&0xffff0000)>>16);
	tilemap_set_scrollx(fuuki32_tilemap_2,0,  (fuuki32_vregs[1]&0x0000ffff));
	tilemap_set_scrolly(fuuki32_tilemap_bg,0, (fuuki32_vregs[2]&0xffff0000)>>16);
	tilemap_set_scrollx(fuuki32_tilemap_bg,0, (fuuki32_vregs[2]&0x0000ffff));

	fuuki32_draw_layer(bitmap,cliprect,tm_back,  0,1);
	fuuki32_draw_layer(bitmap,cliprect,tm_middle,0,2);
	fuuki32_draw_layer(bitmap,cliprect,tm_front, 0,4);

	/* No rasters on sprites */
	if (cliprect->max_y == Machine->visible_area.max_y)
		fuuki32_draw_sprites(bitmap,&Machine->visible_area);
}

VIDEO_EOF( fuuki32 )
{
	/* Buffer sprites and tilebank by 2 frames */

	spr_buffered_tilebank_2 = spr_buffered_tilebank;
	spr_buffered_tilebank = spr_tilebank[0];
	memcpy(buffered_spriteram32_2,buffered_spriteram32,spriteram_size);
	memcpy(buffered_spriteram32,spriteram32,spriteram_size);
}
