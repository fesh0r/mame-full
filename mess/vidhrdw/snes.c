/***************************************************************************

snes.c

Video file to handle emulation of the Nintendo Super NES.

Anthony Kruize
Based on the original code by Lee Hammerton (aka Savoury Snax)

Some notes on the snes video hardware:

Object Attribute Memory(OAM) is made up of 128 blocks of 32 bits, followed
by 128 blocks of 2 bits. The format for each block is:
-First Block----------------------------------------------------------------
| x pos  | y pos  |char no.| v flip | h flip |priority|palette |char no msb|
+--------+--------+--------+--------+--------+--------+--------+-----------+
| 8 bits | 8 bits | 8 bits | 1 bit  | 1 bit  | 2 bits | 3 bits | 1 bit     |
-Second Block---------------------------------------------------------------
| size  | x pos msb |
+-------+-----------+
| 1 bit | 1 bit     |
---------------------

Video RAM contains information for character data and screen maps.
Screen maps are made up of 32 x 32 blocks of 16 bits each.
The format for each block is:
----------------------------------------------
| v flip | x flip |priority|palette |char no.|
+--------+--------+--------+--------+--------+
| 1 bit  | 1 bit  | 1 bit  | 3 bits |10 bits |
----------------------------------------------
Mode 7 is stored differently. Character data and screen map are interleaved.
There are two formats:
-Normal-----------------  -EXTBG-----------------------------
| char data | char no. |  | priority | char data | char no. |
+-----------+----------+  +----------+-----------+----------+
| 8 bits    | 8 bits   |  | 1 bit    | 7 bits    | 8 bits   |
------------------------  -----------------------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"

#define MAINSCREEN      0
#define SUBSCREEN       1
#define SNES_BLEND_NONE 0
#define SNES_BLEND_ADD  1
#define SNES_BLEND_SUB  2
#define SNES_CLIP_NONE  0
#define SNES_CLIP_1     1
#define SNES_CLIP_2     2
#define SNES_CLIP_1_OUT 4
#define SNES_CLIP_2_OUT 8

#ifdef MAME_DEBUG
struct DEBUGOPTS
{
UINT8 input_count;
UINT8 bg_disabled[5];
};
static struct DEBUGOPTS debug_options  = {5, {0,0,0,0}};
#endif

UINT8  ppu_obj_size[2];					/* Object sizes */

/* Lookup tables */
static const UINT8  table_bgd_pty[4][2] = { {5,7}, {4,6}, {1,3}, {0,2} };
static const UINT8  table_obj_pty[4]    = { 2, 4, 6, 8 };
static const UINT16 table_obj_offset[8][8] =
{
	{ (0*32),   (0*32)+32,   (0*32)+64,   (0*32)+96,   (0*32)+128,   (0*32)+160,   (0*32)+192,   (0*32)+224 },
	{ (16*32),  (16*32)+32,  (16*32)+64,  (16*32)+96,  (16*32)+128,  (16*32)+160,  (16*32)+192,  (16*32)+224 },
	{ (32*32),  (32*32)+32,  (32*32)+64,  (32*32)+96,  (32*32)+128,  (32*32)+160,  (32*32)+192,  (32*32)+224 },
	{ (48*32),  (48*32)+32,  (48*32)+64,  (48*32)+96,  (48*32)+128,  (48*32)+160,  (48*32)+192,  (48*32)+224 },
	{ (64*32),  (64*32)+32,  (64*32)+64,  (64*32)+96,  (64*32)+128,  (64*32)+160,  (64*32)+192,  (64*32)+224 },
	{ (80*32),  (80*32)+32,  (80*32)+64,  (80*32)+96,  (80*32)+128,  (80*32)+160,  (80*32)+192,  (80*32)+224 },
	{ (96*32),  (96*32)+32,  (96*32)+64,  (96*32)+96,  (96*32)+128,  (96*32)+160,  (96*32)+192,  (96*32)+224 },
	{ (112*32), (112*32)+32, (112*32)+64, (112*32)+96, (112*32)+128, (112*32)+160, (112*32)+192, (112*32)+224 }
};
static const UINT16 table_hscroll[4][4] = { {0,0,0,0}, {0,0x800,0,0x800}, {0,0,0,0}, {0,0x800,0,0x800} };
static const UINT16 table_vscroll[4][4] = { {0,0,0,0}, {0,0,0,0}, {0,0x800,0,0x800}, {0,0x1000,0,0x1000} };

struct SCANLINE
{
	UINT16 buffer[SNES_SCR_WIDTH + 8];
	UINT8  zbuf[SNES_SCR_WIDTH + 8];
};

struct LAYER
{
	UINT8 blend;
	UINT8 clip;
	UINT32 data;
	UINT32 map;
};

static struct SCANLINE scanlines[2];
static struct LAYER layers[5];

/* Routine for additive/subtractive blending between the main and sub screens */
#define SNES_DRAW_BLEND( offset, colour, mode )														\
	{																								\
		UINT16 r, g, b;																				\
		if( mode == SNES_BLEND_ADD )																\
		{																							\
			r = (colour & 0x1f) + (scanlines[SUBSCREEN].buffer[offset] & 0x1f);						\
			g = ((colour & 0x3e0) >> 5) + ((scanlines[SUBSCREEN].buffer[offset] & 0x3e0) >> 5);		\
			b = ((colour & 0x7c00) >> 10) + ((scanlines[SUBSCREEN].buffer[offset] & 0x7c00) >> 10);	\
			if( r > 0x1f || g > 0x1f || b > 0x1f )													\
			{																						\
				r >>= 1;																			\
				g >>= 1;																			\
				b >>= 1;																			\
			}																						\
			colour = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));							\
		}																							\
		else if( mode == SNES_BLEND_SUB )															\
		{																							\
			r = (colour & 0x1f) - (scanlines[SUBSCREEN].buffer[offset] & 0x1f);						\
			g = ((colour & 0x3e0) >> 5) - ((scanlines[SUBSCREEN].buffer[offset] & 0x3e0) >> 5);		\
			b = ((colour & 0x7c00) >> 10) - ((scanlines[SUBSCREEN].buffer[offset] & 0x7c00) >> 10);	\
			if( r > 0x1f ) r = 0;																	\
			if( g > 0x1f ) g = 0;																	\
			if( b > 0x1f ) b = 0;																	\
			colour = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));							\
		}																							\
	}

