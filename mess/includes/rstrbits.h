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
	RASTERBITS_FLAG_WRAPINROW		= 0x02, /* if on, we wrap around within the row */

	/* graphics flags */
	RASTERBITS_FLAG_ARTIFACT		= 0x04,	/* this is a graphical video mode that artifacts, in which case the
											 * metapalette is of a size twice the size the depth would imply */
	RASTERBITS_FLAG_ARTIFACTREVERSE	= 0x08,

	/* text flags */
	RASTERBITS_FLAG_BLINKNOW		= 0x04, /* this frame represents a transition frame (blinking <==> non-blinking) */
	RASTERBITS_FLAG_BLINKING		= 0x08,	/* we are currently blinking (i.e. - blinking out) */
	RASTERBITS_FLAG_TEXTMODULO		= 0x10  /* bits from the font are displayed modulo the line, as opposed to from the beginning */
};

enum {
	RASTERBITS_CHARATTR_UNDERLINE	= 0x01,
	RASTERBITS_CHARATTR_BLINKING	= 0x02
};

enum {
	RASTERBITS_ARTIFACT_STATICPALLETTE = 0x00,
	RASTERBITS_ARTIFACT_DYNAMICPALETTE = 0x01,
	RASTERBITS_ARTIFACT_REVERSE        = 0x02
};

struct rasterbits_artifacting {
	UINT16 flags;
	UINT16 numfactors;
	const double *colorfactors;		/* pointer to the factors to multiply the colors by */
	void (*getcolorrgb)(int c, UINT8 *red, UINT8 *green, UINT8 *blue);
	union {
		const UINT8 *staticpalette;
		int dynamicpalettebase;
	} u;
};

struct rasterbits_videomode {
	int width;								/* in pixels/chars */
	int height;								/* in pixels/chars */
	int depth;								/* bits per pixel/char */
	int bytesperrow;						/* number of bytes per row */
	int offset;								/* only meaningful if used in conjunction with RASTERBITS_FLAG_WRAPINROW */
	int wrapbytesperrow;					/* the number of bytes per row with regard to RASTERBITS_FLAG_WRAPINROW */
	UINT16 *pens;							/* color translation layer; can be NULL */
	union {
		struct rasterbits_artifacting artifact;
			
		struct {
			UINT8 *(*mapper)(UINT8 *mem, int param, int *fg, int *bg, int *attr);
			int mapper_param;
			int fontheight;					/* height of all characters in the mapper font */
			int underlinepos;				/* underline position on each character (-1=NA) */
		} text;
	} u;

	int flags;
};

struct rasterbits_frame {
	int width;
	int height;
	int border_pen;
	int total_scanlines;
	int top_scanline;	/* The scanline right after the border top */
};

struct rasterbits_clip {
	int ybegin;
	int yend;
};

/* The main call for rastering bits */
void raster_bits(struct osd_bitmap *bitmap, struct rasterbits_source *src, struct rasterbits_videomode *mode,
	struct rasterbits_frame *frame, struct rasterbits_clip *clip);

/* This call is used to create static palettes with precomputed artifact colors */
void setup_artifact_palette(UINT8 *destpalette, int destcolor, UINT16 c0, UINT16 c1,
	const double *colorfactors, int numfactors, int reverse);

#endif /* __RSTRBITS_H__ */
