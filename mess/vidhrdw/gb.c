/***************************************************************************

  gb.c

  Video file to handle emulation of the Nintendo GameBoy.

  Original code                               Carsten Sorensen   1998
  Mess modifications, bug fixes and speedups  Hans de Goede      1998
  Bug fixes, SGB and GBC code                 Anthony Kruize     2002

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/gb.h"
#include "profiler.h"

static UINT8 bg_zbuf[160];
UINT8 gb_vid_regs[0x40];
UINT8 gb_bpal[4];	/* Background palette		*/
UINT8 gb_spal0[4];	/* Sprite 0 palette		*/
UINT8 gb_spal1[4];	/* Sprite 1 palette		*/
UINT8 *gb_oam = NULL;
UINT8 *gb_vram = NULL;

struct layer_struct {
	UINT8  enabled;
	UINT8  *bg_tiles;
	UINT8  *bg_map;
	UINT8  xindex;
	UINT8  xshift;
	UINT8  xstart;
	UINT8  xend;
	/* GBC specific */
	UINT16 *gbc_tiles[2];
	UINT8  *gbc_map;
	INT16  bgline;
};

struct gb_lcd_struct {
	int	window_lines_drawn;
	int	window_enabled;
	int	window_start_line;

	int	lcd_warming_up;		/* Has the video hardware just been switched on? */
	int	lcd_on;			/* Is the video hardware on? */

	/* Things used to render current line */
	int	current_line;		/* Current line */
	int	sprCount;		/* Number of sprites on current line */
	int	sprite[10];		/* References to sprites to draw on current line */
} gb_lcd;

void (*refresh_scanline)(void);

/*
  Select which sprites should be drawn for the current scanline and return the
  number of sprites selected.
 */
int gb_select_sprites( void ) {
	int	i, yindex, line, height;
	UINT8	*oam = gb_oam + 39 * 4;
	UINT8	*vram = gb_vram;

	gb_lcd.sprCount = 0;

	/* If video hardware is enabled and sprites are enabled */
	if ( ( LCDCONT & 0x80 ) && ( LCDCONT & 0x02 ) ) {
		/* Check for stretched sprites */
		if ( LCDCONT & 0x04 ) {
			height = 16;
		} else {
			height = 8;
		}

		yindex = gb_lcd.current_line;
		line = gb_lcd.current_line + 16;

		for( i = 39; i >= 0; i-- ) {
			if ( line >= oam[0] && line < ( oam[0] + height ) && oam[1] && oam[1] < 168 ) {
				/* We limit the sprite count to max 10 here;
				   proper games should not exceed this... */
				if ( gb_lcd.sprCount < 10 ) {
					gb_lcd.sprite[gb_lcd.sprCount] = i;
					gb_lcd.sprCount++;
				}
			}
		}
	}
	return gb_lcd.sprCount;
}

INLINE void gb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram;
	int i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = gb_lcd.current_line;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	vram = gb_vram;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			int xindex, adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

