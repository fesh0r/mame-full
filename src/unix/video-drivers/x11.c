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

/* for xscreensaver support */
/* Commented out for now since it causes problems with some 
 * versions of KDE. */
/* #include "vroot.h" */

#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

static XSizeHints x11_init_hints;

static int x11_parse_geom(struct rc_option *option, const char *arg, int priority);

struct rc_option sysdep_display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
   	{ NULL, NULL, rc_link, aspect_opts, NULL, 0, 0, NULL, NULL },
	{ "X11 Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "geometry", "geo", rc_use_function, NULL, "", 0, 0, x11_parse_geom, "Specify the size (if resizable) and location of the window" },
	{ "xsync", "xs", rc_bool, &use_xsync, "1", 0, 0, NULL, "Use/don't use XSync instead of XFlush as screen refresh method" },
	{ "root_window_id", "rid", rc_int, &root_window_id, "0", 0, 0, NULL, "Create the xmame window in an alternate root window; mostly useful for front-ends!" },
	{ "run-in-root-window", "root", rc_bool, &run_in_root_window, "0", 0, 0, NULL, "Enable/disable running in root window" },
	{ NULL, NULL, rc_link, x11_window_opts, NULL, 0, 0, NULL, NULL },
#ifdef USE_DGA
	{ NULL, NULL, rc_link, xf86_dga_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_XV
	{ NULL, NULL, rc_link, xv_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_OPENGL
	{ NULL, NULL, rc_link, xgl_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_GLIDE
	{ NULL, NULL, rc_link, fx_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_XIL
	{ NULL, NULL, rc_link, xil_opts, NULL, 0, 0, NULL, NULL },
#endif
	{ NULL, NULL, rc_link, x11_input_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

struct x_func_struct {
	int  (*init)(void);
	int  (*open_display)(int reopen);
	void (*close_display)(void);
	void (*update_display)(struct mame_bitmap *bitmap,
	  struct rectangle *src_bounds,  struct rectangle *dest_bounds,
	  struct sysdep_palette_struct *palette, unsigned int flags,
	  const char **status_msg);
        void (*exit)(void);
};

/* HACK - HACK - HACK for fullscreen */
#define MWM_HINTS_DECORATIONS   2
typedef struct {
	long flags;
	long functions;
	long decorations;
	long input_mode;
} MotifWmHints;

static struct x_func_struct x_func[] = {
{ x11_window_init,
  x11_window_open_display,
  x11_window_close_display,
  x11_window_update_display,
  NULL },
#ifdef USE_XV
{ xv_init,
  xv_open_display,
  xv_close_display,
  xv_update_display,
  NULL },
#else
{ NULL, NULL, NULL, NULL, NULL },
#endif
#ifdef USE_OPENGL
{ xgl_init,
  xgl_open_display,
  xgl_close_display,
  xgl_update_display,
  NULL },
#else
{ NULL, NULL, NULL, NULL, NULL },
#endif
#ifdef USE_GLIDE
{ xfx_init,
  xfx_open_display,
  xfx_close_display,
  xfx_update_display,
  xfx_exit },
#else
{ NULL, NULL, NULL, NULL, NULL },
#endif
#ifdef USE_XIL
{ xil_init,
  xil_open_display,
  xil_close_display,
  xil_update_display,
  NULL },
#else
{ NULL, NULL, NULL, NULL, NULL },
#endif
#ifdef USE_DGA
{ xf86_dga_init,
  xf86_dga_open_display,
  xf86_dga_close_display,
  xf86_dga_update_display,
  NULL }
#else
{ NULL, NULL, NULL, NULL, NULL }
#endif
};

static const char *x11_mode_names[] = {
  "Normal",
  "XVideo",
  "OpenGL",
  "Glide",
  "XIL"
};

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

	window = 0;

	memset(sysdep_display_properties.mode, 0,
	  SYSDEP_DISPLAY_VIDEO_MODES * sizeof(int));
        /* to satisfy checking for a valid video_mode for
           handling -help, etc without a display. */
        sysdep_display_properties.mode[X11_WINDOW] =
          SYSDEP_DISPLAY_WINDOWED|SYSDEP_DISPLAY_EFFECTS;

	if(!(display = XOpenDisplay (NULL)))
	{
		/* don't make this a fatal error so that cmdline options
		   like -help will still work. Also don't report this
		   here to not polute the -help output */
		return 0;
	}
	screen=DefaultScreenOfDisplay(display);

	for (i=0;i<SYSDEP_DISPLAY_VIDEO_MODES;i++)
	{
		if(x_func[i].init)
			sysdep_display_properties.mode[i] = x_func[i].init();
		else
			sysdep_display_properties.mode[i] = 0;
	}

	if (x_func[X11_DGA].init)
          sysdep_display_properties.mode[X11_WINDOW] |= x_func[X11_DGA].init();

	return 0;
}

void sysdep_display_exit(void)
{
        int i;
        
	if(display)
	{
                for (i=0;i<=X11_DGA;i++)
                        if(x_func[i].exit)
                                x_func[i].exit();

		XCloseDisplay (display);
        }
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_display_driver_open(int reopen)
{
        int mode = ((sysdep_display_params.video_mode == X11_WINDOW) &&
          sysdep_display_params.fullscreen)? X11_DGA:
           sysdep_display_params.video_mode;
        
        if (!display)
        {
		fprintf (stderr, "Error: could not open display\n");
		return 1;
        }

	return x_func[mode].open_display(reopen);
}

void sysdep_display_close(void)
{
  int mode = ((sysdep_display_params.video_mode == X11_WINDOW) &&
    sysdep_display_params.fullscreen)? X11_DGA:
    sysdep_display_params.video_mode;
  
  if (display)
    (*x_func[mode].close_display)();
}

void sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_area, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags,
  const char **status_msg)
{
        int mode = ((sysdep_display_params.video_mode == X11_WINDOW) &&
          sysdep_display_params.fullscreen)? X11_DGA:
          sysdep_display_params.video_mode;
        
	*status_msg = NULL;

	/* do we need todo a full update? */
	if(x11_exposed)
	{
	 	*dirty_area = *vis_area;
	 	x11_exposed = 0;
	}
   
	(*x_func[mode].update_display)
	  (bitmap, vis_area, dirty_area, palette, flags, status_msg);
	xinput_check_hotkeys(flags);
}

int x11_init_palette_info(void)
{
	if (screen->root_visual->class != TrueColor)
	{
		fprintf(stderr, "X11-Error: only TrueColor visuals are supported\n");
		return 1;
	}

	/* fill the sysdep_display_properties struct */
	sysdep_display_properties.palette_info.fourcc_format = 0;
	sysdep_display_properties.palette_info.red_mask      = screen->root_visual->red_mask;
	sysdep_display_properties.palette_info.green_mask    = screen->root_visual->green_mask;
	sysdep_display_properties.palette_info.blue_mask     = screen->root_visual->blue_mask;
	sysdep_display_properties.palette_info.depth         = screen->root_depth;
	sysdep_display_properties.vector_renderer            = NULL;

	return 0;
}

/* Create a resizable window with the correct aspect ratio, honor
   custom_width, custom_height, run_in_root_window and
   sysdep_display_params.fullscreen, return width and height in width
   and height. */
int x11_create_resizable_window(unsigned int *width, unsigned int *height)
{
	if (run_in_root_window)
	{
	        window  = RootWindowOfScreen (screen);
	        *width  = screen->width;
                *height = screen->height;
                sysdep_display_params.fullscreen = 0;
                return 0;
	}
	
	if(sysdep_display_params.fullscreen)
                return x11_create_window(width, height, 3);

        /* determine window size */
        if (custom_window_width)
        {
          mode_clip_aspect(custom_window_width, custom_window_height,
            width, height);
        }
        else
        {
          *width  = sysdep_display_params.max_width *
            sysdep_display_params.widthscale;
          *height = sysdep_display_params.yarbsize?
            sysdep_display_params.yarbsize:
            sysdep_display_params.max_height *
              sysdep_display_params.heightscale;
          mode_stretch_aspect(window_width, window_height,
            width, height);
        }
        
        return x11_create_window(width, height, 1);
}

void x11_resize_resizable_window(unsigned int *width, unsigned int *height)
{
	if (run_in_root_window || sysdep_display_params.fullscreen)
                return;

        /* determine window size */
        if (custom_window_width)
        {
          mode_clip_aspect(custom_window_width, custom_window_height,
            width, height);
        }
        else
        {
          *width  = sysdep_display_params.max_width *
            sysdep_display_params.widthscale;
          *height = sysdep_display_params.yarbsize?
            sysdep_display_params.yarbsize:
            sysdep_display_params.max_height *
              sysdep_display_params.heightscale;
          mode_stretch_aspect(window_width, window_height,
            width, height);
        }

        /* set window hints to resizable */
        x11_set_window_hints(window_width, window_height, 2);
        /* resize */
        XResizeWindow(display, window, window_width, window_height);
        /* set window hints to keep aspect resizable */
        x11_set_window_hints(window_width, window_height, 1);
}

/* Create a window, type can be:
   0: Fixed size of width and height
   1: Resizable initial size is width and height, aspect is always
      kept to sysdep_display_params.aspect .
   2: Resizable initial size is width and height
   3: Fullscreen return width and height in width and height */
int x11_create_window(unsigned int *width, unsigned int *height, int type)
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
	unsigned int x = 1024;
	unsigned int y = 1024;

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
	    	if (x != y) /* detect -nokeepaspect */
	    	{
	    	  hints.min_aspect.x = x;
	    	  hints.min_aspect.y = y;
	    	  hints.max_aspect.x = x;
	    	  hints.max_aspect.y = y;
		  hints.flags |= PAspect;
		}
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