/* Routine for clipping to the windows */
/* FIXME: Only supports OR mode, needs to handle AND/XOR/XNOR modes etc */
/* FIXME: Needs to differentiate between main and sub screens */
#define SNES_DRAW_CLIP( offset, colour, clip )								\
	{																		\
		if( clip & SNES_CLIP_1 )											\
		{																	\
			if( clip & SNES_CLIP_1_OUT )									\
			{																\
				if( offset <= snes_ram[WH0] || offset >= snes_ram[WH1] )	\
				{															\
					colour = 0;												\
				}															\
			}																\
			else															\
			{																\
				if( offset >= snes_ram[WH0] && offset <= snes_ram[WH1] )	\
				{															\
					colour = 0;												\
				}															\
			}																\
		}																	\
		if( clip & SNES_CLIP_2 )											\
		{																	\
			if( clip & SNES_CLIP_2_OUT )									\
			{																\
				if( offset <= snes_ram[WH2] || offset >= snes_ram[WH3] )	\
				{															\
					colour = 0;												\
				}															\
			}																\
			else															\
			{																\
				if( offset >= snes_ram[WH2] && offset <= snes_ram[WH3] )	\
				{															\
					colour = 0;												\
				}															\
			}																\
		}																	\
	}

VIDEO_UPDATE( snes )
{
}

/*****************************************
 * snes_draw_tile_2()
 *
 * Draw tiles with 2 bit planes(4 colors)
 *****************************************/
INLINE void snes_draw_tile_2( UINT8 screen, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT16 pal, UINT8 blend, UINT8 clip )
{
	UINT8 ii;
	UINT8 plane[2];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & 0x1 ? 1 : 0) | (plane[1] & 0x1 ? 2 : 0);
			plane[0] >>= 1;
			plane[1] >>= 1;
		}
		else
		{
			colour = (plane[0] & 0x80 ? 1 : 0) | (plane[1] & 0x80 ? 2 : 0);
			plane[0] <<= 1;
			plane[1] <<= 1;
		}
		/* Clip to the windows */
/*		SNES_DRAW_CLIP( x + ii, colour, clip );*/

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( scanlines[screen].zbuf[x+ii] <= priority )
			{
				c = Machine->remapped_colortable[pal + colour];
				SNES_DRAW_BLEND( x+ii, c, blend );
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
		else if( !scanlines[screen].zbuf[x+ii] )
		{
			/* FIXME: This is a hack for Super Mario World! */
			scanlines[screen].buffer[x+ii] = scanlines[SUBSCREEN].buffer[x+ii];
		}
	}
}

/*****************************************
 * snes_draw_tile_4()
 *
 * Draw tiles with 4 bit planes(16 colors)
 *****************************************/
INLINE void snes_draw_tile_4( UINT8 screen, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT16 pal, UINT8 blend, UINT8 clip )
{
	UINT8 ii;
	UINT8 plane[4];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & 0x1 ? 1 : 0) | (plane[1] & 0x1 ? 2 : 0) |
					 (plane[2] & 0x1 ? 4 : 0) | (plane[3] & 0x1 ? 8 : 0);
			plane[0] >>= 1;
			plane[1] >>= 1;
			plane[2] >>= 1;
			plane[3] >>= 1;
		}
		else
		{
			colour = (plane[0] & 0x80 ? 1 : 0) | (plane[1] & 0x80 ? 2 : 0) |
					 (plane[2] & 0x80 ? 4 : 0) | (plane[3] & 0x80 ? 8 : 0);
			plane[0] <<= 1;
			plane[1] <<= 1;
			plane[2] <<= 1;
			plane[3] <<= 1;
		}
		/* Clip to the windows */
/*		SNES_DRAW_CLIP( x + ii, colour, clip );*/

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( scanlines[screen].zbuf[x+ii] <= priority )
			{
				c = Machine->remapped_colortable[pal + colour];
				SNES_DRAW_BLEND( x+ii, c, blend );	/* Blend with the subscreen */
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*****************************************
 * snes_draw_tile_8()
 *
 * Draw tiles with 8 bit planes(256 colors)
 *****************************************/
INLINE void snes_draw_tile_8( UINT8 screen, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT8 blend, UINT8 clip )
{
	UINT8 ii;
	UINT8 plane[8];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];
	plane[4] = snes_vram[tileaddr + 32];
	plane[5] = snes_vram[tileaddr + 33];
	plane[6] = snes_vram[tileaddr + 48];
	plane[7] = snes_vram[tileaddr + 49];

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & 0x1 ? 1 : 0)  | (plane[1] & 0x1 ? 2 : 0)  |
					 (plane[2] & 0x1 ? 4 : 0)  | (plane[3] & 0x1 ? 8 : 0)  |
					 (plane[4] & 0x1 ? 16 : 0) | (plane[5] & 0x1 ? 32 : 0) |
					 (plane[6] & 0x1 ? 64 : 0) | (plane[7] & 0x1 ? 128 : 0);
			plane[0] >>= 1;
			plane[1] >>= 1;
			plane[2] >>= 1;
			plane[3] >>= 1;
			plane[4] >>= 1;
			plane[5] >>= 1;
			plane[6] >>= 1;
			plane[7] >>= 1;
		}
		else
		{
			colour = (plane[0] & 0x80 ? 1 : 0)  | (plane[1] & 0x80 ? 2 : 0)  |
					 (plane[2] & 0x80 ? 4 : 0)  | (plane[3] & 0x80 ? 8 : 0)  |
					 (plane[4] & 0x80 ? 16 : 0) | (plane[5] & 0x80 ? 32 : 0) |
					 (plane[6] & 0x80 ? 64 : 0) | (plane[7] & 0x80 ? 128 : 0);
			plane[0] <<= 1;
			plane[1] <<= 1;
			plane[2] <<= 1;
			plane[3] <<= 1;
			plane[4] <<= 1;
			plane[5] <<= 1;
			plane[6] <<= 1;
			plane[7] <<= 1;
		}
		/* Clip to the windows */