/* this should be recoded to become incremental */
void gb_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = gb_lcd.current_line;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* Take care of some initializations */
	if ( gb_lcd.current_line == 0x00 ) {
		gb_lcd.window_lines_drawn = 0;
		gb_lcd.window_enabled = 0;
	}

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->screen[0].visarea;
		r.min_y = r.max_y = yindex;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && gb_lcd.current_line >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + gb_lcd.current_line) & 0xFF;

		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].bg_tiles = gb_chrgen + ( (bgline & 7) << 1 );
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		/* Check if window was just enabled */
		if ( ! gb_lcd.window_enabled ) {
			gb_lcd.window_start_line = gb_lcd.current_line;
		}
		gb_lcd.window_enabled = 1;
		/* this also seems to be influenced by the scrolly register and the time window was enabled */
		bgline = (gb_lcd.window_start_line + SCROLLY + gb_lcd.window_lines_drawn) & 0xFF;
		xpos = WNDPOSX - 7;		/* Window is offset by 7 pixels */
		if (xpos < 0)
			xpos = 0;

		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].bg_tiles = gb_chrgen + ( (bgline & 7) << 1);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	} else {
		gb_lcd.window_enabled = 0;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, xidx, bit, i;
		UINT8 *tiles;
		UINT16 data;
		int xindex, tile_index;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		tiles = layer[l].bg_tiles;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		tile_index = (map[xidx] ^ gb_tile_no_mod) * 16;
		data = tiles[tile_index] | ( tiles[tile_index+1] << 8 ); 
		data <<= bit;

		xindex = layer[l].xstart;
		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
				plot_pixel(bitmap, xindex, yindex, Machine->pens[gb_bpal[colour]]);
				xindex++;
				*zbuf++ = colour;
				data <<= 1;
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			bit = 0;
			tile_index = (map[xidx] ^ gb_tile_no_mod) * 16;
			data = tiles[tile_index] | ( tiles[tile_index+1] << 8 );
		}
		l++;
	}

	if (LCDCONT & 0x02)
		gb_update_sprites();

	if ( layer[1].enabled ) {
		gb_lcd.window_lines_drawn++;
	}

	profiler_mark(PROFILER_END);
}

/* --- Super Gameboy Specific --- */

INLINE void sgb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram, pal;
	INT16 i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	/* Offset to center of screen */
	yindex = gb_lcd.current_line + SGB_YOFFSET;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	vram = gb_vram;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			INT16 xindex;
			int adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height -1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			/* Find the palette to use */
			pal = sgb_pal_map[(xindex >> 3)][((yindex - SGB_YOFFSET) >> 3)] << 2;

			/* Offset to center of screen */
			xindex += SGB_XOFFSET;

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void sgb_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = gb_lcd.current_line;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* Handle SGB mask */
	switch( sgb_window_mask )
	{
		case 1:	/* Freeze screen */
			return;
		case 2:	/* Blank screen (black) */
			{
				rectangle r = Machine->screen[0].visarea;
				r.min_x = SGB_XOFFSET;
				r.max_x -= SGB_XOFFSET;
				r.min_y = SGB_YOFFSET;
				r.max_y -= SGB_YOFFSET;
				fillbitmap( bitmap, Machine->pens[0], &r );
			} return;
		case 3:	/* Blank screen (white - or should it be color 0?) */
			{
				rectangle r = Machine->screen[0].visarea;
				r.min_x = SGB_XOFFSET;
				r.max_x -= SGB_XOFFSET;
				r.min_y = SGB_YOFFSET;
				r.max_y -= SGB_YOFFSET;
				fillbitmap( bitmap, Machine->pens[32767], &r );
			} return;
	}

	/* Draw the "border" if we're on the first line */
	if( gb_lcd.current_line == 0 )
	{
		sgb_refresh_border();
	}

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->screen[0].visarea;
		r.min_x = SGB_XOFFSET;
		r.max_x -= SGB_XOFFSET;
		r.min_y = r.max_y = yindex + SGB_YOFFSET;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && gb_lcd.current_line >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + gb_lcd.current_line) & 0xFF;

		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].bg_tiles = gb_chrgen + ( (bgline & 7) << 1);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (gb_lcd.current_line - WNDPOSY) & 0xFF;
		/* Window X position is offset by 7 so we'll need to adust */
		xpos = WNDPOSX - 7;
		if (xpos < 0)
			xpos = 0;

		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].bg_tiles = gb_chrgen + ( (bgline & 7) << 1);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, xidx, bit, i, pal;
		UINT8 *tiles;
		UINT16 data;
		int xindex, tile_index;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		tiles = layer[l].bg_tiles;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;


		tile_index = (map[xidx] ^ gb_tile_no_mod) * 16;
		data = tiles[tile_index] | ( tiles[tile_index+1] << 8 );
		data <<= bit;

		xindex = layer[l].xstart;

		/* Figure out which palette we're using */
		pal = sgb_pal_map[(xindex >> 3)][(yindex >> 3)] << 2;

		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
				plot_pixel(bitmap, xindex + SGB_XOFFSET, yindex + SGB_YOFFSET, Machine->remapped_colortable[pal + gb_bpal[colour]]);
				xindex++;
				*zbuf++ = colour;
				data <<= 1;
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			pal = sgb_pal_map[(xindex >> 3)][(yindex >> 3)] << 2;
			bit = 0;
			tile_index = (map[xidx] ^ gb_tile_no_mod) * 16;
			data = tiles[tile_index] | ( tiles[tile_index+1] << 8 );
		}
		l++;
	}

	if (LCDCONT & 0x02)
		sgb_update_sprites();

	profiler_mark(PROFILER_END);
}

