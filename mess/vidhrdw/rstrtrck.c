#include "includes/rstrtrck.h"
#include "mame.h"
#include <stdlib.h>

// --------------------------------------------------------------------------------------------

#define LOG_RASTERTRACK 0

struct rasterbits_queueent {
	struct rasterbits_queueent *next;
	struct rasterbits_source rs;
	struct rasterbits_videomode rvm;
	struct rasterbits_frame rf;
	struct rasterbits_clip rc;
};

struct rasterbits_queuehdr {
	struct rasterbits_queueent *head;
	struct rasterbits_queueent *tail;
};

static int visible_top(const struct rastertrack_info *ri)
{
	return (ri->total_scanlines - ri->visible_scanlines) / 2;
}

static int visible_bottom(const struct rastertrack_info *ri)
{
	return visible_top(ri) + ri->visible_scanlines;
}

static void rastertrack_queuevideomode(struct rasterbits_queuehdr *hdr, const struct rastertrack_info *ri,
	int begin, int full_refresh)
{
	struct rasterbits_queueent *e;

	if (begin >= visible_bottom(ri))
		return;	/* We can't be seen */

	if ((begin <= visible_top(ri)) && hdr->tail) {
		e = hdr->tail;
	}
	else {
		e = malloc(sizeof(struct rasterbits_queueent));
		if (!e)
			return; /* PANIC */
		e->rc.yend = visible_bottom(ri) - 1;
		e->next = NULL;
		if (hdr->tail) {
			hdr->tail->next = e;
			hdr->tail->rc.yend = begin - 1;
		}
		else {
			hdr->head = e;
		}
		hdr->tail = e;
	}

	ri->videoproc(full_refresh, &e->rs, &e->rvm, &e->rf);
	e->rc.ybegin = begin;
}

static void rastertrack_render(struct rasterbits_queuehdr *hdr, struct osd_bitmap *bitmap)
{
	struct rasterbits_queueent *e;
	struct rasterbits_queueent *next;

	e = hdr->head;
	while(e) {
		#if LOG_RASTERTRACK
		logerror("rastertrack_render(): Rendering [%i...%i] position=0x%x\n", e->rc.ybegin, e->rc.yend, e->rs.position);
		#endif

		raster_bits(bitmap, &e->rs, &e->rvm, &e->rf, &e->rc);
		next = e->next;
		free(e);
		e = next;
	}

	hdr->head = NULL;
	hdr->tail = NULL;
}

static void rastertrack_clearqueue(struct rasterbits_queuehdr *hdr)
{
	struct rasterbits_queueent *e;
	struct rasterbits_queueent *next;

	if (hdr->head) {
		e = hdr->head;
		while(e) {
			next = e->next;
			free(e);
			e = next;
		}
		hdr->head = NULL;
		hdr->tail = NULL;
	}
}

/* -------------------------------------------------------------------------------------------- */

static const struct rastertrack_info *the_ri;
static int continue_full_refresh;
static int current_scanline;
static int videomodedirty;
static int queue_num;
static struct rasterbits_queuehdr queues[2];
static int do_sync;

static void rastertrack_reset(void)
{
	current_scanline = 0;
	videomodedirty = 0;
	rastertrack_queuevideomode(&queues[queue_num], the_ri, current_scanline, continue_full_refresh);
}

void rastertrack_init(const struct rastertrack_info *ri)
{
	the_ri = ri;
	memset(&queues, '\0', sizeof(queues));
	continue_full_refresh = 1;
	queue_num = 0;
	do_sync = 0;
	rastertrack_reset();
}

int rastertrack_hblank(void)
{
	if (do_sync) {
		if (current_scanline > 0)
			current_scanline = the_ri->total_scanlines - 1;
		do_sync = 0;
	}

	if (videomodedirty) {
		rastertrack_queuevideomode(&queues[queue_num], the_ri, current_scanline, 1);
		videomodedirty = 0;
		schedule_full_refresh();
	}
	current_scanline++;
	return current_scanline;
}

void rastertrack_vblank(void)
{
	queue_num = 1 - queue_num;
	rastertrack_reset();
}

void rastertrack_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	if (videomodedirty)
		full_refresh = 1;

	rastertrack_render(&queues[1 - queue_num], bitmap);
	continue_full_refresh = full_refresh;
}

void rastertrack_touchvideomode(void)
{
	#if LOG_RASTERTRACK
	logerror("rastertrack_touchvideomode(): Touching video mode at scanline #%i\n", current_scanline);
	#endif

	videomodedirty = 1;
}

int rastertrack_scanline(void)
{
	return current_scanline;
}

void rastertrack_sync(void)
{
	if (current_scanline > 0)
		do_sync = 1;
}