/*		SNES_DRAW_CLIP( x + ii, colour, clip );*/

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( scanlines[screen].zbuf[x+ii] <= priority )
			{
				c = Machine->remapped_colortable[colour];
				SNES_DRAW_BLEND( x+ii, c, blend );
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*********************************************
 * snes_update_line_2()
 *
 * Update an entire line of 2 bit plane tiles.
 *********************************************/
static void snes_update_line_2( UINT8 screen, UINT8 layer, UINT16 curline, UINT8 bg3_pty )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll;
	UINT8 vshift, hshift, map_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Find the size of the map */
	map_size = snes_ram[0x2107 + layer] & 0x3;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> 3;
	vshift = bg_voffset[layer] & 0x7;
	hscroll = (bg_hoffset[layer] & 0x3ff) >> 3;
	hshift = bg_hoffset[layer] & 0x7;

	/*  1k words */
	tilemap = layers[layer].map + ((curline >> 3) << 6);
	/* scroll */
	tilemap += table_vscroll[map_size][vscroll >> 5];

	/* Have we scrolled into the next map? */
	if( curline >= 256 - (bg_voffset[layer] & 0xff) )
	{
		tilemap = layers[layer].map + table_vscroll[map_size][(vscroll >> 5) + 1];
		vscroll = ((curline >> 3) + (vscroll & 0x1f)) - 32;
	}
	/* figure out which line to draw */
	line = (curline % 8) + vshift;
	if( line > 7 )	/* scrolled into the next tile */
	{
		vscroll++;	/* pretend we scrolled by 1 line */
		line -= 8;
	}
	/* scroll vertically */
	tilemap += (vscroll & 0x1f) << 6;
	/* Remember the location for later */
	basevmap = tilemap;

	/* scroll horizontally */
	tilemap += table_hscroll[map_size][hscroll >> 5];
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < 66; ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* 8 palettes of 4 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		tile_line = line;
		if( vflip )
			tile_line = -line + 7;
		tile_line <<= 1;

		/* Special case for bg3 */
		if( layer == 2 && bg3_pty )
			priority = 9;

		snes_draw_tile_2( screen, layers[layer].data + (tile << 4) + tile_line, ((ii >> 1) * 8) - hshift, priority, hflip, pal, layers[layer].blend, layers[layer].clip );
	}
}

/*********************************************
 * snes_update_line_4()
 *
 * Update an entire line of 4 bit plane tiles.
 *********************************************/
static void snes_update_line_4( UINT8 screen, UINT8 layer, UINT16 curline )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll;
	UINT8 vshift, hshift, map_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Find the size of the map */
	map_size = snes_ram[0x2107 + layer] & 0x3;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> 3;
	vshift = bg_voffset[layer] & 0x7;
	hscroll = (bg_hoffset[layer] & 0x3ff) >> 3;
	hshift = bg_hoffset[layer] & 0x7;

	/*  1k words */
	tilemap = layers[layer].map + ((curline >> 3) << 6);
	/* scroll */
	tilemap += table_vscroll[map_size][vscroll >> 5];

	/* Have we scrolled into the next map? */
	if( curline >= 256 - (bg_voffset[layer] & 0xff) )
	{
		tilemap = layers[layer].map + table_vscroll[map_size][(vscroll >> 5) + 1];
		vscroll = ((curline >> 3) + (vscroll & 0x1f)) - 32;
	}
	/* figure out which line to draw */
	line = (curline % 8) + vshift;
	if( line > 7 )	/* scrolled into the next tile */
	{
		vscroll++;	/* pretend we scrolled by 1 line */
		line -= 8;
	}
	/* scroll vertically */
	tilemap += (vscroll & 0x1f) << 6;
	/* Remember the location for later */
	basevmap = tilemap;

	/* scroll horizontally */
	tilemap += table_hscroll[map_size][hscroll >> 5];
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < 66; ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = snes_vram[tilemap + ii + 1] & 0x80;
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];		/* is this even right??? */
		pal = (snes_vram[tilemap + ii + 1] & 0x1c) << 2;	/* 8 palettes of 16 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		tile_line = line;
		if( vflip )
			tile_line = -line + 7;
		tile_line <<= 1;

		snes_draw_tile_4( screen, layers[layer].data + (tile << 5) + tile_line, ((ii >> 1) * 8)  - hshift, priority, hflip, pal, layers[layer].blend, layers[layer].clip );
	}
}

/*********************************************
 * snes_update_line_8()
 *
 * Update an entire line of 8 bit plane tiles.
 *********************************************/