void sgb_refresh_border(void)
{
	UINT16 *tiles, *tiles2, data, data2;
	UINT16 *map;
	UINT16 yidx, xidx, xindex;
	UINT8 pal, i;
	mame_bitmap *bitmap = tmpbitmap;

	map = (UINT16 *)sgb_tile_map - 32;

	for( yidx = 0; yidx < 224; yidx++ )
	{
		xindex = 0;
		map += (yidx % 8) ? 0 : 32;
		for( xidx = 0; xidx < 32; xidx++ )
		{
			if( map[xidx] & 0x8000 ) /* Vertical flip */
				tiles = (UINT16 *)sgb_tile_data + (7 - (yidx % 8));
			else /* No vertical flip */
				tiles = (UINT16 *)sgb_tile_data + (yidx % 8);
			tiles2 = tiles + 8;

			pal = (map[xidx] & 0x1C00) >> 10;
			if( pal == 0 )
				pal = 1;
			pal <<= 4;

			if( sgb_hack ) /* A few games do weird stuff */
			{
				UINT16 tileno = map[xidx] & 0xFF;
				if( tileno >= 128 ) tileno = ((64 + tileno) % 128) + 128;
				else tileno = (64 + tileno) % 128;
				data = tiles[tileno * 16];
				data2 = tiles2[tileno * 16];
			}
			else
			{
				data = tiles[(map[xidx] & 0xFF) * 16];
				data2 = tiles2[(map[xidx] & 0xFF) * 16];
			}

			for( i = 0; i < 8; i++ )
			{
				register UINT8 colour;
				if( (map[xidx] & 0x4000) ) /* Horizontal flip */
				{
					colour = ((data  & 0x0001) ? 1 : 0) |
							 ((data  & 0x0100) ? 2 : 0) |
							 ((data2 & 0x0001) ? 4 : 0) |
							 ((data2 & 0x0100) ? 8 : 0);
					data >>= 1;
					data2 >>= 1;
				}
				else /* No horizontal flip */
				{
					colour = ((data  & 0x0080) ? 1 : 0) |
							 ((data  & 0x8000) ? 2 : 0) |
							 ((data2 & 0x0080) ? 4 : 0) |
							 ((data2 & 0x8000) ? 8 : 0);
					data <<= 1;
					data2 <<= 1;
				}
				/* A slight hack below so we don't draw over the GB screen.
				 * Drawing there is allowed, but due to the way we draw the
				 * scanline, it can obscure the screen even when it shouldn't.
				 */
				if( !((yidx >= SGB_YOFFSET && yidx < SGB_YOFFSET + 144) &&
					(xindex >= SGB_XOFFSET && xindex < SGB_XOFFSET + 160)) )
				{
					plot_pixel(bitmap, xindex, yidx, Machine->remapped_colortable[pal + colour]);
				}
				xindex++;
			}
		}
	}
}

/* --- Gameboy Color Specific --- */

INLINE void gbc_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam;
	int i, xindex, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = gb_lcd.current_line;
	line = gb_lcd.current_line + 16;

	oam = gb_oam + 39 * 4;
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, pal;

			/* Handle mono mode for GB games */
			if( gbc_mode == GBC_MODE_MONO )
				pal = (oam[3] & 0x10) ? 8 : 4;
			else
				pal = GBC_PAL_OBJ_OFFSET + ((oam[3] & 0x7) * 4);

			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2]);
			}
			else
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (line - oam[0]) * 2]);
			}
