/*****************************************************************

  Xmame OpenGL driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
/* pretend we're x11.c otherwise display and a few other crucial things don't
   get declared */
#define __X11_C_   
#define __XOPENGL_C_

#define RRand(range) (random()%range)

#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/cursorfont.h>

#include "glxtool.h"

#include "xmame.h"
#include "glmame.h"
#include "x11.h"

#include "driver.h"
#include "xmame.h"

void InitVScreen(void);
void CloseVScreen(void);

static Cursor        cursor;
static XVisualInfo   *myvisual;

typedef struct {
#define MWM_HINTS_DECORATIONS   2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;

int winwidth = 0;
int winheight = 0;
int fullscreen_width = 0;
int fullscreen_height = 0;
int doublebuffer = 1;
int bilinear=1; /* Do binlinear filtering? */
int alphablending=1; /* alphablending */
int bitmap_lod=0; /* level of bitmap-texture detail */

int fullscreen = 0;
int antialias=0;
int translucency = 0;

GLXContext glContext=NULL;

const GLCapabilities glCapsDef = { BUFFER_DOUBLE, COLOR_RGBA, STEREO_OFF,
			           1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 
			           -1
			         };

GLCapabilities glCaps;

static const char * xgl_version_str = 
	"\nGLmame v0.74, by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com,\nbased upon GLmame v0.6 driver for xmame, written by Mike Oliphant\n\n";

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "OpenGL Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "fullscreen",   NULL,    rc_bool,       &fullscreen,
      "0",           0,       0,             NULL,
      "Start fullscreen" },
   { "gldblbuffer",	NULL,			rc_bool,	&doublebuffer,
     "1",		0,			0,		NULL,
     "Enable/disable double buffering" },
   { "gltexture_size",	NULL,			rc_int,	&force_text_width_height,
     "0",		0,			0,		NULL,
     "Force the max width and height of one texture segment (default: autosize)" },
   { "glext",	NULL,			rc_bool,	&useGlEXT,
     "1",		0,			0,		NULL,
     "Force the usage of gl extensions (default: auto)" },
   { "glbilinear",	"glbilin",		rc_bool,	&bilinear,
     "1",		0,			0,		NULL,
     "Enable/disable bilinear filtering" },
   { "glbeam",		NULL,			rc_float,	&gl_beam,
     "1.0",		1.0,			16.0,		NULL,
     "Set the beam size for vector games" },
   { "glalphablending",	"glalpha",		rc_bool,	&alphablending,
     "1",		0,			0,		NULL,
     "Enable/disable alphablending (default: try alphablending)" },
   { "glantialias",	"glaa",			rc_bool,	&antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "gllod",	NULL,			rc_int,	&bitmap_lod,
     "0",		0,			0,		NULL,
     "level of bitmap-texture detail" },
   { "gllibname",	"gllib",		rc_string,	&libGLName,
     "libGL.so",	0,			0,		NULL,
     "Choose the dynamically loaded OpenGL Library (default libGL.so)" },
   { "glulibname",	"glulib",		rc_string,	&libGLUName,
     "libGLU.so",	0,			0,		NULL,
     "Choose the dynamically loaded GLU Library (default libGLU.so)" },
   { "cabview",		NULL,			rc_bool,	&cabview,
     "0",		0,			0,		NULL,
     "Start/ don't start in cabinet view mode" },
   { "cabinet",		NULL,			rc_string,	&cabname,
     "glmamejau",	0,			0,		NULL,
     "Specify which cabinet model to use" },
   { NULL,		NULL,			rc_link,	x11_input_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

int sysdep_init(void)
{
   fprintf(stderr, xgl_version_str);

   /* Open the display. */
   display=XOpenDisplay(NULL);

   if(!display) {
      fprintf (stderr,"OSD ERROR: failed to open the display.\n");
      return OSD_NOT_OK; 
   }
  
   gl_bootstrap_resources();

   return OSD_OK;
}

void sysdep_close(void)
{
   XCloseDisplay(display);
}


/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  XSetWindowAttributes winattr;
  int 		myscreen;
  XEvent	event;
  XSizeHints 	hints;
  XWMHints 	wm_hints;
  MotifWmHints	mwmhints;
  unsigned long winmask;
  Atom mwmatom;
  char *glxfx;
  int resizeEvtMask = 0;
  VisualGC vgc;
  int ownwin = 1;

  /* If using 3Dfx, Resize the window to fullscreen so we don't lose focus
     We have to do this after glXMakeCurrent(), or else the voodoo driver
     will give us the wrong resolution */
  if((glxfx=getenv("MESA_GLX_FX")) && (glxfx[0]=='f') )
  	fullscreen=1;

  if(!fullscreen) 
	  resizeEvtMask = StructureNotifyMask ;

  fprintf(stdout, xgl_version_str);
  
  screen=DefaultScreenOfDisplay(display);
  myscreen=DefaultScreen(display);
  cursor=XCreateFontCursor(display,XC_trek);
  
  fullscreen_width = screen->width;
  fullscreen_height = screen->height;

  if(fullscreen)
  {
	winwidth = screen->width;
	winheight = screen->height;
  } else {
	winwidth = visual_width*widthscale;
	winheight = visual_height*heightscale;
  }

  winattr.background_pixel=0;
  winattr.border_pixel=WhitePixelOfScreen(screen);
  winattr.bit_gravity=ForgetGravity;
  winattr.win_gravity=NorthWestGravity;
  winattr.backing_store=NotUseful;
  winattr.override_redirect=False;
  winattr.save_under=False;
  winattr.event_mask=0; 
  winattr.do_not_propagate_mask=0;
  winattr.colormap=0; /* done later, within findVisualGlX .. */
  winattr.cursor=None;

  if(fullscreen) 
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWDontPropagate | 
                CWColormap    | CWCursor
	        ;
  else
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap    
	        ;

  fetch_GL_FUNCS (1 /* force refetch ... */);

  glCaps = glCapsDef;

  glCaps.alphaBits=(alphablending>0)?1:0;
  glCaps.buffer   =(doublebuffer>0)?BUFFER_DOUBLE:BUFFER_SINGLE;

  window = RootWindow(display,DefaultScreen( display ));
  vgc = findVisualGlX( display, window,
                       &window, winwidth, winheight, &glCaps, 
		       &ownwin, &winattr, winmask,
		       NULL, 0, NULL, 1);

  if(vgc.success==0)
  {
	fprintf(stderr,"OSD ERROR: failed to obtain visual.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }

  myvisual =vgc.visual;
  glContext=vgc.gc;
  colormap=winattr.colormap;

  if (!window) {
	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  if(!glContext) {
	fprintf(stderr,"OSD ERROR: failed to create OpenGL context.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  setGLCapabilities ( display, myvisual, &glCaps);

  alphablending= (glCaps.alphaBits>0)?1:0;
  doublebuffer = (glCaps.buffer==BUFFER_DOUBLE)?1:0;

  /*  Placement hints etc. */
  
  hints.flags=PMinSize|PMaxSize;
  
  if(fullscreen) 
  { 
  	hints.flags|=USPosition|USSize;
  } else {
  	hints.flags|=PSize;
  }

  hints.min_width	= hints.max_width = hints.base_width = winwidth;
  hints.min_height= hints.max_height = hints.base_height = winheight;
  
  wm_hints.input=TRUE;
  wm_hints.flags=InputHint;
  
  XSetWMHints(display,window,&wm_hints);

  if(fullscreen) 
	  XSetWMNormalHints(display,window,&hints);

  XStoreName(display,window,title);
  
  if(fullscreen) 
	  XDefineCursor(display,window,None);
  else
	  XDefineCursor(display,window,cursor);

  /* Hack to get rid of window title bar */
  if(fullscreen) 
  {
	mwmhints.flags=MWM_HINTS_DECORATIONS;
	mwmhints.decorations=0;
	mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);
  
	XChangeProperty(display,window,mwmatom,mwmatom,32,
			PropModeReplace,(unsigned char *)&mwmhints,4);
  }
  
  /* Map and expose the window. */

  if(use_mouse) {
	/* grab the pointer and query MotionNotify events */

	XSelectInput(display, 
				 window, 
				 FocusChangeMask   | ExposureMask | 
				 KeyPressMask      | KeyReleaseMask | 
				 EnterWindowMask   | LeaveWindowMask |
				 PointerMotionMask | ButtonMotionMask |
				 ButtonPressMask   | ButtonReleaseMask |
				 resizeEvtMask 
				 );
	
	XGrabPointer(display,
				 window, /* RootWindow(display,DefaultScreen(display)), */
				 False,
				 PointerMotionMask | ButtonMotionMask |
				 ButtonPressMask   | ButtonReleaseMask | 
				 EnterWindowMask   | LeaveWindowMask ,
				 GrabModeAsync, GrabModeAsync,
				 None, cursor, CurrentTime );
  }
  else {
	XSelectInput(display, 
				 window, 
				 FocusChangeMask | ExposureMask | 
				 KeyPressMask | KeyReleaseMask  |
				 resizeEvtMask 
				 );
  }
  
  XMapRaised(display,window);
  XClearWindow(display,window);
  XWindowEvent(display,window,ExposureMask,&event);
  
  XResizeWindow(display,window,winwidth,winheight);

  fprintf(stdout, "using window size(%dx%d)\n", winwidth, winheight);

  if(fullscreen) {
	hints.min_width	= hints.max_width = hints.base_width = screen->width;
	hints.min_height= hints.max_height = hints.base_height = screen->height;

	XSetWMNormalHints(display,window,&hints);
  }

  InitVScreen();
  
  return OSD_OK;
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void sysdep_display_close (void)
{
   __glXMakeCurrent(display, None, NULL);
   __glXDestroyContext(display,glContext);
   glContext=0;

   CloseVScreen();  /* Shut down GL stuff */

   XFreeColormap(display, colormap);
   colormap=0;
     
   if(window) {
     /* ungrab the pointer */

     if(use_mouse) XUngrabPointer(display,CurrentTime);

     XDestroyWindow(display, window);
     window=0;
   }

   XSync(display,False); /* send all events to sync; */
}

/* Swap GL video buffers */

void SwapBuffers(void)
{
  __glXSwapBuffers(display,window);
}

void switch2Fullscreen()
{
	Window rootwin, childwin;
	int rx, ry, x, y, dx, dy;
	int w, h;
	unsigned int mask;
	Bool ok;

	/* sync with root window to coord 0/0 */
	XMoveWindow(display,window, 0, 0);
        XSync(display,False); /* send all events to sync; */

	/* get the diff of both orig. coord */
	ok = XQueryPointer(display, window, &rootwin, &childwin,
			    &rx, &ry, &x, &y, &mask);
	dx = rx-x;
	dy = ry-y;
	
	/* get the aspect future full screen size */
	w = fullscreen_width;
	h = fullscreen_height;
	xgl_fixaspectratio(&w, &h);

	/*
	printf("GLINFO: query-ptr ok=%d / rx=%d ry=%d / x=%d y=%d / dx=%d dy=%d\n",
		ok, rx, ry, x, y, dx, dy);
	*/

	/* the new coords .. */
	x = ( fullscreen_width  - w  ) / 2 - dx;
	y = ( fullscreen_height - h ) / 2 - dy;

	printf("GLINFO: switching to max aspect %d/%d, %dx%d\n", x, y, w, h);

	XMoveResizeWindow(display, window, x, y, w, h);
}
