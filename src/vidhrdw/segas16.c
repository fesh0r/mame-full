/***************************************************************************

	Sega System 16B hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/res_net.h"



#define PRINT_UNUSUAL_MODES		(0)


/*************************************
 *
 *	Globals
 *
 *************************************/

data16_t *system16_tileram;
data16_t *system16_textram;
data16_t *system16_spriteram;



/*************************************
 *
 *	Statics
 *
 *************************************/

static struct tilemap *tilemaps[16];
static struct tilemap *textmap;

static UINT8 tilemap_page;

static UINT8 tile_bank[2];

static const UINT8 default_banklist[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const UINT8 alternate_banklist[] = { 255,255,255,255, 255,255,255,3, 255,255,255,2, 255,1,0,255 };
static const UINT8 *banklist;

static UINT8 draw_enable;
static UINT8 screen_flip;

static UINT8 normal_pal[32];
static UINT8 shadow_pal[32];
static UINT8 hilight_pal[32];



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void get_tile_info(int tile_index);
static void get_tile_info_timscanr(int tile_index);
static void get_text_info(int tile_index);
static void get_text_info_timscanr(int tile_index);
static void compute_palette_tables(void);



/*************************************
 *
 *	Video startup
 *
 *************************************/

static int video_start_common(void (*tilecb)(int), void (*textcb)(int))
{
	int pagenum;

	/* create the tilemap for the text layer */
	textmap = tilemap_create(textcb, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,28);
	if (!textmap)
		return 1;

	/* configure it */
	tilemap_set_transparent_pen(textmap, 0);
	tilemap_set_scrolldx(textmap, -24*8, -24*8);
	tilemap_set_scrollx(textmap, 0, 0);

	/* create the tilemaps for the tile pages */
	for (pagenum = 0; pagenum < 16; pagenum++)
	{
		/* each page is 64x32 */
		tilemaps[pagenum] = tilemap_create(tilecb, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32);
		if (!tilemaps[pagenum])
			return 1;

		/* configure the tilemap */
		tilemap_set_transparent_pen(tilemaps[pagenum], 0);
		tilemap_mark_all_tiles_dirty(tilemaps[pagenum]);
	}

	/* initialize globals */
	draw_enable = 1;
	screen_flip = 0;

	/* for ROM boards without banking, set up the tile banks to be 0 and 1 */
	tile_bank[0] = 0;
	tile_bank[1] = 1;

	/* compute palette info */
	compute_palette_tables();
	return 0;
}


VIDEO_START( system16b )
{
	return video_start_common(get_tile_info, get_text_info);
}


VIDEO_START( timscanr )
{
	return video_start_common(get_tile_info_timscanr, get_text_info_timscanr);
}



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_tile_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-??------------- Unknown
	---b------------ Tile bank select (0-1)
	---ccccccc------ Palette (0-127)
	----nnnnnnnnnnnn Tile index (0-4095)
*/
	UINT16 data = system16_tileram[tilemap_page * (64*32) + tile_index];
	int bank = tile_bank[(data >> 12) & 1];
	int color = (data >> 6) & 0x7f;
	int code = data & 0x0fff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}


static void get_tile_info_timscanr(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-??------------- Unknown
	---b------------ Tile bank select (0-1)
	----ccccccc----- Palette (0-127)
	----nnnnnnnnnnnn Tile index (0-4095)
*/
	UINT16 data = system16_tileram[tilemap_page * (64*32) + tile_index];
	int bank = tile_bank[(data >> 12) & 1];
	int color = (data >> 5) & 0x7f;
	int code = data & 0x0fff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Textmap callbacks
 *
 *************************************/

static void get_text_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-???------------ Unknown
	----ccc--------- Palette (0-7)
	-------nnnnnnnnn Tile index (0-511)
*/
	UINT16 data = system16_textram[tile_index];
	int bank = tile_bank[0];
	int color = (data >> 9) & 0x07;
	int code = data & 0x1ff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}