#ifndef LSB_FIRST
			data = (data << 8) | (data >> 8);
#endif

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void gbc_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = gb_lcd.current_line;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->screen[0].visarea;
		r.min_y = r.max_y = yindex;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && gb_lcd.current_line >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + gb_lcd.current_line) & 0xFF;

		layer[0].bgline = bgline;
		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].gbc_map = gbc_bgdtab;
		layer[0].gbc_map += (bgline << 2) & 0x3E0;
		layer[0].gbc_tiles[0] = (UINT16 *)gb_chrgen + (bgline & 7);
		layer[0].gbc_tiles[1] = (UINT16 *)gbc_chrgen + (bgline & 7);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (gb_lcd.current_line - WNDPOSY) & 0xFF;
		/* Window X position is offset by 7 so we'll need to adust */
		xpos = WNDPOSX - 7;
		if (xpos < 0)
			xpos = 0;

		layer[1].bgline = bgline;
		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].gbc_map = gbc_wndtab;
		layer[1].gbc_map += (bgline << 2) & 0x3E0;
		layer[1].gbc_tiles[0] = (UINT16 *)gb_chrgen + (bgline & 7);
		layer[1].gbc_tiles[1] = (UINT16 *)gbc_chrgen + (bgline & 7);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, *gbcmap, xidx, bit, i;
		UINT16 *tiles, data;
		int xindex;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		gbcmap = layer[l].gbc_map;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		tiles = layer[l].gbc_tiles[(gbcmap[xidx] & 0x8) >> 3];
		if( (gbcmap[xidx] & 0x40) >> 6 ) /* vertical flip */
			tiles -= ((layer[l].bgline & 7) << 1) - 7;
		data = (UINT16)(tiles[(map[xidx] ^ gb_tile_no_mod) * 8] << bit);
#ifndef LSB_FIRST
		data = (data << 8) | (data >> 8);
#endif

		xindex = layer[l].xstart;
		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour;
				if( ((gbcmap[xidx] & 0x20) >> 5) ) /* horizontal flip */
				{
					colour = ((data & 0x100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					data >>= 1;
				}
				else /* no horizontal flip */
				{
					colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					data <<= 1;
				}
				plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[(((gbcmap[xidx] & 0x7) * 4) + colour)]);
				xindex++;
				/* If the priority bit is set then bump up the value, we'll
				 * check this when drawing sprites */
				*zbuf++ = colour + (gbcmap[xidx] & 0x80);
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			bit = 0;
			tiles = layer[l].gbc_tiles[(gbcmap[xidx] & 0x8) >> 3];
			if( (gbcmap[xidx] & 0x40) >> 6 ) /* vertical flip */
				tiles -= ((layer[l].bgline & 7) << 1) - 7;
			data = (UINT16)(tiles[(map[xidx] ^ gb_tile_no_mod) * 8]);
		}
		l++;
	}

	if (LCDCONT & 0x02)
		gbc_update_sprites();

	profiler_mark(PROFILER_END);
}

void gb_video_init( void ) {
	int	i;

	gb_chrgen = gb_vram;
	gb_bgdtab = gb_vram + 0x1C00;
	gb_wndtab = gb_vram + 0x1C00;

	LCDSTAT = 0x80;
	gb_lcd.current_line = CURLINE = CMPLINE = 0x00;
	SCROLLX = SCROLLY = 0x00;
	WNDPOSX = WNDPOSY = 0x00;

	/* Initialize palette arrays */
	for( i = 0; i < 4; i++ ) {
		gb_bpal[i] = gb_spal0[i] = gb_spal1[i] = i;
	}

	/* set the scanline refresh function */
	refresh_scanline = gb_refresh_scanline;
}

