/* Sysdep display object

   Copyright 2000-2004 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __SYSDEP_DISPLAY_H
#define __SYSDEP_DISPLAY_H

#include "sysdep/rc.h"
#include "sysdep/sysdep_palette.h"
#include "begin_code.h"

/* mouse data */
#ifndef USE_XINPUT_DEVICES
/* only one mouse for now */
#	define SYSDEP_DISPLAY_MOUSE_MAX	1
#else
/* now we have 5 */
#	define SYSDEP_DISPLAY_MOUSE_MAX	5
#endif
#define SYSDEP_DISPLAY_MOUSE_BUTTONS	8
#define SYSDEP_DISPLAY_MOUSE_AXES	8

/* sysdep_display_update flags */
#define SYSDEP_DISPLAY_HOTKEY_OPTION0   0x0001
#define SYSDEP_DISPLAY_HOTKEY_OPTION1   0x0002
#define SYSDEP_DISPLAY_HOTKEY_OPTION2   0x0004
#define SYSDEP_DISPLAY_HOTKEY_OPTION3   0x0008
#define SYSDEP_DISPLAY_HOTKEY_OPTION4   0x0010
#define SYSDEP_DISPLAY_HOTKEY_GRABMOUSE 0x0040
#define SYSDEP_DISPLAY_HOTKEY_GRABKEYB  0x0080

#define SYSDEP_DISPLAY_UI_DIRTY         0x0100

/* orientation flags */
#define SYSDEP_DISPLAY_FLIPX		0x01
#define SYSDEP_DISPLAY_FLIPY		0x02
#define SYSDEP_DISPLAY_SWAPXY		0x04

/* effect type */
#define SYSDEP_DISPLAY_EFFECT_NONE      0
#define SYSDEP_DISPLAY_EFFECT_SCALE2X   1
#define SYSDEP_DISPLAY_EFFECT_SCAN2     2
#define SYSDEP_DISPLAY_EFFECT_RGBSTRIPE 3
#define SYSDEP_DISPLAY_EFFECT_RGBSCAN   4
#define SYSDEP_DISPLAY_EFFECT_SCAN3     5
#define SYSDEP_DISPLAY_EFFECT_LQ2X      6
#define SYSDEP_DISPLAY_EFFECT_HQ2X      7
#define SYSDEP_DISPLAY_EFFECT_6TAP2X    8
#define SYSDEP_DISPLAY_EFFECT_FAKESCAN  9
#define SYSDEP_DISPLAY_EFFECT_LAST      SYSDEP_DISPLAY_EFFECT_FAKESCAN

/* mode flags */
#define SYSDEP_DISPLAY_WINDOWED		0x01
#define SYSDEP_DISPLAY_FULLSCREEN	0x02
#define SYSDEP_DISPLAY_HWSCALE 		0x04
#define SYSDEP_DISPLAY_EFFECTS		0x08
/* number of modes */
#define SYSDEP_DISPLAY_VIDEO_MODES	5

/* from mame's palette.h */
#ifndef PALETTE_H
typedef unsigned int pen_t;
typedef unsigned int rgb_t;
#endif

/* from mame's common.h */
#ifndef COMMON_H
struct mame_bitmap
{
	int width,height;	/* width and height of the bitmap */
	int depth;			/* bits per pixel */
	void **line;		/* pointers to the start of each line - can be UINT8 **, UINT16 ** or UINT32 ** */

	/* alternate way of accessing the pixels */
	void *base;			/* pointer to pixel (0,0) (adjusted for padding) */
	int rowpixels;		/* pixels per row (including padding) */
	int rowbytes;		/* bytes per row (including padding) */

	/* functions to render in the correct orientation */
	void (*plot)(struct mame_bitmap *bitmap,int x,int y,pen_t pen);
	pen_t (*read)(struct mame_bitmap *bitmap,int x,int y);
	void (*plot_box)(struct mame_bitmap *bitmap,int x,int y,int width,int height,pen_t pen);
};
#endif

/* from mame's drawgfx.h */
#ifndef DRAWGFX_H
struct rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};
#endif

/* from mame's vidhrdw/vector.h */
#ifndef __VECTOR__
typedef struct
{
	int x; int y;
	rgb_t col;
	int intensity;
	int arg1; int arg2; /* start/end in pixel array or clipping info */
	int status;         /* for dirty and clipping handling */
	rgb_t (*callback)(void);
} point;
#endif

struct sysdep_display_mousedata
{
	int buttons[SYSDEP_DISPLAY_MOUSE_BUTTONS];
	int deltas[SYSDEP_DISPLAY_MOUSE_AXES];
};

struct sysdep_display_keyboard_event
{
	unsigned char press;
	unsigned char scancode;
	unsigned short unicode;
};

struct sysdep_display_open_params {
  /* initial width and height before scaling of the part of bitmap to be
     displayed */  
  int width;
  int height;
  /* "depth" of the bitmap to be displayed (15/32 direct or 16 palettised) */
  int depth;
  /* should we rotate and or flip ? */
  int orientation;
  /* maximum width and height before scaling of the part of the bitmap to be
     displayed, this gives bounds for the values which can be passed to
     sysdep_display_resize. */
  int max_width;
  int max_height;
  /* title of the window */
  const char *title;
  /* some sysdep display driver have multiple sub-drivers this selects
     which one to use */
  int video_mode;
  /* scaling and effect options */
  int widthscale;
  int heightscale;
  int yarbsize;
  int effect;
  int fullscreen;
  /* aspect ratio of the bitmap, or 0 if the aspect ratio should not be taken
     into account */
  double aspect_ratio;
  /* keyboard event handler */
  void (*keyboard_handler)(struct sysdep_display_keyboard_event *event);
  /* vectorgame bounds (only used by drivers which have special vector code) */
  const struct rectangle *vec_src_bounds;
  const struct rectangle *vec_dest_bounds;
};

struct sysdep_display_properties_struct {
  /* info available after sysdep_display_open */
  struct sysdep_palette_info palette_info;
  int (*vector_renderer)(point *pt, int num_points);
  /* per mode info availabe after sysdep_init */
  int mode[SYSDEP_DISPLAY_VIDEO_MODES];
};

struct sysdep_display_effect_properties_struct {
  int min_widthscale;
  int max_widthscale;
  int min_heightscale;
  int max_heightscale;
  int lock_scale;
  const char *name;
};

/* init / exit */
int sysdep_display_init(void);
void sysdep_display_exit(void);

/* open / close */
int sysdep_display_open(struct sysdep_display_open_params *params);
void sysdep_display_close(void);

/* update */
int sysdep_display_change_params(
  struct sysdep_display_open_params *new_params, int force);
void sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_area, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags,
  const char **status_msg);
void sysdep_display_clear(void);

/* input */
int  sysdep_display_update_keyboard(void);
void sysdep_display_update_mouse(void);
void sysdep_display_set_keybleds(int leds);

/* misc */
/* check if widthscale, heightscale and yarbsize are compatible with
   the choisen effect, if not update them so that they are. */
void sysdep_display_check_effect_params(struct sysdep_display_open_params *params);

extern struct sysdep_display_mousedata sysdep_display_mouse_data[SYSDEP_DISPLAY_MOUSE_MAX];
extern struct rc_option sysdep_display_opts[];
extern struct sysdep_display_properties_struct sysdep_display_properties;
extern const struct sysdep_display_effect_properties_struct sysdep_display_effect_properties[];

#include "end_code.h"
#endif /* ifndef __SYSDEP_DISPLAY_H */
