#ifndef VIDEOMAP_H
#define VIDEOMAP_H

#include "osdepend.h"
#include "drawgfx.h"

/*
 *	terminology
 *		scanlines	- refers both to emulated scanlines and rows on a mame_bitmap
 *		columns		- columns on a mame_bitmap
 *		pixel		- refers to an emulated pixel or a character
 */
struct videomap_framecallback_info
{
	int video_base;
	int bordertop_scanlines;
	int visible_scanlines;
	int pitch;
};

typedef UINT8 (*charproc_callback)(UINT32 c, UINT16 *palette, int row);

enum
{
	VIDEOMAP_FLAGS_ARTIFACT			= 0x01,	/* performs artifacting in certain graphics modes */
	VIDEOMAP_FLAGS_USEPALETTERAM	= 0x02	/* forces use of the 'paletteram' MAME global */
};

struct videomap_linecallback_info
{
	int offset;
	int offset_wrap;
	int borderleft_columns;
	int visible_columns;
	int scanlines_per_row;	/* number of scanlines per pixel/char */
	int grid_width;			/* number of horizonal emulated pixels/chars in a screen */
	int grid_depth;			/* bits per gridsquare (pixel) */
	const UINT16 *metapalette;
	charproc_callback charproc;
	UINT8 border_value;
	UINT8 flags;
};

enum
{
	VIDEOMAP_FLAGS_MEMORY8 = 0,
	VIDEOMAP_FLAGS_MEMORY16_BE = 1,
	VIDEOMAP_FLAGS_MEMORY16_LE = 2
};

struct videomap_interface
{
	int memory_flags;

	/* called once per frame */
	void (*frame_callback)(struct videomap_framecallback_info *info);

	/* possibly called once per scanline */
	void (*line_callback)(struct videomap_linecallback_info *info);

	UINT16 (*get_border_color_callback)(void);
};

struct videomap_config
{
	const struct videomap_interface *intf;
	UINT8 *videoram;
	int videoram_windowsize;
	UINT8 *dirtybuffer;
};

/* called at beginning */
int videomap_init(const struct videomap_config *config);

/* called by hardware */
void videomap_invalidate_border(void);
void videomap_invalidate_lineinfo(void);
void videomap_invalidate_frameinfo(void);

/* called by driver struct */
void videomap_update(struct mame_bitmap *bitmap, const struct rectangle *cliprect);

#endif /* VIDEOMAP_H */
