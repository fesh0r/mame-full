/***************************************************************************

  vidhrdw/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include <math.h>

#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/nes.h"
#include "machine/nes_mmc.h"

//#define BACKGROUND_OFF
//#define LOG_VIDEO
//#define LOG_COLOR
//#define DIRTY_BUFFERS

#define VIDEORAM_SIZE	0x4000
#define SPRITERAM_SIZE	0x100
#define VRAM_SIZE	0x3c0

static void render_sprites (int scanline);

int nes_vram[8]; /* Keep track of 8 .5k vram pages to speed things up */
int nes_vram_sprite[8]; /* Used only by mmc5 for now */
int dirtychar[0x200];

static int gfx_bank;

unsigned char nes_palette[3*64];

#ifdef DIRTY_BUFFERS
unsigned char *dirtybuffer2;
unsigned char *dirtybuffer3;
unsigned char *dirtybuffer4;
#endif
unsigned char line_priority[0x108];

/* Changed at runtime */
static UINT32 nes_colortable[] =
{
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
};

UINT32 colortable_mono[] =
{
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
	0,1,2,3,
};

#ifdef LOG_VIDEO
FILE *videolog;
#endif
#ifdef LOG_COLOR
FILE *colorlog;
#endif

void nes_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
#ifndef M_PI
#define M_PI			(3.14159265358979323846L)
#endif

	/* This routine builds a palette using a transformation from */
	/* the YUV (Y, B-Y, R-Y) to the RGB color space */

	/* The NES has a 64 color palette						 */
	/* 16 colors, with 4 luminance levels for each color	 */
	/* The 16 colors circle around the YUV color space, 	 */

	/* It also returns a fake colortable, for the menus */

	int i,j;
#ifdef COLOR_INTENSITY
	int x;
#endif

	double R, G, B;

	double tint = .5;
	double hue = 332.0;
	double bright_adjust = 1.0;

	double brightness[3][4] =
	{
		{ 0.50, 0.75, 1.0, 1.0 },
		{ 0.29, 0.45, 0.73, 0.9 },
		{ 0, 0.24, 0.47, 0.77 }
	};

	double angle[16] = {0,240,210,180,150,120,90,60,30,0,330,300,270,0,0,0};

#ifdef COLOR_INTENSITY
	/* Loop through the emphasis modes (8 total) */
	for (x = 0; x < 8; x ++)
	{
		double r_mod = 0;
		double g_mod = 0;
		double b_mod = 0;

		switch (x)
		{
			case 0: r_mod = 1.0; g_mod = 1.0; b_mod = 1.0; break;
			case 1: r_mod = 1.24; g_mod = .915; b_mod = .743; break;
			case 2: r_mod = .794; g_mod = 1.09; b_mod = .882; break;
			case 3: r_mod = .905; g_mod = 1.03; b_mod = 1.28; break;
			case 4: r_mod = .741; g_mod = .987; b_mod = 1.0; break;
			case 5: r_mod = 1.02; g_mod = .908; b_mod = .979; break;
			case 6: r_mod = 1.02; g_mod = .98; b_mod = .653; break;
			case 7: r_mod = .75; g_mod = .75; b_mod = .75; break;
		}
#endif

		/* loop through the 4 intensities */
		for (i = 0; i < 4; i++)
		{
			/* loop through the 16 colors */
			for (j = 0; j < 16; j++)
			{
				double sat;
				double y;
				double rad;

				switch (j)
				{
					case 0:
						sat = 0;
						y = brightness[0][i];
						break;
					case 13:
						sat = 0;
						y = brightness[2][i];
						break;
					case 14:
					case 15:
						sat = 0; y = 0;
						break;
					default:
						sat = tint;
						y = brightness[1][i];
						break;
				}

				rad = M_PI * ((angle[j] + hue) / 180.0);

				y *= bright_adjust;

				/* Transform to RGB */
#ifdef COLOR_INTENSITY
				R = (y + sat * sin(rad)) * 255.0 * r_mod;
				G = (y - (27 / 53) * sat * sin(rad) + (10 / 53) * sat * cos(rad)) * 255.0 * g_mod;
				B = (y - sat * cos(rad)) * 255.0 * b_mod;
#else
				R = (y + sat * sin(rad)) * 255.0;
				G = (y - (27 / 53) * sat * sin(rad) + (10 / 53) * sat * cos(rad)) * 255.0;
				B = (y - sat * cos(rad)) * 255.0;
#endif

				/* Clipping, in case of saturation */
				if (R < 0)
					R = 0;
				if (R > 255)
					R = 255;
				if (G < 0)
					G = 0;
				if (G > 255)
					G = 255;
				if (B < 0)
					B = 0;
				if (B > 255)
					B = 255;

				/* Round, and set the value */
				nes_palette[(i*16+j)*3] = *palette = floor(R+.5);
				palette++;
				nes_palette[(i*16+j)*3+1] = *palette = floor(G+.5);
				palette++;
				nes_palette[(i*16+j)*3+2] = *palette = floor(B+.5);
				palette++;
			}
		}
#ifdef COLOR_INTENSITY
	}
