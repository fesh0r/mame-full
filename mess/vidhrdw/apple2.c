/***************************************************************************

  vidhrdw/apple2.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"

/***************************************************************************/

static UINT8 *artifact_map;
static struct tilemap *text_tilemap;
static struct tilemap *lores_tilemap;
static struct tilemap *hires_tilemap;
static int text_videobase;
static int lores_videobase;
static int hires_videobase;

/***************************************************************************
  helpers
***************************************************************************/

static void apple2_draw_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect,
	struct tilemap *tm, int raw_videobase, int *tm_videobase)
{
	if (a2.RAMRD)
		raw_videobase += 0x10000;

	if (raw_videobase != *tm_videobase)
	{
		*tm_videobase = raw_videobase;
		tilemap_mark_all_tiles_dirty(tm);
	}
	tilemap_draw(bitmap, cliprect, tm, 0, 0);
}

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
	apple2_draw_tilemap(bitmap, cliprect, text_tilemap, page ? 0x800 : 0x400, &text_videobase);
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
	apple2_draw_tilemap(bitmap, cliprect, lores_tilemap, page ? 0x800 : 0x400, &lores_videobase);
}

/***************************************************************************
  high resolution graphics
***************************************************************************/

static void apple2_hires_gettileinfo(int memory_offset)
{
	int code = mess_ram[hires_videobase + memory_offset] & 0x7f;
	tile_info.tile_number = code;
	tile_info.pen_data = artifact_map + (code * 14);
	tile_info.pal_data = Machine->pens;
	tile_info.pen_usage = 0;
	tile_info.flags = 0;
}

static UINT32 apple2_hires_getmemoryoffset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* Special Apple II addressing.  Gotta love it. */
	return apple2_text_getmemoryoffset(col, row / 8, num_cols, num_rows) | ((row & 7) << 10);
}

static void apple2_hires_draw(struct mame_bitmap *bitmap, int page, const struct rectangle *cliprect)
{
	apple2_draw_tilemap(bitmap, cliprect, hires_tilemap, page ? 0x4000 : 0x2000, &hires_videobase);
}

/***************************************************************************
  video core
***************************************************************************/

VIDEO_START( apple2 )
{
	int i, j;

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

	artifact_map = auto_malloc((128 * 14) * sizeof(*artifact_map));

	if (!text_tilemap || !hires_tilemap || !lores_tilemap || !artifact_map)
		return 1;

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 7; j++)
		{
			artifact_map[(i*14)+(j*2)+0] = (i & (1 << j)) ? 12 : 0;
			artifact_map[(i*14)+(j*2)+1] = (i & (1 << j)) ? 12 : 0;
		}
	}

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

