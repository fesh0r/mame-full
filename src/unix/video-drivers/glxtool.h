#ifndef _GLXTOOL_H
	#define _GLXTOOL_H

	#include "gltool.h"
	  
	#define BUFFER_SINGLE 0
	#define BUFFER_DOUBLE 1
	 
	#define COLOR_INDEX 0
	#define COLOR_RGBA  1
	 
	#define STEREO_OFF 0
	#define STEREO_ON  1

	typedef struct {
	  int buffer;
	  int color;
	  int stereo;
	  int depthBits;
	  int stencilBits;

	  int redBits;
	  int greenBits;
	  int blueBits;
	  int alphaBits;
	  int accumRedBits;
	  int accumGreenBits;
	  int accumBlueBits;
	  int accumAlphaBits;

	  long nativeVisualID;
	} GLCapabilities;

	typedef struct {
		XVisualInfo * visual;
		GLXContext    gc;
		int           success;  /* 1: OK, 0: ERROR */
	} VisualGC;
  
	int get_GC( Display *display, Window win, XVisualInfo *visual,
		    GLXContext *gc, GLXContext gc_share,
		    int verbose );

	VisualGC findVisualGlX( Display *display, 
				       Window rootWin,
				       Window * pWin, 
				       int width, int height,
				       GLCapabilities * glCaps,
				       int * pOwnWin,
			               XSetWindowAttributes * pOwnWinAttr,
				       unsigned long ownWinmask,
				       GLXContext shareWith,
				       int offscreen,
				       Pixmap *pix,
				       int verbose
				     );

	XVisualInfo * findVisualIdByID( XVisualInfo ** visualList, 
					int visualID, Display *disp,
					Window win, int verbose);

	XVisualInfo * findVisualIdByFeature( XVisualInfo ** visualList, 
					     Display *disp, Window win,
					     GLCapabilities *glCaps,
					     int verbose);

	int testVisualInfo ( Display *display, XVisualInfo * vi, 
			     GLCapabilities *glCaps);

	int setGLCapabilities ( Display * disp, 
				XVisualInfo * visual, GLCapabilities *glCaps);

	void printAllVisualInfo ( Display *disp, Window win, int verbose);
	void printVisualInfo ( Display *display, XVisualInfo * vi);
	void printGLCapabilities ( GLCapabilities *glCaps );

	Window createOwnOverlayWin(Display *display, Window rootwini, Window parentWin,
			           XSetWindowAttributes * pOwnWinAttr,
				   unsigned long ownWinmask,
				   XVisualInfo *visual, int width, int height);
#endif