#endif

#if 0
/* base type of arrays are different now!*/
   memcpy(colortable,nes_colortable,sizeof(nes_colortable));
#else
	for (i=0; i<ARRAY_LENGTH(nes_colortable); i++) {
	    colortable[i]=nes_colortable[i];
	}
#endif
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int nes_vh_start(void)
{
	int i;

	/* We must clear the videoram on startup */
	if ((videoram = calloc (VIDEORAM_SIZE, 1)) == 0)
		return 1;

	/* We use an offscreen bitmap that's 4 times as large as the visible one */
	if ((tmpbitmap = bitmap_alloc_depth(2 * 32*8, 2 * 30*8,Machine->scrbitmap->depth)) == 0)
	{
		free (videoram);
		bitmap_free (tmpbitmap);
		return 1;
	}

	/* sprite RAM must be clear on startup */
	if ((spriteram = calloc (SPRITERAM_SIZE,1)) == 0)
	{
		free (videoram);
		bitmap_free (tmpbitmap);
		return 1;
	}

#ifdef DIRTY_BUFFERS
	dirtybuffer  = malloc (VRAM_SIZE);
	dirtybuffer2 = malloc (VRAM_SIZE);
	dirtybuffer3 = malloc (VRAM_SIZE);
	dirtybuffer4 = malloc (VRAM_SIZE);

	if ((!dirtybuffer) || (!dirtybuffer2) || (!dirtybuffer3) || (!dirtybuffer4))
	{
		free (videoram);
		bitmap_free (tmpbitmap);
		free (spriteram);
		if (dirtybuffer)  free (dirtybuffer);
		if (dirtybuffer2) free (dirtybuffer2);
		if (dirtybuffer3) free (dirtybuffer3);
		if (dirtybuffer4) free (dirtybuffer4);
		return 1;
	}

	memset (dirtybuffer,  1, VRAM_SIZE);
	memset (dirtybuffer2, 1, VRAM_SIZE);
	memset (dirtybuffer3, 1, VRAM_SIZE);
	memset (dirtybuffer4, 1, VRAM_SIZE);
#endif

	/* Mark all chars as 'clean' */
	for (i = 0; i < 0x200; i ++)
		dirtychar[i] = 0;

	if (nes.chr_chunks == 0) gfx_bank = 1;
	else gfx_bank = 0;

#ifdef LOG_VIDEO
	videolog = fopen ("video.log", "w");
#endif
#ifdef LOG_COLOR
	colorlog = fopen ("color.log", "w");
#endif

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void nes_vh_stop(void)
{
	free (videoram);
	free (spriteram);
#ifdef DIRTY_BUFFERS
	free (dirtybuffer);
	free (dirtybuffer2);
	free (dirtybuffer3);
	free (dirtybuffer4);
#endif
	bitmap_free (tmpbitmap);

#ifdef LOG_VIDEO
	if (videolog) fclose (videolog);
#endif
#ifdef LOG_COLOR
	if (colorlog) fclose (colorlog);
#endif
}

/***************************************************************************

  Draw the current scanline

***************************************************************************/

void nes_vh_renderscanline (int scanline)
{
	int x;
	int i;
	int start_x;
	int tile_index;
	UINT8 scroll_x_coarse;
	UINT8 scroll_y_coarse;
	UINT8 scroll_y_fine;
	UINT8 color_mask;
	int total_elements;
	const UINT32 *paldata_1;
//	const unsigned char *sd_1;

#ifdef BIG_SCREEN
	return;
#endif

	profiler_mark(PROFILER_VIDEO);

	if (osd_skip_this_frame ())
		goto draw_nothing;

	if (PPU_Control1 & 0x01)
		color_mask = 0xf0;
	else
		color_mask = 0xff;

#ifndef COLOR_INTENSITY
	/* Set the background color */
	memset (Machine->scrbitmap->line[scanline], Machine->pens[PPU_background_color & color_mask], 0x100);
#endif

	for (i = 0; i < 0x100; i ++)
	{
		/* Clear the priority buffer for this line */
		line_priority[i] = 0;

		/* Set the background color */
//		plot_pixel (Machine->scrbitmap, i, scanline, Machine->pens[PPU_background_color]);
#ifdef COLOR_INTENSITY
		((UINT16 *) Machine->scrbitmap->line[scanline])[i] = Machine->pens[PPU_background_color & color_mask];
#endif
	}

	/* Clear sprite count for this line */
	PPU_Status &= ~PPU_status_8sprites;

	/* If the background is off, don't draw it */
#ifndef BACKGROUND_OFF
	if (!(PPU_Control1 & PPU_c1_background))
#endif
	{
		goto draw_sprites;
	}

	/* Determine where in the nametable to start drawing from, based on the current scanline and scroll regs */
	scroll_x_coarse = PPU_refresh_data & 0x1f;
	scroll_y_coarse = (PPU_refresh_data & 0x3e0) >> 5;
	scroll_y_fine = (PPU_refresh_data & 0x7000) >> 12;

	start_x = (PPU_X_fine ^ 0x07) - 7;

	x = scroll_x_coarse;
	tile_index = ((PPU_refresh_data & 0xc00) | 0x2000) + scroll_y_coarse * 32;

#ifdef LOG_VIDEO
if ((scanline == 0) && (videolog)) fprintf (videolog, "\n");
if (videolog) fprintf (videolog, "%03d: ", scanline);
#endif
#ifdef LOG_COLOR
if ((scanline == 0) && (colorlog)) fprintf (colorlog, "\n");
if (colorlog) fprintf (colorlog, "%03d: ", scanline);
#endif

	total_elements = Machine->gfx[gfx_bank]->total_elements;

//	sd_1 = Machine->gfx[gfx_bank]->gfxdata;
	if (PPU_Control1 & 0x01)
		paldata_1 = colortable_mono;
	else
		paldata_1 = Machine->gfx[0]->colortable;

	/* Draw the 32 or 33 tiles that make up a line */
	while (start_x < 256)
	{
		int color_byte;
		int color_bits;
		int pos;
		int index1;
		int page, address;
		int index2;

		index1 = tile_index + x;

		/* Figure out which byte in the color table to use */
		pos = ((index1 & 0x380) >> 4) | ((index1 & 0x1f) >> 2);

		/* TODO: this only needs calculating every 2nd tile - link it to "x" */
		page = (index1 & 0x0c00) >> 10;
		address = 0x3c0 + pos;
		color_byte = ppu_page[page][address];

		/* Figure out which bits in the color table to use */
		color_bits = ((index1 & 0x40) >> 4) + (index1 & 0x02);

		address = index1 & 0x3ff;
		index2 = nes_vram[(ppu_page[page][address] >> 6) | PPU_tile_page] + (ppu_page[page][address] & 0x3f);

#if 0
if ((nes_vram[(ppu_page[page][address] >> 6) | PPU_tile_page] + (ppu_page[page][address] & 0x3f) == 0xfd) || (nes_vram[(ppu_page[page][address] >> 6) | PPU_tile_page] + (ppu_page[page][address] & 0x3f) == 0xfe))
	index2 = rand() & 0xff;
#endif

#ifdef MMC5_VRAM
		/* Use the extended bits if necessary */
		if (MMC5_vram_control & 0x01)
		{
			index2 |= (MMC5_vram[address] & 0x3f) << 8;
		}
#endif

#ifdef LOG_VIDEO
if (videolog) fprintf (videolog, "%02x ", ppu_page[page][address]);
#endif
#ifdef LOG_COLOR
//if (colorlog) fprintf (colorlog, "%02x ", (color_byte >> color_bits) & 0x03);
//if (colorlog) fprintf (colorlog, "%02x ", sd[i]);
#endif

		{
			const UINT32 *paldata;
			const unsigned char *sd;
//			unsigned char *bm;
			int start;

//			paldata = &Machine->gfx[gfx_bank]->colortable[4 * (((color_byte >> color_bits) & 0x03)/* % 8*/)];
			paldata = &paldata_1[4 * (((color_byte >> color_bits) & 0x03))];
//			bm = Machine->scrbitmap->line[scanline] + start_x;
//			sd = &Machine->gfx[gfx_bank]->gfxdata[start * Machine->gfx[gfx_bank]->width];
			start = (index2 % total_elements) * 8 + scroll_y_fine;
			sd = &Machine->gfx[gfx_bank]->gfxdata[start * 8];
//			sd = &sd_1[start * 8];

#ifdef LOG_COLOR
//if (colorlog) fprintf (colorlog, "%02x ", (color_byte >> color_bits) & 0x03);
if (colorlog) fprintf (colorlog, "%02x ", sd[i]);
#endif

			for (i = 0; i < 8; i ++)
			{
				if (start_x + i >= 0)
				{
					if (sd[i])
					{
						plot_pixel (Machine->scrbitmap, start_x+i, scanline, paldata[sd[i]]);
						line_priority[start_x + i] |= 0x02;
					}
				}
			}

			if (ppu_latch)
			{
				(*ppu_latch)((PPU_tile_page << 10) | (ppu_page[page][address] << 4));
			}

		}

		start_x += 8;

		/* Move to next tile over and toggle the horizontal name table if necessary */
		x ++;
		if (x > 31)
		{
			x = 0;
			tile_index ^= 0x400;
		}
	}

#ifdef LOG_VIDEO
if (videolog) fprintf (videolog, "\n");
#endif
#ifdef LOG_COLOR
if (colorlog) fprintf (colorlog, "\n");
#endif

	/* If the left 8 pixels for the background are off, blank 'em */
	/* TODO: handle this properly, along with sprite clipping */
	if (!(PPU_Control1 & PPU_c1_background_L8))
	{
		memset (Machine->scrbitmap->line[scanline], Machine->pens[PPU_background_color & color_mask], 0x08);
	}

draw_sprites:
	/* If sprites are hidden in the leftmost column, fake a priority flag to mask them */
	if (!(PPU_Control1 & PPU_c1_sprites_L8))
	{
		for (i = 0; i < 8; i ++)
			line_priority[i] |= 0x01;
	}

	/* If sprites are on, draw them */
	if (PPU_Control1 & PPU_c1_sprites)
	{
		render_sprites (scanline);
	}

	/* Does the user not want to see the top/bottom 8 lines? */
	if ((readinputport(7) & 0x01) && ((scanline < 8) || (scanline > 231)))
	{
		/* Clear this line if we're not drawing it */
#ifdef COLOR_INTENSITY
		for (i = 0; i < 0x100; i ++)
			((UINT16 *) Machine->scrbitmap->line[scanline])[i] = Machine->pens[0x3f & color_mask];
#else
		memset (Machine->scrbitmap->line[scanline], Machine->pens[0x3f & color_mask], 0x100);
#endif
	}

draw_nothing:
	/* Increment the fine y-scroll */
	PPU_refresh_data += 0x1000;

	/* If it's rolled, increment the coarse y-scroll */
	if (PPU_refresh_data & 0x8000)
	{
		UINT16 tmp;
		tmp = (PPU_refresh_data & 0x03e0) + 0x20;
		PPU_refresh_data &= 0x7c1f;
		/* Handle bizarro scrolling rollover at the 30th (not 32nd) vertical tile */
		if (tmp == 0x03c0)
		{
			PPU_refresh_data ^= 0x0800;
		}
		else
		{
			PPU_refresh_data |= (tmp & 0x03e0);
		}
	}

	profiler_mark(PROFILER_END);
}

static void render_sprites (int scanline)
{
	int x,y;
	int tile, i, index1, page;
	int pri;

	int flipx, flipy, color;
	int size;
	int spriteCount;

	/* Determine if the sprites are 8x8 or 8x16 */
	size = (PPU_Control0 & PPU_c0_sprite_size) ? 16 : 8;

	spriteCount = 0;

	for (i = 0; i < 0x100; i += 4)
//	for (i = 0xfc; i >= 0; i -= 4)
	{
		y = spriteram[i] + 1;

		/* If the sprite isn't visible, skip it */
		if ((y + size <= scanline) || (y > scanline)) continue;

		x	 = spriteram[i+3];
		tile = spriteram[i+1];
		color = (spriteram[i+2] & 0x03) + 4;
		pri = spriteram[i+2] & 0x20;
		flipx = spriteram[i+2] & 0x40;
		flipy = spriteram[i+2] & 0x80;

		if (size == 16)
		{
			/* If it's 8x16 and odd-numbered, draw the other half instead */
			if (tile & 0x01)
			{
				tile &= ~0x01;
				tile |= 0x100;
			}
			/* Note that the sprite page value has no effect on 8x16 sprites */
			page = tile >> 6;
		}
		else page = (tile >> 6) | PPU_sprite_page;

//		if (Mapper == 5)
//			index1 = nes_vram_sprite[page] + (tile & 0x3f);
//		else
			index1 = nes_vram[page] + (tile & 0x3f);

//if (priority == 0)
{
		if (ppu_latch)
		{
//			if ((tile == 0x1fd) || (tile == 0x1fe)) Debugger ();
			(*ppu_latch)((PPU_sprite_page << 10) | ((tile & 0xff) << 4));
		}
//		continue;
}

		{
			int sprite_line;
			int drawn = 0;
			const UINT32 *paldata;
			const unsigned char *sd;
//			unsigned char *bm;
			int start;

			sprite_line = scanline - y;
			if (flipy) sprite_line = (size-1)-sprite_line;

if ((i == 0) /*&& (spriteram[i+2] & 0x20)*/)
{
//	if (y2 == 0)
//		logerror ("sprite 0 (%02x/%02x) tile: %04x, bank: %d, color: %02x, flags: %02x\n", x, y, index1, bank, color, spriteram[i+2]);
//	color = rand() & 0xff;
//	if (y == 0xc0)
//		Debugger ();
}

			paldata = &Machine->gfx[gfx_bank]->colortable[4 * color];
			start = (index1 % Machine->gfx[gfx_bank]->total_elements) * 8 + sprite_line;
//			bm = Machine->scrbitmap->line[scanline] + x;
			sd = &Machine->gfx[gfx_bank]->gfxdata[start * Machine->gfx[gfx_bank]->width];

			if (pri)
			{
				/* Draw the low-priority sprites */
				int j;

				if (flipx)
				{
					for (j = 0; j < 8; j ++)
					{
						/* Is this pixel non-transparent? */
						if (sd[7-j])
						{
							/* Has the background (or another sprite) already been drawn here? */
							if (!line_priority [x+j])
							{
								/* No, draw */
								plot_pixel (Machine->scrbitmap, x+j, scanline, paldata[sd[7-j]]);
								drawn = 1;
							}
							/* Indicate that a sprite was drawn at this location, even if it's not seen */
							line_priority [x+j] |= 0x01;

							/* Set the "sprite 0 hit" flag if appropriate */
							if (i == 0) PPU_Status |= PPU_status_sprite0_hit;
						}
					}
				}
				else
				{
					for (j = 0; j < 8; j ++)
					{
						/* Is this pixel non-transparent? */
						if (sd[j])
						{
							/* Has the background (or another sprite) already been drawn here? */
							if (!line_priority [x+j])
							{
								plot_pixel (Machine->scrbitmap, x+j, scanline, paldata[sd[j]]);
								drawn = 1;
							}
							/* Indicate that a sprite was drawn at this location, even if it's not seen */
							line_priority [x+j] |= 0x01;

							/* Set the "sprite 0 hit" flag if appropriate */
							if (i == 0) PPU_Status |= PPU_status_sprite0_hit;
						}
					}
				}
			}
			else
			{
				/* Draw the high-priority sprites */
				int j;

				if (flipx)
				{
					for (j = 0; j < 8; j ++)
					{
						/* Is this pixel non-transparent? */
						if (sd[7-j])
						{
							/* Has another sprite been drawn here? */
							if (!(line_priority[x+j] & 0x01))
							{
								/* No, draw */
								plot_pixel (Machine->scrbitmap, x+j, scanline, paldata[sd[7-j]]);
								line_priority [x+j] |= 0x01;
								drawn = 1;
							}

							/* Set the "sprite 0 hit" flag if appropriate */
							if ((i == 0) && (line_priority[x+j] & 0x02))
								PPU_Status |= PPU_status_sprite0_hit;
						}
					}
				}
				else
				{
					for (j = 0; j < 8; j ++)
					{
						/* Is this pixel non-transparent? */
						if (sd[j])
						{
							/* Has another sprite been drawn here? */
							if (!(line_priority[x+j] & 0x01))
							{
								/* No, draw */
								plot_pixel (Machine->scrbitmap, x+j, scanline, paldata[sd[j]]);
								line_priority [x+j] |= 0x01;
								drawn = 1;
							}

							/* Set the "sprite 0 hit" flag if appropriate */
							if ((i == 0) && (line_priority[x+j] & 0x02))
								PPU_Status |= PPU_status_sprite0_hit;
						}
					}
				}
			}

			if (drawn)
			{
				/* If there are more than 8 sprites on this line, set the flag */
				spriteCount ++;
				if (spriteCount == 8)
				{
					PPU_Status |= PPU_status_8sprites;
					logerror ("> 8 sprites (%d), scanline: %d\n", spriteCount, scanline);

					/* The real NES only draws up to 8 sprites - the rest should be invisible */
					if ((readinputport (7) & 0x02) == 0)
						break;
				}
			}
		}
	}
}

WRITE_HANDLER(nes_vh_sprite_dma_w)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy (spriteram, &RAM[data * 0x100], 0x100);
#ifdef MAME_DEBUG
#ifdef macintosh
	if (data >= 0x40) SysBeep (0);
#endif
#endif
}

