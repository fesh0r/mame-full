/*
 *     XFree86 VidMode and DGA support by Jens Vaasjo <jvaasjo@iname.com>
 *     Modified for DGA 2.0 support
 *     by Shyouzou Sugitani <shy@debian.or.jp>
 *     Stea Greene <stea@cs.binghamton.edu>
 */

#ifdef USE_DGA
#define __XF86_DGA_C

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#endif
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

#ifdef USE_DGA

static int  (*p_xf86_dga_open_display)(void);
static void (*p_xf86_dga_close_display)(void);
static void (*p_xf86_dga_update_display)(struct mame_bitmap *,
	  struct rectangle *src_bounds,  struct rectangle *dest_bounds,
	  struct sysdep_palette_struct *palette, unsigned int flags);

int xf86_dga_init(void)
{
	int i, j;
	char *s;

	if(geteuid())
		fprintf(stderr,"DGA requires root rights\n");
	else if (!(s = getenv("DISPLAY")) || (s[0] != ':'))
		fprintf(stderr,"DGA only works on a local display\n");
	else if(!XF86DGAQueryExtension(display, &i, &i))
		fprintf(stderr,"XF86DGAQueryExtension failed\n");
	else if(!XF86DGAQueryVersion(display, &i, &j))
		fprintf(stderr,"XF86DGAQueryVersion failed\n");
#ifdef X_XDGASetMode
	else if (i >= 2)
	{
		p_xf86_dga_open_display = xf86_dga2_open_display;
		p_xf86_dga_close_display  = xf86_dga2_close_display;
		p_xf86_dga_update_display = xf86_dga2_update_display;
		return xf86_dga2_init();
	}
#endif
	else
	{
		p_xf86_dga_open_display = xf86_dga1_open_display;
		p_xf86_dga_close_display  = xf86_dga1_close_display;
		p_xf86_dga_update_display = xf86_dga1_update_display;
		return xf86_dga1_init();
	}

	fprintf(stderr,"Use of DGA-modes is disabled\n");
	return 1;
}

int  xf86_dga_open_display(void)
{
	return (*p_xf86_dga_open_display)();
}

void xf86_dga_close_display(void)
{
	(*p_xf86_dga_close_display)();
}

void xf86_dga_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *src_bounds,  struct rectangle *dest_bounds,
	  struct sysdep_palette_struct *palette, unsigned int flags)
{
	(*p_xf86_dga_update_display)(bitmap, src_bounds, dest_bounds,
		palette, flags);
}

#endif /*def USE_DGA*/
