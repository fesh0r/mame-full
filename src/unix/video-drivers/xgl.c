/*****************************************************************

  Xmame OpenGL driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

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
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "xmame.h"
#include "x11.h"

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

int winwidth = 640;
int winheight = 480;
int doublebuffer = 1;

static int dbdepat[]={GLX_RGBA,GLX_DOUBLEBUFFER,GLX_DEPTH_SIZE,16,None};
static int sbdepat[]={GLX_RGBA,GLX_DEPTH_SIZE,16,None};
static int dbnodepat[]={GLX_RGBA,GLX_DOUBLEBUFFER,None};
static int sbnodepat[]={GLX_RGBA,None};
static GLXContext cx;

extern int screendirty;
extern int dodepth;

static int xgl_handle_resolution(struct rc_option *option, const char *arg,
   int priority)
{
  int width, height;
  
  if (sscanf(arg, "%dx%d", &width, &height) != 2)
  {
      fprintf(stderr, "error: invalid resolution: \"%s\".\n", arg);
      return -1;
  }
  winwidth  = width;
  winheight = height;
  return 0;
}

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "OpenGL Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "resolution",	"res",			rc_use_function, NULL,
     "640x480",		0,			0,		xgl_handle_resolution,
     "Specify the resolution/ windowsize to use in the form of XRESxYRES" },
   { "dblbuffer",	NULL,			rc_bool,	&doublebuffer,
     "1",		0,			0,		NULL,
     "Enable/disable double buffering" },
   { "cabview",		NULL,			rc_bool,	&cabview,
     "1",		0,			0,		NULL,
     "Start/ don't start in cabinet view mode" },
   { "cabinet",		NULL,			rc_string,	&cabname,
     "glmame",		0,			0,		NULL,
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
   fprintf(stdout, "Using GLmame v0.6 driver for xmame, written by Mike Oliphant\n");

   /* Open the display. */
   display=XOpenDisplay(NULL);

   if(!display) {
      fprintf (stderr,"OSD ERROR: failed to open the display.\n");
      return OSD_NOT_OK; 
   }
  
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
  Atom mwmatom;
  char *glxfx;
  int fx = 0;
  int x, y;

  if(depth == 16)
  {
     fprintf(stderr_file, "%s doesn't support 16bpp video modes\n", title);
     return OSD_NOT_OK;
  }

  printf("Using GLmame v0.6 driver for xmame, written by Mike Oliphant\n");
  
  if ((glxfx=getenv("MESA_GLX_FX")) && (glxfx[0]=='f') )
  {
     fx=1;
     putenv("FX_GLIDE_NO_SPLASH=");
  }

  screen=DefaultScreenOfDisplay(display);
  myscreen=DefaultScreen(display);
  cursor=XCreateFontCursor(display,XC_trek);
  
  if(doublebuffer) {
	if(dodepth)
	  myvisual=glXChooseVisual(display,myscreen,dbdepat);
	else 
	  myvisual=glXChooseVisual(display,myscreen,dbnodepat);
  }
  else {
	if(dodepth)
	  myvisual=glXChooseVisual(display,myscreen,sbdepat);
	else
	  myvisual=glXChooseVisual(display,myscreen,sbnodepat);
  }

  if(!myvisual) {
	fprintf(stderr,"OSD ERROR: failed to obtain visual.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }

  cx=glXCreateContext(display,myvisual,0,GL_TRUE);

  if(!cx) {
	fprintf(stderr,"OSD ERROR: failed to create OpenGL context.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  colormap=XCreateColormap(display,
						   RootWindow(display,myvisual->screen),
						   myvisual->visual,AllocNone);

  winattr.background_pixel=0;
  winattr.border_pixel=WhitePixelOfScreen(screen);
  winattr.bit_gravity=ForgetGravity;
  winattr.win_gravity=NorthWestGravity;
  winattr.backing_store=NotUseful;
  winattr.override_redirect=False;
  winattr.save_under=False;
  winattr.event_mask=0;
  winattr.do_not_propagate_mask=0;
  winattr.colormap=colormap;
  winattr.cursor=None;

  window=XCreateWindow(display,RootWindowOfScreen(screen),0,0,winwidth,winheight,
					   0,myvisual->depth,
					   InputOutput,myvisual->visual,
					   CWBorderPixel | CWBackPixel |
					   CWEventMask | CWDontPropagate |
					   CWColormap | CWCursor,&winattr);
  
  if (!window) {
	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  /*  Placement hints etc. */
  
  hints.flags=PMinSize|PMaxSize;
  
  if(fx) hints.flags|=USPosition|USSize;
  else hints.flags|=PSize;

  hints.min_width	= hints.max_width = hints.base_width = winwidth;
  hints.min_height= hints.max_height = hints.base_height = winheight;
  
  wm_hints.input=TRUE;
  wm_hints.flags=InputHint;
  
  XSetWMHints(display,window,&wm_hints);
  XSetWMNormalHints(display,window,&hints);
  XStoreName(display,window,title);
  
  XDefineCursor(display,window,cursor);

  /* Hack to get rid of window title bar */
  
  if(fx) {
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
				 ButtonPressMask   | ButtonReleaseMask
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
				 KeyPressMask | KeyReleaseMask
				 );
  }
  
  XMapRaised(display,window);
  XClearWindow(display,window);
  XWindowEvent(display,window,ExposureMask,&event);
  
  glXMakeCurrent(display,window,cx);

  /* If using 3Dfx, Resize the window to fullscreen so we don't lose focus
     We have to do this after glXMakeCurrent(), or else the voodoo driver
     will give us the wrong resolution */

  if(fx) {
	XResizeWindow(display,window,screen->width,screen->height);

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
   glXDestroyContext(display,cx);

   XFreeColormap(display, colormap);
     
   if(window) {
     /* ungrab the pointer */

     if(use_mouse) XUngrabPointer(display,CurrentTime);

     CloseVScreen();  /* Shut down GL stuff */
   }

   XSync(display,False); /* send all events to sync; */
}

/* Swap GL video buffers */

void SwapBuffers(void)
{
  glXSwapBuffers(display,window);
}
