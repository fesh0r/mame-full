#ifndef __X11_H_
#define __X11_H_

#include <X11/Xlib.h>
#include "effect.h"

#ifdef __X11_C_
#define EXTERN
#else
#define EXTERN extern
#endif

enum { X11_WINDOW, X11_XV, X11_OPENGL, X11_GLIDE, X11_XIL, X11_DGA };

extern struct rc_option x11_window_opts[];
extern struct rc_option	x11_input_opts[];

EXTERN Display 		*display;
EXTERN Window		window;
EXTERN Screen 		*screen;
EXTERN unsigned int	window_width;
EXTERN unsigned int	window_height;
EXTERN unsigned int	custom_window_width;
EXTERN unsigned int	custom_window_height;
EXTERN int		use_xsync;
EXTERN int		root_window_id; /* root window id (for swallowing the mame window) */
EXTERN int		run_in_root_window;
EXTERN int		x11_exposed;
#ifdef USE_MITSHM
EXTERN int		x11_mit_shm_error;
#endif
#ifdef USE_XV
extern struct rc_option	xv_opts[];
#endif
#ifdef USE_OPENGL
extern struct rc_option	xgl_opts[];
#endif
#ifdef USE_GLIDE
extern struct rc_option	fx_opts[];
#endif
#ifdef USE_XIL
extern struct rc_option	xil_opts[];
#endif
#ifdef USE_DGA
EXTERN int		xf86_dga_fix_viewport;
EXTERN int		xf86_dga_first_click;
extern struct rc_option xf86_dga_opts[];
extern struct rc_option xf86_dga2_opts[];
#endif
#ifdef X11_JOYSTICK
EXTERN int devicebuttonpress;
EXTERN int devicebuttonrelease;
EXTERN int devicemotionnotify;
EXTERN int devicebuttonmotion;
#endif

/*** prototypes ***/

/* device related */
void process_x11_joy_event(XEvent *event);

/* xinput functions */
int xinput_open(int force_grab, int event_mask);
void xinput_close(void);
void xinput_check_hotkeys(unsigned int flags);

/* generic helper functions */
int x11_init_palette_info(void);
/* Create a resizable window with the correct aspect ratio, honor
   custom_width, custom_height, run_in_root_window and
   sysdep_display_params.fullscreen, return width and height in width
   and height. */
int x11_create_resizable_window(unsigned int *width, unsigned int *height);
/* Create a window, type can be:
   0: Fixed size of width and height
   1: Resizable initial size is width and height, aspect is always
      kept to sysdep_display_params.aspect .
   2: Resizable initial size is width and height
   3: Fullscreen return width and height in width and height */
void x11_resize_resizable_window(unsigned int *width, unsigned int *height);
int x11_create_window(unsigned int *width, unsigned int *height, int type);
/* Set the hints for a window, window-type can be:
   0: Fixed size
   1: Resizable, aspect is always kept to sysdep_display_params.aspect .
   2: Resizable
   3: Fullscreen */
void x11_set_window_hints(unsigned int width, unsigned int height, int type);

/* Normal x11_window functions */
int  x11_window_init(void);
int  x11_window_open_display(int reopen);
void x11_window_close_display(void);
void x11_window_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#ifdef USE_MITSHM
int  x11_test_mit_shm (Display * display, XErrorEvent * error);
#endif
/* XV functions */
#ifdef USE_XV
int  xv_init(void);
int  xv_open_display(int reopen);
void xv_close_display(void);
void xv_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#endif
/* OpenGL functions */
#ifdef USE_OPENGL
int  xgl_init(void);
int  xgl_open_display(int reopen);
void xgl_close_display(void);
void xgl_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#endif
/* Glide functions */
#ifdef USE_GLIDE
int  xfx_init(void);
void xfx_exit(void);
int  xfx_open_display(int reopen);
void xfx_close_display(void);
void xfx_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#endif
/* XIL functions */
#ifdef USE_XIL
int  xil_init(void);
int  xil_open_display(int reopen);
void xil_close_display(void);
void xil_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#endif
/* Xf86_dga functions */
#ifdef USE_DGA
int  xf86_dga_init(void);
int  xf86_dga_open_display(int reopen);
void xf86_dga_close_display(void);
void xf86_dga_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
int  xf86_dga1_init(void);
int  xf86_dga1_open_display(int reopen);
void xf86_dga1_close_display(void);
void xf86_dga1_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
int  xf86_dga2_init(void);
int  xf86_dga2_open_display(int reopen);
void xf86_dga2_close_display(void);
void xf86_dga2_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags, const char **status_msg);
#endif

#undef EXTERN
#endif /* ifndef __X11_H_ */
