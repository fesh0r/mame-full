/*
	Macintosh video hardware

	Emulates the video hardware for compact Macintosh series (original Macintosh (128k, 512k,
	512ke), Macintosh Plus, Macintosh SE, Macintosh Classic)
*/

#include "driver.h"
#include "videomap.h"
#include "includes/mac.h"

static int screen_buffer;



PALETTE_INIT( mac )
{
	palette_set_color(0, 0xff, 0xff, 0xff);
	palette_set_color(1, 0x00, 0x00, 0x00);
}



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
	info->video_base = mess_ram_size - (screen_buffer ? MAC_MAIN_SCREEN_BUF_OFFSET : MAC_ALT_SCREEN_BUF_OFFSET);
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
	cfg.videoram_windowsize = mess_ram_size;

	return videomap_init(&cfg);
}
