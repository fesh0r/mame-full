#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/mac.h"

VIDEO_START( mac )
{
	videoram_size = (512 * 384 / 8);
	return 0;
}

VIDEO_UPDATE( mac )
{
	UINT16 scanline_data[32*16];
	UINT16 data;
	int x, y, b, fg, bg;

	bg = Machine->pens[0];
	fg = Machine->pens[1];

	for (y = 0; y < 342; y++)
	{
		for ( x = 0; x < 32; x++ )
		{
			data = ((data16_t *) videoram)[y*32+x];
			for (b = 0; b < 16; b++)
				scanline_data[x*16+b] = (data & (1 << (15-b))) ? fg : bg;
		}
		draw_scanline16(bitmap, 0, y, 32*16, scanline_data, NULL, -1);
	}
}

