/*****************************************************************

  Xmame glide driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
#define __XFX_C_


#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* from fxgen.c */
int  InitVScreen(void);
void CloseVScreen(void);
int  InitGlide(void);
void ExitGlide(void);
void VScreenCatchSignals(void);
void VScreenRestoreSignals(void);
void UpdateFXDisplay(struct mame_bitmap *bitmap,
	  struct rectangle *dirty_area,  struct rectangle *vis_area,
	  struct sysdep_palette_struct *palette, unsigned int flags);

extern int fxwidth;
extern int fxheight;

int xfx_init(void)
{
  fprintf(stderr,
     "info: using FXmame v0.5 driver for xmame, written by Mike Oliphant\n");
  
  return InitGlide();
}

void xfx_exit(void)
{
  ExitGlide();
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xfx_open_display(void)
{
  sysdep_display_check_params();
  mode_check_params((double)fxwidth/fxheight);

  if (x11_create_window(&fxwidth, &fxheight, 0))
    return 1;
    
  xinput_open(2, 0);
  
  if (InitVScreen() != 0)
     return 1;

  VScreenCatchSignals();
  
  return 0;
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void xfx_close_display (void)
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

void xfx_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *dirty_area,  struct rectangle *vis_area,
	  struct sysdep_palette_struct *palette, unsigned int flags)
{
   UpdateFXDisplay(bitmap, dirty_area, vis_area, palette, flags);
}
