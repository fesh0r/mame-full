/* m6847.h -- Implementation of Motorola 6847 video hardware chip */

#ifndef _M6847_H
#define _M6847_H

#include "driver.h"
#include "vidhrdw/generic.h"

#define M6847_TOTAL_COLORS 17

typedef void (*m6847_vblank_proc)(void);

void m6847_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int m6847_vh_start(void);
void m6847_vh_stop(void);
void m6847_vh_update(struct osd_bitmap *bitmap,int full_refresh);
#define m6847_vh_stop			generic_vh_stop
#define m6847_vh_eof_callback	0

void m6847_set_vblank_proc(m6847_vblank_proc proc);
void m6847_set_vram(void *ram, int rammask);
void m6847_set_artifact_dipswitch(int dipswitch);

void m6847_set_vram_mask(int rammask);

enum {
	/* Color sets can be OR'ed in to the other modes below */
	M6847_MODE_COLORSET_0		= 0x00,
	M6847_MODE_COLORSET_1		= 0x01,

	/* Text modes */
	M6847_MODE_TEXT				= 0x00,	/* 32x16 characters */
	M6847_MODE_SEMIGRAPHIC_6	= 0x02,	/* 64x48   4 color in 2x3 blocks */

	/* Graphics modes */
	M6847_MODE_G1C				= 0x10,	/* 64x64   4 color */
	M6847_MODE_G1R				= 0x12,	/* 128x64  2 color */
	M6847_MODE_G2C				= 0x14,	/* 128x64  4 color */
	M6847_MODE_G2R				= 0x16,	/* 128x96  2 color (aka PMODE 0) */
	M6847_MODE_G3C				= 0x18,	/* 128x96  4 color (aka PMODE 1) */
	M6847_MODE_G3R				= 0x1a,	/* 128x192 2 color (aka PMODE 2) */
	M6847_MODE_G4C				= 0x1c,	/* 128x192 4 color (aka PMODE 3) */
	M6847_MODE_G4R				= 0x1e	/* 256x192 2 color (aka PMODE 4) */
};

/* The 'gmode' is the actual graphics mode, and the 'vmode' determines the
 * vertical aspect of the graphics mode.  Normally, the two modes are set to
 * be the same thing, but some hardware (like the CoCo) had the option to set
 * them independently.  On the CoCo 2, the gmode was determined by the PIA, and
 * the vmode was determined by the SAM, and when mismatched created fancy
 * semigraphics modes
 *
 * If your driver always keeps these matched, use m6847_set_mode() and
 * m6847_get_mode() instead of setting these independently
 */

void m6847_set_gmode(int mode);	/* mode is between 0 and 31 */
int m6847_get_gmode(void);
void m6847_set_vmode(int mode);	/* mode is between 0 and 31 */
int m6847_get_vmode(void);

#define m6847_set_mode(mode)	{ m6847_set_gmode(mode); m6847_set_vmode(mode); }
#define m6847_get_mode()		m6847_get_gmode()

void m6847_set_video_offset(int offset);
int m6847_get_video_offset(void);
void m6847_touch_vram(int offset);

/* This call returns the size of video ram given the current settings */
int m6847_get_vram_size(void);

enum {
	M6847_BORDERCOLOR_BLACK,
	M6847_BORDERCOLOR_GREEN,
	M6847_BORDERCOLOR_WHITE,
	M6847_BORDERCOLOR_ORANGE
};

int m6847_get_bordercolor(void);
void m6847_get_bordercolor_rgb(int *red, int *green, int *blue);

struct m6847_state {
	int vram_mask;
	int video_offset;
	int video_gmode;
	int video_vmode;
};

extern struct m6847_state the_state;

#endif /* _M6847_H */

