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
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "xmame.h"
#include "glmame.h"
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

int winwidth = 0;
int winheight = 0;
int doublebuffer = 1;
int bilinear=1; /* Do binlinear filtering? */
int alphablending=0; /* alphablending */
int bitmap_lod=0; /* level of bitmap-texture detail */

int fullscreen = 0;
int antialias=0;

int translucency;

GLXContext glContext=NULL;

int visualAttribList[64];

static void setVisualAttribList(int color, int accumPerColor, 
				int alpha, int dbuffer, int stencil)
{
    int i=0;
    visualAttribList[i++] = GLX_RED_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_GREEN_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_BLUE_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_DEPTH_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_ACCUM_RED_SIZE;
    visualAttribList[i++] = (accumPerColor>0)?1:0;
    visualAttribList[i++] = GLX_ACCUM_GREEN_SIZE;
    visualAttribList[i++] = (accumPerColor>0)?1:0;
    visualAttribList[i++] = GLX_ACCUM_BLUE_SIZE;
    visualAttribList[i++] = (accumPerColor>0)?1:0;

    if(GLX_RGBA == color)
    {
	    visualAttribList[i++] = GLX_RGBA;
	    visualAttribList[i++] = GLX_ALPHA_SIZE;
	    visualAttribList[i++] = (alpha>0)?1:0;
	    visualAttribList[i++] = GLX_ACCUM_ALPHA_SIZE;
	    visualAttribList[i++] = (accumPerColor>0)?1:0;
    }
    if(dbuffer)
	    visualAttribList[i++] = GLX_DOUBLEBUFFER;

    visualAttribList[i++] = GLX_STENCIL_SIZE;
    visualAttribList[i++] = stencil;
    visualAttribList[i] = None;
}

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
   { "glbilinear",	"glbilin",		rc_bool,	&bilinear,
     "1",		0,			0,		NULL,
     "Enable/disable bilinear filtering" },
   { "glbeam",		NULL,			rc_float,	&gl_beam,
     "1.0",		1.0,			16.0,		NULL,
     "Set the beam size for vector games" },
   { "glalphablending",	"glalpha",		rc_bool,	&alphablending,
     "0",		0,			0,		NULL,
     "Enable/disable alphablending (auto on if using vector games, else off)" },
   { "glantialias",	"glaa",			rc_bool,	&antialias,
     "0",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "gltranslucency",	"gltrans",		rc_float,	&gl_translucency,
     "1.0",		0,			0,		NULL,
     "Enable/disable tranlucency" },
   { "gllod",	NULL,			rc_int,	&bitmap_lod,
     "0",		0,			0,		NULL,
     "level of bitmap-texture detail" },
   { "cabview",		NULL,			rc_bool,	&cabview,
     "0",		0,			0,		NULL,
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
   fprintf(stdout, "GLmame v0.7, by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com,\n");
   fprintf(stdout, "based upon GLmame v0.6 driver for xmame, written by Mike Oliphant\n");

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
  unsigned long winmask;
  Atom mwmatom;
  char *glxfx;
  int resizeEvtMask = 0;

  /* If using 3Dfx, Resize the window to fullscreen so we don't lose focus
     We have to do this after glXMakeCurrent(), or else the voodoo driver
     will give us the wrong resolution */
  if((glxfx=getenv("MESA_GLX_FX")) && (glxfx[0]=='f') )
  	fullscreen=1;

  if(!fullscreen) 
	  resizeEvtMask = StructureNotifyMask ;

  fprintf(stdout, "Using GLmame v0.7, by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com,\n");
  fprintf(stdout, "based upon GLmame v0.6 driver for xmame, written by Mike Oliphant\n");
  
  screen=DefaultScreenOfDisplay(display);
  myscreen=DefaultScreen(display);
  cursor=XCreateFontCursor(display,XC_trek);
  
  if(fullscreen)
  {
	winwidth = screen->width;
	winheight = screen->height;
  } else {
	winwidth = visual_width*widthscale;
	winheight = visual_height*heightscale;
  }

  setVisualAttribList(GLX_RGBA, 0 /* accum */, alphablending, doublebuffer, 
  		      0 /* stencil */);

  myvisual=glXChooseVisual(display,myscreen,visualAttribList);

  if(!myvisual) {
	fprintf(stderr,"OSD ERROR: failed to obtain visual.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }

  glContext=glXCreateContext(display,myvisual,0,GL_TRUE);

  if(!glContext) {
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

  if(fullscreen) 
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWDontPropagate | 
                CWColormap    | CWCursor
	        ;
  else
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap    
	        ;

  window=XCreateWindow(display,RootWindowOfScreen(screen),
		       0,0,winwidth,winheight,
		       0,myvisual->depth,
		       InputOutput,myvisual->visual,
		       winmask, &winattr);
  
  if (!window) {
	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
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
  
  glXMakeCurrent(display,window,glContext);

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
   glXDestroyContext(display,glContext);

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