static void snes_update_line_8( UINT8 screen, UINT8 layer, UINT16 curline )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll;
	UINT8 vshift, hshift, map_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Find the size of the map */
	map_size = snes_ram[0x2107 + layer] & 0x3;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> 3;
	vshift = bg_voffset[layer] & 0x7;
	hscroll = (bg_hoffset[layer] & 0x3ff) >> 3;
	hshift = bg_hoffset[layer] & 0x7;

	/*  1k words */
	tilemap = layers[layer].map + ((curline >> 3) << 6);
	/* scroll */
	tilemap += table_vscroll[map_size][vscroll >> 5];

	/* Have we scrolled into the next map? */
	if( curline >= 256 - (bg_voffset[layer] & 0xff) )
	{
		tilemap = layers[layer].map + table_vscroll[map_size][(vscroll >> 5) + 1];
		vscroll = ((curline >> 3) + (vscroll & 0x1f)) - 32;
	}
	/* figure out which line to draw */
	line = (curline % 8) + vshift;
	if( line > 7 )	/* scrolled into the next tile */
	{
		vscroll++;	/* pretend we scrolled by 1 line */
		line -= 8;
	}
	/* scroll vertically */
	tilemap += (vscroll & 0x1f) << 6;
	/* Remember the location for later */
	basevmap = tilemap;

	/* scroll horizontally */
	tilemap += table_hscroll[map_size][hscroll >> 5];
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < 66; ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* what does this do for 8 bit screen? */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		tile_line = line;
		if( vflip )
			tile_line = -line + 7;
		tile_line <<= 1;

		snes_draw_tile_8( screen, layers[layer].data + (tile << 6) + tile_line, (ii >> 1) * 8, priority, hflip, layers[layer].blend, layers[layer].clip );
	}
}

/*********************************************
 * snes_update_line_mode7()
 *
 * Update an entire line of mode7 tiles.
 *********************************************/
static void snes_update_line_mode7( UINT16 curline )
{
	UINT32 tilen, tiled;
	INT16 a,b,c,d;
	INT16 xc, yc, tx, ty, sx, sy, hs, vs;
	register UINT8 colour = 0;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[0] )
	{
		return;
	}
#endif

	a = mode7_data[0];
	b = mode7_data[1];
	c = mode7_data[2];
	d = mode7_data[3];
	xc = mode7_data[4];
	yc = mode7_data[5];
	hs = bg_hoffset[0];
	vs = bg_voffset[0];

	/* Sign extend */
	xc <<= 3;
	xc >>= 3;
	yc <<= 3;
	yc >>= 3;
	hs <<= 3;
	hs >>= 3;
	vs <<= 3;
	vs >>= 3;

	sy = curline;

#if 1
	/* Let's do some mode7 drawing huh? */
	for( sx = 0; sx < 256; sx++ )
	{
		tx = (((a * ((sx + hs) - xc)) + (c * ((sy + vs) - yc))) >> 8) + xc;
		ty = (((b * ((sx + hs) - xc)) + (d * ((sy + vs) - yc))) >> 8) + yc;
		switch( snes_ram[M7SEL] & 0xc0 )
		{
			case 0x00:	/* Repeat if outside screen area */
				tx &= 0x3ff;
				ty &= 0x3ff;
				tilen = snes_vram[((tx >> 3) * 2) + ((ty >> 3) * 128 * 2)];
				tiled = tilen * 128;
				colour = snes_vram[tiled + ((ty & 0x7) * 16) + ((tx & 0x7) * 2) + 1];
				break;
			case 0x80:	/* Character 0x00 repeat if outside screen area */
				tx &= 0x3ff;
				ty &= 0x3ff;
				tilen = snes_vram[((tx >> 3) * 2) + ((ty >> 3) * 128 * 2)];
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
					tiled = tilen * 128;
				else
					tiled = 1;
				colour = snes_vram[tiled + ((ty & 0x7) * 16) + ((tx & 0x7) * 2) + 1];
				break;
			case 0xC0:	/* Single colour backdrop screen if outside screen area */
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
				{
					tilen = snes_vram[((tx >> 3) * 2) + ((ty >> 3) * 128 * 2)];
					tiled = tilen * 128;
					colour = snes_vram[tiled + ((ty & 0x7) * 16) + ((tx & 0x7) * 2) + 1];
				}
				else
				{
					tilen = 0;
					tiled = 0;
					colour = 0;
				}
				break;
		}

		if( scanlines[MAINSCREEN].zbuf[sx] < table_bgd_pty[0][0] && colour > 0 )
		{
			scanlines[MAINSCREEN].buffer[sx] = Machine->remapped_colortable[colour];
			scanlines[MAINSCREEN].zbuf[sx] = table_bgd_pty[0][0];
		}
	}

