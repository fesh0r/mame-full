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
int rastertrack_hblank(void);
void rastertrack_vblank(void);
void rastertrack_refresh(struct osd_bitmap *bitmap, int full_refresh);
void rastertrack_touchvideomode(void);
int rastertrack_scanline(void);
void rastertrack_sync(void);
int rastertrack_indrawingarea(void);

#endif /* RSTRTRCK_H */
