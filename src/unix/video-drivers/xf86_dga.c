/*
 *     XFree86 VidMode and DGA support by Jens Vaasjo <jvaasjo@iname.com>
 *     Modified for DGA 2.0 support
 *                                      by Shyouzou Sugitani <shy@debian.or.jp>
 *                                         Stea Greene <stea@cs.binghamton.edu>
 */

#ifdef USE_DGA
#define __XF86_DGA_C

#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#endif
#include "xmame.h"
#include "x11.h"

static int  (*p_xf86_dga_create_display)(int);
static void (*p_xf86_dga_close_display)(void);
static void (*p_xf86_dga_update_display)(struct mame_bitmap *);

#ifdef USE_DGA

int xf86_dga_init(void)
{
	int i, j;
	char *s;

	mode_available[X11_DGA] = FALSE;


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
		p_xf86_dga_create_display = xf86_dga2_create_display;
		p_xf86_dga_close_display  = xf86_dga2_close_display;
		p_xf86_dga_update_display = xf86_dga2_update_display;
		return xf86_dga2_init();
	}
#endif
	else
	{
		p_xf86_dga_create_display = xf86_dga1_create_display;
		p_xf86_dga_close_display  = xf86_dga1_close_display;
		p_xf86_dga_update_display = xf86_dga1_update_display;
		return xf86_dga1_init();
	}

	if (!mode_available[X11_DGA])
		fprintf(stderr,"Use of DGA-modes is disabled\n");

	return OSD_OK;
}

int  xf86_dga_create_display(int depth)
{
	return (*p_xf86_dga_create_display)(depth);
}

void xf86_dga_close_display(void)
{
	(*p_xf86_dga_close_display)();
}

void xf86_dga_update_display(struct mame_bitmap *bitmap)
{
	(*p_xf86_dga_update_display)(bitmap);
}

#endif /*def USE_DGA*/