void sgb_video_init( void ) {
	gb_video_init();

	/* Override the scanline refresh function */
	refresh_scanline = sgb_refresh_scanline;
}

void gbc_video_init( void ) {
	gb_video_init();

	gb_chrgen = GBC_VRAMMap[0];
	gbc_chrgen = GBC_VRAMMap[1];
	gb_bgdtab = gb_wndtab = GBC_VRAMMap[0] + 0x1C00;
	gbc_bgdtab = gbc_wndtab = GBC_VRAMMap[1] + 0x1C00;

	/* Override the scanline refresh function */
	refresh_scanline = gbc_refresh_scanline;
}

void gb_increment_scanline( void ) {
	gb_lcd.current_line = ( gb_lcd.current_line + 1 ) % 154;
	if ( LCDCONT & 0x80 ) {
		CURLINE = gb_lcd.current_line;
		if ( CURLINE == CMPLINE ) {
			LCDSTAT |= 0x04;
			/* Generate lcd interrupt if requested */
			if ( LCDSTAT & 0x40 )
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		} else {
			LCDSTAT &= 0xFB;
		}
	}
}

int	gb_skip_increment_scanline = 0;

/* Handle changing LY to 00 on line 153 */
void gb_scanline_start_frame( int param ) {
	gb_increment_scanline();	/* LY 153 -> 00 */
	gb_skip_increment_scanline = 1;
	/* Initialize some other internal things to start drawing a frame ? */
	/* Initialize window state machine(?) */
}                                                
         
void gb_scanline_interrupt_set_mode0 (int param) {
	/* refresh current scanline */
	refresh_scanline();

	/* only perform mode changes when LCD controller is still on */
	if (LCDCONT & 0x80) {
		/* Set Mode 0 lcdstate */
		LCDSTAT &= 0xFC;
		/* Generate lcd interrupt if requested */
		if( LCDSTAT & 0x08 )
			cpunum_set_input_line(0, LCD_INT, HOLD_LINE);

		/* Check for HBLANK DMA */
		if( gbc_hdma_enabled && (gb_lcd.current_line < 144) )
			gbc_hdma(0x10);
	}
}

void gb_scanline_interrupt_set_mode3 (int param) {
	int mode3_cycles = 172 /*+ gb_select_sprites() * 10*/;

	/* only perform mode changes when LCD controller is still on */
	if (LCDCONT & 0x80) {
		/* Set Mode 3 lcdstate */
		LCDSTAT = (LCDSTAT & 0xFC) | 0x03;
	}
	/* Second lcdstate change after aprox 172+#sprites*10 clock cycles / 60 uS */
	timer_set ( TIME_IN_CYCLES(mode3_cycles,0), 0, gb_scanline_interrupt_set_mode0 );
}

void gb_video_scanline_interrupt (void) {                                                
	/* skip incrementing scanline when entering scanline 0 */
	if ( ! gb_skip_increment_scanline ) {    
		gb_increment_scanline();
	} else {
		gb_skip_increment_scanline = 0;
	}

	if (gb_lcd.current_line < 144) {
		if ( LCDCONT & 0x80 ) {
			/* Set Mode 2 lcdstate */
			LCDSTAT = (LCDSTAT & 0xFC) | 0x02;
			/* Generate lcd interrupt if requested */
			if (LCDSTAT & 0x20)
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		}

		/* First  lcdstate change after aprox 80 clock cycles / 19 uS */
		timer_set ( TIME_IN_CYCLES(80,0), 0, gb_scanline_interrupt_set_mode3);
	} else {
		/* Generate VBlank interrupt (if display is enabled) */
		if (gb_lcd.current_line == 144 && ( LCDCONT & 0x80 ) ) {
			/* Cause VBlank interrupt */
			cpunum_set_input_line(0, VBL_INT, HOLD_LINE);
			/* Set VBlank lcdstate */
			LCDSTAT = (LCDSTAT & 0xFC) | 0x01;
			/* Generate lcd interrupt if requested */
			if( LCDSTAT & 0x10 )
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		}
		if ( gb_lcd.current_line == 153 ) {
			timer_set( TIME_IN_CYCLES(32,0), 0, gb_scanline_start_frame );
		}
	}
}

