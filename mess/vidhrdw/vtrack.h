#ifndef VTRACK_H
#define VTRACK_H

#include <stdlib.h>

struct vtrack_args {
	int totalrows;
	int visiblerows;
	void (*fillstate)(void *);
	size_t statesize;
	void (*screenrefresh)(struct osd_bitmap *bitmap, int full_refresh, void *state, int beginrow, int endrow);
};

void vtrack_start(const struct vtrack_args *args);
void vtrack_stop(void);
void vtrack_touchvars(void);

void vtrack_hblank(void);
void vtrack_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

#endif /* VTRACK_H */
