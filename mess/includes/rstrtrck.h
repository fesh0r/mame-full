#ifndef RSTRTRCK_H
#define RSTRTRCK_H

#include "rstrbits.h"

typedef void (*rastertrack_getvideoinfoproc)(int full_refresh, struct rasterbits_source *rs,
	struct rasterbits_videomode *rvm, struct rasterbits_frame *rf);

struct rastertrack_info {
	int total_scanlines;
	int visible_scanlines;
	rastertrack_getvideoinfoproc videoproc;
};

void rastertrack_init(const struct rastertrack_info *ri);
void rastertrack_newline(void);
void rastertrack_newscreen(int toplines, int contentlines);
void rastertrack_refresh(struct osd_bitmap *bitmap, int full_refresh);
void rastertrack_touchvideomode(void);
int rastertrack_scanline(void);

#endif /* RSTRTRCK_H */
