/***************************************************************************

  vidhrdw/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "driver.h"
#include "generic.h"

#include "machine/nes.h"

#define VIDEORAM_SIZE	0x4000
#define SPRITERAM_SIZE	0x100
#define VRAM_SIZE	0x3c0

int nes_vram[8]; /* Keep track of 8 .5k vram pages to speed things up */
int dirtychar[0x200];

unsigned char *dirtybuffer2;
unsigned char *dirtybuffer3;
unsigned char *dirtybuffer4;

static void render_sprites (int scanline);

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int nes_vh_start(void)
{
	int i;

	if ((videoram = malloc (VIDEORAM_SIZE)) == 0)
		return 1;

	/* We use an offscreen bitmap that's 4 times as large as the visible one */
	if ((tmpbitmap = osd_new_bitmap(2 * Machine->drv->screen_width, 2 * Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		free (videoram);
		osd_free_bitmap (tmpbitmap);
		return 1;
	}

	if ((spriteram = malloc (SPRITERAM_SIZE)) == 0)
	{
		free (videoram);
		osd_free_bitmap (tmpbitmap);
		return 1;
	}

	dirtybuffer  = malloc (VRAM_SIZE);
	dirtybuffer2 = malloc (VRAM_SIZE);
	dirtybuffer3 = malloc (VRAM_SIZE);
	dirtybuffer4 = malloc (VRAM_SIZE);
	
	if ((!dirtybuffer) || (!dirtybuffer2) || (!dirtybuffer3) || (!dirtybuffer4))
	{
		free (videoram);
		osd_free_bitmap (tmpbitmap);
		free (spriteram);
		free (dirtybuffer);
		free (dirtybuffer2);
		free (dirtybuffer3);
		free (dirtybuffer4);
		return 1;
	}

	memset (dirtybuffer,  1, VRAM_SIZE);
	memset (dirtybuffer2, 1, VRAM_SIZE);
	memset (dirtybuffer3, 1, VRAM_SIZE);
	memset (dirtybuffer4, 1, VRAM_SIZE);

	/* Mark all chars as 'clean' */
	for (i = 0; i < 0x200; i ++)
		dirtychar[i] = 0;
	
	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void nes_vh_stop(void)
{
	free (videoram);
	free (spriteram);
	free (dirtybuffer);
	free (dirtybuffer2);
	free (dirtybuffer3);
	free (dirtybuffer4);
	osd_free_bitmap (tmpbitmap);
}

/***************************************************************************

  Draw the current scanline

***************************************************************************/

void nes_vh_renderscanline (int scanline, int drawline)
{
	int x;
	int name_table;
	int start_x;
	int start_y,start_y_mod_eight;
	int tile_index;
	int bank;

	if (readinputport(2)) return;

	if ((scanline < TOP_VISIBLE_SCANLINE) || (scanline > BOTTOM_VISIBLE_SCANLINE))
		return;
	
	/* Determine where in the nametable to start drawing from, based on the current scanline and scroll regs */
	start_x = ((PPU_Scroll_X & 0x07) ^ 0x07) - 7;
	start_y = drawline + PPU_Scroll_Y;

	if (start_y < 0) return;
	
	if (PPU_one_screen)
		name_table = PPU_one_screen;
	else
		name_table = PPU_name_table;

	/* See if we've scrolled off the bottom of the current name table */
	if (start_y > 239)
	{
		start_y -= 240;
		/* If normal mirroring, toggle the vertical name table */
		if (!PPU_one_screen) name_table ^= 0x0800;
	}
	start_y_mod_eight=start_y%8;

	/* The tile is found using this formula: ((start_y / 8) * 32) + (start_x / 8) */
	x = PPU_Scroll_X >> 3;
	tile_index = name_table + (start_y / 8 * 32);
	
	if (CHR_Rom == 0) bank = 1;
	else bank = 0;
			
	/* Draw the 32 or 33 tiles that make up a line */
	while (start_x < 256)
	{
		int color_byte;
		int color_bits;
		int pos;
		int index;
		int index2;

		index = tile_index + x;
		
		/* Figure out which byte in the color table to use */
		pos = ((index & 0x380) >> 4) | ((index & 0x1f) >> 2);
		/* TODO: this only needs calculating every 4th tile - link it to "x" */
		color_byte = videoram [(index & 0x3c00) + 0x3c0 + pos];

		/* Figure out which bits in the color table to use */
		color_bits = ((index & 0x40) >> 4) + (index & 0x02);
		
		index2 = nes_vram[(videoram[index] >> 6) | PPU_tile_page] + (videoram[index] & 0x3f);

		{
			const unsigned short *paldata;
			const unsigned char *sd;
			unsigned char *bm;
			int start;
//			int sx;
			
//			sx = start_x;
			
//			if (sx < 0) sx = 0;

			paldata = &Machine->gfx[bank]->colortable[4 * (((color_byte >> color_bits) & 0x03) % 8)];
			start = (index2 % Machine->gfx[bank]->total_elements) * 8 + start_y_mod_eight;
			bm = Machine->scrbitmap->line[scanline] + start_x;
//			bm = Machine->scrbitmap->line[scanline] + sx;
			sd = Machine->gfx[bank]->gfxdata->line[start];

//			if (sx + 7 < 256)
//			{
			bm[0] = paldata[sd[0]];
			bm[1] = paldata[sd[1]];
			bm[2] = paldata[sd[2]];
			bm[3] = paldata[sd[3]];
			bm[4] = paldata[sd[4]];
			bm[5] = paldata[sd[5]];
			bm[6] = paldata[sd[6]];
			bm[7] = paldata[sd[7]];
//			}
		}
		
		start_x += 8;

		/* Move to next tile over and toggle the horizontal name table if necessary */
		x ++;
		if (x > 31)
		{
			x = 0;
			if (!PPU_one_screen) tile_index ^= 0x400;
		}
	}

	/* If sprites are on, draw them */
	if (PPU_Control1 & 0x10)
	{
		render_sprites (scanline);
	}
}

static void render_sprites (int scanline)
{
	int     x,y;
	int     tile, i, index, page;
	int     flipx, flipy, color;
	int     Size;

	/* Determine if the sprites are 8x8 or 8x16 */
	Size = (PPU_Control0 & 0x20) ? 16 : 8;

	for (i = 0xfc; i >= 0; i -= 4)        
	{
		y = spriteram[i] + 1; /* TODO: is the +1 hack needed? */

		/* If the sprite isn't visible, skip it */
		if ((y + Size <= scanline) || (y > scanline)) continue;
		
		x    = spriteram[i+3];
		tile = spriteram[i+1];
		color = (spriteram[i+2] & 0x03) + 4;
		flipx = spriteram[i+2] & 0x40;
		flipy = spriteram[i+2] & 0x80;

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

		index = nes_vram[page] + (tile & 0x3f);
		{
			int y2;
			int bank;
			
			y2 = scanline - y;
			if (flipy) y2 = (Size-1)-y2;

			if (CHR_Rom == 0)
				bank = 1;
			else bank = 0;

			/* TODO: inline the drawgfx_line code */
			if (spriteram[i+2] & 0x20)
				/* Draw sprites with the priority bit set behind the background */
				drawgfx_line (Machine->scrbitmap, Machine->gfx[bank],
					index,
					color,
					flipx,y2,
					x,scanline,
					&Machine->drv->visible_area,TRANSPARENCY_THROUGH, PPU_background_color);
			else
				drawgfx_line (Machine->scrbitmap, Machine->gfx[bank],
					index,
					color,
					flipx,y2,
					x,scanline,
					&Machine->drv->visible_area,TRANSPARENCY_PEN, 0);
		}
	}
}

void nes_vh_sprite_dma_w (int offset, int data)
{
	memcpy (spriteram, &RAM[data * 0x100], 0x100);
}

/* This routine is called at the start of vblank to refresh the screen */
void nes_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int page;
	int offs;
	int Size;
	int i;
	int scrollx, scrolly;
	
	/* If we're using the scanline engine, flag every frame as having scanline effects */
	if (!readinputport (2))
	{
		PPU_scanline_effects = 1;
		return;
	}

#if 0
	/* If a scanline-effect has happened, assume the rest of the screen has already been rendered */
	if (PPU_scanline_effects)
	{
		PPU_scanline_effects = 0;
		return;
	}
#endif

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < VRAM_SIZE; offs++)
	{
		/* Do page 1 */
		page = 0x2000;
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int index, index2, index3;
			int pos, color;
			int bank;

			dirtybuffer[offs] = 0;
			
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;
		
			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			color = videoram [(index2 & 0x3c00) + 0x3c0 + pos];

			/* Figure out which bits in the color table to use */
			index = ((index2 & 0x40) >> 4) + (index2 & 0x02);
		
			index3 = nes_vram[(videoram[index2] >> 6) | PPU_tile_page] + (videoram[index2] & 0x3f);
			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;
			
			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index) & 0x03,
				0,0,
				8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 2 - drawn to the right of page 1 */
		page += 0x400;
		if (dirtybuffer2[offs])
		{
			int sx,sy;
			int index, index2, index3;
			int pos, color;
			int bank;

			dirtybuffer2[offs] = 0;
			
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;
		
			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			color = videoram [(index2 & 0x3c00) + 0x3c0 + pos];

			/* Figure out which bits in the color table to use */
			index = ((index2 & 0x40) >> 4) + (index2 & 0x02);
		
			index3 = nes_vram[(videoram[index2] >> 6) | PPU_tile_page] + (videoram[index2] & 0x3f);
			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;
			
			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index) & 0x03,
				0,0,
				Machine->drv->screen_width + 8*sx,8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 3 - drawn below page 1 */
		page += 0x400;
		if (dirtybuffer3[offs])
		{
			int sx,sy;
			int index, index2, index3;
			int pos, color;
			int bank;

			dirtybuffer3[offs] = 0;
			
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;
		
			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			color = videoram [(index2 & 0x3c00) + 0x3c0 + pos];

			/* Figure out which bits in the color table to use */
			index = ((index2 & 0x40) >> 4) + (index2 & 0x02);
		
			index3 = nes_vram[(videoram[index2] >> 6) | PPU_tile_page] + (videoram[index2] & 0x3f);
			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;
			
			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index) & 0x03,
				0,0,
				8*sx, Machine->drv->screen_height + 8*sy,
				0,TRANSPARENCY_NONE,0);
		}

		/* Do page 4 - drawn to the right and below page 1 */
		page += 0x400;
		if (dirtybuffer4[offs])
		{
			int sx,sy;
			int index, index2, index3;
			int pos, color;
			int bank;

			dirtybuffer4[offs] = 0;
			
			sx = offs % 32;
			sy = offs / 32;

			index2 = page + offs;
		
			/* Figure out which byte in the color table to use */
			pos = (index2 & 0x380) >> 4;
			pos += (index2 & 0x1f) >> 2;
			color = videoram [(index2 & 0x3c00) + 0x3c0 + pos];

			/* Figure out which bits in the color table to use */
			index = ((index2 & 0x40) >> 4) + (index2 & 0x02);
		
			index3 = nes_vram[(videoram[index2] >> 6) | PPU_tile_page] + (videoram[index2] & 0x3f);
			if (use_vram[index3])
				bank = 1;
			else
				bank = 0;
			
			drawgfx(tmpbitmap, Machine->gfx[bank],
				index3,
				(color >> index) & 0x03,
				0,0,
				Machine->drv->screen_width + 8*sx, Machine->drv->screen_height + 8*sy,
				0,TRANSPARENCY_NONE,0);
		}
	}
	
	/* TODO: take into account the currently selected page or one-page mode, plus scrolling */
	/* 1 */
	scrollx = (Machine->drv->screen_width - PPU_Scroll_X) & 0xff;
	if (PPU_Scroll_Y)
		scrolly = (Machine->drv->screen_height - PPU_Scroll_Y) & 0xff;
	else scrolly = 0;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
