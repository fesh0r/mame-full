/*
 *	XFree86 VidMode and DGA support by Jens Vaasjo <jvaasjo@iname.com>
 *      Modified for DGA 2.0 native API support
 *      by Shyouzou Sugitani <shy@debian.or.jp>
 *      Stea Greene <stea@cs.binghamton.edu>
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

#ifdef X_XDGASetMode

#define TDFX_DGA_WORKAROUND

static struct
{
	int screen;
	unsigned char *addr;
	int width;
	Colormap cmap;
	blit_func_p update_display_func;
	XDGADevice *device;
	XDGAMode *modes;
	int mode_count;
	int vidmode_changed;
	int aligned_viewport_height;
	int page;
	int max_page;
	int max_page_limit;
	int page_dirty;
#ifdef TDFX_DGA_WORKAROUND
	int current_X11_mode;
#endif
} xf86ctx = {-1,NULL,-1,0,NULL,NULL,NULL,0,0,0,0,0,2,-1};
	
struct rc_option xf86_dga2_opts[] = {
  /* name, shortname, type, dest, deflt, min, max, func, help */
  { "vsync-pagelimit", "vspl", rc_int, &(xf86ctx.max_page_limit), "2", 0, 16, NULL, "Maximum number of pages (frames) to queue in videomemory for display after vsync" },
  { NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

static int xf86_dga_set_mode(void);

int xf86_dga2_init(void)
{
	int i,j ;
	
	xf86ctx.screen           = DefaultScreen(display);
	
	if(!XDGAQueryVersion(display, &i, &j))
		fprintf(stderr,"XDGAQueryVersion failed\n");
	else if (i < 2)
		fprintf(stderr,"This driver requires DGA 2.0 or newer\n");
	else if(!XDGAQueryExtension(display, &i, &j))
		fprintf(stderr,"XDGAQueryExtension failed\n");
	else if(!XDGAOpenFramebuffer(display,xf86ctx.screen))
		fprintf(stderr,"XDGAOpenFramebuffer failed\n");
	else
		return 0; 
		
	fprintf(stderr,"Use of DGA-modes is disabled\n");
	return 1;
}

static int xf86_dga_vidmode_find_best_vidmode(void)
{
	int i;
	int bestmode = 0;
	int score, best_score = 0;

#ifdef TDFX_DGA_WORKAROUND
	int dotclock;
	XF86VidModeModeLine modeline;

	XF86VidModeGetModeLine(display, xf86ctx.screen, &dotclock, &modeline);
#endif

	if (!xf86ctx.modes)
	{
		xf86ctx.modes = XDGAQueryModes(display, xf86ctx.screen, &xf86ctx.mode_count);
		fprintf(stderr, "XDGA: info: found %d modes:\n", xf86ctx.mode_count);
	}

	for(i=0;i<xf86ctx.mode_count;i++)
	{
#ifdef TDFX_DGA_WORKAROUND
		if (!xf86ctx.vidmode_changed &&
			xf86ctx.modes[i].viewportWidth == modeline.hdisplay &&
			xf86ctx.modes[i].viewportHeight == modeline.vdisplay)
			xf86ctx.current_X11_mode = xf86ctx.modes[i].num;
#endif
#if 0 /* DEBUG */
		fprintf(stderr, "XDGA: info: (%d) %s\n",
		   xf86ctx.modes[i].num, xf86ctx.modes[i].name);
		fprintf(stderr, "          : VRefresh = %f [Hz]\n",
		   xf86ctx.modes[i].verticalRefresh);
		/* flags */
		fprintf(stderr, "          : viewport = %dx%d\n",
		   xf86ctx.modes[i].viewportWidth, xf86ctx.modes[i].viewportHeight);
		fprintf(stderr, "          : image = %dx%d\n",
		   xf86ctx.modes[i].imageWidth, xf86ctx.modes[i].imageHeight);
		if (xf86ctx.modes[i].flags & XDGAPixmap)
			fprintf(stderr, "          : pixmap = %dx%d\n",
				xf86ctx.modes[i].pixmapWidth, xf86ctx.modes[i].pixmapHeight);
		fprintf(stderr, "          : bytes/scanline = %d\n",
		   xf86ctx.modes[i].bytesPerScanline);
		fprintf(stderr, "          : byte order = %s\n",
			xf86ctx.modes[i].byteOrder == MSBFirst ? "MSBFirst" :
			xf86ctx.modes[i].byteOrder == LSBFirst ? "LSBFirst" :
			"Unknown");
		fprintf(stderr, "          : bpp = %d, depth = %d\n",
			xf86ctx.modes[i].bitsPerPixel, xf86ctx.modes[i].depth);
		fprintf(stderr, "          : RGBMask = (%lx, %lx, %lx)\n",
			xf86ctx.modes[i].redMask,
			xf86ctx.modes[i].greenMask,
			xf86ctx.modes[i].blueMask);
		fprintf(stderr, "          : visual class = %s\n",
			xf86ctx.modes[i].visualClass == TrueColor ? "TrueColor":
			xf86ctx.modes[i].visualClass == DirectColor ? "DirectColor" :
			xf86ctx.modes[i].visualClass == PseudoColor ? "PseudoColor" : "Unknown");
		fprintf(stderr, "          : xViewportStep = %d, yViewportStep = %d\n",
			xf86ctx.modes[i].xViewportStep, xf86ctx.modes[i].yViewportStep);
		fprintf(stderr, "          : maxViewportX = %d, maxViewportY = %d\n",
			xf86ctx.modes[i].maxViewportX, xf86ctx.modes[i].maxViewportY);
		/* viewportFlags */
#endif
		score = mode_match(xf86ctx.modes[i].viewportWidth,
			xf86ctx.modes[i].viewportHeight,
			(xf86ctx.modes[i].depth==24)?
			xf86ctx.modes[i].bitsPerPixel:
			xf86ctx.modes[i].depth, 1);
		if(score > best_score)
		{
			best_score = score;
			bestmode   = xf86ctx.modes[i].num;
		}
	}

	return bestmode;
}

static int xf86_dga_vidmode_setup_mode_restore(void)
{
	Display *disp;
	int status;
	pid_t pid;

	pid = fork();
	if(pid > 0)
	{
		waitpid(pid,&status,0);
		disp = XOpenDisplay(NULL);
		XDGACloseFramebuffer(disp, xf86ctx.screen);
		XDGASetMode(disp, xf86ctx.screen, 0);
		XCloseDisplay(disp);
		_exit(!WIFEXITED(status));
	}

	if (pid < 0)
	{
		perror("fork");
		return 1;
	}

	return 0;
}

static int xf86_dga_setup_graphics(XDGAMode modeinfo)
{
	int scaled_height = sysdep_display_params.yarbsize?
	        sysdep_display_params.yarbsize:
	        sysdep_display_params.height*sysdep_display_params.heightscale;
	
	xf86ctx.update_display_func = sysdep_display_get_blitfunc_doublebuffer(modeinfo.bitsPerPixel);
	if (xf86ctx.update_display_func == NULL)
	{
		fprintf(stderr, "\nError: bitmap depth %d isnot supported on %dbpp displays\n", sysdep_display_params.depth, modeinfo.bitsPerPixel);
		return 1;
	}
	
	fprintf(stderr, "XF86-DGA2 color depth: %d, %dbpp\n", modeinfo.depth, modeinfo.bitsPerPixel);
	
	xf86ctx.addr  = (unsigned char*)xf86ctx.device->data;
	xf86ctx.addr += (((modeinfo.viewportWidth - sysdep_display_params.width*
		sysdep_display_params.widthscale) / 2) & ~3)
		* modeinfo.bitsPerPixel / 8;
	xf86ctx.addr += ((modeinfo.viewportHeight - scaled_height)
		/ 2) * modeinfo.bytesPerScanline;

	/* setup page flipping */
	if(modeinfo.viewportFlags & XDGAFlipRetrace)
	{
	  xf86ctx.aligned_viewport_height = (modeinfo.viewportHeight+
	    modeinfo.yViewportStep-1) & ~(modeinfo.yViewportStep-1);
	  xf86ctx.page = 0;
	  xf86ctx.max_page = modeinfo.maxViewportY /
	    xf86ctx.aligned_viewport_height;
	  if (xf86ctx.max_page > xf86ctx.max_page_limit)
	    xf86ctx.max_page = xf86ctx.max_page_limit;
	  if (xf86ctx.max_page)
	    fprintf(stderr, "DGA using vsynced page flipping, hw-buffering max %d frames\n", xf86ctx.max_page);
	}
	else
	  xf86ctx.max_page = 0;
	
	/* force a full update during the max_page-s first frames */
	xf86ctx.page_dirty = xf86ctx.max_page + 1;

	return 0;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xf86_dga2_open_display(void)
{
	int i;
	/* only have todo the fork's the first time we go DGA, otherwise people
	   who do a lott of dga <-> window switching will get a lott of
	   children */
	static int first_time  = 1;
	xf86_dga_first_click   = 0;
	
	window  = RootWindow(display,xf86ctx.screen);
	
	if (first_time)
	{
		if(xf86_dga_vidmode_setup_mode_restore())
			return 1;
		first_time = 0;
	}

	/* HACK HACK HACK, keys get stuck when they are pressed when
	   XDGASetMode is called, so wait for all keys to be released */
	do {
	  char keys[32];
	  XQueryKeymap(display, keys);
	  for (i=0; (i<32) && (keys[i]==0); i++) {}
	} while(i<32);

        /* 2 means grab keyb and mouse ! */
	if(xinput_open(2, 0))
	{
	    fprintf(stderr,"XGrabKeyboard failed\n");
	    return 1;
	}

	if(effect_open())
	    return 1;
	    
	if(xf86_dga_set_mode())
	    return 1;
	    
	/* setup the colormap */
	xf86ctx.cmap = XDGACreateColormap(display, xf86ctx.screen,
					  xf86ctx.device, AllocNone);
	XDGAInstallColormap(display,xf86ctx.screen,xf86ctx.cmap);
	
	return 0;
}

static int xf86_dga_set_mode(void)
{
	int bestmode;
	Visual dga_xvisual;

	bestmode = xf86_dga_vidmode_find_best_vidmode();
	if (!bestmode)
	{
		fprintf(stderr,"no suitable mode found\n");
		return 1;
	}
	xf86ctx.device = XDGASetMode(display,xf86ctx.screen,bestmode);
	if (xf86ctx.device == NULL) {
		fprintf(stderr,"XDGASetMode failed\n");
		return 1;
	}
	xf86ctx.width = xf86ctx.device->mode.bytesPerScanline * 8
		/ xf86ctx.device->mode.bitsPerPixel;
	xf86ctx.vidmode_changed = 1;

#if 0 /* DEBUG */
	fprintf(stderr, "Debug: bitmap_depth =%d   mode.bitsPerPixel = %d"
			"   mode.depth = %d\n", bitmap_depth, 
			xf86ctx.device->mode.bitsPerPixel, 
			xf86ctx.device->mode.depth);
#endif

	fprintf(stderr,"XF86DGA2 switched To Mode: %d x %d\n",
		xf86ctx.device->mode.viewportWidth,
		xf86ctx.device->mode.viewportHeight);

	/* setup the palette_info struct */
	dga_xvisual.class = xf86ctx.device->mode.visualClass;
	dga_xvisual.red_mask = xf86ctx.device->mode.redMask;
	dga_xvisual.green_mask = xf86ctx.device->mode.greenMask;
	dga_xvisual.blue_mask = xf86ctx.device->mode.blueMask;
	if (x11_init_palette_info(&dga_xvisual) != 0)
	    return 1;
        
	mode_set_aspect_ratio((double)xf86ctx.device->mode.viewportWidth/
		xf86ctx.device->mode.viewportHeight);

	if(xf86_dga_setup_graphics(xf86ctx.device->mode))
	    return 1;
	
	/* setup the viewport */
	XDGASetViewport(display,xf86ctx.screen,0,0,0);
	while(XDGAGetViewportStatus(display, xf86ctx.screen))
		;

	/* clear the screen */
	memset(xf86ctx.device->data, 0,
	       xf86ctx.device->mode.bytesPerScanline
	       * xf86ctx.device->mode.imageHeight);

	return 0;
}

int  xf86_dga2_resize_display(void)
{
	XFree(xf86ctx.device);
	xf86ctx.device = 0;
	if (xf86_dga_set_mode())
	{
		fprintf(stderr, "FATAL Error changing videomode, aborting\n");
		sysdep_display_close();
		exit(1);
	}
	return 1;
}

void xf86_dga2_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette, unsigned int flags)
{
  if (xf86ctx.max_page)
  {
    /* force a full screen update */
    if (xf86ctx.page_dirty)
    {
      *dirty_area = *vis_area;
      xf86ctx.page_dirty--;
    }
    while(XDGAGetViewportStatus(display, xf86ctx.screen) & (0x01 <<
          (xf86ctx.max_page - 1)))
    {
    }
  }

  xf86ctx.page = (xf86ctx.page + 1) % (xf86ctx.max_page + 1);
  xf86ctx.update_display_func(bitmap, vis_area, dirty_area,
        palette, xf86ctx.addr + xf86ctx.aligned_viewport_height *
	xf86ctx.page * xf86ctx.device->mode.bytesPerScanline, xf86ctx.width);

  if (xf86ctx.max_page)
    XDGASetViewport(display, xf86ctx. screen, 0, xf86ctx.page *
      xf86ctx.aligned_viewport_height, XDGAFlipRetrace);

  XDGASync(display,xf86ctx.screen);
}

void xf86_dga2_close_display(void)
{
	if(xf86ctx.device)
	{
		XFree(xf86ctx.device);
		xf86ctx.device = 0;
	}
	if(xf86ctx.modes)
	{
		XFree(xf86ctx.modes);
		xf86ctx.modes = 0;
	}
	if(xf86ctx.cmap)
	{
		XFreeColormap(display,xf86ctx.cmap);
		xf86ctx.cmap = 0;
	}
    	effect_close();
	xinput_close();
	if(xf86ctx.vidmode_changed)
	{
		/* HDG: is this really nescesarry ? */
		XDGASync(display,xf86ctx.screen);
#ifdef TDFX_DGA_WORKAROUND
		/* Restore the right video mode before leaving DGA  */
		/* The tdfx driver would have to do it, but it doesn't work ...*/
		XDGASetMode(display, xf86ctx.screen, xf86ctx.current_X11_mode);
#endif
		XDGASetMode(display, xf86ctx.screen, 0);
		xf86ctx.vidmode_changed = 0;
	}
	XSync(display, True);
}

#endif /*def X_XDGASetMode*/
