#ifndef RSTRTRCK_H
#define RSTRTRCK_H

#include "rstrbits.h"

struct rastertrack_vvars
{
	int bordertop;
	int rows;
	int baseaddress;
};

struct rastertrack_hvars
{
	struct rasterbits_videomode mode;
	int border_pen;
	int frame_height;
	int frame_width;
};

enum {
	RI_PALETTERECALC = 1
};

struct rastertrack_interface
{
	/* The total real scanlines on this system */
	int real_scanlines;

	/* function to be called at the beginning of the screen */
	void (*newscreen)(struct rastertrack_vvars *vvars, struct rastertrack_hvars *hvars);
	/* function to be called at the beginning of the first scanline of the content */
	void (*begincontent)(void);
	/* function to be called at the beginning of the first scanline after the content */
	void (*endcontent)(void);

	void (*getvideomode)(struct rastertrack_hvars *hvars);

	int flags;
};

struct rastertrack_initvars
{
	struct rastertrack_interface *intf;
	UINT8 *vram;
	int vramwrap;
};

void rastertrack_init(struct rastertrack_initvars *initvars);
void rastertrack_touchvideomode(void);
int rastertrack_hblank(void);
void rastertrack_refresh(struct mame_bitmap *bitmap, int full_refresh);
int rastertrack_scanline(void);

#endif /* RSTRTRCK_H */