static void draw_sight(int playerNum, int x_center, int y_center)
{
	int x,y;
	UINT16 color;

	if (playerNum == 2)
		color = Machine->pens[0]; /* grey */
	else
		color = Machine->pens[0x30]; /* white */

	if (x_center<2)   x_center=2;
	if (x_center>253) x_center=253;

	if (y_center<2)   y_center=2;
	if (y_center>253) y_center=253;

	for(y = y_center-5; y < y_center+6; y++)
		if((y >= 0) && (y < 256))
			plot_pixel (Machine->scrbitmap, x_center, y, color);

	for(x = x_center-5; x < x_center+6; x++)
		if((x >= 0) && (x < 256))
			plot_pixel (Machine->scrbitmap, x, y_center, color);
}

/* This routine is called at the start of vblank to refresh the screen */
void nes_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
#ifdef BIG_SCREEN
	int page;
	int offs;
	int Size;
	int i;
	int page1, address1;
#endif

	if (readinputport (2) == 0x01) /* zapper on port 1 */
	{
		draw_sight (1, readinputport (3), readinputport (4));
	}
	else if (readinputport (2) == 0x10) /* zapper on port 2 */
	{
		draw_sight (1, readinputport (3), readinputport (4));
	}
	else if (readinputport (2) == 0x11) /* zapper on both ports */
	{
		draw_sight (1, readinputport (3), readinputport (4));
		draw_sight (2, readinputport (5), readinputport (6));
	}

	/* if this is a disk system game, check for the flip-disk key */
	if (nes.mapper == 20)
	{
		if (readinputport (11) & 0x01)
		{
			while (readinputport (11) & 0x01) { update_input_ports (); };

			nes_fds.current_side ++;
			if (nes_fds.current_side > nes_fds.sides)
				nes_fds.current_side = 0;

			if (nes_fds.current_side == 0)
			{
				usrintf_showmessage ("No disk inserted.");
			}
			else
			{
				usrintf_showmessage ("Disk set to side %d", nes_fds.current_side);
			}
		}
	}


