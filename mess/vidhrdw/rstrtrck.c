#include "includes/rstrtrck.h"
#include "mame.h"
#include <stdlib.h>
#include <assert.h>

// --------------------------------------------------------------------------------------------

#ifdef MAME_DEBUG
#define LOG_RASTERTRACK 1
#else
#define LOG_RASTERTRACK 0
#endif

#define ADHOC_DRAWING 0

struct rastertrack_queueent {
#if !ADHOC_DRAWING
	struct rastertrack_queueent *next;
	int end;
#endif
	struct rasterbits_source rs;
	struct rasterbits_videomode rvm;
	struct rasterbits_frame rf;
};

struct rastertrack_queuehdr {
	struct rastertrack_queueent *head;
	struct rastertrack_queueent *tail;
};

#if ADHOC_DRAWING
static struct osd_bitmap *the_bitmap;
#else
static struct rastertrack_queuehdr queue;
#endif

static struct rastertrack_queueent *createentry(rastertrack_getvideoinfoproc proc, int full_refresh)
{
	struct rastertrack_queueent *newentry;

	newentry = malloc(sizeof(struct rastertrack_queueent));
	if (!newentry)
		return NULL; /* PANIC */

	memset(newentry, '\0', sizeof(struct rastertrack_queueent));
	proc(full_refresh, &newentry->rs, &newentry->rvm, &newentry->rf);
	return newentry;
}

static int equalentry(const struct rastertrack_queueent *e1, const struct rastertrack_queueent *e2)
{
	return !memcmp(&e1->rs, &e2->rs, sizeof(e1->rs))
		&& !memcmp(&e1->rvm, &e2->rvm, sizeof(e1->rvm))
		&& !memcmp(&e1->rf, &e2->rf, sizeof(e1->rf));
}

/* The queue is simply a linked list that fully accomodates all physical
 * scanlines on the screen
 *
 * "Demarcating" the queue means "for all scanlines from A to B; these
 * characteristics are associated with it
 */
static void demarcate(int begin, int end, struct rastertrack_queueent *newentry)
{
#if LOG_RASTERTRACK
	logerror("demarcate(): Demarcating [%i..%i] with entry 0x%08x\n", begin, end, newentry);
#endif

	/* Check to see if this is a bogus demarcation */
	if (begin > end) {
		free(newentry);
		return;
	}

#if ADHOC_DRAWING
	if (the_bitmap) {
		struct rasterbits_clip rc;
		rc.ybegin = begin;
		rc.yend = end;

		raster_bits(the_bitmap, &newentry->rs, &newentry->rvm, &newentry->rf, &rc);
	}
#else /* !ADHOC_DRAWING */
	{
		struct rastertrack_queueent **e;
		struct rastertrack_queueent *dupentry;
		struct rastertrack_queueent *afterthis;
		struct rastertrack_queueent *ent;

		newentry->end = end;

		/* First we need to find the entry which we are after */ 
		e = &queue.head;
		afterthis = NULL;
		while(*e && ((*e)->end < begin)) {
			afterthis = *e;
			e = &(*e)->next;
		}

		/* Now afterthis points to the last unmodified entry that we go after.  Now
		 * there are two possibilities:
		 *
		 * 1.  afterthis->end+1 == begin; in this case, afterthis->next should be newentry
		 * 2.  afterthis->end+1 < begin; in this case, afterthis->next->next should be newentry
		 * 3.  afterthis->next is NULL; we are appending to the end
		 * 4.  afterthis is NULL; we are prepending to the beginning
		 *
		 */
		if (afterthis) {
			assert(afterthis->end+1 <= begin);

			if (afterthis->next) {
				newentry->next = afterthis->next;
				if (afterthis->end+1 < begin) {
					dupentry = malloc(sizeof(struct rastertrack_queueent));
					if (!dupentry) {
						free(newentry);
						return; /* PANIC */
					}
					memcpy(dupentry, afterthis->next, sizeof(struct rastertrack_queueent));
					dupentry->next = newentry;
					afterthis->next = dupentry;
					dupentry->end = begin - 1;
				}
				else {
					afterthis->next = newentry;
				}
			}
			else {
				/* we are appending to the end of the list */
				assert(queue.tail == afterthis);
				afterthis->next = newentry;
			}
		}
		else {
			/* afterthis is NULL; we are at the head */
			newentry->next = queue.head;
			queue.head = newentry;
		}

		/* Now that our entry is in, we need to remove extraneous entries */
		while(newentry->next && (newentry->end >= newentry->next->end)) {
			ent = newentry->next;
			newentry->next = ent->next;
			free(ent);
		}

		/* We also need to combine equivalent entries */
		while(newentry->next && equalentry(newentry, newentry->next)) {
			ent = newentry->next;
			newentry->next = ent->next;
			newentry->end = ent->end;
			free(ent);
		}

		if (!newentry->next)
			queue.tail = newentry;

#if LOG_RASTERTRACK
		/* This code verifies the integrity of the queue */
		ent = queue.head;
		while(ent) {
			if (ent->next) {
				assert(ent->end < ent->next->end);
			}
			else {
				assert(queue.tail == ent);
			}
			ent = ent->next;
		}
#endif /* LOG_RASTERTRACK */
	}
#endif /* ADHOC_DRAWING */
}

