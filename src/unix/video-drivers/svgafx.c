/***************************************************************************

  Xmame 3Dfx console-mode driver

  Written based on Phillip Ezolt's svgalib driver by Mike Oliphant -

    oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

***************************************************************************/
#define __SVGAFX_C

#include <vga.h>
#include "fxgen.h"
#include "fxcompat.h"
#include "svgainput.h"
#include "sysdep/sysdep_display_priv.h"

unsigned int custom_window_width;
unsigned int custom_window_height;
unsigned int window_width;
unsigned int window_height;

static int svgafx_set_resolution(struct rc_option *option, const char *arg,
   int priority);

struct rc_option sysdep_display_opts[] =
{
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "FX (Glide) Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "resolution",	"res",			rc_use_function, NULL,
     "640x480",		0,			0,		svgafx_set_resolution,
     "Specify the resolution/ windowsize to use in the form of XRESxYRES" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static int svgafx_set_resolution(struct rc_option *option, const char *arg,
   int priority)
{
   if (sscanf(arg, "%ux%u", &custom_window_width, &custom_window_height) != 2)
      return 1;
   if (InitParams())
      return 1;
   option->priority = priority;
   return 0;
}

int sysdep_display_init(void)
{
   fprintf(stderr,
      "info: using FXmame v0.5 driver for xmame, written by Mike Oliphant\n");
   
   /* do this before calling vga_init, since this might need root rights */
   if (InitGlide()!=0)
      return 1;
   
   if (vga_init())
      return 1;
   
   if (svga_input_init())
      return 1;
   
   return 0;
}

void sysdep_display_exit(void)
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
int sysdep_display_open(const struct sysdep_display_open_params *params)
{
  sysdep_display_set_params(params);

  /* do this first since it seems todo some stuff which messes up svgalib
     when called after vga_setmode */
  if (InitVScreen() != 0)
     return 1;
   
  /* with newer svgalib's the console switch signals are only active if a
     graphics mode is set, so we set one which each card should support */
  vga_setmode(G320x200x16);
  
  /* init input */
  if(svga_input_open(release_function, acquire_function))
     return 1;

  /* call this one last since it needs to catch some signals
     which are also catched by svgalib */
  VScreenCatchSignals();
  
  return 0;
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

int sysdep_display_update(struct mame_bitmap *bitmap,
  struct rectangle *vis_area, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags)
{
   xfx_update_display(bitmap, vis_area, dirty_area, palette, flags);
   return 0;
}
