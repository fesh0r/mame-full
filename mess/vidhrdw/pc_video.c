/*********************************************************************

	pc_video.c

	PC Video code

*********************************************************************/

#include "includes/pc_video.h"
#include "vidhrdw/generic.h"
#include "state.h"



/***************************************************************************

	Local variables

***************************************************************************/

static pc_video_update_proc (*pc_choosevideomode)(int *width, int *height, struct crtc6845 *crtc);
static struct crtc6845 *pc_crtc;
static int pc_anythingdirty;
static int pc_current_height;
static int pc_current_width;



/**************************************************************************/

static void pc_video_postload(void)
{
	pc_anythingdirty = 1;
	pc_current_height = -1;
	pc_current_width = -1;
}



struct crtc6845 *pc_video_start(const struct crtc6845_config *config,
	pc_video_update_proc (*choosevideomode)(int *width, int *height, struct crtc6845 *crtc),
	size_t vramsize)
{
	pc_choosevideomode = choosevideomode;
	pc_crtc = NULL;
	pc_anythingdirty = 1;
	pc_current_height = -1;
	pc_current_width = -1;
	tmpbitmap = NULL;

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

	state_save_register_func_postload(pc_video_postload);
	return pc_crtc;
}

VIDEO_UPDATE( pc_video )
{
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
		if ((pc_current_width != w) || (pc_current_height != h)) 
		{
			pc_current_width = w;
			pc_current_height = h;
			pc_anythingdirty = 1;

			if (pc_current_width > Machine->scrbitmap->width)
				pc_current_width = Machine->scrbitmap->width;
			if (pc_current_height > Machine->scrbitmap->height)
				pc_current_height = Machine->scrbitmap->height;

			if ((pc_current_width > 100) && (pc_current_height > 100))
				set_visible_area(0, pc_current_width-1, 0, pc_current_height-1);

			fillbitmap(bitmap, 0, cliprect);
		}

		video_update(tmpbitmap ? tmpbitmap : bitmap, pc_crtc);

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
