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

static UINT16 scanline[SNES_SCR_WIDTH + 8];	/* Scanline buffer. Allow for drawing 1 tile past edge of the screen */
static UINT8  zbuf[SNES_SCR_WIDTH + 8];		/* Z buffer. Allow for drawing 1 tile past edge of the screen */
UINT8  obj_size[2];					/* Object sizes */
/* Lookup tables */
static UINT8  table_bgd_pty[4][2] = { {5,7}, {4,6}, {1,3}, {0,2} };
static UINT8  table_obj_pty[4]    = { 2, 4, 6, 8 };
static UINT16 table_hscroll[4][4] = { {0,0,0,0}, {0,0x800,0,0x800}, {0,0,0,0}, {0,0x800,0,0x800} };
static UINT16 table_vscroll[4][4] = { {0,0,0,0}, {0,0,0,0}, {0,0x800,0,0x800}, {0,0x1000,0,0x1000} };

struct SCANLINE
{
	UINT16 line[SNES_SCR_WIDTH + 8];
	UINT8  zbuf[SNES_SCR_WIDTH + 8];
	UINT32 data;
	UINT32 map;
};

static struct SCANLINE layers[5];

VIDEO_UPDATE( snes )
{
}

static void snes_draw_windows( UINT8 layer, UINT8 curline, UINT8 priority )
{
	UINT8 w1en = 0, w2en = 0, w1mode = 0, w2mode = 0;
	UINT16 ii;

	/* This is the wrong way to do it I think */
	return;

#ifdef MAME_DEBUG
	if( !(readinputport( 10 ) & 0x20) )
	{
		return;
	}
#endif

	/* Enable windows based on the specified background */
	switch( layer )
	{
		case 0:
			if( snes_ram[W12SEL] & 0x2 ) w1en = 1;
			if( snes_ram[W12SEL] & 0x8 ) w2en = 1;
			w1mode = snes_ram[W12SEL & 0x1];
			w2mode = snes_ram[W12SEL & 0x4];
			break;
		case 1:
			if( snes_ram[W12SEL] & 0x20 ) w1en = 1;
			if( snes_ram[W12SEL] & 0x80 ) w2en = 1;
			w1mode = snes_ram[W12SEL & 0x10];
			w2mode = snes_ram[W12SEL & 0x40];
			break;
		case 2:
			if( snes_ram[W34SEL] & 0x2 ) w1en = 1;
			if( snes_ram[W34SEL] & 0x8 ) w2en = 1;
			w1mode = snes_ram[W34SEL & 0x1];
			w2mode = snes_ram[W34SEL & 0x4];
			break;
		case 3:
			if( snes_ram[W34SEL] & 0x20 ) w1en = 1;
			if( snes_ram[W34SEL] & 0x80 ) w2en = 1;
			w1mode = snes_ram[W34SEL & 0x10];
			w2mode = snes_ram[W34SEL & 0x40];
			break;
	}

	for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
	{
		/* Draw window 1 */
		if( w1en )
		{
			if( w1mode )	/* Window mask is in */
			{
				if( ii >= snes_ram[WH0] || ii <= snes_ram[WH1] )
				{
					layers[layer].line[ii] = Machine->pens[0];
				}
			}
			else	/* Window mask is out */
			{
				if( ii <= snes_ram[WH0] || ii >= snes_ram[WH1] )
				{
					layers[layer].line[ii] = Machine->pens[0];
				}
			}
		}
		/* Draw window 2 */
		if( w2en )
		{
			if( w2mode )	/* Window mask is in */
			{
				if( ii >= snes_ram[WH2] || ii <= snes_ram[WH3] )
				{
					layers[layer].line[ii] = Machine->pens[0];
				}
			}
			else	/* Window mask is out */
			{
				if( ii <= snes_ram[WH2] || ii >= snes_ram[WH3] )
				{
					layers[layer].line[ii] = Machine->pens[0];
				}
			}
		}
	}
}

