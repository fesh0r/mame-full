/*
 * X-Mame video specifics code
 *
 */
#ifdef x11
#define __X11_C_

/*
 * Include files.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

struct rc_option sysdep_display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11 Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "x11-mode", "x11", rc_int, &x11_video_mode, "0", 0, X11_MODE_COUNT-2, NULL, "Select x11 video mode: (if compiled in)\n0 Normal windowed (hotkey left-alt + insert)\n1 Xv windowed (hotkey left-alt + home)" },
	{ NULL, NULL, rc_link, x11_window_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, x11_input_opts, NULL, 0, 0, NULL, NULL },
   	{ NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
#ifdef USE_DGA
	{ NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_OPENGL
	{ NULL, NULL, rc_link, xgl_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_GLIDE
	{ NULL, NULL, rc_link, fx_opts, NULL, 0, 0, NULL, NULL },
#endif
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

struct x_func_struct {
	int  (*init)(void);
	int  (*open_display)(void);
	void (*close_display)(void);
	void (*update_display)(struct mame_bitmap *bitmap,
	  struct rectangle *src_bounds,  struct rectangle *dest_bounds,
	  struct sysdep_palette_struct *palette, unsigned int flags);
};

static struct x_func_struct x_func[X11_MODE_COUNT] = {
{ NULL,
  x11_window_open_display,
  x11_window_close_display,
  x11_window_update_display },
#ifdef USE_XV
{ NULL,
  x11_window_open_display,
  x11_window_close_display,
  x11_window_update_display },
#else
{ NULL, NULL, NULL, NULL },
#endif
#ifdef USE_OPENGL
{ NULL,
  xgl_open_display,
  xgl_close_display,
  xgl_update_display },
#else
{ NULL, NULL, NULL, NULL },
#endif
#ifdef USE_GLIDE
{ xfx_init,
  xfx_open_display,
  xfx_close_display,
  xfx_update_display },
#else
{ NULL, NULL, NULL, NULL },
#endif
#ifdef USE_DGA
{ xf86_dga_init,
  xf86_dga_open_display,
  xf86_dga_close_display,
  xf86_dga_update_display }
#else
{ NULL, NULL, NULL, NULL }
#endif
};

static int mode_available[X11_MODE_COUNT];
static struct sysdep_display_open_params orig_params;

int sysdep_display_init (void)
{
	int i;

	if(!(display = XOpenDisplay (NULL)))
	{
		/* Don't use stderr_file here it isn't assigned a value yet ! */
		fprintf (stderr, "Could not open display\n");
		return 1;
	}
	screen=DefaultScreenOfDisplay(display);

	for (i=0;i<X11_MODE_COUNT;i++)
	{
		if(x_func[i].init)
			mode_available[i] = !x_func[i].init();
		else if(x_func[i].open_display)
			mode_available[i] = 1;
		else
			mode_available[i] = 0;
	}

	return 0;
}

void sysdep_display_exit(void)
{
	if(display)
		XCloseDisplay (display);
#ifdef USE_GLIDE
	xfx_exit();
#endif
}

static void x11_check_mode(int *mode)
{
	if ((*mode == X11_WINDOW) && mode_available[X11_DGA] &&
	    sysdep_display_params.fullscreen)
	  *mode = X11_DGA;
	if ((*mode == X11_DGA) && !sysdep_display_params.fullscreen)
	  *mode = X11_WINDOW;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_display_open (const struct sysdep_display_open_params *params)
{
	orig_params = sysdep_display_params = *params;

	if (!mode_available[x11_video_mode])
	{
		fprintf (stderr, "X11-mode %d not available, falling back to normal window code\n",
				x11_video_mode);
		x11_video_mode = X11_WINDOW;
	}

	x11_check_mode(&x11_video_mode);
	return (*x_func[x11_video_mode].open_display)();
}

void sysdep_display_close(void)
{
	(*x_func[x11_video_mode].close_display)();
}

int x11_init_palette_info(Visual *xvisual)
{
	if (xvisual->class != TrueColor)
	{
		fprintf(stderr, "X11-Error: only TrueColor visuals are supported\n");
		return 1;
	}

	/* fill the sysdep_display_properties struct */
	memset(&sysdep_display_properties, 0, sizeof(sysdep_display_properties));
	sysdep_display_properties.palette_info.red_mask   = xvisual->red_mask;
	sysdep_display_properties.palette_info.green_mask = xvisual->green_mask;
	sysdep_display_properties.palette_info.blue_mask  = xvisual->blue_mask;

	return 0;
}

int sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_area, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags)
{
	int new_video_mode = x11_video_mode;

	if (flags & SYSDEP_DISPLAY_HOTKEY_VIDMODE0)
		new_video_mode = X11_WINDOW;

	if (flags & SYSDEP_DISPLAY_HOTKEY_VIDMODE1)
		new_video_mode = X11_XV;

	if (flags & SYSDEP_DISPLAY_HOTKEY_VIDMODE2)
		new_video_mode = X11_OPENGL;

	if (flags & SYSDEP_DISPLAY_HOTKEY_VIDMODE3)
		new_video_mode = X11_GLIDE;

	x11_check_mode(&new_video_mode);
	if (new_video_mode != x11_video_mode && mode_available[new_video_mode])
	{
		int old_video_mode = x11_video_mode;

		(*x_func[x11_video_mode].close_display)();
		x11_video_mode = new_video_mode;
		sysdep_display_params = orig_params;
		if ((*x_func[x11_video_mode].open_display)())
		{
			fprintf(stderr,
					"X11-Warning: Couldn't create display for new x11-mode\n"
					"   Trying again with the old x11-mode\n");
			(*x_func[x11_video_mode].close_display)();
			x11_video_mode = old_video_mode;
			sysdep_display_params = orig_params;
			if ((*x_func[x11_video_mode].open_display)())
			{
				(*x_func[x11_video_mode].close_display)();
				fprintf (stderr, "X11-Error: couldn't create new display while switching display modes\n");
				exit (1); /* ugly, anyone know a better way ? */
			}
		}
		
		return 1;
	}
	
	/* dirty_area gives the src_bounds and sysdep_display_check_params
	   will convert vis_area to the dest bounds */
	(*x_func[x11_video_mode].update_display) (bitmap, dirty_area, vis_area, palette, flags);
	xinput_check_hotkeys(flags);
	return 0;
}

#endif /* ifdef x11 */
