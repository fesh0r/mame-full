/***************************************************************************

  vidhrdw/superman.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"

size_t supes_videoram_size;
size_t supes_attribram_size;

unsigned char *supes_videoram;
unsigned char *supes_attribram;
//static unsigned char *dirtybuffer;		/* foreground */
//static unsigned char *dirtybuffer2;		/* background */

static int tilemask      = 0x3fff;		// Superman : 0x3fff, Balloon Bros : 0x0fff
static int xstart        = 0;			// For Daisenpu
static int fill_backdrop = 0;			// For Daisenpu

int superman_vh_start (void)
{
	tilemask = 0x3fff;
	xstart   = 16;
	return 0;
}

void superman_vh_stop (void)
{
}

int ballbros_vh_start (void)
{
	tilemask = 0x0fff;
	xstart   = 16;
	return 0;
}

int daisenpu_vh_start (void)
{
	tilemask = 0x3fff;
	xstart   = 16;
	fill_backdrop = 1;
	return 0;
}


/*************************************
 *
 *		Foreground RAM
 *
 *************************************/

WRITE_HANDLER( supes_attribram_w )
{
   int oldword = READ_WORD (&supes_attribram[offset]);
   int newword = COMBINE_WORD (oldword, data);

   if (oldword != newword)
   {
		WRITE_WORD (&supes_attribram[offset], data);
//		dirtybuffer2[offset/2] = 1;
   }
}

READ_HANDLER( supes_attribram_r )
{
   return READ_WORD (&supes_attribram[offset]);
}



/*************************************
 *
 *		Background RAM
 *
 *************************************/

WRITE_HANDLER( supes_videoram_w )
{
   int oldword = READ_WORD (&supes_videoram[offset]);
   int newword = COMBINE_WORD (oldword, data);

   if (oldword != newword)
   {
		WRITE_WORD (&supes_videoram[offset], data);
//		dirtybuffer[offset/2] = 1;
   }
}

READ_HANDLER( supes_videoram_r )
{
   return READ_WORD (&supes_videoram[offset]);
}


void superman_update_palette (int bankbase)
{
	unsigned short palette_map[32]; /* range of color table is 0-31 */
	int i;

	memset (palette_map, 0, sizeof (palette_map));

	/* Find colors used in the background tile plane */
	for (i = 0; i < 0x400; i += 0x40)
	{
		int i2;

		for (i2 = i; i2 < (i + 0x40); i2 += 2)
		{
			int tile;
			int color;

			color = 0;

			tile = READ_WORD (&supes_videoram[0x800 + bankbase + i2]) & tilemask;
			if (tile)
				color = READ_WORD (&supes_videoram[0xc00 + bankbase + i2]) >> 11;
			else
				color = 0x1f;
				
			palette_map[color] |= Machine->gfx[0]->pen_usage[tile];

		}
	}

	/* Find colors used in the sprite plane */
	for (i = 0x3fe; i >= 0; i -= 2)
	{
		int tile;
		int color;

		color = 0;

		tile = READ_WORD (&supes_videoram[i + bankbase]) & tilemask;
		if (tile)
			color = READ_WORD (&supes_videoram[0x400 + bankbase + i]) >> 11;

		palette_map[color] |= Machine->gfx[0]->pen_usage[tile];
	}

	/* Now tell the palette system about those colors */
	for (i = 0;i < 32;i++)
	{
		int usage = palette_map[i];
		int j;

		if (usage)
		{
			palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
				else
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
		}
		else
			memset(&palette_used_colors[i * 16],PALETTE_COLOR_UNUSED,16);
	}

	palette_recalc ();

}

void superman_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;
	int bankbase;
	int attribfix;
	int cocktail;
	
	/* Set bank base */
	if ( (READ_WORD(&supes_attribram[0x602]) & 0x40 ) )
		bankbase = 0x2000;
	else
		bankbase = 0x0000;
	
	osd_clearbitmap(bitmap);
	
	/* attribute fix */
	attribfix = ( ( READ_WORD(&supes_attribram[0x604]) & 0xff ) << 8 ) |
	            ( ( READ_WORD(&supes_attribram[0x606]) & 0xff ) << 16 );
	
	/* cocktail mode */
	cocktail = READ_WORD(&supes_attribram[0x600]) & 0x40;
	
	/* Update palette */
	superman_update_palette (bankbase);
	
	/* Fill backdrop color for Daisenpu */
	if (fill_backdrop)
		fillbitmap(bitmap, Machine->pens[0x34], &Machine->visible_area);			/* This may be wrong. */
	
	/* Refresh the background tile plane */
	for (i = 0; i < 0x400 ; i += 0x40)
	{
		int x1, y1;
		int i2;
		int x, y;
				
		x1 = READ_WORD (&supes_attribram[0x408 + (i >> 1)]) | (attribfix & 0x100);
		y1 = READ_WORD (&supes_attribram[0x400 + (i >> 1)]);
		
		attribfix >>= 1;
		
		for (i2 = i; i2 < (i + 0x40); i2 += 2)
		{
			int tile;

			tile = READ_WORD (&supes_videoram[0x800 + bankbase + i2]) & tilemask;

			if (tile)
			{
				int flipx = READ_WORD (&supes_videoram[0x800 + bankbase + i2]) & 0x8000;
				int flipy = READ_WORD (&supes_videoram[0x800 + bankbase + i2]) & 0x4000;
				int color = READ_WORD (&supes_videoram[0xc00 + bankbase + i2]) >> 11;

				x = ((x1 + ((i2 & 0x03) << 3)) + xstart) & 0x1ff;
				if ( !cocktail )
					y = (265 -  (y1 - ((i2 & 0x3c) << 2))    ) & 0xff;
				else
					y = (       (y1 - ((i2 & 0x3c) << 2)) - 7) & 0xff;
			
				if (cocktail){
					flipx ^= 0x8000;
					flipy ^= 0x4000;
				}

				/* Some tiles are transparent, e.g. the gate, so we use TRANSPARENCY_PEN */
				drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					flipx,flipy,
					x,y,
					&Machine->visible_area,
					TRANSPARENCY_PEN,0);
			}
		}
	}

	/* Refresh the sprite plane */
	for (i = 0x3fe; i >= 0; i -= 2)
	{
		int sprite;

		sprite = READ_WORD (&supes_videoram[i + bankbase]) & tilemask;
		
		if (sprite)
		{
			int x, y;

			int flipx = READ_WORD (&supes_videoram[        bankbase + i]) & 0x8000;
			int flipy = READ_WORD (&supes_videoram[        bankbase + i]) & 0x4000;
			int color = READ_WORD (&supes_videoram[0x400 + bankbase + i]) >> 11;

			if (cocktail){
				flipx ^= 0x8000;
				flipy ^= 0x4000;
			}

			x = (READ_WORD (&supes_videoram [0x400 + bankbase + i])  + xstart)  & 0x1ff;
			
			if (!cocktail)
				y = (250 - READ_WORD (&supes_attribram[i])) & 0xff;
			else
				y = (10  + READ_WORD (&supes_attribram[i])) & 0xff;
			
			drawgfx(bitmap,Machine->gfx[0],
				sprite,
				color,
				flipx,flipy,
				x,y,
				&Machine->visible_area,
				TRANSPARENCY_PEN,0);
		}
	}
}
