/*****************************************************************

  Xmame glide driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
/* pretend we're x11.c otherwise display and a few other crucial things don't
   get declared */
#define __X11_C_   
#define __XFX_C_


#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "xmame.h"
#include "x11.h"

int  InitVScreen(void);
void CloseVScreen(void);
int  InitGlide(void);
void ExitGlide(void);
void VScreenCatchSignals(void);
void VScreenRestoreSignals(void);

extern int fxwidth;
extern int fxheight;
extern struct rc_option fx_opts[];

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL, 		NULL,			rc_link,	fx_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL, 		NULL,			rc_link,	x11_input_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

int sysdep_init(void)
{
  fprintf(stderr,
     "info: using FXmame v0.5 driver for xmame, written by Mike Oliphant\n");
  
  /* Open the display. */
  display=XOpenDisplay(NULL);
  screen=DefaultScreenOfDisplay(display);

  if(!display) {
	fprintf (stderr,"OSD ERROR: failed to open the display.\n");
	return OSD_NOT_OK; 
  }

  return InitGlide();
}

void sysdep_close(void)
{
   ExitGlide();
   XCloseDisplay(display);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  if (x11_create_window(&fxwidth, &fxheight, 0))
    return 1;
    
  xinput_open(2, 0);
  
  if (InitVScreen() != OSD_OK)
     return OSD_NOT_OK;

  VScreenCatchSignals();
  
  return OSD_OK;
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void sysdep_display_close (void)
{
   VScreenRestoreSignals();

   CloseVScreen();  /* Shut down glide stuff */
   
   xinput_close();

   if(window) {
     XDestroyWindow(display, window);
     window = 0;
   }

   XSync(display, True); /* send all events to sync; */
}
