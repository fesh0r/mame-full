#ifndef __RSTRBITS_H__
#define __RSTRBITS_H__

#include "osdepend.h"
#include "drawgfx.h"

struct rasterbits_source {
	UINT8 *videoram;	/* base of video ram */
	int size;			/* size of video ram (to support wrap around) */
	int position;		/* position in video ram */
	UINT8 *db;			/* dirtybuffer; if NULL, then everything is dirty */
};

enum {
	RASTERBITS_FLAG_GRAPHICS		= 0x00,
	RASTERBITS_FLAG_TEXT			= 0x01,	/* 0=graphics 1=text */

	/* graphics flags */
	RASTERBITS_FLAG_ARTIFACT		= 0x02,	/* this is a graphical video mode that artifacts, in which case the
											 * metapalette is of a size twice the size the depth would imply */

	/* text flags */
	RASTERBITS_FLAG_BLINKNOW		= 0x02, /* this frame, we are now blinking */
	RASTERBITS_FLAG_BLINKING		= 0x04	/* we are currently blinking */
};

enum {
	RASTERBITS_CHARATTR_UNDERLINE	= 0x01,
	RASTERBITS_CHARATTR_BLINKING	= 0x02
};

struct rasterbits_videomode {
	int width;						/* in pixels/chars */
	int height;						/* in pixels/chars */
	int depth;						/* bits per pixel/char */
	int bytesperrow;				/* number of bytes per row */
	const int *metapalette;			/* color translation layer; can be NULL */
	union {
		struct {
			UINT8 *(*mapper)(UINT8 *mem, int param, int *fg, int *bg, int *attr);
			int mapper_param;
			int modulo;
		} text;
	} u;

	int flags;
};

struct rasterbits_frame {
	int width;
	int height;
	int border_pen;
};

struct rasterbits_clip {
	int ybegin;
	int yend;
};

void raster_bits(struct osd_bitmap *bitmap, struct rasterbits_source *src, struct rasterbits_videomode *mode,
	struct rasterbits_frame *frame, struct rasterbits_clip *clip);

#endif /* __RSTRBITS_H__ */
