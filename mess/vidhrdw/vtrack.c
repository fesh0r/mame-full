#include <assert.h>
#include "vidhrdw/vtrack.h"
#include "timer.h"

struct vtrack_entry {
	struct vtrack_entry *next;
	int beginrow;
	int endrow;
	int dummy;
};

static const struct vtrack_args *theargs;
static struct vtrack_entry *entries;
static int touched;
static int row;

/* vtrack_pushvars is the call that queries for the video hardware state and
 * pushes it onto the state stack
 *
 * Calls to vtrack_pushvars will record the state like this:
 *
 *	vtrack_pushvars(0);
 *	vtrack_pushvars(100);
 *	vtrack_pushvars(200);
 *
 * Will record the state for 0-99, 100-199, 200-end
 *
 * Also, vtrack_pushvars() is aware of the visible area and it will store the
 * state of only the visible portions of the screen
 */
static void vtrack_pushvars(int currentrow)
{
	struct vtrack_entry *e;
	int bordersize;

	bordersize = (theargs->totalrows - theargs->visiblerows) / 2;
	if (currentrow < bordersize) {
		/* We are before the visible area */
		currentrow = 0;
	}+
	else {
		currentrow -= bordersize;	
		if (currentrow >= theargs->visiblerows)
			return; /* We are after the bottom of the visible area */
	}

	assert(!entries || (entries->begin <= currentrow));

	if (!entries || (entries->beginrow < currentrow)) {
		e = malloc(sizeof(struct vtrack_entry) - sizeof(int) + theargs->statesize);
		if (!e)
			return;
		e->next = entries;
		e->beginrow = currentrow;
		e->endrow = theargs->visiblerows;
		if (entries)
			entries->endrow = currentrow - 1;
		entries = e;
	}

	theargs->fillstate((void *) &entries->dummy);
	touched = 0;
}

void vtrack_start(const struct vtrack_args *args)
{
	entries = NULL;
	theargs = args;
	push_at_end = 0;
	basetime = 0;

	row = 0;
	vtrack_pushvars(row);
}

void vtrack_stop(void)
{
	struct vtrack_entry *ent;

	while(entries) {
		ent = entries->next;
		free(entries);
		entries = ent->next;
	}
}

void vtrack_touchvars(void)
{
	touched = 1;
}

void vtrack_hblank(void)
{
	/* We are postincrementing rows instead of preincrementing because I
	 * believe that most programs that do fancy tricks wait for hblank or
	 * vblank and _immediately_ manipulate the variables
	 */
	if (touched) {
		vtrack_pushvars(row);
		touched = 0;
	}
	row++;
}

void vtrack_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	struct vtrack_entry *e;

	while(entries) {
		theargs->screenrefresh(bitmap, full_refresh, (void *) &entries->dummy, entries->beginrow, entries->endrow);
		e = entries->next;
		free(entries);
		entries = e;
	}

	basetime = timer_get_time();

	row = 0;
	vtrack_pushvars(row);
}

