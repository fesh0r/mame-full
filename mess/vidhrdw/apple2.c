/***************************************************************************

  vidhrdw/apple2.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple2.h"

unsigned char *apple2_lores_text1_ram;
unsigned char *apple2_lores_text2_ram;
unsigned char *apple2_hires1_ram;
unsigned char *apple2_hires2_ram;

/* temp bitmaps for quicker blitting */
static struct mame_bitmap *apple2_text[2];
static struct mame_bitmap *apple2_lores[2];
static struct mame_bitmap *apple2_hires[2];

/* dirty video markers */
static unsigned char *dirty_text[2];
static unsigned char *dirty_lores[2];
static unsigned char *dirty_hires[2];
static unsigned char any_dirty_text[2];
static unsigned char any_dirty_lores[2];
static unsigned char any_dirty_hires[2];

/***************************************************************************
  apple2_vh_start
***************************************************************************/
VIDEO_START( apple2 )
{
	int i;

	apple2_text[0] = apple2_text[1] = 0;
	apple2_lores[0] = apple2_lores[1] = 0;
	apple2_hires[0] = apple2_hires[1] = 0;

	dirty_text[0] = dirty_text[1] = 0;
	dirty_lores[0] = dirty_lores[1] = 0;
	dirty_hires[0] = dirty_hires[1] = 0;

	for (i=0; i<2; i++)
	{
		if ((apple2_text[i] = auto_bitmap_alloc(280*2,192*2)) == 0)
			return 1;
		if ((apple2_lores[i] = auto_bitmap_alloc(280*2,192*2)) == 0)
			return 1;
		if ((apple2_hires[i] = auto_bitmap_alloc(280*2,192*2)) == 0)
			return 1;
		if ((dirty_text[i] = auto_malloc(0x400)) == 0)
			return 1;
		if ((dirty_lores[i] = auto_malloc(0x400)) == 0)
			return 1;
		if ((dirty_hires[i] = auto_malloc(0x2000)) == 0)
			return 1;

		memset(dirty_text[i],1,0x400);
		memset(dirty_lores[i],1,0x400);
		memset(dirty_hires[i],1,0x2000);
		any_dirty_text[i] = any_dirty_lores[i] = any_dirty_hires[i] = 1;
	}

	return 0;
}

/***************************************************************************
  apple2_text_draw
***************************************************************************/
static void apple2_text_draw(int page)
{
	int x,y;
	int offset;
	unsigned char *text_ram;

#if 0
	if (!any_dirty_text[page])
		return;

	if (page==0)
		text_ram = apple2_lores_text1_ram;
	else
		text_ram = apple2_lores_text2_ram;
#else
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;

	if (page==0)
		text_ram = &RAM[0x400 + auxram];
	else
		text_ram = &RAM[0x800 + auxram];
}
#endif

	for (offset = 0x3FF; offset >= 0; offset--)
	{
//		if (dirty_text[page][offset])
		{
			dirty_text[page][offset] = 0;

			/* Special Apple II addressing.  Gotta love it. */
			x = (offset & 0x7F);
			y = (offset >> 7) & 0x07;
			if (x >= 0x50)
				y += 16;
			else if (x >= 0x28)
				y += 8;
			x = x % 40;

			drawgfx(apple2_text[page], Machine->gfx[0],
					text_ram[offset],	/* Character */
					12,		/* Color */
					0,0,
					x*7*2,y*8,
					0,TRANSPARENCY_NONE,0);
		}
	}
	any_dirty_text[page] = 0;
}

/***************************************************************************
  apple2_lores_draw
***************************************************************************/
static void apple2_lores_draw(int page)
{
	int x,y;
	int offset;
	unsigned char *lores_ram;

#if 0
	if (!any_dirty_lores[page])
		return;

	if (page==0)
		lores_ram = apple2_lores_text1_ram;
	else
		lores_ram = apple2_lores_text2_ram;
#else
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;

	if (page==0)
		lores_ram = &RAM[0x400 + auxram];
	else
		lores_ram = &RAM[0x800 + auxram];
}
#endif

	for (offset = 0x3FF; offset >= 0; offset--)
	{
//		if (dirty_lores[page][offset])
		{
			dirty_lores[page][offset] = 0;

			/* Special Apple II addressing.  Gotta love it. */
			x = (offset & 0x7F);
			y = (offset >> 7) & 0x07;
			if (x >= 0x50)
				y += 16;
			else if (x >= 0x28)
				y += 8;
			x = x % 40;

			drawgfx(apple2_lores[page], Machine->gfx[2 + (x & 0x01)],
					lores_ram[offset],	/* Character */
					12,		/* Color */
					0,0,
					x*7*2,y*8,
					0,TRANSPARENCY_NONE,0);
		}
	}
	any_dirty_lores[page] = 0;
}