/*****************************************
 * snes_draw_tile_2()
 *
 * Draw tiles with 2 bit planes(4 colors)
 *****************************************/
INLINE void snes_draw_tile_2( UINT8 layer, UINT8 line, UINT16 tileaddr, UINT16 x, UINT16 y, UINT8 priority, UINT8 flip, UINT16 pal )
{
	UINT8 ii;
	UINT8 plane[2];

	tileaddr += (line << 1);
	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];

	for( ii = 0; ii < 8; ii++ )
	{
		if( flip )
		{
			register UINT8 colour = (plane[0] & 0x1 ? 1 : 0) |
									(plane[1] & 0x1 ? 2 : 0);
			if( colour )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[layer].zbuf[x+ii] = priority;
			}
			plane[0] >>= 1;
			plane[1] >>= 1;
		}
		else
		{
			register UINT8 colour = (plane[0] & 0x80 ? 1 : 0) |
									(plane[1] & 0x80 ? 2 : 0);
			if( colour )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[layer].zbuf[x+ii] = priority;
			}
			plane[0] <<= 1;
			plane[1] <<= 1;
		}
	}
}

/*****************************************
 * snes_draw_tile_4()
 *
 * Draw tiles with 4 bit planes(16 colors)
 *****************************************/
INLINE void snes_draw_tile_4( UINT8 layer, UINT8 line, UINT16 tileaddr, INT16 x, INT16 y, UINT8 priority, UINT8 flip, UINT16 pal )
{
	UINT8 ii;
	UINT8 plane[4];

	tileaddr += (line << 1);
	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];

	for( ii = 0; ii < 8; ii++ )
	{
		if( flip )
		{
			register UINT8 colour = (plane[0] & 0x1 ? 1 : 0) | (plane[1] & 0x1 ? 2 : 0) |
									(plane[2] & 0x1 ? 4 : 0) | (plane[3] & 0x1 ? 8 : 0);
			if( colour )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[layer].zbuf[x+ii] = priority;
			}
			plane[0] >>= 1;
			plane[1] >>= 1;
			plane[2] >>= 1;
			plane[3] >>= 1;
		}
		else
		{
			register UINT8 colour = (plane[0] & 0x80 ? 1 : 0) | (plane[1] & 0x80 ? 2 : 0) |
									(plane[2] & 0x80 ? 4 : 0) | (plane[3] & 0x80 ? 8 : 0);
			if( colour )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[layer].zbuf[x+ii] = priority;
			}
			plane[0] <<= 1;
			plane[1] <<= 1;
			plane[2] <<= 1;
			plane[3] <<= 1;
		}
	}
}

/*****************************************
 * snes_draw_tile_8()
 *
 * Draw tiles with 8 bit planes(8 colors)
 *****************************************/
static void snes_draw_tile_8( UINT8 layer, UINT8 line, UINT16 tileaddr, UINT16 x, UINT16 y, UINT8 priority, UINT8 flip )
{
	UINT8 ii;
	UINT8 plane[8];

	tileaddr += (line << 1);
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
		if( flip )
		{
			register UINT8 colour = (plane[0] & 0x1 ? 1 : 0)  | (plane[1] & 0x1 ? 2 : 0)  |
									(plane[2] & 0x1 ? 4 : 0)  | (plane[3] & 0x1 ? 8 : 0)  |
									(plane[4] & 0x1 ? 16 : 0) | (plane[5] & 0x1 ? 32 : 0) |
									(plane[6] & 0x1 ? 64 : 0) | (plane[7] & 0x1 ? 128 : 0);
			if( colour )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[colour];
				layers[layer].zbuf[x+ii] = priority;
			}
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
			register UINT8 colour = (plane[0] & 0x80 ? 1 : 0)  | (plane[1] & 0x80 ? 2 : 0)  |
									(plane[2] & 0x80 ? 4 : 0)  | (plane[3] & 0x80 ? 8 : 0)  |
									(plane[4] & 0x80 ? 16 : 0) | (plane[5] & 0x80 ? 32 : 0) |
									(plane[6] & 0x80 ? 64 : 0) | (plane[7] & 0x80 ? 128 : 0);
			if( colour && (priority > zbuf[x+ii]) )
			{
				layers[layer].line[x+ii] = Machine->remapped_colortable[colour];
				layers[layer].zbuf[x+ii] = priority;
			}
			plane[0] <<= 1;
			plane[1] <<= 1;
			plane[2] <<= 1;
			plane[3] <<= 1;
			plane[4] <<= 1;
			plane[5] <<= 1;
			plane[6] <<= 1;
			plane[7] <<= 1;
		}
	}
}