static const struct rastertrack_info *the_ri;
static struct rastertrack_queueent *current_state;
static int current_base;
static int current_scanline;
static int screen_adjustment;
static int videomode_dirty;
static int last_full_refresh;

void rastertrack_init(const struct rastertrack_info *ri)
{
	the_ri = ri;
	current_state = NULL;
	videomode_dirty = 0;

#if ADHOC_DRAWING
	the_bitmap = NULL;
#else /* !ADHOC_DRAWING */
	memset(&queue, '\0', sizeof(queue));
#endif

	/* Start with a full refresh */
	last_full_refresh = 1;
	current_base = 0;
	current_scanline = the_ri->total_scanlines;

	rastertrack_newscreen(28, 192);
}

void rastertrack_newscreen(int toplines, int contentlines)
{
	if (!current_state) {
		current_state = createentry(the_ri->videoproc, last_full_refresh);
	}
	if (current_state) {
		demarcate(current_base, current_scanline, current_state);
		current_state = NULL;
	}

	screen_adjustment = (the_ri->visible_scanlines - contentlines) / 2 - toplines;

#if LOG_RASTERTRACK
	logerror("rastertrack_newscreen(): In new screen; current_scanline was %i; screen_adjustment=%i\n", current_scanline, screen_adjustment);
#endif

	current_base = 0;
	current_scanline = 0;
}

void rastertrack_newline(void)
{
	/*
	if ((current_scanline % 40) == 0) {
		rastertrack_touchvideomode();
	}
	*/

	if (videomode_dirty) {
		if (current_state) {
			demarcate(current_base, current_scanline, current_state);
		}
		else {
#if LOG_RASTERTRACK
			logerror("rastertrack_newline(): Would have demarcated [%i..%i]... but no current_state!\n", current_base, current_scanline);
#endif
		}

		current_base = ++current_scanline;
		current_state = NULL;
		videomode_dirty = 0;
	}
	else {
		current_scanline++;
	}
}

void rastertrack_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
	if (full_refresh)
		last_full_refresh = 2;
	else if (last_full_refresh > 0)
		last_full_refresh--;

#if ADHOC_DRAWING
	the_bitmap = bitmap;
#else /* !ADHOC_DRAWING */
	{
		int begin;
		struct rastertrack_queueent *ent;
		struct rasterbits_clip rc;

		ent = queue.head;
		begin = 0;

		while(ent) {
			rc.ybegin = (ent == queue.head) ? 0 : begin + screen_adjustment;
			rc.yend = ent->end + screen_adjustment;

#if LOG_RASTERTRACK
			logerror("rastertrack_refresh(): Calling with clip [%i..%i] rvm->offset=%i\n", rc.ybegin, rc.yend, ent->rvm.offset);
#endif

			raster_bits(bitmap, &ent->rs, &ent->rvm, &ent->rf, &rc);
			begin = ent->end + 1;

			ent = ent->next;
		}
	}
#endif /* ADHOC_DRAWING */
}


void rastertrack_touchvideomode(void)
{
#if LOG_RASTERTRACK
	logerror("rastertrack_touchvideomode(): Touching video mode at scanline #%i\n", current_scanline);
#endif

	schedule_full_refresh();
	videomode_dirty = 1;

	if (!current_state)
		current_state = createentry(the_ri->videoproc, 1);
}

int rastertrack_scanline(void)
{
	return current_scanline;
}

