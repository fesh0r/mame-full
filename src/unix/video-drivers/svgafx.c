/***************************************************************************

  Xmame 3Dfx console-mode driver

  Written based on Phillip Ezolt's svgalib driver by Mike Oliphant -

    oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

***************************************************************************/
#define __SVGAFX_C

#include <vga.h>
#include "fxcompat.h"
#include "xmame.h"
#include "svgainput.h"

int  InitVScreen(void);
void CloseVScreen(void);
void VScreenCatchSignals(void);
void VScreenRestoreSignals(void);
int  InitGlide(void);
void ExitGlide(void);

extern struct rc_option fx_opts[];

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL, 		NULL,			rc_link,	fx_opts,
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
   
   /* do this before calling vga_init, since this might need root rights */
   if (InitGlide()!=OSD_OK)
      return OSD_NOT_OK;
   
   if (vga_init())
      return OSD_NOT_OK;
   
   if (svga_input_init())
      return OSD_NOT_OK;
   
   return OSD_OK;
}

void sysdep_close(void)
{
   svga_input_exit();
   ExitGlide();
}

static void release_function(void)
{
   grEnablePassThru();
}

static void acquire_function(void)
{
   grDisablePassThru();
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  /* do this first since it seems todo some stuff which messes up svgalib
     when called after vga_setmode */
  if (InitVScreen() != OSD_OK)
     return OSD_NOT_OK;
   
  /* with newer svgalib's the console switch signals are only active if a
     graphics mode is set, so we set one which each card should support */
  vga_setmode(G320x200x16);
  
  /* init input */
  if(svga_input_open(release_function, acquire_function))
     return OSD_NOT_OK;

  /* call this one last since it needs to catch some signals
     which are also catched by svgalib */
  VScreenCatchSignals();
  
  return OSD_OK;
}


/* shut up the display */
void sysdep_display_close(void)
{
   /* restore svgalib's signal handlers before closing svgalib down */
   VScreenRestoreSignals();
   
   /* close input */
   svga_input_close();
   
   /* close svgalib */
   vga_setmode(TEXT);

   /* do this last since it seems todo some stuff which messes up svgalib
      when done before vga_setmode(TEXT) */
   CloseVScreen();
}
