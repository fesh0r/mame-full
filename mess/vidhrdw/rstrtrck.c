#include "driver.h"
#include "rstrtrck.h"
#include "vidhrdw/generic.h"

static struct rastertrack_interface *intf;
static UINT8 *vram;
static int vramwrap;
static struct rastertrack_vvars vvars;
static struct rastertrack_hvars hvars;
static struct osd_bitmap *bitmap;
static UINT8 *dirtybuffer_position;

#ifdef MAME_DEBUG
#define LOG_LINES		1
#define LOG_FRAMES		1
#define LOG_POSITIONS	0
#else
#define LOG_LINES		0
#define LOG_FRAMES		0
#define LOG_POSITIONS	0
#endif

/* If you don't want to use dirty buffers, turn this on */
#define ALWAYS_REFRESH	0

#define myMIN(a, b) ((a) < (b) ? (a) : (b))
#define myMAX(a, b) ((a) > (b) ? (a) : (b))

/* ----------------------------------------------------------------------- */

static void update_lines(int beginline, int endline, int full_refresh)
{
	int visible_base;
	int total_coverage;
	int before_frames, after_frames;
	struct rasterbits_source src;
	struct rasterbits_frame frame;
	struct rasterbits_clip clip;

	visible_base = (intf->real_scanlines - Machine->drv->screen_height) / 2;

#if LOG_LINES
	logerror("update_lines(): Updating lines [%i..%i] (logical [%i..%i])... %s refresh\n",
		beginline, endline, beginline - visible_base, endline - visible_base,
		full_refresh ? "full" : "partial");
#endif

	src.videoram = vram;
	src.size = vramwrap;
	src.position = vvars.baseaddress;
	src.db = full_refresh ? NULL : dirtybuffer_position;
	frame.width = hvars.frame_width;
	frame.height = hvars.frame_height;
	frame.border_pen = full_refresh ? hvars.border_pen : -1;
	frame.total_scanlines = intf->real_scanlines;
	frame.top_scanline = vvars.bordertop;
	clip.ybegin = beginline - visible_base;
	clip.yend = endline - visible_base;

	if (bitmap) {
		total_coverage = raster_bits(bitmap, &src, &hvars.mode, &frame, &clip, vvars.bordertop - visible_base);
		before_frames = vvars.baseaddress / vramwrap;
		vvars.baseaddress += total_coverage;
		after_frames = vvars.baseaddress / vramwrap;
		vvars.baseaddress -= (after_frames - before_frames) * vramwrap;

		if (dirtybuffer_position)
			dirtybuffer_position += total_coverage;
	}
}

/* ----------------------------------------------------------------------- */

static int dirty;
static int scanline_current;
static int scanline_updatebase;
static int frame_state;

enum {
	FRAME_PARTIALREFRESH,
	FRAME_FULLREFRESH,
	FRAME_SKIP
};

void rastertrack_init(struct rastertrack_initvars *initvars)
{
	intf = initvars->intf;
	vram = initvars->vram;
	vramwrap = initvars->vramwrap;
	bitmap = NULL;
	dirty = 0;
	scanline_current = 0;
	scanline_updatebase = 0;
	frame_state = FRAME_FULLREFRESH;
	dirtybuffer_position = NULL;
}

void rastertrack_touchvideomode(void)
{
	dirty = 1;
	schedule_full_refresh();
}

enum {
	POSITION_OTHER = 0,
	POSITION_TOP,
	POSITION_BEGINCONTENT,
	POSITION_ENDCONTENT
};

int rastertrack_hblank(void)
{
	int position;
	int frame_end;
	int scanline_before;

	scanline_before = scanline_current++;

	/* Did we wrap around without refreshing? */
	frame_end = (scanline_current >= intf->real_scanlines);
	scanline_current %= intf->real_scanlines;

	/* What position are we at? */
	if (scanline_current == 0)
		position = POSITION_TOP;
	else if (scanline_current == vvars.bordertop)
		position = POSITION_BEGINCONTENT;
	else if (scanline_current == (vvars.bordertop + vvars.rows))
		position = POSITION_ENDCONTENT;
	else
		position = POSITION_OTHER;

	/* If appropriate, update the lines */
	if (dirty || frame_end || (position > POSITION_TOP)) {
		if ((frame_state != FRAME_SKIP) && (hvars.mode.width != 0) && (hvars.mode.height != 0))
			update_lines(scanline_updatebase, scanline_before, frame_state == FRAME_FULLREFRESH);

		scanline_updatebase = scanline_before + 1;

		/* If we are dirty; subsequent frames must be full refresh */
		if (dirty && (frame_state != FRAME_SKIP)) {
			intf->getvideomode(&hvars);
			frame_state = FRAME_FULLREFRESH;
		}
	}

	if (frame_end) {
#if LOG_FRAMES
		logerror("rastertrack_hblank(): Setting state to FRAME_SKIP\n");
#endif
		frame_state = FRAME_SKIP;
	}

#if LOG_POSITIONS
	if (position) {
		static char *msgs[] = {
			NULL,				/* POSITION_OTHER */
			"At new frame",		/* POSITION_TOP */
			"At content",		/* POSITION_BEGINCONTENT */
			"At bottom border"	/* POSITION_ENDCONTENT */
		};
		logerror("rastertrack_hblank(): %s; scanline=%i\n", msgs[position], scanline_current);
	}
#endif

	switch(position) {
	case POSITION_TOP:
		if (frame_state == FRAME_SKIP)
			intf->newscreen(NULL, NULL);
		else
			intf->newscreen(&vvars, &hvars);
		dirtybuffer_position = dirtybuffer;
		break;

	case POSITION_BEGINCONTENT:
		if (intf->begincontent)
			intf->begincontent();
		break;

	case POSITION_ENDCONTENT:
		if (intf->endcontent)
			intf->endcontent();
		break;
	}
	return ignore_interrupt();
}

void rastertrack_refresh(struct osd_bitmap *bmap, int full_refresh)
{
	bitmap = bmap;

#if LOG_FRAMES
	logerror("rastertrack_refresh(): full_refresh=%i frame_state=%i\n", full_refresh, frame_state);
#endif

	if ((intf->flags & RI_PALETTERECALC) && palette_recalc())
		full_refresh = 1;

	switch(frame_state) {
	case FRAME_PARTIALREFRESH:
	case FRAME_FULLREFRESH:
		update_lines(scanline_updatebase, scanline_current, (frame_state == FRAME_FULLREFRESH) || full_refresh || ALWAYS_REFRESH);
		break;
	}

	frame_state = full_refresh ? FRAME_FULLREFRESH : FRAME_PARTIALREFRESH;
	scanline_current = -1;
	scanline_updatebase = 0;
	dirty = 0;
}

int rastertrack_scanline(void)
{
	return scanline_current;
}

