/*
	Macintosh video hardware

	Emulates the video hardware for compact Macintosh series (original Macintosh (128k, 512k,
	512ke), Macintosh Plus, Macintosh SE, Macintosh Classic)
*/

#include "driver.h"
#include "videomap.h"
#include "includes/mac.h"

extern int mac_ram_size;

int screen_buffer;

void mac_set_screen_buffer(int buffer)
{
	screen_buffer = buffer;
	videomap_invalidate_frameinfo();
}

#define MAC_MAIN_SCREEN_BUF_OFFSET	0x5900
#define MAC_ALT_SCREEN_BUF_OFFSET	0xD900

static void mac_videomap_frame_callback(struct videomap_framecallback_info *info)
{
	info->visible_scanlines = Machine->scrbitmap->height;
	info->video_base = mac_ram_size - (screen_buffer ? MAC_MAIN_SCREEN_BUF_OFFSET : MAC_ALT_SCREEN_BUF_OFFSET);
	info->pitch = Machine->scrbitmap->width / 8;
}

static void mac_videomap_line_callback(struct videomap_linecallback_info *info)
{
	info->visible_columns = Machine->scrbitmap->width;
	info->grid_width = Machine->scrbitmap->width;
	info->grid_depth = 1;
	info->scanlines_per_row = 1;
}

static struct videomap_interface intf =
{
	VIDEOMAP_FLAGS_MEMORY16_BE,
	&mac_videomap_frame_callback,
	&mac_videomap_line_callback,
	NULL
};

VIDEO_START( mac )
{
	struct videomap_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.intf = &intf;
	cfg.videoram = memory_region(REGION_CPU1);
	cfg.videoram_windowsize = mac_ram_size;

	return videomap_init(&cfg);
}

VIDEO_UPDATE( mac )
{
	videomap_update(bitmap, cliprect);

/*	UINT16 scanline_data[32*16];
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
*/
}

