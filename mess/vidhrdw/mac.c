#include "driver.h"
#include "vidhrdw/generic.h"

#include "mess/systems/mac.h"

static UINT16 *old_display;





int mac_vh_start(void)
{
	videoram_size = (512 * 384 / 8);

	old_display = (UINT16 *) malloc(videoram_size);
	if (! old_display)
	{
		return 1;
	}
	memset(old_display, 0, videoram_size);

	return 0;
}

void mac_vh_stop(void)
{
	free(old_display);
}

void mac_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	UINT16	data;
	UINT16	*old;
	UINT8	*v;
	int		fg, bg, x, y;

	v = videoram;
	bg = Machine->pens[0];
	fg = Machine->pens[1];
	old = old_display;

	for (y = 0; y < 342; y++) {
		for ( x = 0; x < 32; x++ ) {
			data = READ_WORD( v );
			if (full_refresh || (data != *old)) {
				plot_pixel( bitmap, ( x << 4 ) + 0x00, y, ( data & 0x8000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x01, y, ( data & 0x4000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x02, y, ( data & 0x2000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x03, y, ( data & 0x1000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x04, y, ( data & 0x0800 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x05, y, ( data & 0x0400 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x06, y, ( data & 0x0200 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x07, y, ( data & 0x0100 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x08, y, ( data & 0x0080 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x09, y, ( data & 0x0040 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0a, y, ( data & 0x0020 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0b, y, ( data & 0x0010 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0c, y, ( data & 0x0008 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0d, y, ( data & 0x0004 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0e, y, ( data & 0x0002 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0f, y, ( data & 0x0001 ) ? fg : bg );
				*old = data;
			}
			v += 2;
			old++;
		}
	}
}