#else
	/* From the original driver */
	switch( snes_ram[M7SEL] & 0xc0 )
	{
		case 0x00:
			for( sx = 0; sx < 256; sx++ )
			{
				tx = (((a * ((sx + hs) - xc) + c * ((sy + vs) - yc)) >> 8) + xc) & 0x3ff;
				ty = (((b * ((sx + hs) - xc) + d * ((sy + vs) - yc)) >> 8) + yc) & 0x3ff;
				tilen = snes_vram[((ty / 8) * 128 * 2) + ((tx / 8) * 2)];
				tiled = 1 + (tilen * 8 * 8 * 2);
				colour = snes_vram[tiled + ((ty & 7) * 8 * 2) + ((tx & 7) * 2)];
				if( scanlines[MAINSCREEN].zbuf[sx] <= table_bgd_pty[0][1] && colour > 0 )
				{
					scanlines[MAINSCREEN].buffer[sx] = Machine->remapped_colortable[colour];
					scanlines[MAINSCREEN].zbuf[sx] = table_bgd_pty[0][1];
				}
			}
			break;
		case 0x80:
			for( sx = 0; sx < 256; sx++ )
			{
				tx = (((a * ((sx + hs) - xc) + c * ((sy + vs) - yc)) >> 8) + xc);
				ty = (((b * ((sx + hs) - xc) + d * ((sy + vs) - yc)) >> 8) + yc);
				tilen = snes_vram[(((ty & 0x3ff) / 8) * 128 * 2) + (((tx & 0x3ff) / 8) * 2)];
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
					tiled = 1 + (tilen * 8 * 8 * 2);
				else
					tiled = 1;
				colour = snes_vram[tiled + ((ty & 7) * 8 * 2) + ((tx & 7) * 2)];
				if( scanlines[MAINSCREEN].zbuf[sx] <= table_bgd_pty[0][1] && colour > 0 )
				{
					scanlines[MAINSCREEN].buffer[sx] = Machine->remapped_colortable[colour];
					scanlines[MAINSCREEN].zbuf[sx] = table_bgd_pty[0][1];
				}
			}
			break;
		case 0xc0:
			for( sx = 0; sx < 256; sx++ )
			{
				tx = (((a * ((sx + hs) - xc) + c * ((sy + vs) - yc)) >> 8) + xc);
				ty = (((b * ((sx + hs) - xc) + d * ((sy + vs) - yc)) >> 8) + yc);
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
				{
					tilen = snes_vram[((ty / 8) * 128 * 2) + ((tx / 8) * 2)];
					tiled = 1 + (tilen * 8 * 8 * 2);
					colour = snes_vram[tiled + ((ty & 7) * 8 * 2) + ((tx & 7) * 2)];
					if( scanlines[MAINSCREEN].zbuf[sx] <= table_bgd_pty[0][1] && colour > 0 )
					{
						scanlines[MAINSCREEN].buffer[sx] = Machine->remapped_colortable[colour];
						scanlines[MAINSCREEN].zbuf[sx] = table_bgd_pty[0][1];
					}
				}
			}
			break;
	}
#endif
}

/*********************************************
 * snes_update_objects()
 *
 * Update an entire line of sprites.
 *********************************************/
INLINE void snes_update_objects( UINT8 screen, UINT16 curline )
{
	INT8 xs, ys;
	UINT8 line;
	UINT16 oam, oam_extra, extra;
	UINT8 range_over = 0, time_over = 0;
	UINT8 size, vflip, hflip, priority, pal;
	UINT16 x, y, tile;
	INT16 i;
	UINT8 *oamram = (UINT8 *)snes_oam;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[4] )
	{
		return;
	}
#endif

	oam = 0x1ff;
	oam_extra = oam + 0x20;
	extra = 0;
	for( i = 128; i > 0; i-- )
	{
		if( (i % 4) == 0 )
			extra = oamram[oam_extra--];

		vflip = (oamram[oam] & 0x80) >> 7;
		hflip = (oamram[oam] & 0x40) >> 6;
		priority = table_obj_pty[(oamram[oam] & 0x30) >> 4];
		pal = 128 + ((oamram[oam] & 0xE) << 3);
		tile = (oamram[oam--] & 0x1) << 8;
		tile |= oamram[oam--];
		y = oamram[oam--];
		x = oamram[oam--];
		size = (extra & 0x80) >> 7;
		extra <<= 1;
		x |= ((extra & 0x80) << 1);
		extra <<= 1;

		if( y > 239 )
			y -= 256;	/* y is past sprite max pos */

		/* Draw sprite if it intersects the current line */
		if( curline >= y && curline < (y + (ppu_obj_size[size] << 3)) )
		{
			ys = (curline - y) >> 3;
			line = (curline - y) % 8;
			if( vflip )
			{
				ys = ppu_obj_size[size] - ys - 1;
				line = (-1 * line) + 7;
			}
			line <<= 1;
			tile <<= 5;
			if( hflip )
			{
				UINT8 count = 0;
				for( xs = (ppu_obj_size[size] - 1); xs >= 0; xs-- )
				{
					if( (x + (count << 3) < SNES_SCR_WIDTH + 8) )
						snes_draw_tile_4( screen, layers[4].data + tile + table_obj_offset[ys][xs] + line, x + (count++ << 3), priority, hflip, pal, layers[4].blend, layers[4].clip );
					time_over++;	/* Increase time_over. Should we stop drawing if exceeded 34 tiles? */
				}
			}
			else
			{
				for( xs = 0; xs < ppu_obj_size[size]; xs++ )
				{
					if( (x + (xs << 3) < SNES_SCR_WIDTH + 8) )
						snes_draw_tile_4( screen, layers[4].data + tile + table_obj_offset[ys][xs] + line, x + (xs << 3), priority, hflip, pal, layers[4].blend, layers[4].clip );
					time_over++;	/* Increase time_over. Should we stop drawing if exceeded 34 tiles? */
				}
			}

			/* Increase range_over. 
			 * Stop drawing if exceeded 32 objects and
			 * enforcing that limit is enabled */
			range_over++;
			if( range_over == 32 && (readinputport( 8 ) & 0x1) )
			{
				/* Set the flag in STAT77 register */
				snes_ram[STAT77] |= 0x40;
				return;
			}
		}
	}

	if( time_over >= 34 )
	{
		/* Set the flag in STAT77 register */
		snes_ram[STAT77] |= 0x80;
	}
}

#ifdef MAME_DEBUG
/*********************************************
 * snes_dbg_draw_maps()
 *
 * Debug to display maps
 * Doesn't handle vflip.
 *********************************************/
