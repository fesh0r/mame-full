#include "includes/pc_video.h"
#include "vidhrdw/generic.h"

static pc_video_update_proc (*pc_choosevideomode)(int *xfactor, int *yfactor);
static struct _CRTC6845 *pc_crtc;

int pc_video_start(struct _CRTC6845 *crtc, CRTC6845_CONFIG *config,
	pc_video_update_proc (*choosevideomode)(int *xfactor, int *yfactor))
{
	pc_choosevideomode = choosevideomode;
	pc_crtc = crtc;

	crtc6845_init(crtc, config);
	return 0;
}

VIDEO_UPDATE( pc_video )
{
	static int width=0, height=0;
	int w, h;
	int xfactor = 1, yfactor = 1;
	pc_video_update_proc video_update;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	/*if( crtc6845_do_full_refresh(cga.crtc)||full_refresh )
	{
		cga.full_refresh = 0;
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
    }*/

	w = crtc6845_get_char_columns(pc_crtc);
	h = crtc6845_get_char_height(pc_crtc) * crtc6845_get_char_lines(pc_crtc);

	video_update = pc_choosevideomode(&xfactor, &yfactor);

	if (video_update)
	{
		video_update(bitmap);

		w *= xfactor;
		h *= yfactor;

		if ((width != w) || (height != h)) 
		{
			width = w;
			height = h;

			if (width>Machine->visible_area.max_x)
				width = Machine->visible_area.max_x+1;
			if (height>Machine->visible_area.max_y)
				height = Machine->visible_area.max_y+1;

			if ((width > 100) && (height > 100))
				set_visible_area(0,width-1,0, height-1);
			else
				logerror("video %d %d\n",width, height);
		}
	}
}

WRITE_HANDLER ( pc_video_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		if (dirtybuffer)
			dirtybuffer[offset] = 1;
	}
}
