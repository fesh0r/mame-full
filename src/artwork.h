/*********************************************************************

	artwork.h

	Second generation artwork implementation.

*********************************************************************/

#ifndef ARTWORK_H
#define ARTWORK_H


/***************************************************************************

	Constants

***************************************************************************/

/* the various types of built-in overlay primitives */
#define OVERLAY_TYPE_END			0
#define OVERLAY_TYPE_RECTANGLE		1
#define OVERLAY_TYPE_DISK			2

/* flags for the primitives */
#define OVERLAY_FLAG_NOBLEND		0x10
#define OVERLAY_FLAG_MASK			(OVERLAY_FLAG_NOBLEND)

/* the tag assigned to all the internal overlays */
#define OVERLAY_TAG					"overlay"



/***************************************************************************

	Macros

***************************************************************************/

#define OVERLAY_START(name)	\
	static const struct overlay_piece name[] = {

#define OVERLAY_END \
	{ OVERLAY_TYPE_END } };

#define OVERLAY_RECT(l,t,r,b,c) \
	{ OVERLAY_TYPE_RECTANGLE, (c), (l), (t), (r), (b) },

#define OVERLAY_DISK(x,y,r,c) \
	{ OVERLAY_TYPE_DISK, (c), (x), (y), (r), 0 },

#define OVERLAY_DISK_NOBLEND(x,y,r,c) \
	{ OVERLAY_TYPE_DISK | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (r), 0 },



/***************************************************************************

	Type definitions

***************************************************************************/

struct overlay_piece
{
	UINT8 type;
	rgb_t color;
	float left, top, right, bottom;
};



/***************************************************************************

	Prototypes

***************************************************************************/

int artwork_create_display(int width, int height, int depth, float fps, int attributes, int orientation, UINT32 *rgb_components);
void artwork_update_video_and_audio(struct mame_display *display);
void artwork_save_snapshot(struct mame_bitmap *bitmap);

void artwork_set_overlay(const struct overlay_piece *overlist);
void artwork_show(const char *tag, int show);

#endif
