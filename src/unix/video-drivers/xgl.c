/*****************************************************************

  Xmame OpenGL driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
#define __XOPENGL_C_

#define RRand(range) (random()%range)

#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include "glxtool.h"
#include "glmame.h"
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* options initialised through the rc_option struct */
int bilinear;
int antialias;
int antialiasvec;
int force_text_width_height;
float gl_beam;
int cabview;
char *cabname;

/* local vars */
static GLCapabilities glCaps = { BUFFER_DOUBLE, COLOR_RGBA, STEREO_OFF,
  1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, -1 };
static XSetWindowAttributes window_attr;
static GLXContext glContext=NULL;
static const char * xgl_version_str = 
	"\nGLmame v0.94 - the_peace_version , by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com,\nbased upon GLmame v0.6 driver for xmame, written by Mike Oliphant\n\n";
static int window_type;

struct rc_option xgl_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "OpenGL Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "gldblbuffer",	NULL,			rc_bool,	&(glCaps.buffer),
     "1",		0,			0,		NULL,
     "Enable/disable double buffering (default: true)" },
   { "gltexture_size",	NULL,			rc_int,		&force_text_width_height,
     "0",		0,			2048,		rc_verify_power_of_2,
     "Force the max width and height of one texture segment (default: 0 (autosize))" },
   { "glbilinear",	"glbilin",		rc_bool,	&bilinear,
     "1",		0,			0,		NULL,
     "Enable/disable bilinear filtering (default: true)" },
   { "glbeam",		NULL,			rc_float,	&gl_beam,
     "1.0",		1.0,			16.0,		NULL,
     "Set the beam size for vector games (default: 1.0)" },
   { "glantialias",	"glaa",			rc_bool,	&antialias,
     "0",		0,			0,		NULL,
     "Enable/disable antialiasing (default: false)" },
   { "glantialiasvec",	"glaav",		rc_bool,	&antialiasvec,
     "0",		0,			0,		NULL,
     "Enable/disable vector antialiasing (default: false)" },
   { "cabview",		NULL,			rc_bool,	&cabview,
     "0",		0,			0,		NULL,
     "Start/Don't start in cabinet view mode (default: false)" },
   { "cabinet",		NULL,			rc_string,	&cabname,
     "glmamejau",	0,			0,		NULL,
     "Specify which cabinet model to use (default: glmamejau)" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

int xgl_init(void)
{
  const char *libGLName = "libGL.so.1";
  const char *libGLUName = "libGLU.so.1";
  
  if (!loadGLLibrary(libGLName, libGLUName))
  {
    fprintf(stderr, "Use of OpenGL mode disabled\n");
    return 0;
  }

  fetch_GL_FUNCS(libGLName, libGLUName, 0);

  return SYSDEP_DISPLAY_WINDOWED|SYSDEP_DISPLAY_FULLSCREEN|
    SYSDEP_DISPLAY_HWSCALE;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xgl_open_display(int reopen)
{
  char *glxfx;
  int force_grab;
  int ownwin = 1;
  unsigned long winmask = 0;
  Window parent_win = RootWindowOfScreen (screen);

  /* set aspect_ratio, do this early since this can change yarbsize */
  mode_set_aspect_ratio((double)screen->width/screen->height);

  /* Determine window size, type, etc. If using 3Dfx */
  if((glxfx=getenv("MESA_GLX_FX")) && (glxfx[0]=='f'))
  {
    winmask     = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap;
    if(custom_window_width)
    {
      window_width  = custom_window_width;
      window_height = custom_window_height;
    }
    else
    {
      window_width  = 640;
      window_height = 480;
    }
    window_type = X11_FIXED; /* non resizable */
    force_grab  = X11_FORCE_INPUT_GRAB; /* grab mouse and keyb */
    run_in_root_window = 0;
    sysdep_display_params.fullscreen = 1;
  }
  else if (run_in_root_window)
  {
    window_type = X11_FIXED; /* non resizable */
    force_grab  = X11_NO_FORCED_GRAB; /* no grab */
    ownwin      = 0;
  }
  else
  {
    if(sysdep_display_params.fullscreen)
    {
      winmask       = CWBorderPixel | CWBackPixel | CWEventMask |
                      CWColormap | CWDontPropagate | CWCursor;
      window_type   = X11_FULLSCREEN; /* fullscreen */
      force_grab    = X11_FORCE_MOUSE_GRAB; /* grab mouse */
    }
    else
    {
      if (root_window_id)
        parent_win = root_window_id;
      
      winmask       = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap;
      if (cabview)
        window_type = X11_RESIZABLE; /* resizable */
      else
        window_type = X11_RESIZABLE_ASPECT; /* resizable, keep aspect */
      force_grab    = X11_NO_FORCED_GRAB;   /* no grab */
    }
  }

  if(!reopen)
  {
    XEvent event;
    VisualGC vgc;
    XVisualInfo *myvisual;
    int x, y;
    
    fprintf(stderr, xgl_version_str);

    x11_get_geometry(&x, &y, &window_width, &window_height,
      &window_attr.win_gravity, NULL, window_type);

    window_attr.background_pixel=0;
    window_attr.border_pixel=WhitePixelOfScreen(screen);
    window_attr.bit_gravity=ForgetGravity;
    window_attr.backing_store=NotUseful;
    window_attr.override_redirect=False;
    window_attr.save_under=False;
    window_attr.event_mask=0; 
    window_attr.do_not_propagate_mask=0;
    window_attr.colormap=0; /* done later, within findVisualGlX .. */
    window_attr.cursor=None;
    window = parent_win;

    vgc = findVisualGlX( display, RootWindowOfScreen (screen),
                         &window, x, y, window_width, window_height, &glCaps, 
  		       &ownwin, &window_attr, winmask,
  		       NULL, 0, NULL, 
  #ifdef GLDEBUG
  		       1);
  #else
  		       0);
  #endif

    if(vgc.success==0)
    {
  	fprintf(stderr,"OSD ERROR: failed to obtain visual.\n");
  	return 1; 
    }

    myvisual =vgc.visual;
    glContext=vgc.gc;

    if (!window) {
  	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
  	return 1; 
    }
    
    if(!glContext) {
  	fprintf(stderr,"OSD ERROR: failed to create OpenGL context.\n");
  	return 1; 
    }
    
    setGLCapabilities ( display, myvisual, &glCaps);
    
    /* set the hints */
    x11_set_window_hints(window_width, window_height, window_type);

    if (!run_in_root_window)
    {
      /* Map and expose the window. */
      XSelectInput(display, window, ExposureMask);
      XMapRaised(display,window);
      XClearWindow(display,window);
      XWindowEvent(display,window,ExposureMask,&event);
    }
    
    xinput_open(force_grab, 0);
  }
  else
    x11_resize_window(&window_width, &window_height, window_type);
    
  return gl_open_display(reopen);
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void xgl_close_display (void)
{
   gl_close_display();  /* Shut down GL stuff */
   xinput_close();

   disp__glXMakeCurrent(display, None, NULL);
   if (glContext)
   {
     disp__glXDestroyContext(display,glContext);
     glContext=0;
   }

   if(window)
   {
     destroyOwnOverlayWin(display, &window, &window_attr);
     window = 0;
   }

   XSync (display, True); /* send all events to sync; discard events */

#ifdef GLDEBUG
   fprintf(stderr, "GLINFO: xgl display closed !\n");
#endif
}

const char *xgl_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette, int flags)
{
  Window _dw;
  int _dint;
  unsigned int _duint,w,h;
  const char *msg;
  
  XGetGeometry(display, window, &_dw, &_dint, &_dint, &w, &h, &_duint, &_duint);
  if ( (w != window_width) || (h != window_height) )
  {
    window_width  = w;
    window_height = h;
    gl_set_windowsize();
  }

  msg = gl_update_display(bitmap, vis_area, dirty_area, palette, flags);

  if (flags & SYSDEP_DISPLAY_HOTKEY_OPTION1)
  {
    gl_set_cabview (1-cabview);
    if(cabview)
    {
      msg = "cabinet view on";
      if(window_type == X11_RESIZABLE_ASPECT)
      {
        window_type = X11_RESIZABLE;
        x11_set_window_hints(window_width, window_height, window_type);
      }
    }
    else
    {
      msg = "cabinet view off";
      if(window_type == X11_RESIZABLE)
      {
        window_type = X11_RESIZABLE_ASPECT;
        x11_set_window_hints(window_width, window_height, window_type);
      }
    }
  }
  
  if (glCaps.buffer)
    disp__glXSwapBuffers(display,window);
  else
    disp__glFlush ();
    
  return msg;
}
