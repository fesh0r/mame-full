#include "includes/pc_video.h"
#include "vidhrdw/generic.h"

static pc_video_update_proc (*pc_choosevideomode)(int *width, int *height, struct crtc6845 *crtc);
static struct crtc6845 *pc_crtc;
static int pc_anythingdirty;

struct crtc6845 *pc_video_start(const struct crtc6845_config *config,
	pc_video_update_proc (*choosevideomode)(int *width, int *height, struct crtc6845 *crtc),
	size_t vramsize)
{
	pc_choosevideomode = choosevideomode;
	pc_crtc = NULL;
	pc_anythingdirty = 1;
	videoram_size = vramsize;

	if (config)
	{
		pc_crtc = crtc6845_init(config);
		if (!pc_crtc)
			return NULL;
	}

	if (videoram_size)
	{
		if (video_start_generic())
			return NULL;
	}

	return pc_crtc;
}

VIDEO_UPDATE( pc_video )
{
	static int width=0, height=0;
	int w = 0, h = 0;
	pc_video_update_proc video_update;

	if (pc_crtc)
	{
		w = crtc6845_get_char_columns(pc_crtc);
		h = crtc6845_get_char_height(pc_crtc) * crtc6845_get_char_lines(pc_crtc);
	}

	video_update = pc_choosevideomode(&w, &h, pc_crtc);

	if (video_update)
	{
		video_update(tmpbitmap ? tmpbitmap : bitmap, pc_crtc);

		if ((width != w) || (height != h)) 
		{
			width = w;
			height = h;
			pc_anythingdirty = 1;

			if (width > Machine->scrbitmap->width)
				width = Machine->scrbitmap->width;
			if (height > Machine->scrbitmap->height)
				height = Machine->scrbitmap->height;

			if ((width > 100) && (height > 100))
				set_visible_area(0,width-1,0, height-1);
			else
				logerror("video %d %d\n",width, height);
		}

		if (tmpbitmap)
		{
			copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
			*do_skip = !pc_anythingdirty;
			pc_anythingdirty = 0;
		}
	}
}

WRITE_HANDLER ( pc_video_videoram_w )
{
	if (videoram && videoram[offset] != data)
	{
		videoram[offset] = data;
		if (dirtybuffer)
			dirtybuffer[offset] = 1;
		pc_anythingdirty = 1;
	}
}