/*****************************************
 * snes_draw_object()
 *
 * Draw objects. The reason we don't use snes_draw_tile_4()
 * is because we need to handle priorities of the objects
 * on a per-pixel basis. I know it's ugly but oh well.
 *****************************************/
INLINE void snes_draw_object( UINT8 line, UINT16 tileaddr, INT16 x, INT16 y, UINT8 priority, UINT8 flip, UINT16 pal )
{
	UINT8 ii;
	UINT8 plane[4];

	tileaddr += (line << 1);
	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];

	for( ii = 0; ii < 8; ii++ )
	{
		if( flip )
		{
			register UINT8 colour = (plane[0] & 0x1 ? 1 : 0) | (plane[1] & 0x1 ? 2 : 0) |
									(plane[2] & 0x1 ? 4 : 0) | (plane[3] & 0x1 ? 8 : 0);
			if( colour && (priority >= layers[4].zbuf[x+ii]) )
			{
				layers[4].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[4].zbuf[x+ii] = priority;
			}
			plane[0] >>= 1;
			plane[1] >>= 1;
			plane[2] >>= 1;
			plane[3] >>= 1;
		}
		else
		{
			register UINT8 colour = (plane[0] & 0x80 ? 1 : 0) | (plane[1] & 0x80 ? 2 : 0) |
									(plane[2] & 0x80 ? 4 : 0) | (plane[3] & 0x80 ? 8 : 0);
			if( colour && (priority >= layers[4].zbuf[x+ii]) )
			{
				layers[4].line[x+ii] = Machine->remapped_colortable[pal + colour];
				layers[4].zbuf[x+ii] = priority;
			}
			plane[0] <<= 1;
			plane[1] <<= 1;
			plane[2] <<= 1;
			plane[3] <<= 1;
		}
	}
}

/*********************************************
 * snes_update_line_2()
 *
 * Update an entire line of 2 bit plane tiles.
 *********************************************/
static void snes_update_line_2( UINT8 layer, UINT16 curline, UINT8 bg3_pty )
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
	if( !(readinputport( 10 ) & (1 << layer)) )
	{
		return;
	}
#endif

	/* Bail if the bg is disabled */
	if( !((snes_ram[TM]|snes_ram[TS]) & (1 << layer)) )
	{
		return;
	}

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

		/* Special case for bg3 */
		if( layer == 2 && bg3_pty )
			priority = 9;

		if( tile > 0 )
			snes_draw_tile_2( layer, (UINT8)tile_line, layers[layer].data + (tile << 4), (ii >> 1) * 8, curline, priority, hflip, pal );
	}

	if( (snes_ram[TMW]|snes_ram[TSW]) & (1 << layer) )
		snes_draw_windows( layer, curline, priority );
}

/*********************************************
 * snes_update_line_4()
 *
 * Update an entire line of 4 bit plane tiles.
 *********************************************/
static void snes_update_line_4( UINT8 layer, UINT16 curline )
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
	if( !(readinputport( 10 ) & (1 << layer)) )
	{
		return;
	}