/***************************************************************************
  apple2_hires_draw
***************************************************************************/
static void apple2_hires_draw(int page)
{
	int x,y;
	int offset;
	unsigned char *hires_ram;

#if 0
	if (!any_dirty_hires[page])
		return;

	if (page==0)
		hires_ram = apple2_hires1_ram;
	else
		hires_ram = apple2_hires2_ram;
#else
	unsigned char *RAM = memory_region(REGION_CPU1);
	int auxram = a2.RAMRD ? 0x10000 : 0x0000;

	if (page==0)
		hires_ram = &RAM[0x2000 + auxram];
	else
		hires_ram = &RAM[0x4000 + auxram];
#endif

	for (offset = 0x1FFF; offset >= 0; offset--)
	{
//		if (dirty_hires[page][offset])
		{
			dirty_hires[page][offset] = 0;

			/* Special Apple II addressing.  Gotta love it. */
			x = (offset & 0x007F);
			y = (offset & 0x0380) >> 7;
			if (x >= 0x50)
				y += 16;
			else if (x >= 0x28)
				y += 8;
			x = x % 40;

			y = y * 8;
			y += ((offset & 0x1C00) >> 10);

#if 0
			drawgfx(apple2_hires[page], Machine->gfx[5 + ((hires_ram[offset] & 0x80) >> 7)],
					(hires_ram[offset] & 0x7F),	/* Character */
					12,		/* Color */
					0,0,
					x*7*2,y,
					0,TRANSPARENCY_NONE,0);
#else
			if (hires_ram[offset] & 0x80)
				drawgfx(apple2_hires[page], Machine->gfx[5],
					(hires_ram[offset] & 0x7F),	/* Character */
					12,		/* Color */
					0,0,
					x*7*2+1,y,
					0,TRANSPARENCY_NONE,0);
			else
				drawgfx(apple2_hires[page], Machine->gfx[5],
					(hires_ram[offset] & 0x7F),	/* Character */
					12,		/* Color */
					0,0,
					x*7*2,y,
					0,TRANSPARENCY_NONE,0);

#endif
		}
	}
	any_dirty_hires[page] = 0;
}

/***************************************************************************
  apple2_lores_text1_w
***************************************************************************/
void apple2_lores_text1_w(int offset, int data)
{
	if (apple2_lores_text1_ram[offset] == data)
		return;

	apple2_lores_text1_ram[offset] = data;

	if (((offset & 0x007F) >= 0x0078) && ((offset & 0x007F) <= 0x007F))
		return;

	dirty_text[0][offset] = 1;
	dirty_lores[0][offset] = 1;
	any_dirty_text[0] = any_dirty_lores[0] = 1;
}

/***************************************************************************
  apple2_lores_text2_w
***************************************************************************/
void apple2_lores_text2_w(int offset, int data)
{
	if (apple2_lores_text2_ram[offset] == data)
		return;

	apple2_lores_text2_ram[offset] = data;

	if (((offset & 0x007F) >= 0x0078) && ((offset & 0x007F) <= 0x007F))
		return;

	dirty_text[1][offset] = 1;
	dirty_lores[1][offset] = 1;
	any_dirty_text[1] = any_dirty_lores[1] = 1;
}

/***************************************************************************
  apple2_hires1_w
***************************************************************************/
void apple2_hires1_w(int offset, int data)
{
	if (apple2_hires1_ram[offset] == data)
		return;

	apple2_hires1_ram[offset] = data;

	if (((offset & 0x007F) >= 0x0078) && ((offset & 0x007F) <= 0x007F))
		return;

	dirty_hires[0][offset] = 1;
	any_dirty_hires[0] = 1;
}

/***************************************************************************
  apple2_hires2_w
***************************************************************************/
void apple2_hires2_w(int offset, int data)
{
	if (apple2_hires2_ram[offset] == data)
		return;

	apple2_hires2_ram[offset] = data;

	if (((offset & 0x007F) >= 0x0078) && ((offset & 0x007F) <= 0x007F))
		return;

	dirty_hires[1][offset] = 1;
	any_dirty_hires[1] = 1;
}

/***************************************************************************
  apple2_vh_screenrefresh
***************************************************************************/
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
		apple2_text_draw(page);
		copybitmap(bitmap,apple2_text[page],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
	else if ((a2.HIRES) && (a2.MIXED))
	{
		apple2_text_draw(page);
		apple2_hires_draw(page);
		copybitmap(bitmap,apple2_hires[page],0,0,0,0,&mixed_rect,TRANSPARENCY_NONE,0);
	    mixed_rect.min_y=(160*2);
	    mixed_rect.max_y=(192*2)-1;
		copybitmap(bitmap,apple2_text[page],0,0,0,0,&mixed_rect,TRANSPARENCY_NONE,0);
	}
	else if (a2.HIRES)
	{
		apple2_hires_draw(page);
		copybitmap(bitmap,apple2_hires[page],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
	else if (a2.MIXED)
	{
		apple2_text_draw(page);
		apple2_lores_draw(page);
		copybitmap(bitmap,apple2_lores[page],0,0,0,0,&mixed_rect,TRANSPARENCY_NONE,0);
	    mixed_rect.min_y=(160*2);
	    mixed_rect.max_y=(192*2)-1;
		copybitmap(bitmap,apple2_text[page],0,0,0,0,&mixed_rect,TRANSPARENCY_NONE,0);
	}
	else
	{
		apple2_lores_draw(page);
		copybitmap(bitmap,apple2_lores[page],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
}

