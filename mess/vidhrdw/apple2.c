/***************************************************************************

  vidhrdw/apple2.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"

/***************************************************************************/

static struct tilemap *text_tilemap;
static struct tilemap *lores_tilemap;
static struct tilemap *hires_tilemap;
static int text_videobase;
static int lores_videobase;
static int hires_videobase;

/***************************************************************************
  text
***************************************************************************/

static void apple2_text_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		0,											/* gfx */
		mess_ram[text_videobase + memory_offset],	/* character */
		12,											/* color */
		0)											/* flags */
}

static UINT32 apple2_text_getmemoryoffset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* Special Apple II addressing.  Gotta love it. */
	return (((row & 0x07) << 7) | ((row & 0x18) * 5 + col));
}

static void apple2_text_draw(struct mame_bitmap *bitmap, int page, const struct rectangle *cliprect)
{
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;
	int new_videobase;

	if (page == 0)
		new_videobase = 0x400 + auxram;
	else
		new_videobase = 0x800 + auxram;

	if (new_videobase != text_videobase)
	{
		text_videobase = new_videobase;
		tilemap_mark_all_tiles_dirty(text_tilemap);
	}
	tilemap_draw(bitmap, cliprect, text_tilemap, 0, 0);
}

/***************************************************************************
  low resolution graphics
***************************************************************************/

static void apple2_lores_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		2 + (memory_offset & 0x01),							/* gfx */
		mess_ram[lores_videobase + memory_offset] & 0x7f,	/* character */
		12,													/* color */
		0)													/* flags */
}

static void apple2_lores_draw(struct mame_bitmap *bitmap, int page, const struct rectangle *cliprect)
{
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;
	int new_videobase;

	if (page == 0)
		new_videobase = 0x400 + auxram;
	else
		new_videobase = 0x800 + auxram;

	if (new_videobase != lores_videobase)
	{
		lores_videobase = new_videobase;
		tilemap_mark_all_tiles_dirty(lores_tilemap);
	}
	tilemap_draw(bitmap, cliprect, lores_tilemap, 0, 0);
}

/***************************************************************************
  high resolution graphics
***************************************************************************/

static void apple2_hires_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		5,													/* gfx */
		mess_ram[hires_videobase + memory_offset] & 0x7f,	/* character */
		12,													/* color */
		0)													/* flags */
}

static UINT32 apple2_hires_getmemoryoffset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* Special Apple II addressing.  Gotta love it. */
	return apple2_text_getmemoryoffset(col, row / 8, num_cols, num_rows) | ((row & 7) << 10);
}

static void apple2_hires_draw(struct mame_bitmap *bitmap, int page, const struct rectangle *cliprect)
{
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;
	int new_videobase;

	if (page == 0)
		new_videobase = 0x2000 + auxram;
	else
		new_videobase = 0x4000 + auxram;

	if (new_videobase != hires_videobase)
	{
		hires_videobase = new_videobase;
		tilemap_mark_all_tiles_dirty(hires_tilemap);
	}
	tilemap_draw(bitmap, cliprect, hires_tilemap, 0, 0);
}

/***************************************************************************
  video core
***************************************************************************/

VIDEO_START( apple2 )
{
	text_tilemap = tilemap_create(
		apple2_text_gettileinfo,
		apple2_text_getmemoryoffset,
		TILEMAP_OPAQUE,
		7*2, 8,
		40, 24);

	hires_tilemap = tilemap_create(
		apple2_hires_gettileinfo,
		apple2_hires_getmemoryoffset,
		TILEMAP_OPAQUE,
		14, 1,
		40, 192);

	lores_tilemap = tilemap_create(
		apple2_lores_gettileinfo,
		apple2_text_getmemoryoffset,
		TILEMAP_OPAQUE,
		14, 8,
		40, 24);

	if (!text_tilemap || !hires_tilemap || !lores_tilemap)
		return 1;

	text_videobase = lores_videobase = hires_videobase = 0;
	return 0;
}

VIDEO_UPDATE( apple2 )
{
    struct rectangle mixed_rect;
	int page;

    mixed_rect.min_x=0;
    mixed_rect.max_x=280*2-1;
    mixed_rect.min_y=0;
    mixed_rect.max_y=(160*2)-1;

	page = (a2.PAGE2>>7);

	if (a2.TEXT)
	{
		apple2_text_draw(bitmap, page, &Machine->visible_area);
	}
	else if ((a2.HIRES) && (a2.MIXED))
	{
		apple2_hires_draw(bitmap, page, &mixed_rect);

		mixed_rect.min_y = 160*2;
	    mixed_rect.max_y = 192*2-1;
		apple2_text_draw(bitmap, page, &mixed_rect);
	}
	else if (a2.HIRES)
	{
		apple2_hires_draw(bitmap, page, &Machine->visible_area);
	}
	else if (a2.MIXED)
	{
		apple2_lores_draw(bitmap, page, &mixed_rect);
	    mixed_rect.min_y = 160*2;
	    mixed_rect.max_y = 192*2-1;
		apple2_text_draw(bitmap, page, &mixed_rect);
	}
	else
	{
		apple2_lores_draw(bitmap, page, &Machine->visible_area);
	}
}

void apple2_video_touch(offs_t offset)
{
	if (offset >= text_videobase)
		tilemap_mark_tile_dirty(text_tilemap, offset - text_videobase);
	if (offset >= lores_videobase)
		tilemap_mark_tile_dirty(lores_tilemap, offset - lores_videobase);
	if (offset >= hires_videobase)
		tilemap_mark_tile_dirty(hires_tilemap, offset - hires_videobase);
}

