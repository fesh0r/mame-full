/***************************************************************************

  vidhrdw/apple2.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"

/***************************************************************************/

static struct tilemap *text_tilemap;
static struct tilemap *lores_tilemap;
static int text_videobase;
static int lores_videobase;

#define	BLACK	0
#define PURPLE	3
#define	BLUE	6
#define ORANGE	9
#define GREEN	12
#define	WHITE	15

/***************************************************************************
  helpers
***************************************************************************/

static void apple2_draw_tilemap(struct mame_bitmap *bitmap, int beginrow, int endrow,
	struct tilemap *tm, int raw_videobase, int *tm_videobase)
{
	struct rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.max_x = 280*2-1;
	cliprect.min_y = beginrow;
	cliprect.max_y = endrow;

	if (a2.RAMRD)
		raw_videobase += 0x10000;

	if (raw_videobase != *tm_videobase)
	{
		*tm_videobase = raw_videobase;
		tilemap_mark_all_tiles_dirty(tm);
	}
	tilemap_draw(bitmap, &cliprect, tm, 0, 0);
}

/***************************************************************************
  text
***************************************************************************/

static void apple2_text_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		0,											/* gfx */
		mess_ram[text_videobase + memory_offset],	/* character */
		WHITE,										/* color */
		0)											/* flags */
}

static UINT32 apple2_text_getmemoryoffset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* Special Apple II addressing.  Gotta love it. */
	return (((row & 0x07) << 7) | ((row & 0x18) * 5 + col));
}

static void apple2_text_draw(struct mame_bitmap *bitmap, int page, int beginrow, int endrow)
{
	apple2_draw_tilemap(bitmap, beginrow, endrow, text_tilemap, page ? 0x800 : 0x400, &text_videobase);
}

/***************************************************************************
  low resolution graphics
***************************************************************************/

static void apple2_lores_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		2 + (memory_offset & 0x01),							/* gfx */
		mess_ram[lores_videobase + memory_offset] & 0x7f,	/* character */
		WHITE,												/* color */
		0)													/* flags */
}

static void apple2_lores_draw(struct mame_bitmap *bitmap, int page, int beginrow, int endrow)
{
	apple2_draw_tilemap(bitmap, beginrow, endrow, lores_tilemap, page ? 0x800 : 0x400, &lores_videobase);
}

/***************************************************************************
  high resolution graphics
***************************************************************************/

static UINT32 apple2_hires_getmemoryoffset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	/* Special Apple II addressing.  Gotta love it. */
	return apple2_text_getmemoryoffset(col, row / 8, num_cols, num_rows) | ((row & 7) << 10);
}

static void apple2_hires_draw(struct mame_bitmap *bitmap, int page, int beginrow, int endrow)
{
	int row, col, b;
	int offset;
	UINT8 *vram;
	UINT8 vram_row[42];
	UINT16 v;
	UINT16 *p;

	static int artifact_color_table[] =
	{
		BLACK,	PURPLE,	GREEN,	WHITE,
		BLACK,	BLUE,	ORANGE,	WHITE
	};

	vram = mess_ram + (page ? 0x4000 : 0x2000);
	if (a2.RAMRD)
		vram += 0x10000;

	vram_row[0] = 0;
	vram_row[41] = 0;

	for (row = beginrow; row <= endrow; row++)
	{
		for (col = 0; col < 40; col++)
		{
			offset = apple2_hires_getmemoryoffset(col, row, 0, 0);
			vram_row[1+col] = vram[offset];
		}

		p = (UINT16 *) bitmap->line[row];

		for (col = 0; col < 40; col++)
		{
			for (b = 0; b < 7; b++)
			{
				int current = (vram_row[col+1] & (1 << b));
				int before = (b == 6) ? (vram_row[col+2] & 1) : (vram_row[col+1] & (1 << (b+1)));
				int after = (b == 0) ? (vram_row[col+0] & 0x40) : (vram_row[col+1] & (1 << (b-1)));

				if (current)
					if (before || after)
						v = 3;
					else
						v = ((col ^ b) & 1) ? 2 : 1;
				else
					if (before && after)
						v = ((col ^ b) & 1) ? 1 : 2;
					else
						v = 0;

				if (vram_row[col+1] & 0x80)
					v += 4;

				v = artifact_color_table[v];
				*(p++) = v;
				*(p++) = v;
			}
		}
	}
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

	lores_tilemap = tilemap_create(
		apple2_lores_gettileinfo,
		apple2_text_getmemoryoffset,
		TILEMAP_OPAQUE,
		14, 8,
		40, 24);

	if (!text_tilemap || !lores_tilemap)
		return 1;

	text_videobase = lores_videobase = 0;
	return 0;
}

VIDEO_UPDATE( apple2 )
{
	int page;

	page = (a2.PAGE2>>7);

	if (a2.TEXT)
	{
		apple2_text_draw(bitmap, page, 0, 191);
	}
	else if ((a2.HIRES) && (a2.MIXED))
	{
		apple2_hires_draw(bitmap, page, 0, 159);
		apple2_text_draw(bitmap, page, 160, 191);
	}
	else if (a2.HIRES)
	{
		apple2_hires_draw(bitmap, page, 0, 191);
	}
	else if (a2.MIXED)
	{
		apple2_lores_draw(bitmap, page, 0, 159);
		apple2_text_draw(bitmap, page, 160, 191);
	}
	else
	{
		apple2_lores_draw(bitmap, page, 0, 191);
	}
}

void apple2_video_touch(offs_t offset)
{
	if (offset >= text_videobase)
		tilemap_mark_tile_dirty(text_tilemap, offset - text_videobase);
	if (offset >= lores_videobase)
		tilemap_mark_tile_dirty(lores_tilemap, offset - lores_videobase);
}

