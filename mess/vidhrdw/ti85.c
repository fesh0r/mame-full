/***************************************************************************
  TI-85 driver by Krzysztof Strzecha

  Functions to emulate the video hardware of the TI-85

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/ti85.h"

int ti85_vh_start (void)
{
	logerror("video init\n");
	return 0;
}

void ti85_vh_stop (void)
{
}


void ti85_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int x,y,b;
	int pen0, pen1;
	int lcdmem;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

	lcdmem =  ((LCD_memory & 0x3F) + 0xc0) * 0x100;

/*	if (!LCD_status)
	{
        	for (y=0; y<64; y++)
			for (x=0; x<16; x++)
				for (b=0; b<8; b++)
					plot_pixel(bitmap, x*8+b, y, pen0);
		return;
	}
*/
        for (y=0; y<64; y++)
		for (x=0; x<16; x++)
			for (b=0; b<8; b++)
				plot_pixel(bitmap, x*8+b, y, cpu_readmem16(lcdmem+y*16+x) & (0x80>>b) ? pen0 : pen1);

}