void snes_dbg_draw_maps( struct mame_bitmap *bitmap, UINT32 tilemap, UINT8 bpl, UINT16 curline, UINT8 layer )
{
	UINT32 tile, addr = tilemap;
	UINT16 ii, vflip, hflip, pal;
	INT8 line;
	char str[50];

	tilemap += (curline >> 3) * 64;
	for( ii = 0; ii < 64; ii += 2 )
	{
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* 8 palettes of 4 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		line = curline % 8;
		if( vflip )
			line = -line + 7;

		if( tile != 0 )
		{
			switch( bpl )
			{
				case 1:
					snes_draw_tile_2( layer, layers[layer].data + (tile << 4) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip, pal, SNES_BLEND_NONE, SNES_CLIP_NONE );
					break;
				case 2:
					pal <<= 2;
					snes_draw_tile_4( layer, layers[layer].data + (tile << 5) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip, pal, SNES_BLEND_NONE, SNES_CLIP_NONE );
					break;
				case 4:
					snes_draw_tile_8( layer, layers[layer].data + (tile << 6) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip, SNES_BLEND_NONE, SNES_CLIP_NONE );
					break;
			}
		}
	}

	sprintf( str, "%d : %8X  ", layer, addr );
	ui_text( bitmap, str, 300, 227 );
}

/*********************************************
 * snes_dbg_draw_all_tiles()
 *
 * Debug to display everything in VRAM.
 *********************************************/
static void snes_dbg_draw_all_tiles( struct mame_bitmap *bitmap, UINT32 tileaddr, UINT8 bpl, UINT16 pal )
{
	UINT16 ii, jj, kk;
	UINT32 addr = tileaddr;
	char str[50];

	for( jj = 0; jj < 32; jj++ )
	{
		addr = tileaddr + (jj * bpl * 16 * 32);
		for( kk = 0; kk < 8; kk++ )
		{
			/* Clear zbuffer */
			memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
			for( ii = 0; ii < 32; ii++ )
			{
				switch( bpl )
				{
					case 1:
						snes_draw_tile_2( MAINSCREEN, addr, ii * 8, 255, 0, pal, SNES_BLEND_NONE, SNES_CLIP_NONE );
						break;
					case 2:
						snes_draw_tile_4( MAINSCREEN, addr, ii * 8, 255, 0, pal, SNES_BLEND_NONE, SNES_CLIP_NONE  );
						break;
					case 4:
						snes_draw_tile_8( MAINSCREEN, addr, ii * 8, 255, 0, SNES_BLEND_NONE, SNES_CLIP_NONE  );
						break;
				}
				addr += (bpl * 16);
			}
			draw_scanline16( bitmap, 0, jj * 8 + kk, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, 200 );
			addr -= (32 * (bpl * 16)) - 2;
		}
	}

	sprintf( str, "  %8X  ", tileaddr );
	ui_text( bitmap, str, 300, 227 );
}
#endif /* MAME_DEBUG */

/*********************************************
 * snes_refresh_scanline()
 *
 * Redraw the current line.
 *********************************************/
