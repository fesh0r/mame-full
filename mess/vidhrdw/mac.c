/*
	Macintosh video hardware

	Emulates the video hardware for compact Macintosh series (original Macintosh (128k, 512k,
	512ke), Macintosh Plus, Macintosh SE, Macintosh Classic)
*/

#include "driver.h"

#include "includes/mac.h"

#define MAC_MAIN_SCREEN_BUF_OFFSET	0x5900
#define MAC_ALT_SCREEN_BUF_OFFSET	0xD900

static UINT16 *mac_screen_buf_ptr;


VIDEO_START( mac )
{
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
			data = mac_screen_buf_ptr[y*32+x];
			for (b = 0; b < 16; b++)
				scanline_data[x*16+b] = (data & (1 << (15-b))) ? fg : bg;
		}
		draw_scanline16(bitmap, 0, y, 32*16, scanline_data, NULL, -1);
	}
}

void mac_set_screen_buffer(int buffer)
{
	if (buffer)
		mac_screen_buf_ptr = (UINT16 *) (mac_ram_ptr+mac_ram_size-MAC_MAIN_SCREEN_BUF_OFFSET);
	else
		mac_screen_buf_ptr = (UINT16 *) (mac_ram_ptr+mac_ram_size-MAC_ALT_SCREEN_BUF_OFFSET);
}

