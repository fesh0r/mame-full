/*
 *	XFree86 VidMode and DGA support by Jens Vaasjo <jvaasjo@iname.com>
 */
#ifdef USE_DGA
#define __XF86_DGA_C

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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

static struct
{
	int screen;
	unsigned char *addr;
	char *base_addr;
	int width;
	int bank_size;
	int ram_size;
	blit_func_p update_display_func;
	XF86VidModeModeInfo **modes;
	XF86VidModeModeInfo orig_mode;
	int vidmode_changed;
} xf86ctx = {-1,NULL,NULL,-1,-1,-1,NULL,NULL,{0},0};
		
int xf86_dga1_init(void)
{
	int i;
	
	xf86ctx.screen          = DefaultScreen(display);
	
	if(!XF86DGAQueryDirectVideo(display, xf86ctx.screen, &i))
		fprintf(stderr,"XF86DGAQueryDirectVideo failed\n");
	else if(!(i & XF86DGADirectPresent))
		fprintf(stderr,"XF86DGADirectVideo support is not present\n");
	else if(!XF86DGAGetVideo(display,xf86ctx.screen,
		 &xf86ctx.base_addr,&xf86ctx.width,
		 &xf86ctx.bank_size,&xf86ctx.ram_size))
		fprintf(stderr,"XF86DGAGetVideo failed\n");
	else
		return 0; 
		
	fprintf(stderr,"Use of DGA-modes is disabled\n");
	return 1;
}

static int xf86_dga_vidmode_check_exts(void)
{
	int major,minor,event_base,error_base;

	if(!XF86VidModeQueryVersion(display,&major,&minor))
	{
		fprintf(stderr,"XF86VidModeQueryVersion failed\n");
		return 1;
	}

	if(!XF86VidModeQueryExtension(display,&event_base,&error_base))
	{
		fprintf(stderr,"XF86VidModeQueryExtension failed\n");
		return 1;
	}

	return 0;
}

static XF86VidModeModeInfo *xf86_dga_vidmode_find_best_vidmode(int depth)
{
	XF86VidModeModeInfo *bestmode = NULL;
	int score, best_score = 0;
	int i,modecount = 0;

	if(!XF86VidModeGetAllModeLines(display,xf86ctx.screen,
						&modecount,&xf86ctx.modes))
	{
		fprintf(stderr,"XF86VidModeGetAllModeLines failed\n");
		return NULL;
	}
	
	fprintf(stderr, "XF86DGA: info: found %d modes:\n", modecount);

	for(i=0;i<modecount;i++)
	{
		fprintf(stderr, "XF86DGA: info: found mode: %dx%d\n",
		   xf86ctx.modes[i]->hdisplay, xf86ctx.modes[i]->vdisplay);
		score = mode_match(xf86ctx.modes[i]->hdisplay, 
				xf86ctx.modes[i]->vdisplay, depth, 1);
		if(score > best_score)
		{
			best_score = score;
			bestmode   = xf86ctx.modes[i];
		}
	}

	return bestmode;
}

static Bool xf86_dga_vidmode_getmodeinfo(XF86VidModeModeInfo *modeinfo)
{
	XF86VidModeModeLine modeline;
	int dotclock;
	Bool err;

	err = XF86VidModeGetModeLine(display,xf86ctx.screen,
					&dotclock,&modeline);

	modeinfo->dotclock = dotclock;
	modeinfo->hdisplay = modeline.hdisplay;
	modeinfo->hsyncstart = modeline.hsyncstart;
	modeinfo->hsyncend = modeline.hsyncend;
	modeinfo->htotal = modeline.htotal;
	modeinfo->vdisplay = modeline.vdisplay;
	modeinfo->vsyncstart = modeline.vsyncstart;
	modeinfo->vsyncend = modeline.vsyncend;
	modeinfo->vtotal = modeline.vtotal;
	modeinfo->flags = modeline.flags;
	modeinfo->privsize = modeline.privsize;
	modeinfo->private = modeline.private;

	return err;
}

static void xf86_dga_vidmode_restoremode(Display *disp)
{
	XF86VidModeSwitchToMode(disp, xf86ctx.screen, &xf86ctx.orig_mode);
	/* 'Mach64-hack': restores screen when screwed up */
	XF86VidModeSwitchMode(disp,xf86ctx.screen,-1);
	XF86VidModeSwitchMode(disp,xf86ctx.screen,1);
	/**************************************************/
}