void snes_refresh_scanline( UINT16 curline )
{
	UINT16 ii;
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	struct rectangle r = Machine->visible_area;
	r.min_y = r.max_y = curline;

#ifdef MAME_DEBUG
	/* Check if the user has enabled or disabled stuff */
	if( curline == 0 )
	{
		char t[100];

		if( !debug_options.input_count-- )
		{
			UINT8 toggles = readinputport( 10 );
			if( toggles & 0x1 )
				debug_options.bg_disabled[0] = !debug_options.bg_disabled[0];
			if( toggles & 0x2 )
				debug_options.bg_disabled[1] = !debug_options.bg_disabled[1];
			if( toggles & 0x4 )
				debug_options.bg_disabled[2] = !debug_options.bg_disabled[2];
			if( toggles & 0x8 )
				debug_options.bg_disabled[3] = !debug_options.bg_disabled[3];
			if( toggles & 0x10 )
				debug_options.bg_disabled[4] = !debug_options.bg_disabled[4];
			debug_options.input_count = 5;
		}
		/* Display some debug info on the screen */
		sprintf( t, "BG0 %X h%d v%d",(snes_ram[BG1SC] & 0xfc) << 9, (bg_hoffset[0] & 0x3ff) >> 3, (bg_voffset[0] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, 0 );
		sprintf( t, "BG1 %X h%d v%d",(snes_ram[BG2SC] & 0xfc) << 9, (bg_hoffset[1] & 0x3ff) >> 3, (bg_voffset[1] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, 9 );
		sprintf( t, "BG2 %X h%d v%d",(snes_ram[BG3SC] & 0xfc) << 9, (bg_hoffset[2] & 0x3ff) >> 3, (bg_voffset[2] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, 18 );
		sprintf( t, "BG3 %X h%d v%d",(snes_ram[BG4SC] & 0xfc) << 9, (bg_hoffset[3] & 0x3ff) >> 3, (bg_voffset[3] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, 27 );
		sprintf( t, "OBJ %X", (((snes_ram[OBSEL] & 0x3) * 0x2000) + (((snes_ram[OBSEL] & 0x18)>>3) * 0x1000)) * 2 );
		ui_text( Machine->scrbitmap, t, 300, 36 );
		sprintf( t, "a %d", mode7_data[0] );
		ui_text( Machine->scrbitmap, t, 300, 50 );
		sprintf( t, "b %d", mode7_data[1] );
		ui_text( Machine->scrbitmap, t, 300, 59 );
		sprintf( t, "c %d", mode7_data[2] );
		ui_text( Machine->scrbitmap, t, 300, 68 );
		sprintf( t, "d %d", mode7_data[3] );
		ui_text( Machine->scrbitmap, t, 300, 77 );
		sprintf( t, "flip X: %s  flip y: %s", snes_ram[M7SEL] & 0x1 ? "yes" : "no", snes_ram[M7SEL] & 0x2 ? "yes" : "no" );
		ui_text( Machine->scrbitmap, t, 300, 86 );
		sprintf( t, "hscroll: %d  vscroll: %d", bg_hoffset[0], bg_voffset[0] );
		ui_text( Machine->scrbitmap, t, 300, 95 );
	}
	/* Just for testing, draw as many tiles as possible */
	{
		UINT8 adjust = readinputport( 1 );
		UINT8 dip = readinputport( 9 );
		UINT8 inp = readinputport( 11 );
		UINT8 dt = 1 << ((dip & 0x3) - 1);
		UINT8 dm = 1 << (((dip & 0xc) >> 2) - 1);
		if( dt )
		{
			static INT16 pal = 0;
			static UINT32 addr = 0;
			if( curline == 0 )
			{
				if( adjust & 0x1 ) addr += (dt * 16);
				if( adjust & 0x2 ) addr -= (dt * 16);
				if( adjust & 0x4 ) addr += (dt * 16 * 32);
				if( adjust & 0x8 ) addr -= (dt * 16 * 32);
				if( inp & 0x1 ) pal -= 1;
				if( inp & 0x2 ) pal += 1;
				if( pal < 0 ) pal = 0;
				if( pal > 8 ) pal = 8;
				for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
				{
					scanlines[MAINSCREEN].buffer[ii] = 0;
				}
				snes_dbg_draw_all_tiles( bitmap, addr, dt, pal * 16 );
			}
			return;
		}
		if( dm )
		{
			static UINT32 tmaddr = 0;
			static INT8 tmbg = 0;
			if( curline == 0 )
			{
				if( adjust & 0x1 ) tmaddr += 2;
				if( adjust & 0x2 ) tmaddr -= 2;
				if( adjust & 0x4 ) tmaddr += 64;
				if( adjust & 0x8 ) tmaddr -= 64;
				if( inp & 0x1 ) tmbg -= 1;
				if( inp & 0x2 ) tmbg += 1;
				if( tmbg < 0 ) tmbg = 0;
				if( tmbg > 3 ) tmbg = 3;
			}
			/* Clear zbuffer */
			memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
			/* Draw back colour */
			for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
				scanlines[MAINSCREEN].buffer[ii] = Machine->pens[0];
			snes_dbg_draw_maps( bitmap, tmaddr, dm, curline, tmbg );
			draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, 200 );
			return;
		}
	}
#endif /* MAME_DEBUG */

	if( snes_ram[INIDISP] & 0x80 ) /* screen is forced blank */
	{
		fillbitmap(bitmap, Machine->pens[0], &r);
	}
	else
	{
		/* Find the addresses for tile data */
		layers[0].data = (snes_ram[BG12NBA] & 0xf) << 13;
		layers[1].data = (snes_ram[BG12NBA] & 0xf0) << 9;
		layers[2].data = (snes_ram[BG34NBA] & 0xf) << 13;
		layers[3].data = (snes_ram[BG34NBA] & 0xf0) << 9;
		layers[4].data = (((snes_ram[OBSEL] & 0x3) * 0x2000) + (((snes_ram[OBSEL] & 0x18)>>3) * 0x1000)) * 2;
		/* Find the addresses for tile maps */
		layers[0].map = ((snes_ram[BG1SC] & 0xfc) << 9);
		layers[1].map = ((snes_ram[BG2SC] & 0xfc) << 9);
		layers[2].map = ((snes_ram[BG3SC] & 0xfc) << 9);
		layers[3].map = ((snes_ram[BG4SC] & 0xfc) << 9);
		/* Window clipping stuff */
		layers[0].clip = SNES_CLIP_NONE;
		layers[1].clip = SNES_CLIP_NONE;
		layers[2].clip = SNES_CLIP_NONE;
		layers[3].clip = SNES_CLIP_NONE;
		layers[4].clip = SNES_CLIP_NONE;
		if( snes_ram[W12SEL] & 0x2 )
		{
			layers[0].clip |= SNES_CLIP_1;
			if( snes_ram[W12SEL] & 0x1 )
				layers[0].clip |= SNES_CLIP_1_OUT;
		}
		if( snes_ram[W12SEL] & 0x8 )
		{
			layers[0].clip |= SNES_CLIP_2;
			if( snes_ram[W12SEL] & 0x4 )
				layers[0].clip |= SNES_CLIP_2_OUT;
		}
		if( snes_ram[W12SEL] & 0x20 )
		{
			layers[1].clip |= SNES_CLIP_1;
			if( snes_ram[W12SEL] & 0x10 )
				layers[1].clip |= SNES_CLIP_1_OUT;
		}
		if( snes_ram[W12SEL] & 0x80 )
		{
			layers[1].clip |= SNES_CLIP_2;
			if( snes_ram[W12SEL] & 0x40 )
				layers[1].clip |= SNES_CLIP_2_OUT;
		}
		if( snes_ram[W34SEL] & 0x2 )
		{
			layers[2].clip |= SNES_CLIP_1;
			if( snes_ram[W34SEL] & 0x1 )
				layers[2].clip |= SNES_CLIP_1_OUT;
		}
		if( snes_ram[W34SEL] & 0x8 )
		{
			layers[2].clip |= SNES_CLIP_2;
			if( snes_ram[W34SEL] & 0x4 )
				layers[2].clip |= SNES_CLIP_2_OUT;
		}
		if( snes_ram[W34SEL] & 0x20 )
		{
			layers[3].clip |= SNES_CLIP_1;
			if( snes_ram[W34SEL] & 0x10 )
				layers[3].clip |= SNES_CLIP_1_OUT;
		}
		if( snes_ram[W34SEL] & 0x80 )
		{
			layers[3].clip |= SNES_CLIP_2;
			if( snes_ram[W34SEL] & 0x40 )
				layers[3].clip |= SNES_CLIP_2_OUT;
		}
		if( snes_ram[WOBJSEL] & 0x2 )
		{
			layers[4].clip |= SNES_CLIP_1;
			if( snes_ram[WOBJSEL] & 0x1 )
				layers[4].clip |= SNES_CLIP_1_OUT;
		}
		if( snes_ram[WOBJSEL] & 0x8 )
		{
			layers[4].clip |= SNES_CLIP_2;
			if( snes_ram[WOBJSEL] & 0x4 )
				layers[4].clip |= SNES_CLIP_2_OUT;
		}
		/* Blending stuff */
		layers[0].blend = SNES_BLEND_NONE;
		layers[1].blend = SNES_BLEND_NONE;
		layers[2].blend = SNES_BLEND_NONE;
		layers[3].blend = SNES_BLEND_NONE;
		layers[4].blend = SNES_BLEND_NONE;
		if( snes_ram[CGWSEL] & 0x2 )
		{
			if( snes_ram[CGADSUB] & 0x80 )
			{
				if( snes_ram[CGADSUB] & 0x1 )
					layers[0].blend = SNES_BLEND_SUB;
				if( snes_ram[CGADSUB] & 0x2 )
					layers[1].blend = SNES_BLEND_SUB;
				if( snes_ram[CGADSUB] & 0x4 )
					layers[2].blend = SNES_BLEND_SUB;
				if( snes_ram[CGADSUB] & 0x8 )
					layers[3].blend = SNES_BLEND_SUB;
				if( snes_ram[CGADSUB] & 0x10 )
					layers[4].blend = SNES_BLEND_SUB;
			}
			else
			{
				if( snes_ram[CGADSUB] & 0x1 )
					layers[0].blend = SNES_BLEND_ADD;
				if( snes_ram[CGADSUB] & 0x2 )
					layers[1].blend = SNES_BLEND_ADD;
				if( snes_ram[CGADSUB] & 0x4 )
					layers[2].blend = SNES_BLEND_ADD;
				if( snes_ram[CGADSUB] & 0x8 )
					layers[3].blend = SNES_BLEND_ADD;
				if( snes_ram[CGADSUB] & 0x10 )
					layers[4].blend = SNES_BLEND_ADD;
			}
		}

		/* Clear zbuffers */
		memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
		memset( scanlines[SUBSCREEN].zbuf, 0, SNES_SCR_WIDTH );
		/* Clear Mainscreen buffer */
		memset( scanlines[MAINSCREEN].buffer, 0, SNES_SCR_WIDTH * 2 );

		/* Draw back colour */
		for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
		{
			scanlines[SUBSCREEN].buffer[ii] = Machine->remapped_colortable[FIXED_COLOUR];
		}

		/* Draw the subscreen sprites */
		if( snes_ram[TS] & 0x10 )
			snes_update_objects( SUBSCREEN, curline );

		/* Draw backgrounds */
		switch( snes_ram[BGMODE] & 0x7 )
		{
			case 0:			/* Mode 0 - 4 Bitplanes - 4 / 4 / 4 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_2( SUBSCREEN, 0, curline, 0 );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				if( snes_ram[TS] & 0x4 ) snes_update_line_2( SUBSCREEN, 2, curline, 0 );
				if( snes_ram[TS] & 0x8 ) snes_update_line_2( SUBSCREEN, 3, curline, 0 );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_2( MAINSCREEN, 0, curline, 0 );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				if( snes_ram[TM] & 0x4 ) snes_update_line_2( MAINSCREEN, 2, curline, 0 );
				if( snes_ram[TM] & 0x8 ) snes_update_line_2( MAINSCREEN, 3, curline, 0 );
				break;
			case 1:			/* Mode 1 - 3 Bitplanes - 16 / 16 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				if( snes_ram[TS] & 0x4 )
				{
					if( snes_ram[BGMODE] & 0x8 )	/* BG3 can have highest priority if bit 3 if BGMODE is set */
						snes_update_line_2( SUBSCREEN, 2, curline, 1 );
					else
						snes_update_line_2( SUBSCREEN, 2, curline, 0 );
				}
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				if( snes_ram[TM] & 0x4 )
				{
					if( snes_ram[BGMODE] & 0x8 )	/* BG3 can have highest priority if bit 3 if BGMODE is set */
						snes_update_line_2( MAINSCREEN, 2, curline, 1 );
					else
						snes_update_line_2( MAINSCREEN, 2, curline, 0 );
				}
				break;
			case 2:			/* Mode 2 - 2 Bitplanes - 16 / 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				break;
			case 3:			/* Mode 3 - 2 Bitplanes - 256 / 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_8( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_8( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				break;
			case 4:			/* Mode 4 - 2 Bitplanes - 256 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_8( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_8( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				break;
			case 5:			/* Mode 5 - 2 Bitplanes - 16 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				break;
			case 6:			/* Mode 6 - 1 Bitplane - 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				break;
			case 7:			/* Mode 7 - 1 Bitplanes - 256 colours - matrix math stuff */
				snes_update_line_mode7( curline );
				break;
		}

		/* Draw the mainscreen sprites */
		if( snes_ram[TM] & 0x10 )
			snes_update_objects( MAINSCREEN, curline );

		/* Phew! Draw the line to screen */
		draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, 1 );
	}
}