READ8_HANDLER( gb_video_r ) {
	switch( offset ) {
	case 0x01:			/* STAT - high bit is always set */
		return 0x80 | gb_vid_regs[offset];
	case 0x04:			/* LY - returns 00 when video hardware is disabled */
		return gb_vid_regs[offset];
	case 0x06:
		return 0xFF;
	case 0x36:
	case 0x37:
		return 0;
	default:
		return gb_vid_regs[offset];
	}
}

/* Ignore write when LCD is on and STAT is 02 or 03 */
int gb_video_oam_locked( void ) {
	if ( ( LCDCONT & 0x80 ) && ( LCDSTAT & 0x02 ) ) {
		return 1;
	}
	return 0;
}

/* Ignore write when LCD is on and STAT is not 03 */
int gb_video_vram_locked( void ) {
	if ( ( LCDCONT & 0x80 ) && ( ( LCDSTAT & 0x03 ) == 0x03 ) ) {
		return 1;
	}
	return 0;
}

WRITE8_HANDLER ( gb_video_w ) {
	switch (offset) {
	case 0x00:						/* LCDC - LCD Control */
		gb_chrgen = gb_vram + ((data & 0x10) ? 0x0000 : 0x0800);
		gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
		gb_bgdtab = gb_vram + ((data & 0x08) ? 0x1C00 : 0x1800 );
		gb_wndtab = gb_vram + ((data * 0x40) ? 0x1C00 : 0x1800 );
		/* if LCD controller is switched off, set STAT and LY to 00 */
		if ( ! ( data & 0x80 ) ) {
			LCDSTAT &= ~0x03;
			CURLINE = 0;
		}
		/* If LCD is being switched on */
		if ( !( LCDCONT & 0x80 ) && ( data & 0x80 ) ) {
			gb_lcd.current_line = 0;
		}
		break;
	case 0x01:						/* STAT - LCD Status */
		data = (data & 0xF8) | (LCDSTAT & 0x07);
		break;
	case 0x04:						/* LY - LCD Y-coordinate */
		data = 0;
		break;
	case 0x06:						/* DMA - DMA Transfer and Start Address */
		{
			UINT8 *P = gb_oam;
			offset = (UINT16) data << 8;
			for (data = 0; data < 0xA0; data++)
				*P++ = program_read_byte_8 (offset++);
		}
		return;
	case 0x07:						/* BGP - Background Palette */
		gb_bpal[0] = data & 0x3;
		gb_bpal[1] = (data & 0xC) >> 2;
		gb_bpal[2] = (data & 0x30) >> 4;
		gb_bpal[3] = (data & 0xC0) >> 6;
		break;
	case 0x08:						/* OBP0 - Object Palette 0 */
		gb_spal0[0] = data & 0x3;
		gb_spal0[1] = (data & 0xC) >> 2;
		gb_spal0[2] = (data & 0x30) >> 4;
		gb_spal0[3] = (data & 0xC0) >> 6;
		break;
	case 0x09:						/* OBP1 - Object Palette 1 */
		gb_spal1[0] = data & 0x3;
		gb_spal1[1] = (data & 0xC) >> 2;
		gb_spal1[2] = (data & 0x30) >> 4;
		gb_spal1[3] = (data & 0xC0) >> 6;
		break;
	}
	gb_vid_regs[ offset ] = data;
}