#ifndef BIG_SCREEN
	/* If we're using the scanline engine, the screen has already been drawn */
	return;
#endif

#ifdef BIG_SCREEN /* This is all debugging code */
	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < VRAM_SIZE; offs++)
	{
		/* Do page 1 */
		page = 0x2000;
#ifndef BIG_SCREEN
		if (dirtybuffer[offs])
#endif
		{
			int sx,sy;
			int index1, index2, index3;

			int pos, color;
			int bank;

#ifdef DIRTY_BUFFERS
			dirtybuffer[offs] = 0;
#endif
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;

			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			page1 = (index2 & 0x0c00) >> 10;
			address1 = 0x3c0 + pos;
			color = ppu_page[page1][address1];

			/* Figure out which bits in the color table to use */
			index1 = ((index2 & 0x40) >> 4) + (index2 & 0x02);

			address1 = index2 & 0x3ff;
			index3 = nes_vram[(ppu_page[page1][address1] >> 6) | PPU_tile_page] + (ppu_page[page1][address1] & 0x3f);

			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;

			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index1) & 0x03,
				0,0,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 2 - drawn to the right of page 1 */
		page += 0x400;
#ifndef BIG_SCREEN
		if (dirtybuffer2[offs])
#endif
		{
			int sx,sy;
			int index1, index2, index3;

			int pos, color;
			int bank;

#ifdef DIRTY_BUFFERS
			dirtybuffer2[offs] = 0;
#endif
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;

			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			page1 = (index2 & 0x0c00) >> 10;
			address1 = 0x3c0 + pos;
			color = ppu_page[page1][address1];

			/* Figure out which bits in the color table to use */
			index1 = ((index2 & 0x40) >> 4) + (index2 & 0x02);

			address1 = index2 & 0x3ff;
			index3 = nes_vram[(ppu_page[page1][address1] >> 6) | PPU_tile_page] + (ppu_page[page1][address1] & 0x3f);

			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;

			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index1) & 0x03,
				0,0,
				32*8 + 8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 3 - drawn below page 1 */
		page += 0x400;
#ifndef BIG_SCREEN
		if (dirtybuffer3[offs])
#endif
		{
			int sx,sy;
			int index1, index2, index3;

			int pos, color;
			int bank;

#ifdef DIRTY_BUFFERS
			dirtybuffer3[offs] = 0;
#endif
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;

			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			page1 = (index2 & 0x0c00) >> 10;
			address1 = 0x3c0 + pos;
			color = ppu_page[page1][address1];

			/* Figure out which bits in the color table to use */
			index1 = ((index2 & 0x40) >> 4) + (index2 & 0x02);

			address1 = index2 & 0x3ff;
			index3 = nes_vram[(ppu_page[page1][address1] >> 6) | PPU_tile_page] + (ppu_page[page1][address1] & 0x3f);

			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;

			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index1) & 0x03,
				0,0,
				8*sx, 30*8 + 8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 4 - drawn to the right and below page 1 */
		page += 0x400;
#ifndef BIG_SCREEN
		if (dirtybuffer4[offs])
#endif
		{
			int sx,sy;
			int index1, index2, index3;

			int pos, color;
			int bank;

#ifdef DIRTY_BUFFERS
			dirtybuffer4[offs] = 0;
#endif
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;

			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			page1 = (index2 & 0x0c00) >> 10;
			address1 = 0x3c0 + pos;
			color = ppu_page[page1][address1];

			/* Figure out which bits in the color table to use */
			index1 = ((index2 & 0x40) >> 4) + (index2 & 0x02);

			address1 = index2 & 0x3ff;
			index3 = nes_vram[(ppu_page[page1][address1] >> 6) | PPU_tile_page] + (ppu_page[page1][address1] & 0x3f);

			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;

			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index1) & 0x03,
				0,0,
				32*8 + 8*sx, 30*8 + 8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	/* TODO: take into account the currently selected page or one-page mode, plus scrolling */
#if 0
	/* 1 */
	scrollx = (Machine->drv->screen_width - PPU_Scroll_X) & 0xff;
	if (PPU_Scroll_Y)
		scrolly = (Machine->drv->screen_height - PPU_Scroll_Y) & 0xff;
	else scrolly = 0;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	/* 2 */
	scrollx += Machine->drv->screen_width;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	/* 4 */
	scrolly += Machine->drv->screen_height;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	/* 3 */
	scrollx = (Machine->drv->screen_width - PPU_Scroll_X);
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
#endif

#ifdef BIG_SCREEN
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
#endif

	/* Now draw the sprites */

	/* Determine if the sprites are 8x8 or 8x16 */
	Size = (PPU_Control0 & 0x20) ? 16 : 8;

	for (i = 0xfc; i >= 0; i -= 4)
	{
		int y, tile, index1;


		y = spriteram[i] + 1; /* TODO: is the +1 hack needed? see if PPU_Scroll_Y of 255 has any effect */

		/* If the sprite isn't visible, skip it */
		if (y > BOTTOM_VISIBLE_SCANLINE) continue;

		tile = spriteram[i+1];

		if (Size == 16)
		{
			/* If it's 8x16 and odd-numbered, draw the other half instead */
			if (tile & 0x01)
			{
				tile &= 0xfe;
				tile |= 0x100;
			}
			/* Note that the sprite page value has no effect on 8x16 sprites */
			page = tile >> 6;
		}
		else page = (tile >> 6) | PPU_sprite_page;

		/* TODO: add code to draw 8x16 sprites */
		index1 = nes_vram[page] + (tile & 0x3f);
		{
			int bank;

			if (CHR_Rom == 0)
				bank = 1;
			else bank = 0;

			if (spriteram[i+2] & 0x20)
				/* Draw sprites with the priority bit set behind the background */
				drawgfx (bitmap, Machine->gfx[bank],
					index1,
					(spriteram[i+2] & 0x03) + 4,
					spriteram[i+2] & 0x40,spriteram[i+2] & 0x80,
					spriteram[i+3],y,
					&Machine->visible_area,TRANSPARENCY_THROUGH, PPU_background_color);
			else
				drawgfx (bitmap, Machine->gfx[bank],
					index1,
					(spriteram[i+2] & 0x03) + 4,
					spriteram[i+2] & 0x40,spriteram[i+2] & 0x80,
					spriteram[i+3],y,
					&Machine->visible_area,TRANSPARENCY_PEN, 0);
		}
	}
#endif
}

