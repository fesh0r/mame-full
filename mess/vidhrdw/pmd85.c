/***************************************************************************

  pmd85.c

  Functions to emulate the video hardware of PMD-85.

  Krzysztof Strzecha

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/pmd85.h"

unsigned char pmd85_palette[3*3] =
{
	0x00, 0x00, 0x00,
	0x7f, 0x7f, 0x7f,
	0xff, 0xff, 0xff
};

unsigned short pmd85_colortable[1][3] ={
	{ 0, 1, 2 }
};

PALETTE_INIT( pmd85 )
{
	palette_set_colors(0, pmd85_palette, sizeof(pmd85_palette) / 3);
	memcpy(colortable, pmd85_colortable, sizeof (pmd85_colortable));
}

VIDEO_START( pmd85 )
{
	return 0;
}

VIDEO_UPDATE( pmd85 )
{
	int x,y;
	int pen0, pen1;
	UINT8 data;

	UINT8* pmd85_video_ram = mess_ram + 0xc000;

	for (y=0; y<256; y++)                  	
		for (x=0; x<288; x+=6)
		{
			data = pmd85_video_ram[x/6+0x40*y];

			pen0 = 0;
			pen1 = data & 0x80 ? 1 : 2;

			plot_pixel(bitmap, x+5, y, Machine->pens[(data & 0x20) ? pen1 : pen0]);
			plot_pixel(bitmap, x+4, y, Machine->pens[(data & 0x10) ? pen1 : pen0]);
			plot_pixel(bitmap, x+3, y, Machine->pens[(data & 0x08) ? pen1 : pen0]);
			plot_pixel(bitmap, x+2, y, Machine->pens[(data & 0x04) ? pen1 : pen0]);
			plot_pixel(bitmap, x+1, y, Machine->pens[(data & 0x02) ? pen1 : pen0]);
			plot_pixel(bitmap, x, y, Machine->pens[(data & 0x01) ? pen1 : pen0]);
		}
}