WRITE8_HANDLER ( gbc_video_w ) {
	static const UINT16 gbc_to_gb_pal[4] = {32767, 21140, 10570, 0};
	static UINT16 BP = 0, OP = 0;

	switch( offset ) {
	case 0x00:      /* LCDC - LCD Control */
		gb_chrgen = GBC_VRAMMap[0] + ((data & 0x10) ? 0x0000 : 0x0800);
		gbc_chrgen = GBC_VRAMMap[1] + ((data & 0x10) ? 0x0000 : 0x0800);
		gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
		gb_bgdtab = GBC_VRAMMap[0] + ((data & 0x08) ? 0x1C00 : 0x1800);
		gbc_bgdtab = GBC_VRAMMap[1] + ((data & 0x08) ? 0x1C00 : 0x1800);
		gb_wndtab = GBC_VRAMMap[0] + ((data & 0x40) ? 0x1C00 : 0x1800);
		gbc_wndtab = GBC_VRAMMap[1] + ((data & 0x40) ? 0x1C00 : 0x1800);
		/* if LCD controller is switched off, set STAT to 00 */
		if ( ! ( data & 0x80 ) ) {
			LCDSTAT &= ~0x03;
		}
		break;
	case 0x07:      /* BGP - GB background palette */
		/* Some GBC games are lazy and still call this */
		if( gbc_mode == GBC_MODE_MONO ) {
			Machine->remapped_colortable[0] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[1] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[2] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[3] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x08:      /* OBP0 - GB Object 0 palette */
		if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
		{
			Machine->remapped_colortable[4] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[5] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[6] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[7] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x09:      /* OBP1 - GB Object 1 palette */
		if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
		{
			Machine->remapped_colortable[8] = gbc_to_gb_pal[(data & 0x03)];
			Machine->remapped_colortable[9] = gbc_to_gb_pal[(data & 0x0C) >> 2];
			Machine->remapped_colortable[10] = gbc_to_gb_pal[(data & 0x30) >> 4];
			Machine->remapped_colortable[11] = gbc_to_gb_pal[(data & 0xC0) >> 6];
		}
		break;
	case 0x11:      /* HDMA1 - HBL General DMA - Source High */
		break;
	case 0x12:      /* HDMA2 - HBL General DMA - Source Low */
		data &= 0xF0;
		break;
	case 0x13:      /* HDMA3 - HBL General DMA - Destination High */
		data &= 0x1F;
		break;
	case 0x14:      /* HDMA4 - HBL General DMA - Destination Low */
		data &= 0xF0;
		break;
	case 0x15:      /* HDMA5 - HBL General DMA - Mode, Length */
		if( !(data & 0x80) )
		{
			if( gbc_hdma_enabled )
			{
				gbc_hdma_enabled = 0;
				data = HDMA5 & 0x80;
			}
			else
			{
				/* General DMA */
				gbc_hdma( ((data & 0x7F) + 1) * 0x10 );
				lcd_time -= ((KEY1 & 0x80)?110:220) + (((data & 0x7F) + 1) * 7.68);
				data = 0xff;
			}
		}
		else
		{
			/* H-Blank DMA */
			gbc_hdma_enabled = 1;
			data &= 0x7f;
		}
		break;
	case 0x28:      /* BCPS - Background palette specification */
		break;
	case 0x29:      /* BCPD - background palette data */
		if( GBCBCPS & 0x1 )
			Machine->remapped_colortable[(GBCBCPS & 0x3e) >> 1] = ((UINT16)(data & 0x7f) << 8) | BP;
		else
			BP = data;
		if( GBCBCPS & 0x80 )
		{
			GBCBCPS++;
			GBCBCPS &= 0xBF;
		}
		break;
	case 0x2A:      /* OCPS - Object palette specification */
		break;
	case 0x2B:      /* OCPD - Object palette data */
		if( GBCOCPS & 0x1 )
			Machine->remapped_colortable[GBC_PAL_OBJ_OFFSET + ((GBCOCPS & 0x3e) >> 1)] = ((UINT16)(data & 0x7f) << 8) | OP;
		else
			OP = data;
		if( GBCOCPS & 0x80 )
		{
			GBCOCPS++;
			GBCOCPS &= 0xBF;
		}
		break;
	default:
		/* we didn't handle the write, so pass it to the GB handler */
		gb_video_w( offset, data );
		return;
	}

	gb_vid_regs[offset] = data;
}