static void get_text_info_timscanr(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-????----------- Unknown
	-----ccc-------- Palette (0-7)
	--------nnnnnnnn Tile index (0-255)
*/
	UINT16 data = system16_textram[tile_index];
	int bank = tile_bank[0];
	int color = (data >> 8) & 0x07;
	int code = data & 0xff;

	SET_TILE_INFO(0, bank * 0x1000 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Miscellaneous setters
 *
 *************************************/

void system16_set_draw_enable(int enable)
{
	force_partial_update(cpu_getscanline());
	draw_enable = enable;
}


void system16_set_screen_flip(int flip)
{
	force_partial_update(cpu_getscanline());
	screen_flip = flip;
}


void system16_configure_sprite_banks(int use_default)
{
	if (use_default)
		banklist = default_banklist;
	else
		banklist = alternate_banklist;
}


void system16_set_tile_bank(int which, int bank)
{
	int i;

	if (bank != tile_bank[which])
	{
		force_partial_update(cpu_getscanline());
		tile_bank[which] = bank;

		/* mark all tilemaps dirty */
		for (i = 0; i < 16; i++)
			tilemap_mark_all_tiles_dirty(tilemaps[i]);

		/* if this is bank 0, also mark the text tilemap dirty */
		if (which == 0)
			tilemap_mark_all_tiles_dirty(textmap);
	}
}



/*************************************
 *
 *	Palette computation
 *
 *************************************/

/*
	Color generation details

	Each color is made up of 5 bits, connected through one or more resistors like so:

	Bit 0 = 1 x 3.9K ohm
	Bit 1 = 1 x 2.0K ohm
	Bit 2 = 1 x 1.0K ohm
	Bit 3 = 2 x 1.0K ohm
	Bit 4 = 4 x 1.0K ohm

	Another data bit is connected by a tristate buffer to the color output through a
	470 ohm resistor. The buffer allows the resistor to have no effect (tristate),
	halve brightness (pull-down) or double brightness (pull-up). The data bit source
	is bit 15 of each color RAM entry.
*/

static void compute_palette_tables(void)
{
	static const int resistances_normal[6] = { 3900, 2000, 1000, 1000/2, 1000/4, 0   };
	static const int resistances_sh[6]     = { 3900, 2000, 1000, 1000/2, 1000/4, 470 };
	double weights[2][6];
	int i;

	/* compute weight table for regular palette entries */
	compute_resistor_weights(0, 255, -1.0,
		6, resistances_normal, weights[0], 0, 0,
		0, NULL, NULL, 0, 0,
		0, NULL, NULL, 0, 0);

	/* compute weight table for shadow/hilight palette entries */
	compute_resistor_weights(0, 255, -1.0,
		6, resistances_sh, weights[1], 0, 0,
		0, NULL, NULL, 0, 0,
		0, NULL, NULL, 0, 0);

	/* compute R, G, B for each weight */
	for (i = 0; i < 32; i++)
	{
		int i4 = (i >> 4) & 1;
		int i3 = (i >> 3) & 1;
		int i2 = (i >> 2) & 1;
		int i1 = (i >> 1) & 1;
		int i0 = (i >> 0) & 1;

		normal_pal[i] = combine_6_weights(weights[0], i0, i1, i2, i3, i4, 0);
		shadow_pal[i] = combine_6_weights(weights[1], i0, i1, i2, i3, i4, 0);
		hilight_pal[i] = combine_6_weights(weights[1], i0, i1, i2, i3, i4, 1);
	}
}



/*************************************
 *
 *	Palette accessors
 *
 *************************************/

WRITE16_HANDLER( system16_paletteram_w )
{
	data16_t newval;
	int r, g, b;

	/* get the new value */
	newval = paletteram16[offset];
	COMBINE_DATA(&newval);
	paletteram16[offset] = newval;

	/*	   byte 0    byte 1 */
	/*	sBGR BBBB GGGG RRRR */
	/*	x000 4321 4321 4321 */
	r = ((newval >> 12) & 0x01) | ((newval << 1) & 0x1e);
	g = ((newval >> 13) & 0x01) | ((newval >> 3) & 0x1e);
	b = ((newval >> 14) & 0x01) | ((newval >> 7) & 0x1e);

	/* normal colors */
	palette_set_color(offset,        normal_pal[r],  normal_pal[g],  normal_pal[b]);
	palette_set_color(offset + 2048, shadow_pal[r],  shadow_pal[g],  shadow_pal[b]);
	palette_set_color(offset + 4096, hilight_pal[r], hilight_pal[g], hilight_pal[b]);
}



/*************************************
 *
 *	Tilemap accessors
 *
 *************************************/

READ16_HANDLER( system16_textram_r )
{
	return system16_textram[offset];
}


WRITE16_HANDLER( system16_textram_w )
{
	/* certain ranges need immediate updates */
	if (offset >= 0xe80/2)
		force_partial_update(cpu_getscanline());

	COMBINE_DATA(&system16_textram[offset]);
	tilemap_mark_tile_dirty(textmap, offset);
}


READ16_HANDLER( system16_tileram_r )
{
	return system16_tileram[offset];
}


WRITE16_HANDLER( system16_tileram_w )
{
	COMBINE_DATA(&system16_tileram[offset]);
	tilemap_mark_tile_dirty(tilemaps[offset / (64*32)], offset % (64*32));
}



/*************************************
 *
 *	Draw a split tilemap in up to
 *	four pieces
 *
 *************************************/

static void draw_virtual_tilemap(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT16 pages, UINT16 xscroll, UINT16 yscroll, UINT32 flags, UINT32 priority)
{
	int leftmin = -1, leftmax=1, rightmin = -1, rightmax=1;
	int topmin = -1, topmax=1, bottommin = -1, bottommax=1;
	struct rectangle pageclip;

	xscroll = (-320 - xscroll) & 0x3ff;
	yscroll = (-256 + yscroll) & 0x1ff;

	/* which half/halves of the virtual tilemap do we intersect in the X direction? */
	if (xscroll < 64*8 - Machine->drv->screen_width)
	{
		leftmin = 0;
		leftmax = Machine->drv->screen_width - 1;
		rightmin = -1;
	}
	else if (xscroll < 64*8)
	{
		leftmin = 0;
		leftmax = 64*8 - xscroll - 1;
		rightmin = leftmax + 1;
		rightmax = Machine->drv->screen_width - 1;
	}
	else if (xscroll < 128*8 - Machine->drv->screen_width)
	{
		rightmin = 0;
		rightmax = Machine->drv->screen_width - 1;
		leftmin = -1;
	}
	else
	{
		rightmin = 0;
		rightmax = 128*8 - xscroll - 1;
		leftmin = rightmax + 1;
		leftmax = Machine->drv->screen_width - 1;
	}

	/* which half/halves of the virtual tilemap do we intersect in the Y direction? */
	if (yscroll < 32*8 - Machine->drv->screen_height)
	{
		topmin = 0;
		topmax = Machine->drv->screen_height - 1;
		bottommin = -1;
	}
	else if (yscroll < 32*8)
	{
		topmin = 0;
		topmax = 32*8 - yscroll - 1;
		bottommin = topmax + 1;
		bottommax = Machine->drv->screen_height - 1;
	}
	else if (yscroll < 64*8 - Machine->drv->screen_height)
	{
		bottommin = 0;
		bottommax = Machine->drv->screen_height - 1;
		topmin = -1;
	}
	else
	{
		bottommin = 0;
		bottommax = 64*8 - yscroll - 1;
		topmin = bottommax + 1;
		topmax = Machine->drv->screen_height - 1;
	}

	/* draw the upper-left chunk */
	if (leftmin != -1 && topmin != -1)
	{
		pageclip.min_x = (leftmin < cliprect->min_x) ? cliprect->min_x : leftmin;
		pageclip.max_x = (leftmax > cliprect->max_x) ? cliprect->max_x : leftmax;
		pageclip.min_y = (topmin < cliprect->min_y) ? cliprect->min_y : topmin;
		pageclip.max_y = (topmax > cliprect->max_y) ? cliprect->max_y : topmax;
		if (pageclip.min_x <= pageclip.max_x && pageclip.min_y <= pageclip.max_y)
		{
			tilemap_page = (pages >> 12) & 0xf;
			tilemap_set_scrollx(tilemaps[tilemap_page], 0, xscroll);
			tilemap_set_scrolly(tilemaps[tilemap_page], 0, yscroll);
			tilemap_draw(bitmap, &pageclip, tilemaps[tilemap_page], flags, priority);
		}
	}

	/* draw the upper-right chunk */
	if (rightmin != -1 && topmin != -1)
	{
		pageclip.min_x = (rightmin < cliprect->min_x) ? cliprect->min_x : rightmin;
		pageclip.max_x = (rightmax > cliprect->max_x) ? cliprect->max_x : rightmax;
		pageclip.min_y = (topmin < cliprect->min_y) ? cliprect->min_y : topmin;
		pageclip.max_y = (topmax > cliprect->max_y) ? cliprect->max_y : topmax;
		if (pageclip.min_x <= pageclip.max_x && pageclip.min_y <= pageclip.max_y)
		{
			tilemap_page = (pages >> 8) & 0xf;
			tilemap_set_scrollx(tilemaps[tilemap_page], 0, xscroll);
			tilemap_set_scrolly(tilemaps[tilemap_page], 0, yscroll);
			tilemap_draw(bitmap, &pageclip, tilemaps[tilemap_page], flags, priority);
		}
	}

	/* draw the lower-left chunk */
	if (leftmin != -1 && bottommin != -1)
	{
		pageclip.min_x = (leftmin < cliprect->min_x) ? cliprect->min_x : leftmin;
		pageclip.max_x = (leftmax > cliprect->max_x) ? cliprect->max_x : leftmax;
		pageclip.min_y = (bottommin < cliprect->min_y) ? cliprect->min_y : bottommin;
		pageclip.max_y = (bottommax > cliprect->max_y) ? cliprect->max_y : bottommax;
		if (pageclip.min_x <= pageclip.max_x && pageclip.min_y <= pageclip.max_y)
		{
			tilemap_page = (pages >> 4) & 0xf;
			tilemap_set_scrollx(tilemaps[tilemap_page], 0, xscroll);
			tilemap_set_scrolly(tilemaps[tilemap_page], 0, yscroll);
			tilemap_draw(bitmap, &pageclip, tilemaps[tilemap_page], flags, priority);
		}
	}

	/* draw the lower-right chunk */
	if (rightmin != -1 && bottommin != -1)
	{
		pageclip.min_x = (rightmin < cliprect->min_x) ? cliprect->min_x : rightmin;
		pageclip.max_x = (rightmax > cliprect->max_x) ? cliprect->max_x : rightmax;
		pageclip.min_y = (bottommin < cliprect->min_y) ? cliprect->min_y : bottommin;
		pageclip.max_y = (bottommax > cliprect->max_y) ? cliprect->max_y : bottommax;
		if (pageclip.min_x <= pageclip.max_x && pageclip.min_y <= pageclip.max_y)
		{
			tilemap_page = (pages >> 0) & 0xf;
			tilemap_set_scrollx(tilemaps[tilemap_page], 0, xscroll);
			tilemap_set_scrolly(tilemaps[tilemap_page], 0, yscroll);
			tilemap_draw(bitmap, &pageclip, tilemaps[tilemap_page], flags, priority);
		}
	}
}



/*************************************
 *
 *	Draw a single tilemap layer
 *
 *************************************/

static void draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int which, int flags, int priority)
{
	UINT16 xscroll, yscroll, pages;
	int x, y;

	/* foreground layer */
	xscroll = system16_textram[0xe98/2 + which];
	yscroll = system16_textram[0xe90/2 + which];
	pages = system16_textram[0xe80/2 + which];

	/* column AND row scroll */
	if ((xscroll & 0x8000) && (yscroll & 0x8000))
	{
		if (PRINT_UNUSUAL_MODES) printf("Column AND row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowcolclip;

			/* adjust to clip this row only */
			rowcolclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowcolclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* loop over column chunks */
			for (x = cliprect->min_x & ~15; x <= cliprect->max_x; x += 16)
			{
				UINT16 effxscroll, effyscroll;
				UINT16 effpages = pages;

				/* adjust to clip this column only */
				rowcolclip.min_x = (x < cliprect->min_x) ? cliprect->min_x : x;
				rowcolclip.max_x = (x + 15 > cliprect->max_x) ? cliprect->max_x : x + 15;

				/* get the effective scroll values */
				effxscroll = system16_textram[0xf80/2 + 0x40/2 * which + y/8];
				effyscroll = system16_textram[0xf00/2 + 0x40/2 * which + x/16];

				/* are we using an alternate? */
				if (effxscroll & 0x8000)
				{
					effxscroll = system16_textram[0xe9c/2 + which];
					effyscroll = system16_textram[0xe94/2 + which];
					effpages = system16_textram[0xe84/2 + which];
				}

				/* draw the chunk */
				draw_virtual_tilemap(bitmap, &rowcolclip, effpages, effxscroll, effyscroll, flags, priority);
			}
		}
	}
	else if (yscroll & 0x8000)
	{
		if (PRINT_UNUSUAL_MODES) printf("Column scroll\n");

		/* loop over column chunks */
		for (x = cliprect->min_x & ~15; x <= cliprect->max_x; x += 16)
		{
			struct rectangle colclip = *cliprect;
			UINT16 effyscroll;

			/* adjust to clip this row only */
			colclip.min_x = (x < cliprect->min_x) ? cliprect->min_x : x;
			colclip.max_x = (x + 15 > cliprect->max_x) ? cliprect->max_x : x + 15;

			/* get the effective scroll values */
			effyscroll = system16_textram[0xf00/2 + 0x40/2 * which + x/16];

			/* draw the chunk */
			draw_virtual_tilemap(bitmap, &colclip, pages, xscroll, effyscroll, flags, priority);
		}
	}
	else if (xscroll & 0x8000)
	{
		if (PRINT_UNUSUAL_MODES) printf("Row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowclip = *cliprect;
			UINT16 effxscroll, effyscroll;
			UINT16 effpages = pages;

			/* adjust to clip this row only */
			rowclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* get the effective scroll values */
			effxscroll = system16_textram[0xf80/2 + 0x40/2 * which + y/8];
			effyscroll = yscroll;

			/* are we using an alternate? */
			if (effxscroll & 0x8000)
			{
				effxscroll = system16_textram[0xe9c/2 + which];
				effyscroll = system16_textram[0xe94/2 + which];
				effpages = system16_textram[0xe84/2 + which];
			}

			/* draw the chunk */
			draw_virtual_tilemap(bitmap, &rowclip, effpages, effxscroll, effyscroll, flags, priority);
		}
	}
	else
	{
		draw_virtual_tilemap(bitmap, cliprect, pages, xscroll, yscroll, flags, priority);
	}
}



/*************************************
 *
 *	Draw a single sprite
 *
 *************************************/

/*
	A note about zooming:

	The current implementation is a guess at how the hardware works. Hopefully
	we will eventually get some good tests run on the hardware to understand
	which rows/columns are skipped during a zoom operation.

	The ring sprites in hwchamp are excellent testbeds for the zooming.
*/

#define draw_pixel() 														\
	/* only draw if onscreen, not 0 or 15, and high enough priority */		\
	if (x >= cliprect->min_x && pix != 0 && pix != 15 && sprpri > pri[x])	\
	{																		\
		/* shadow/hilight mode? */											\
		if (color == 1024 + (0x3f << 4))									\
			dest[x] += (paletteram16[dest[x]] & 0x8000) ? 4096 : 2048;		\
																			\
		/* regular draw */													\
		else																\
			dest[x] = pix | color;											\
																			\
		/* always mark priority so no one else draws here */				\
		pri[x] = 0xff;														\
	}																		\


static void draw_one_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT16 *data)
{
	int bottom  = data[0] >> 8;
	int top     = data[0] & 0xff;
	int xpos    = (data[1] & 0x1ff) - 0xb8;
	int hide    = data[2] & 0x4000;
	int flip    = data[2] & 0x100;
	int pitch   = (INT8)(data[2] & 0xff);
	UINT16 addr = data[3];
	int bank    = banklist[(data[4] >> 8) & 0xf];
	int sprpri  = 1 << ((data[4] >> 6) & 0x3);
	int color   = 1024 + ((data[4] & 0x3f) << 4);
	int vzoom   = (data[5] >> 5) & 0x1f;
	int hzoom   = data[5] & 0x1f;
	int x, y, pix, numbanks;
	UINT16 *spritedata;

	/* initialize the end address to the start address */
	data[7] = addr;

	/* if hidden, or top greater than/equal to bottom, or invalid bank, punt */
	if (hide || (top >= bottom) || bank == 255)
		return;

	/* clamp to within the memory region size */
	numbanks = memory_region_length(REGION_GFX2) / 0x20000;
	if (numbanks)
		bank %= numbanks;
	spritedata = (UINT16 *)memory_region(REGION_GFX2) + 0x10000 * bank;

	/* reset the yzoom counter */
	data[5] &= 0x03ff;

	/* for the non-flipped case, we start one row ahead */
	if (!flip)
		addr += pitch;

	/* loop from top to bottom */
	for (y = top; y < bottom; y++)
	{
		/* skip drawing if not within the cliprect */
		if (y >= cliprect->min_y && y <= cliprect->max_y)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT8 *pri = (UINT8 *)priority_bitmap->line[y];
			int xacc = 0x20;

			/* non-flipped case */
			if (!flip)
			{
				/* start at the word before because we preincrement below */
				data[7] = addr - 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[++data[7]];

					/* draw four pixels */
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}

			/* flipped case */
			else
			{
				/* start at the word after because we predecrement below */
				data[7] = addr + pitch + 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[--data[7]];

					/* draw four pixels */
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}
		}

		/* advance a row */
		addr += pitch;

		/* accumulate zoom factors; if we carry into the high bit, skip an extra row */
		data[5] += vzoom << 10;
		if (data[5] & 0x8000)
		{
			addr += pitch;
			data[5] &= ~0x8000;
		}
	}
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 *cursprite;

	/* first scan forward to find the end of the list */
	for (cursprite = system16_spriteram; cursprite < system16_spriteram + 0xfff/2; cursprite += 8)
		if (cursprite[2] & 0x8000)
			break;

	/* now scan backwards and render the sprites in order */
	for (cursprite -= 8; cursprite >= system16_spriteram; cursprite -= 8)
		draw_one_sprite(bitmap, cliprect, cursprite);
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( system16b )
{
	/* if no drawing is happening, fill with black and get out */
	if (!draw_enable)
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* reset priorities */
	fillbitmap(priority_bitmap, 0, cliprect);

	/* draw background */
	draw_layer(bitmap, cliprect, 1, 0 | TILEMAP_IGNORE_TRANSPARENCY, 0x01);
	draw_layer(bitmap, cliprect, 1, 1 | TILEMAP_IGNORE_TRANSPARENCY, 0x02);

	/* draw foreground */
	draw_layer(bitmap, cliprect, 0, 0, 0x02);
	draw_layer(bitmap, cliprect, 0, 1, 0x04);

	/* text layer */
	tilemap_draw(bitmap, cliprect, textmap, 0, 0x04);
	tilemap_draw(bitmap, cliprect, textmap, 1, 0x08);

	/* draw the sprites */
	draw_sprites(bitmap, cliprect);
}