#endif

	/* Bail if the bg is disabled */
	if( !((snes_ram[TM]|snes_ram[TS]) & (1 << layer)) )
	{
		return;
	}

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
		if( tile > 0 )
			snes_draw_tile_4( layer, (UINT8)tile_line, layers[layer].data + (tile << 5), ((ii >> 1) * 8)  - hshift, curline, priority, hflip, pal );
	}

	if( (snes_ram[TMW]|snes_ram[TSW]) & (1 << layer) )
		snes_draw_windows( layer, curline, priority );
}

/*********************************************
 * snes_update_line_8()
 *
 * Update an entire line of 8 bit plane tiles.
 *********************************************/
static void snes_update_line_8( UINT8 layer, UINT16 curline )
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
	if( !(readinputport( 10 ) & (1 << layer)) )
	{
		return;
	}
#endif

	/* Bail if the bg is disabled */
	if( !((snes_ram[TM]|snes_ram[TS]) & (1 << layer)) )
	{
		return;
	}

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

		if( tile > 0 )
			snes_draw_tile_8( layer, (UINT8)tile_line, layers[layer].data + (tile << 6), (ii >> 1) * 8, curline, priority, hflip );
	}

	if( (snes_ram[TMW]|snes_ram[TSW]) & (1 << layer) )
		snes_draw_windows( layer, curline, priority );
}

/*********************************************
 * snes_update_line_mode7()
 *
 * Update an entire line of mode7 tiles.
 *********************************************/
static void snes_update_line_mode7( UINT16 curline )
{
	UINT16 tilen, tiled, ii;
	INT16 a, b, c, d, x0, y0, x1, y1, x2, y2, hs, vs;
	register UINT8 colour;

	a = mode7_data[M7A];
	b = mode7_data[M7B];
	c = mode7_data[M7C];
	d = mode7_data[M7D];
	x0 = mode7_data[M7X];
	y0 = mode7_data[M7Y];
	hs = bg_hoffset[0];
	vs = bg_voffset[0];

	/* sign extend */
	x0 <<= 3;
	x0 >>= 3;
	y0 <<= 3;
	y0 >>= 3;
	hs <<= 3;
	hs >>= 3;
	vs <<= 3;
	vs >>= 3;

	/* From the original driver */
	x1 = 0;
	y1 = curline;
	switch( snes_ram[M7SEL] & 0xc0 )
	{
		case 0x00:
			for( ii = 0; ii < 256; ii++ )
			{
				x2 = (((a * ((x1 + hs) - x0) + c * ((y1 + vs) - y0)) >> 8) + x0) & 0x3ff;
				y2 = (((b * ((x1 + hs) - x0) + d * ((y1 + vs) - y0)) >> 8) + y0) & 0x3ff;
				tilen = snes_vram[((y2 / 8) * 128 * 2) + ((x2 / 8) * 2)];
				tiled = 1 + (tilen * 8 * 8 * 2);
				colour = snes_vram[tiled + ((y2 & 7) * 8 * 2) + ((x2 & 7) * 2)];
				if( zbuf[ii] <= 1 )
					scanline[ii] = Machine->remapped_colortable[colour];
				x1++;
			}
			break;
		case 0x80:
			for( ii = 0; ii < 256; ii++ )
			{
				x2 = (((a * ((x1 + hs) - x0) + c * ((y1 + vs) - y0)) >> 8) + x0);
				y2 = (((b * ((x1 + hs) - x0) + d * ((y1 + vs) - y0)) >> 8) + y0);
				tilen = snes_vram[(((y2 & 0x3ff) / 8) * 128 * 2) + (((x2 & 0x3ff) / 8) * 2)];
				if( (x2 & 0x7fff) < 1024 && (y2 & 0x7fff) < 1024 )
					tiled = 1 + (tilen * 8 * 8 * 2);
				else
					tiled = 1;
				colour = snes_vram[tiled + ((y2 & 7) * 8 * 2) + ((x2 & 7) * 2)];
				if( zbuf[ii] <= 1 )
					scanline[ii] = Machine->remapped_colortable[colour];
				x1++;
			}
			break;
		case 0xc0:
			for( ii = 0; ii < 256; ii++ )
			{
				x2 = (((a * ((x1 + hs) - x0) + c * ((y1 + vs) - y0)) >> 8) + x0);
				y2 = (((b * ((x1 + hs) - x0) + d * ((y1 + vs) - y0)) >> 8) + y0);
				if( (x2 & 0x7fff) < 1024 && (y2 & 0x7fff) < 1024 )
				{
					tilen = snes_vram[((y2 / 8) * 128 * 2) + ((x2 / 8) * 2)];
					tiled = 1 + (tilen * 8 * 8 * 2);
					colour = snes_vram[tiled + ((y2 & 7) * 8 * 2) + ((x2 & 7) * 2)];
					if( zbuf[ii] <= 1 )
						scanline[ii] = Machine->remapped_colortable[colour];
					x1++;
				}
			}
			break;
	}
}

