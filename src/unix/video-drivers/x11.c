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
#include <X11/Xutil.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

static XSizeHints x11_init_hints;

static int x11_verify_mode(struct rc_option *option, const char *arg, int priority);
static int x11_parse_geom(struct rc_option *option, const char *arg, int priority);

struct rc_option sysdep_display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11 Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "x11-mode", "x11", rc_int, &x11_video_mode, "0", 0, X11_MODE_COUNT-2, x11_verify_mode, "Select x11 video mode: (if compiled in)\n0 Normal (hotkey left-alt + insert)\n1 XVideo (hotkey left-alt + home)\n2 OpenGL (hotkey left-alt + page-up)\n3 Glide (hotkey left-alt + delete)" },
	{ "geometry", "geo", rc_use_function, NULL, "", 0, 0, x11_parse_geom, "Specify the size (if resizable) and location of the window" },
	{ "root_window_id", "rid", rc_int, &root_window_id, "0", 0, 0, NULL, "Create the xmame window in an alternate root window; mostly useful for front-ends!" },
	{ NULL, NULL, rc_link, x11_input_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, x11_window_opts, NULL, 0, 0, NULL, NULL },
   	{ NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
#ifdef USE_DGA
	{ NULL, NULL, rc_link, xf86_dga_opts, NULL, 0, 0, NULL, NULL },
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

/* HACK - HACK - HACK for fullscreen */
#define MWM_HINTS_DECORATIONS   2
typedef struct {
	long flags;
	long functions;
	long decorations;
	long input_mode;
} MotifWmHints;


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
{ xgl_init,
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

static int x11_verify_mode(struct rc_option *option, const char *arg,
		int priority)
{
  if(!mode_available[x11_video_mode])
  {
    fprintf(stderr, "Error x11-mode %d is not available\n", x11_video_mode);
    return 1;
  }

  option->priority = priority;

  return 0;
}

static int x11_parse_geom(struct rc_option *option, const char *arg, int priority)
{
	int i,x,y,ok = 0;
	
	if (strlen(arg) == 0)
	{
	   memset(&x11_init_hints, 0, sizeof(x11_init_hints));
	   x11_init_hints.win_gravity = NorthWestGravity;
	   custom_window_width = custom_window_height = 0;
	}
	else
	{
		i = XParseGeometry(arg, &x, &y, &custom_window_width, &custom_window_height);
		if (i & (XValue|YValue))
		{
		  x11_init_hints.x = x;
		  x11_init_hints.y = y;
		  switch (i& (XNegative|YNegative))
		  {
		     case 0:
		        x11_init_hints.win_gravity = NorthWestGravity;
		        break;
		     case YNegative:
		        x11_init_hints.win_gravity = SouthWestGravity;
		        break;
		     case XNegative:
		        x11_init_hints.win_gravity = NorthEastGravity;
		        break;		     
		     case (YNegative|XNegative):
		        x11_init_hints.win_gravity = SouthEastGravity;
		        break;
		  }
		  x11_init_hints.flags = PPosition | PWinGravity;
		  ok = 1;
		}
		if (i & (WidthValue|HeightValue))
		  ok = 1;
		if (!ok)
		{
		  fprintf(stderr,"Invalid geometry: %s.\n", arg);
		  return 1;
		}
	}
	return 0;
}

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
	orig_params = *params;
	sysdep_display_set_params(params);

	x11_check_mode(&x11_video_mode);
	return (*x_func[x11_video_mode].open_display)();
}

void sysdep_display_close(void)
{
	(*x_func[x11_video_mode].close_display)();
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
		sysdep_display_set_params(&orig_params);
		if ((*x_func[x11_video_mode].open_display)())
		{
			fprintf(stderr,
					"X11-Warning: Couldn't create display for new x11-mode\n"
					"   Trying again with the old x11-mode\n");
			(*x_func[x11_video_mode].close_display)();
			x11_video_mode = old_video_mode;
			sysdep_display_set_params(&orig_params);
			if ((*x_func[x11_video_mode].open_display)())
			{
				(*x_func[x11_video_mode].close_display)();
				fprintf (stderr, "X11-Error: couldn't create new display while switching display modes\n");
				exit (1); /* ugly, anyone know a better way ? */
			}
		}
		
		return 1;
	}
	
	/* do we need todo a full update? */
	if(x11_exposed || (flags&SYSDEP_DISPLAY_VIS_AREA_CHANGED))
	{
	 	*dirty_area = *vis_area;
	 	x11_exposed = 0;
	}
   
	(*x_func[x11_video_mode].update_display) (bitmap, vis_area, dirty_area, palette, flags);
	xinput_check_hotkeys(flags);
	return 0;
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

/* Create a window, type can be:
   0: Fixed size of width and height
   1: Resizable initial size is width and height, aspect is always
      kept to sysdep_display_params.aspect .
   2: Resizable initial size is width and height
   3: Fullscreen return width and height in width and height */
int x11_create_window(int *width, int *height, int type)
{
	XSetWindowAttributes winattr;
	XEvent event;
	int x=0,y=0,win_gravity = NorthWestGravity;
	Window root = RootWindowOfScreen (screen);
	
	switch (type)
	{
	    case 0: /* fixed size */
	    case 1: /* resizable, keep aspect */
	    case 2: /* resizable */
	        x = x11_init_hints.x;
	        y = x11_init_hints.y;
	        win_gravity = x11_init_hints.win_gravity;
	        if (root_window_id)
	           root = root_window_id;
		break;
	    case 3: /* fullscreen */
		*width  = screen->width;
		*height = screen->height;
		break;
	}

        /* Create and setup the window. No buttons, no fancy stuff. */
        winattr.background_pixel  = BlackPixelOfScreen (screen);
        winattr.border_pixel      = WhitePixelOfScreen (screen);
        winattr.bit_gravity       = ForgetGravity;
        winattr.win_gravity       = win_gravity;
        winattr.backing_store     = NotUseful;
        winattr.override_redirect = False;
        winattr.save_under        = False;
        winattr.event_mask        = 0;
        winattr.do_not_propagate_mask = 0;
        winattr.colormap          = DefaultColormapOfScreen (screen);
        winattr.cursor            = None;
        
        window = XCreateWindow(display, root, x, y, *width, *height, 0,
			screen->root_depth, InputOutput, screen->root_visual,
                        (CWBorderPixel | CWBackPixel | CWBitGravity |
                         CWWinGravity | CWBackingStore |
                         CWOverrideRedirect | CWSaveUnder | CWEventMask |
                         CWDontPropagate | CWColormap | CWCursor),
                        &winattr);
        if (!window)
        {
                fprintf (stderr, "OSD ERROR: failed in XCreateWindow().\n");
                return 1;
        }
        
        /* set the hints */
        x11_set_window_hints(*width, *height, type);
        
        XSelectInput (display, window, ExposureMask);
        XMapRaised (display, window);
        XClearWindow (display, window);
        XWindowEvent (display, window, ExposureMask, &event);
        
        return 0;
}

/* Set the hints for a window, window-type can be:
   0: Fixed size
   1: Resizable, aspect is always kept to sysdep_display_params.aspect .
   2: Resizable
   3: Fullscreen */
void x11_set_window_hints(unsigned int width, unsigned int height, int type)
{
        XWMHints wm_hints;
	XSizeHints hints = x11_init_hints;
	int x = 1024;
	int y = 1024;

	/* WM hints */
        wm_hints.input    = True;
        wm_hints.flags    = InputHint;
        XSetWMHints (display, window, &wm_hints);

	/* Size hints */
	switch (type)
	{
	    case 0: /* fixed size */
		hints.flags |= PSize | PMinSize | PMaxSize;
		break;
	    case 1: /* resizable, keep aspect */
	    	mode_clip_aspect(x, y, &x, &y);
	    	hints.min_aspect.x = x;
	    	hints.min_aspect.y = y;
	    	hints.max_aspect.x = x;
	    	hints.max_aspect.y = y;
		hints.flags |= PAspect;
	    case 2: /* resizable */
		hints.flags |= PSize;
		break;
	    case 3: /* fullscreen */
		hints.x = hints.y = 0;
		hints.flags = PMinSize|PMaxSize|USPosition|USSize;
		break;
	}
	hints.min_width  = hints.max_width  = hints.base_width  = width;
	hints.min_height = hints.max_height = hints.base_height = height;
        
        XSetWMNormalHints (display, window, &hints);
        
        /* Hack to get rid of window title bar */
        if(type == 3)
        {
                Atom mwmatom;
                MotifWmHints mwmhints;
                mwmhints.flags=MWM_HINTS_DECORATIONS;
                mwmhints.decorations=0;
                mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);

                XChangeProperty(display,window,mwmatom,mwmatom,32,
                                PropModeReplace,(unsigned char *)&mwmhints,4);
        }

#if defined(__sgi)
{
	/* Needed for setting the application class */
	XClassHint class_hints = { NAME, NAME, };

        /* Force first resource class char to be uppercase */
        class_hints.res_class[0] &= 0xDF;
        /*
         * Set the application class (WM_CLASS) so that 4Dwm can display
         * the appropriate pixmap when the application is iconified
         */
        XSetClassHint(display, window, &class_hints);
        /* Use a simpler name for the icon */
        XSetIconName(display, window, NAME);
}
#endif
        
        XStoreName (display, window, sysdep_display_params.title);
}

#endif /* ifdef x11 */