static int xf86_dga_vidmode_setup_mode_restore(void)
{
	Display *disp;
	int status;
	pid_t pid;

	if(!xf86_dga_vidmode_getmodeinfo(&xf86ctx.orig_mode))
	{
		fprintf(stderr,"XF86VidModeGetModeLine failed\n");
		return 1;
	}

	pid = fork();
	if(pid > 0)
	{
		waitpid(pid,&status,0);
		disp = XOpenDisplay(NULL);
		xf86_dga_vidmode_restoremode(disp);
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

static int xf86_dga_setup_graphics(XF86VidModeModeInfo *modeinfo, int dest_bpp)
{
	if(xf86ctx.bank_size != (xf86ctx.ram_size * 1024))
	{
		fprintf(stderr,"banked graphics modes not supported\n");
		return 1;
	}

	xf86ctx.update_display_func = sysdep_display_get_blitfunc_doublebuffer(dest_bpp);
	if (xf86ctx.update_display_func == NULL)
	{
		fprintf(stderr, "unsupported bpp: %dbpp\n", dest_bpp);
		return 1;
	}
	
	fprintf(stderr, "XF86-DGA1 running at: %dbpp\n", dest_bpp);
	
	xf86ctx.addr  = (unsigned char*)xf86ctx.base_addr;
	xf86ctx.addr += (((modeinfo->hdisplay - sysdep_display_params.width*
		sysdep_display_params.widthscale) / 2) &
		~sysdep_display_params.x_align) * dest_bpp / 8;
	xf86ctx.addr += ((modeinfo->vdisplay - sysdep_display_params.yarbsize)
		/ 2) * xf86ctx.width * dest_bpp / 8;
	return 0;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xf86_dga1_open_display(void)
{
	int i, count, dest_bpp=0;
	XPixmapFormatValues *pixmaps;
	XF86VidModeModeInfo *bestmode;
	/* only have todo the fork's the first time we go DGA, otherwise people
	   who do a lott of dga <-> window switching will get a lott of
	   children */
	static int first_time  = 1;
	xf86_dga_fix_viewport  = 0;
	xf86_dga_first_click   = 1;
	
	window  = RootWindow(display,xf86ctx.screen);
	/* detect dest_bpp */
	pixmaps = XListPixmapFormats(display, &count);
	if (!pixmaps)
	{
		fprintf(stderr, "X11-Error: Couldn't list pixmap formats.\n"
				"Probably out of memory.\n");
		return 1;
	}

	for(i=0; i<count; i++)
	{
		if(pixmaps[i].depth==DefaultDepth(display,xf86ctx.screen))
		{
			dest_bpp = pixmaps[i].bits_per_pixel;
			break;
		}  
	}
	if(i==count)
	{
		fprintf(stderr, "Couldn't find a zpixmap with the defaultcolordepth\nThis should not happen!\n");
		return 1;
	}
	XFree(pixmaps);

	/* setup the palette_info struct */
	if (x11_init_palette_info(DefaultVisual(display,xf86ctx.screen)) != 0)
		return 1;

	if(xf86_dga_vidmode_check_exts())
		return 1;

	bestmode = xf86_dga_vidmode_find_best_vidmode(
		(DefaultDepth(display,xf86ctx.screen)==24)? dest_bpp:
		DefaultDepth(display,xf86ctx.screen));
	if(!bestmode)
	{
		fprintf(stderr,"no suitable mode found\n");
		return 1;
	}
	mode_set_aspect_ratio((double)bestmode->hdisplay/bestmode->vdisplay);

	/* HACK HACK HACK, keys get stuck when they are pressed when
	   XDGASetMode is called, so wait for all keys to be released */
	do {
		char keys[32];
		XQueryKeymap(display, keys);
		for (i=0; (i<32) && (keys[i]==0); i++) {}
	} while(i<32);

	if(xf86_dga_setup_graphics(bestmode, dest_bpp))
		return 1;

	if (first_time)
	{
		if(xf86_dga_vidmode_setup_mode_restore())
			return 1;
	}

	fprintf(stderr,"VidMode Switching To Mode: %d x %d\n",
		bestmode->hdisplay, bestmode->vdisplay);

	if(!XF86VidModeSwitchToMode(display,xf86ctx.screen,bestmode))
	{
		fprintf(stderr,"XF86VidModeSwitchToMode failed\n");
		return 1;
	}
	xf86ctx.vidmode_changed = 1;

        /* 2 means grab keyb and mouse ! */
	if(xinput_open(2, 0))
	{
		fprintf(stderr,"XGrabKeyboard failed\n");
		return 1;
	}

	if(first_time)
	{
		if(XF86DGAForkApp(xf86ctx.screen))
		{
			perror("fork");
			return 1;
		}
		first_time = 0;
	}

	if(!XF86DGADirectVideo(display,xf86ctx.screen,
				XF86DGADirectGraphics|XF86DGADirectMouse|XF86DGADirectKeyb))
	{
		fprintf(stderr,"XF86DGADirectVideo failed\n");
		return 1;
	}

	if(!XF86DGASetViewPort(display,xf86ctx.screen,0,0))
	{
		fprintf(stderr,"XF86DGASetViewPort failed\n");
		return 1;
	}

	memset(xf86ctx.base_addr,0,xf86ctx.bank_size);

	return effect_open();
}

void xf86_dga1_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *src_bounds,  struct rectangle *dest_bounds,
	  struct sysdep_palette_struct *palette, unsigned int flags)
{
	if(xf86_dga_fix_viewport)
	{
		XF86DGASetViewPort(display,xf86ctx.screen,0,0);
		xf86_dga_fix_viewport = 0;
	}
	
	xf86ctx.update_display_func(bitmap, src_bounds, dest_bounds,
		palette, xf86ctx.addr, xf86ctx.width);
}

void xf86_dga1_close_display(void)
{
    	effect_close();
	xinput_close();
	XF86DGADirectVideo(display,xf86ctx.screen, 0);
	if(xf86ctx.vidmode_changed)
	{
		xf86_dga_vidmode_restoremode(display);
		xf86ctx.vidmode_changed = 0;
	}
	if(xf86ctx.modes)
	{
		XFree(xf86ctx.modes);
		xf86ctx.modes = NULL;
	}
	XSync(display,True);
}

#endif /*def USE_DGA*/
