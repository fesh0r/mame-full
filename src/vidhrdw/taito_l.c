#include "driver.h"
#include "generic.h"


extern unsigned char *taitol_rambanks;

static struct tilemap *bg18_tilemap;
static struct tilemap *bg19_tilemap;
static struct tilemap *ch1a_tilemap;

static int cur_ctrl = 0;
static int cur_bankg = 0;
static int bankc[4];
static int flipscreen;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg18_tile_info(int tile_index)
{
	int attr = taitol_rambanks[2*tile_index+0x4000+1];
	int code = taitol_rambanks[2*tile_index+0x4000]|((attr&0x03)<<8)|(bankc[(attr&0xc)>>2]<<10);

	SET_TILE_INFO (0, code, (attr & 0xf0)>>4);
}

static void get_bg19_tile_info(int tile_index)
{
	int attr = taitol_rambanks[2*tile_index+0x5000+1];
	int code = taitol_rambanks[2*tile_index+0x5000]|((attr&0x03)<<8)|(bankc[(attr&0xc)>>2]<<10);

	SET_TILE_INFO (0, code, (attr & 0xf0)>>4);
}

static void get_ch1a_tile_info(int tile_index)
{
	int attr = taitol_rambanks[2*tile_index+0x6000+1];
	int code = taitol_rambanks[2*tile_index+0x6000]|((attr&0x01)<<8)|((attr&0x04)<<7);

	SET_TILE_INFO (2, code, (attr & 0xf0)>>4);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int taitol_vh_start(void)
{
	int i;

	bg18_tilemap = tilemap_create(get_bg18_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	bg19_tilemap = tilemap_create(get_bg19_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,32);
	ch1a_tilemap = tilemap_create(get_ch1a_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!ch1a_tilemap || !bg18_tilemap || !bg19_tilemap)
		return 1;

	bankc[0] = bankc[1] = bankc[2] = bankc[3] = 0;
	cur_ctrl = 0;

	bg18_tilemap->transparent_pen = 0;
	ch1a_tilemap->transparent_pen = 0;

	for (i=0;i<256;i++)
		palette_change_color(i, 0, 0, 0);

	tilemap_set_scrolldx(ch1a_tilemap,-8,-8);
	tilemap_set_scrolldx(bg18_tilemap,28,-11);
	tilemap_set_scrolldx(bg19_tilemap,38,-21);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( taitol_bankg_w )
{
	if (data != cur_bankg)
	{
		int i;
		for(i=0;i<4;i++)
			bankc[i] = bankc[i] + (data - cur_bankg)*4;
		cur_bankg = data;

		tilemap_mark_all_tiles_dirty(bg18_tilemap);
		tilemap_mark_all_tiles_dirty(bg19_tilemap);
	}
}

READ_HANDLER( taitol_bankg_r )
{
	return cur_bankg;
}

WRITE_HANDLER( taitol_bankc_w )
{
	if (bankc[offset] != data + cur_bankg*4)
	{
		bankc[offset] = data + cur_bankg*4;
//		logerror("Bankc %d, %02x (%04x)\n", offset, data, cpu_get_pc());

		tilemap_mark_all_tiles_dirty(bg18_tilemap);
		tilemap_mark_all_tiles_dirty(bg19_tilemap);
	}
}

READ_HANDLER( taitol_bankc_r )
{
	return bankc[offset] - cur_bankg*4;
}


WRITE_HANDLER( taitol_control_w )
{
//	logerror("Control Write %02x (%04x)\n", data, cpu_get_pc());

	cur_ctrl = data;
//usrintf_showmessage("%02x",data);

	/* bit 0 unknown */

	/* bit 1 unknown */

	/* bit 3 controls sprite/tile priority - handled in vh_screenrefresh() */

	/* bit 4 flip screen */
	flipscreen = data & 0x10;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* bit 5 display enable - handled in vh_screenrefresh() */
}

READ_HANDLER( taitol_control_r )
{
//	logerror("Control Read %02x (%04x)\n", cur_ctrl, cpu_get_pc());
	return cur_ctrl;
}

void taitol_chardef14_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32,     taitol_rambanks,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef15_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+128, taitol_rambanks,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef16_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+256, taitol_rambanks,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef17_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+384, taitol_rambanks,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef1c_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+512, taitol_rambanks + 0x4000,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef1d_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+640, taitol_rambanks + 0x4000,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef1e_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+768, taitol_rambanks + 0x4000,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_chardef1f_m(int offset)
{
	decodechar(Machine->gfx[2], offset/32+896, taitol_rambanks + 0x4000,
			   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	tilemap_mark_all_tiles_dirty(ch1a_tilemap);
}

void taitol_bg18_m(int offset)
{
	tilemap_mark_tile_dirty(bg18_tilemap,offset/2);
}

void taitol_bg19_m(int offset)
{
	tilemap_mark_tile_dirty(bg19_tilemap,offset/2);
}

void taitol_char1a_m(int offset)
{
	tilemap_mark_tile_dirty(ch1a_tilemap,offset/2);
}

void taitol_obj1b_m(int offset)
{
#if 0
	if (offset>=0x3f0 && offset<=0x3ff)
	{
		/* scroll, handled in vh-screenrefresh */
	}
#endif
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;


	spriteram = taitol_rambanks + 0x7000;
	spriteram_size = 0x3e8;
	/* at spriteram + 0x3f0 and 03f8 are the tilemap control registers;
		spriteram + 0x3e8 seems to be unused
	*/

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int code,color,sx,sy,flipx,flipy;

		color = spriteram[offs + 2] & 0x0f;
		code = spriteram[offs] | (spriteram[offs + 1] << 8);
		sx = spriteram[offs + 4] | ((spriteram[offs + 5] & 1) << 8);
		sy = spriteram[offs + 6] | ((spriteram[offs + 7] & 1) << 8);
		if (sx >= 320) sx -= 512;
		flipx = spriteram[offs + 3] & 0x01;
		flipy = spriteram[offs + 3] & 0x02;

		if (flipscreen)
		{
			sx = 304 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

if (keyboard_pressed(KEYCODE_Q) && (color & 8)) color = rand();
if (keyboard_pressed(KEYCODE_W) && (~color & 8)) color = rand();

		pdrawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				(color & 0x08) ? 0xaa : 0x00);
	}
}


void taitol_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int dx,dy;


	/* tilemap bug? If I do this just in vh_start(), it won't work */
	tilemap_set_scrollx(ch1a_tilemap,0,0);	/* won't change at run time */

	dx = taitol_rambanks[0x73f4]|(taitol_rambanks[0x73f5]<<8);
	if (flipscreen)
		dx = ((dx & 0xfffc) | ((dx - 3) & 0x0003)) ^ 0xf;
	dy = taitol_rambanks[0x73f6];
	tilemap_set_scrollx(bg18_tilemap,0,-dx);
	tilemap_set_scrolly(bg18_tilemap,0,-dy);

	dx = taitol_rambanks[0x73fc]|(taitol_rambanks[0x73fd]<<8);
	if (flipscreen)
		dx = ((dx & 0xfffc) | ((dx - 3) & 0x0003)) ^ 0xf;
	dy = taitol_rambanks[0x73fe];
	tilemap_set_scrollx(bg19_tilemap,0,-dx);
	tilemap_set_scrolly(bg19_tilemap,0,-dy);


	tilemap_update(ALL_TILEMAPS);

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	if (cur_ctrl & 0x20)	/* display enable */
	{
		fillbitmap(priority_bitmap,0,NULL);

		tilemap_draw(bitmap,bg19_tilemap,0);

		if (cur_ctrl & 0x08)	/* sprites always over BG1 */
			tilemap_draw(bitmap,bg18_tilemap,0);
		else					/* split priority */
			tilemap_draw(bitmap,bg18_tilemap,1<<16);
		draw_sprites(bitmap);

		tilemap_draw(bitmap,ch1a_tilemap,0);
	}
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
}