#if 0
	/* 2 */
	scrollx += Machine->drv->screen_width;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	/* 4 */
	scrolly += Machine->drv->screen_height;
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	/* 3 */
	scrollx = (Machine->drv->screen_width - PPU_Scroll_X);
	copyscrollbitmap (bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
#endif

	/* copy the character mapped graphics */
//	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Now draw the sprites */

	/* Determine if the sprites are 8x8 or 8x16 */
	Size = (PPU_Control0 & 0x20) ? 16 : 8;

	for (i = 0xfc; i >= 0; i -= 4)        
	{
		int y, tile, index;
		
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
		index = nes_vram[page] + (tile & 0x3f);
		{
			int bank;
			
			if (CHR_Rom == 0)
				bank = 1;
			else bank = 0;

			if (spriteram[i+2] & 0x20)
				/* Draw sprites with the priority bit set behind the background */
				drawgfx (bitmap, Machine->gfx[bank],
					index,
					(spriteram[i+2] & 0x03) + 4,
					spriteram[i+2] & 0x40,spriteram[i+2] & 0x80,
					spriteram[i+3],y,
					&Machine->drv->visible_area,TRANSPARENCY_THROUGH, PPU_background_color);
			else
				drawgfx (bitmap, Machine->gfx[bank],
					index,
					(spriteram[i+2] & 0x03) + 4,
					spriteram[i+2] & 0x40,spriteram[i+2] & 0x80,
					spriteram[i+3],y,
					&Machine->drv->visible_area,TRANSPARENCY_PEN, 0);
		}
	}
}