#ifndef __X11_H_
#define __X11_H_

#include <X11/Xlib.h>
#include "effect.h"

#ifdef __X11_C_
#define EXTERN
#else
#define EXTERN extern
#endif

enum { X11_WINDOW, X11_XV, X11_OPENGL, X11_GLIDE, X11_DGA, X11_MODE_COUNT };

extern struct rc_option x11_window_opts[];
extern struct rc_option	x11_input_opts[];

EXTERN Display 		*display;
EXTERN Window		window;
EXTERN Screen 		*screen;
EXTERN unsigned int	window_width;
EXTERN unsigned int	window_height;
EXTERN unsigned int	custom_window_width;
EXTERN unsigned int	custom_window_height;
EXTERN unsigned char	*scaled_buffer_ptr;
EXTERN int		x11_video_mode;
EXTERN int		root_window_id; /* root window id (for swallowing the mame window) */
EXTERN int		run_in_root_window;
EXTERN int		x11_exposed;
#ifdef USE_XIL
EXTERN int		use_xil;
EXTERN int		use_mt_xil;
#endif
#ifdef USE_DGA
EXTERN int		xf86_dga_fix_viewport;
EXTERN int		xf86_dga_first_click;
extern struct rc_option xf86_dga_opts[];
extern struct rc_option xf86_dga2_opts[];
#endif
#ifdef USE_OPENGL
extern struct rc_option	xgl_opts[];
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
/* Create a window, type can be:
   0: Fixed size of width and height
   1: Resizable initial size is width and height
   2: Fullscreen return width and height in width and height */
int x11_create_window(int *width, int *height, int type);
/* Set the hints for a window, window-type can be:
   0: Fixed size of width and height
   1: Resizable initial size is width and height
   2: Fullscreen of width and height */
void x11_set_window_hints(unsigned int width, unsigned int height, int type);

/* generic helper functions */
int x11_init_palette_info(Visual *xvisual);

/* Normal x11_window functions */
int  x11_window_open_display(void);
void x11_window_close_display(void);
void x11_window_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);

/* Xf86_dga functions */
#ifdef USE_DGA
int  xf86_dga_init(void);
int  xf86_dga_open_display(void);
void xf86_dga_close_display(void);
void xf86_dga_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);
int  xf86_dga1_init(void);
int  xf86_dga1_open_display(void);
void xf86_dga1_close_display(void);
void xf86_dga1_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);
int  xf86_dga2_init(void);
int  xf86_dga2_open_display(void);
void xf86_dga2_close_display(void);
void xf86_dga2_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);
#endif
/* Glide functions */
#ifdef USE_GLIDE
int  xfx_init(void);
void xfx_exit(void);
int  xfx_open_display(void);
void xfx_close_display(void);
void xfx_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);
#endif
/* OpenGL functions */
#ifdef USE_OPENGL
int  xgl_open_display(void);
void xgl_close_display(void);
void xgl_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags);
#endif
/* XIL functions */
#ifdef USE_XIL
void init_xil( void );
void setup_xil_images( int, int );
void refresh_xil_screen( void );
#endif
/* DBE functions */
#ifdef USE_DBE
void setup_dbe( void );
void swap_dbe_buffers( void );
#endif

#undef EXTERN
#endif /* ifndef __X11_H_ */