/*********************************************
 * snes_update_sprites()
 *
 * Update an entire line of sprites.
 *********************************************/
INLINE void snes_update_objects( UINT16 curline )
{
	INT8 xs, tileincr;
	UINT8 ys, line;
	UINT16 oam, oam_extra, extra;
	UINT8 range_over = 0, time_over = 0;
	UINT8 size, vflip, hflip, priority, pal, count;
	UINT16 x, y, tile;
	INT16 i;

#ifdef MAME_DEBUG
	if( !(readinputport( 10 ) & 0x10) )
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
			extra = snes_oam[oam_extra--];

		vflip = (snes_oam[oam] & 0x80) >> 7;
		hflip = (snes_oam[oam] & 0x40) >> 6;
		priority = table_obj_pty[(snes_oam[oam] & 0x30) >> 4];
		pal = 128 + ((snes_oam[oam] & 0xE) << 3);
		tile = (snes_oam[oam--] & 0x1) << 8;
		tile |= snes_oam[oam--];
		y = snes_oam[oam--];
		x = snes_oam[oam--];
		size = (extra & 0x80) >> 7;
		extra <<= 1;
		x |= ((extra & 0x80) << 1);
		extra <<= 1;

		tileincr = 16;
		if( vflip )
		{
			tile += tileincr;
			tileincr = -tileincr;
		}

		if( y > 239 )
			y -= 256;	/* y is past sprite max pos */

		/* Draw sprite if it intersects the current line */
		for( ys = 0; ys < obj_size[size]; ys++ )
		{
			count = 0;
			line = curline - (y + (ys << 3));
			if( vflip )
				line = (-1 * line) + 7;

			/* If the object intersects this line, then draw it */
			if( curline >= (y + (ys << 3)) && curline < (y + (ys << 3) + 8) )
			{
				if( hflip )
				{
					for( xs = (obj_size[size] - 1); xs >= 0; xs-- )
					{
						if( (x + (count << 3) < SNES_SCR_WIDTH + 8) )
							snes_draw_object( line, layers[4].data + ((tile + xs) << 5), x + (count++ << 3), curline, priority, hflip, pal );
						time_over++;	/* Increase time_over. Should we stop drawing if exceeded 34 tiles? */
					}
				}
				else
				{
					for( xs = 0; xs < obj_size[size]; xs++ )
					{
						if( (x + (count << 3) < SNES_SCR_WIDTH + 8) )
							snes_draw_object( line, layers[4].data + ((tile + xs) << 5), x + (count++ << 3), curline, priority, hflip, pal );
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
			tile += tileincr;
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
 *********************************************/
static void snes_dbg_draw_maps( struct mame_bitmap *bitmap, UINT32 tilemap, UINT8 bpl, UINT16 curline, UINT8 layer )
{
	UINT32 tile, addr = tilemap;
	UINT16 ii, vflip, hflip, pal;
	INT8 line;

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
					snes_draw_tile_2( layer, (UINT8)line, layers[layer].data + (tile << 4), (ii >> 1) * 8, curline, 255, hflip, pal );
					break;
				case 2:
					pal <<= 2;
					snes_draw_tile_4( layer, (UINT8)line, layers[layer].data + (tile << 5), (ii >> 1) * 8, curline, 255, hflip, pal );
					break;
				case 4:
					snes_draw_tile_8( layer, (UINT8)line, layers[layer].data + (tile << 6), (ii >> 1) * 8, curline, 255, hflip );
					break;
			}
		}
	}

	{
		char str[50];
		sprintf( str, "%d : %8X  ", layer, addr );
		ui_drawbox( bitmap, 2, 225, 87, 12 );
		ui_text( bitmap, str, 3, 227 );
	}
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

	for( jj = 0; jj < 32; jj++ )
	{
		for( kk = 0; kk < 8; kk++ )
		{
			for( ii = 0; ii < 32; ii++ )
			{
				/* Clear zbuffer */
				memset( zbuf, 0, SNES_SCR_WIDTH );

				switch( bpl )
				{
					case 1:
						snes_draw_tile_2( 0, kk, tileaddr, ii * 8, jj * 8 + kk, 255, 0, pal );
						break;
					case 2:
						snes_draw_tile_4( 0, kk, tileaddr, ii * 8, jj * 8 + kk, 255, 0, pal );
						break;
					case 4:
						snes_draw_tile_8( 0, kk, tileaddr, ii * 8, jj * 8 + kk, 255, 0 );
						break;
				}
				tileaddr += (bpl * 16);
			}
			tileaddr = addr + kk;
			draw_scanline16( bitmap, 0, jj * 8 + kk, SNES_SCR_WIDTH, scanline, Machine->pens, 200 );
		}
	}

	{
		char str[50];
		sprintf( str, "  %8X  ", addr );
		ui_drawbox( bitmap, 2, 225, 75, 12 );
		ui_text( bitmap, str, 3, 227 );
	}
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
	INT8 jj;
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	struct rectangle r = Machine->visible_area;
	r.min_y = r.max_y = curline;

#ifdef MAME_DEBUG
	/* Just for testing, draw as many tiles as possible */
	{
		UINT8 dip = readinputport( 9 );
		UINT8 dt = 1 << ((dip & 0x3) - 1);
		UINT8 dm = 1 << (((dip & 0xc) >> 2) - 1);
		if( dt )
		{
			/*static INT8 tstep = 1;*/
			static INT16 pal = 0;
			static UINT32 addr = 0;
	
			if( curline == 0 )
			{
				struct rectangle r_;
				UINT8 adjust = readinputport( 1 );
				if( adjust & 0x1 ) addr += (dt * 16);
				if( adjust & 0x2 ) addr -= (dt * 16);
				if( adjust & 0x1 ) addr += (dt * 16 * 32);
				if( adjust & 0x2 ) addr -= (dt * 16 * 32);
				r_ = Machine->visible_area;
				fillbitmap(bitmap, Machine->pens[0], &r_);
				snes_dbg_draw_all_tiles( bitmap, addr, dt, pal * 16 );
			}
			return;
		}
		if( dm )
		{
			/*static INT8 mstep = 1;*/
			static UINT32 tmaddr = 0;
			static INT8 tmbg = 0;
			if( curline == 0 )
			{
				UINT8 adjust = readinputport( 1 );
				if( adjust & 0x1 ) tmaddr += 2;
				if( adjust & 0x2 ) tmaddr -= 2;
				if( adjust & 0x4 ) tmaddr += 64;
				if( adjust & 0x8 ) tmaddr -= 64;
			}
			/* Clear zbuffer */
			memset( zbuf, 0, SNES_SCR_WIDTH );
			/* Draw back colour */
			for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
				scanline[ii] = Machine->pens[0];
			snes_dbg_draw_maps( bitmap, tmaddr, dm, curline, tmbg );
			draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanline, Machine->pens, 200 );
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
		/* Clear zbuffers */
		memset( layers[0].zbuf, 0, SNES_SCR_WIDTH );
		memset( layers[1].zbuf, 0, SNES_SCR_WIDTH );
		memset( layers[2].zbuf, 0, SNES_SCR_WIDTH );
		memset( layers[3].zbuf, 0, SNES_SCR_WIDTH );
		memset( layers[4].zbuf, 0, SNES_SCR_WIDTH );
		memset( zbuf, 0, SNES_SCR_WIDTH );

		/* Draw back colour */
		for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
		{
			if( snes_ram[CGADSUB] & 0x20 )	/* Back colour enabled for fixed colour add/sub */
				scanline[ii] = Machine->remapped_colortable[FIXED_COLOUR];
			else
				scanline[ii] = Machine->pens[0];
		}

		/* Draw the sprites */
		if( (snes_ram[TM]|snes_ram[TS]) & 0x10 )
			snes_update_objects( curline );

		/* Draw backgrounds */
		switch( snes_ram[BGMODE] & 0x7 )
		{
			case 0:			/* Mode 0 - 4 Bitplanes - 4 / 4 / 4 / 4 colours */
				snes_update_line_2( 0, curline, 0 );
				snes_update_line_2( 1, curline, 0 );
				snes_update_line_2( 2, curline, 0 );
				snes_update_line_2( 3, curline, 0 );
				break;
			case 1:			/* Mode 1 - 3 Bitplanes - 16 / 16 / 4 colours */
				snes_update_line_4( 0, curline );
				snes_update_line_4( 1, curline );
				if( snes_ram[BGMODE] & 0x8 )	/* BG3 can have highest priority if bit 3 if BGMODE is set */
					snes_update_line_2( 2, curline, 1 );
				else
					snes_update_line_2( 2, curline, 0 );
				break;
			case 2:			/* Mode 2 - 2 Bitplanes - 16 / 16 colours */
				snes_update_line_4( 0, curline );
				snes_update_line_4( 1, curline );
				break;
			case 3:			/* Mode 3 - 2 Bitplanes - 256 / 16 colours */
				snes_update_line_8( 0, curline );
				snes_update_line_4( 1, curline );
				break;
			case 4:			/* Mode 4 - 2 Bitplanes - 256 / 4 colours */
				snes_update_line_8( 0, curline );
				snes_update_line_2( 1, curline, 0 );
				break;
			case 5:			/* Mode 5 - 2 Bitplanes - 16 / 4 colours */
				snes_update_line_4( 0, curline );
				snes_update_line_2( 1, curline, 0 );
				break;
			case 6:			/* Mode 6 - 1 Bitplane - 16 colours */
				snes_update_line_4( 0, curline );
				break;
			case 7:			/* Mode 7 - 1 Bitplanes - 256 colours - matrix math stuff */
				snes_update_line_mode7( curline );
				break;
		}

		/* Merge all the lines together */
		for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
		{
			for( jj = 4; jj >= 0; jj-- )
			{
				if( layers[jj].zbuf[ii] > zbuf[ii] )
				{
					/* An attempt at transparency that doesn't work */
/*					if( (snes_ram[CGADSUB] & (1 << jj)) && snes_ram[CGWSEL] & 0x2 )
					{
						UINT8 r,g,b;
						for( kk = jj + 1; kk < 3; kk++ )
						{
							r = (layers[jj].line[ii] & 0x1f) + (layers[kk].line[ii] & 0x1f);
							g = ((layers[jj].line[ii] & 0x3e0) >> 5) + ((layers[kk].line[ii] & 0x3e0) >> 5);
							b = ((layers[jj].line[ii] & 0x7c00) >> 10) + ((layers[kk].line[ii] & 0x7c00) >> 10);
							layers[jj].line[ii] = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
						}
					}*/
					scanline[ii] = layers[jj].line[ii];
					zbuf[ii] = layers[jj].zbuf[ii];
				}
			}
		}

		/* Phew! Draw the line to screen */
		draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanline, Machine->pens, 200 );
	}
}
