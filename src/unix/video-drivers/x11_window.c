/*
 * Modifications For OpenVMS By:  Robert Alan Byer
 *                                byer@mail.ourservers.net
 *                                Jan. 9, 2004
 */

/*
 * X-Mame video specifics code
 *
 */
#ifdef x11
#define __X11_WINDOW_C_

/*
 * Include files.
 */

/* for FLT_MAX */
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef USE_MITSHM
#  if defined(__DECC) && defined(VMS)
#    include <X11/extensions/ipc.h>
#    include <X11/extensions/shm.h>
#  else
#    include <sys/ipc.h>
#    include <sys/shm.h>
#  endif
#  include <X11/extensions/XShm.h>
#endif

#ifdef USE_XV
/* we need MITSHM! */
#ifndef USE_MITSHM
#error "USE_XV defined but USE_MITSHM is not defined, XV needs MITSHM!"
#endif
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* for xscreensaver support */
/* Commented out for now since it causes problems with some 
 * versions of KDE.
 */
/* #include "vroot.h" */

#ifdef USE_HWSCALE
static void x11_window_update_16_to_YV12 (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void x11_window_update_16_to_YV12_perfect (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void x11_window_update_32_to_YV12_direct (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void x11_window_update_32_to_YV12_direct_perfect (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
#endif
static blit_func_p x11_window_update_display_func;

/* we need these to do the clean up correctly */
#ifdef USE_MITSHM
static int mit_shm_attached = 0;
static XShmSegmentInfo shm_info;
static int use_mit_shm = 1;  /* use mitshm if available */
#endif

#ifdef USE_HWSCALE
static int use_hwscale=0;
static int hwscale_bpp=0;
static long hwscale_format=0;
static int hwscale_yuv=0;
static int hwscale_yv12=0;
static char *hwscale_yv12_rotate_buf0=NULL;
static char *hwscale_yv12_rotate_buf1=NULL;
static int hwscale_perfect_yuv=1;
static Visual hwscale_visual;
#endif

#ifdef USE_XV
static XvImage *xvimage = NULL;
static int xv_port=-1;
static int use_xv=0;
#endif

static XImage *image = NULL;
static GC gc;
static int image_width;
enum { X11_NORMAL, X11_MITSHM, X11_XVSHM, X11_XIL };
static int x11_window_update_method;
static int startx = 0;
static int starty = 0;
static int use_xsync = 0;

struct rc_option x11_window_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11-window Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL,  NULL },
#ifdef USE_MITSHM
	{ "mitshm", "ms", rc_bool, &use_mit_shm, "1", 0, 0, NULL, "Use/don't use MIT Shared Mem (if available and compiled in)" },
#endif
#ifdef USE_XIL
	{ "xil", "x", rc_bool, &use_xil, "1", 0, 0, NULL, "Enable/disable use of XIL for scaling (if available and compiled in)" },
	{ "mtxil", "mtx", rc_bool, &use_mt_xil, "0", 0, 0, NULL, "Enable/disable multi threading of XIL" },
#endif	
#ifdef USE_HWSCALE
	{ "yuv", NULL, rc_bool, &hwscale_yuv, "0", 0, 0, NULL, "Force YUV mode (for video cards with broken RGB hwscales)" },
	{ "yv12", NULL, rc_bool, &hwscale_yv12, "0", 0, 0, NULL, "Force YV12 mode (for video cards with broken RGB hwscales)" },
	{ "perfect-yuv", NULL, rc_bool, &hwscale_perfect_yuv, "1", 0, 0, NULL, "Use perfect (slower) blitting code for XV YUV blits" },
#endif
	{ "xsync", "xs", rc_bool, &use_xsync, "1", 0, 0, NULL, "Use/don't use XSync instead of XFlush as screen refresh method" },
	{ "run-in-root-window", "root", rc_bool, &run_in_root_window, "0", 0, 0, NULL, "Enable/disable running in root window" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};


/*
 * Create a display screen, or window, large enough to accomodate a bitmap
 * of the given dimensions.
 */

#ifdef USE_MITSHM
/* following routine traps missused MIT-SHM if not available */
int test_mit_shm (Display * display, XErrorEvent * error)
{
	char msg[256];
	unsigned char ret = error->error_code;

	XGetErrorText (display, ret, msg, 256);
	/* if MIT-SHM request failed, note and continue */
	if ((ret == BadAccess) || (ret == BadAlloc))
	{
		use_mit_shm = 0;
		return 0;
	}
	/* else unexpected error code: notify and exit */
	fprintf (stderr, "Unexpected X Error %d: %s\n", ret, msg);
	exit(1);
	/* to make newer gcc's shut up, grrr */
	return 0;
}
#endif

#ifdef USE_XV
int FindXvPort(Display *dpy, long format, int *port)
{
	int i,j,p,ret,num_formats;
	unsigned int num_adaptors;
	XvAdaptorInfo *ai;
	XvImageFormatValues *fo;

	ret = XvQueryAdaptors(dpy, DefaultRootWindow(dpy),
			&num_adaptors, &ai);

	if (ret != Success)
	{
		fprintf(stderr,"XV: QueryAdaptors failed\n");
		return 0;
	}

	for (i = 0; i < num_adaptors; i++)
	{
		int firstport=ai[i].base_id;
		int portcount=ai[i].num_ports;

		for (p = firstport; p < ai[i].base_id+portcount; p++)
		{
			fo = XvListImageFormats(dpy, p, &num_formats);
			for (j = 0; j < num_formats; j++)
			{
				if((fo[j].id==format))
				{
					if(XvGrabPort(dpy,p,CurrentTime)==Success)
					{
						*port=p;
						XFree(fo);
						return 1;
					}
				}
			}
			XFree(fo);
		}
	}
	XvFreeAdaptorInfo(ai);
	return 0;
}

int FindRGBXvFormat(Display *dpy, int *port,long *format,int *bpp)
{
	int i,j,p,ret,num_formats;
	unsigned int num_adaptors;
	XvAdaptorInfo *ai;
	XvImageFormatValues *fo;

	ret = XvQueryAdaptors(dpy, DefaultRootWindow(dpy),
			&num_adaptors, &ai);

	if (ret != Success)
	{
		fprintf(stderr,"XV: QueryAdaptors failed\n");
		return 0;
	}

	for (i = 0; i < num_adaptors; i++)
	{
		int firstport=ai[i].base_id;
		int portcount=ai[i].num_ports;

		for (p = firstport; p < ai[i].base_id+portcount; p++)
		{
			fo = XvListImageFormats(dpy, p, &num_formats);
			for (j = 0; j < num_formats; j++)
			{
				if((fo[j].type==XvRGB) && (fo[j].format==XvPacked))
				{
					if(XvGrabPort(dpy,p,CurrentTime)==Success)
					{
						*bpp=fo[j].bits_per_pixel;
						*port=p;
						*format=fo[j].id;
						hwscale_visual.red_mask  =fo[j].red_mask;
						hwscale_visual.green_mask=fo[j].green_mask;
						hwscale_visual.blue_mask =fo[j].blue_mask;
						XFree(fo);
						return 1;
					}
				}
			}
			XFree(fo);
		}
	}
	XvFreeAdaptorInfo(ai);
	return 0;
}

#endif
#ifdef USE_HWSCALE

/* Since with YUV formats a field of zeros is generally
   loud green, rather than black, it makes sense
   to clear the image before use (since scanline algorithms
   leave alternate lines "black") */
void ClearYUY2()
{
  int i,j;
  char *yuv=(xvimage->data+xvimage->offsets[0]);
  fprintf(stderr,"Clearing YUY2\n");
  for (i = 0; i < xvimage->height; i++)
  {
    for (j = 0; j < xvimage->width; j++)
    {
      int offset=(xvimage->width*i+j)*2;
      yuv[offset] = 0;
      yuv[offset+1]=-128;
    }
  }
}

void ClearYV12()
{
  int i,j;
  char *y=(xvimage->data+xvimage->offsets[0]);
  char *u=(xvimage->data+xvimage->offsets[1]);
  char *v=(xvimage->data+xvimage->offsets[2]);
  fprintf(stderr,"Clearing YV12\n");
  for (i = 0; i < xvimage->height; i++) {
    for (j = 0; j < xvimage->width; j++) {
      int offset=(xvimage->width*i+j);
      y[offset] = 0;
      if((i&1) && (j&1))
      {
        offset = (xvimage->width/2)*(i/2) + (j/2);
        u[offset] = -128;
        v[offset] = -128;
      }
    }
  }
}
#endif

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int x11_window_open_display(void)
{
        Visual *xvisual;
	XGCValues xgcv;
	int i;
	int image_height;
	int event_mask = ExposureMask;
	int dest_bpp;
	int orig_widthscale, orig_yarbsize;

	/* set all the default values */
	window = 0;
	x11_window_update_method = X11_NORMAL;
	image  = NULL;
#ifdef USE_XV
	xvimage = NULL;
#endif
#ifdef USE_MITSHM
	mit_shm_attached = 0;
#endif
        /* set the aspect_ratio, do this here since
           this can change yarbsize */
        mode_set_aspect_ratio((double)screen->width/screen->height);

	orig_widthscale  = sysdep_display_params.widthscale;
	orig_yarbsize    = sysdep_display_params.yarbsize;
	window_width     = sysdep_display_params.widthscale * 
	  sysdep_display_params.width;
	window_height    = sysdep_display_params.yarbsize;
	image_width      = sysdep_display_params.widthscale *
	  sysdep_display_params.aligned_width;
	image_height     = sysdep_display_params.yarbsize;
#ifdef USE_HWSCALE
        use_hwscale	 = 0;

	if (hwscale_yv12)
		hwscale_yuv=1;
#endif
#ifdef USE_XV
	use_xv = (x11_video_mode == X11_XV);

        /* we need MIT-SHM ! */
        if (use_xv)
           use_mit_shm=1;
#endif

	/* check the available extensions if compiled in */
#ifdef USE_MITSHM
        if (use_mit_shm) /* look for available Mitshm extensions */
        {
                /* get XExtensions to be if mit shared memory is available */
                if (XQueryExtension (display, "MIT-SHM", &i, &i, &i))
                {
                        x11_window_update_method = X11_MITSHM;
                }
                else
                {
                        fprintf (stderr, "X-Server Doesn't support MIT-SHM extension\n");
                        use_mit_shm = 0;
                }
        }
#endif
#ifdef USE_XIL
	if (use_xil)
	{
		init_xil ();
	}

	/*
	 *  If the XIL initialization worked, then use_xil will still be set.
	 */
	if (use_xil)
	{
	    /* xil does normal scaling for us */
            if (sysdep_display_params.effect == 0)
            {
		image_width  = sysdep_display_params.aligned_width;
		image_height = sysdep_display_params.height;
		sysdep_display_params.widthscale = 1;
		sysdep_display_params.yarbsize   = image_height;
            }
            x11_window_update_method = X11_XIL;
	}
#endif
#ifdef USE_XV
	if (use_mit_shm && use_xv) /* look for available Xv extensions */
	{
		unsigned int p_version,p_release,p_request_base,p_event_base,p_error_base;
		if(XvQueryExtension(display, &p_version, &p_release, &p_request_base,
					&p_event_base, &p_error_base)==Success)
		{
	            /* Xv does normal scaling for us */
		    if(sysdep_display_params.effect==0)
		    {
                        if (hwscale_perfect_yuv)
                        {
                          image_width = 2*sysdep_display_params.aligned_width;
                          sysdep_display_params.widthscale = 2;
                        }
                        else
                        {
                          image_width = sysdep_display_params.aligned_width;
                          sysdep_display_params.widthscale = 1;
                        }
                        image_height = sysdep_display_params.height;
                        sysdep_display_params.yarbsize = sysdep_display_params.height;
                    }
#ifdef USE_XIL
                    use_xil = 0;
#endif
                    x11_window_update_method = X11_XVSHM;
		}
		else
		{
			fprintf (stderr, "X-Server Doesn't support Xv extension\n");
			use_xv = 0;
		}
	}
	else
	   use_xv = 0;
#endif

        /* get a visual */
	xvisual = screen->root_visual;
	fprintf(stderr, "Using a Visual with a depth of %dbpp.\n", screen->root_depth);

	/* create a window */
	if (run_in_root_window)
	{
		int width  = screen->width;
		int height = screen->height;
		if (window_width > width || window_height > height)
		{
			fprintf (stderr, "OSD ERROR: Root window is to small: %dx%d, needed %dx%d\n",
					width, height, window_width, window_height);
			return 1;
		}

		startx        = ((width  - window_width)  / 2) & ~0x07;
		starty        = ((height - window_height) / 2) & ~0x07;
		window        = RootWindowOfScreen (screen);
		window_width  = width;
		window_height = height;
	}
	else
	{
                /* create the actual window */
                if (x11_create_window(&window_width, &window_height, 0))
                        return 1;
	}

	/* create gc */
	gc = XCreateGC (display, window, 0, &xgcv);

	/* create and setup the image */
	switch (x11_window_update_method)
	{
#ifdef USE_XIL
		case X11_XIL:
			/*
			 *  XIL takes priority over MITSHM
			 */
			setup_xil_images (image_width, image_height);
			break;
#endif
#ifdef USE_XV
		case X11_XVSHM:
			/* Create an XV MITSHM image. */
                        fprintf (stderr, "MIT-SHM & XV Extensions Available. trying to use... ");
                        /* find a suitable format */
                        if(!hwscale_yuv)
                        {
                                if(!(FindRGBXvFormat(display, &xv_port,&hwscale_format,&hwscale_bpp)))
                                {
                                        hwscale_yuv=1;
                                        fprintf(stderr,"\nCan't find a suitable RGB format - trying YUY2 instead... ");
                                }
                        }
                        if(hwscale_yuv && !hwscale_yv12)
                        {
                                hwscale_format=FOURCC_YUY2;
                                if(!(FindXvPort(display, hwscale_format, &xv_port)))
                                {
                                        fprintf(stderr,"\nYUY2 not available - trying YV12... ");
                                        hwscale_yv12=1;
                                }
                        }
                        if(hwscale_yv12)
                        {
                                hwscale_format=FOURCC_YV12;
                                if(!(FindXvPort(display, hwscale_format, &xv_port)))
                                {
                                        fprintf(stderr,"\nError: Couldn't initialise Xv port - ");
                                        fprintf(stderr,"\n  Either all ports are in use, or the video card");
                                        fprintf(stderr,"\n  doesn't provide a suitable format.\n");
                                        use_xv = 0;
                                }
                                else
                                {
                                        /* YV12 always does normal scaling, no effects! */
                                        if (sysdep_display_params.effect)
                                          fprintf(stderr, "\nWarning: YV12 doesn't do effects... ");
                                        /* setup the image size and scaling params for YV12:
                                           -always make yarbsize the normal height, since
                                            although perfect blitting does 2x heightscaling,
                                            the YV12 blit code wants unscaled Y-dest_bounds
                                           -align image_width and x-coordinates to 8, I don't know why,
                                            this is needed, but it is.
                                           -align height and y-coodinates to 2 when not using perfect
                                            blit */
                                        if (hwscale_perfect_yuv)
                                        {
                                          image_width  = 2*sysdep_display_params.aligned_width;
                                          image_height = 2*sysdep_display_params.height;
                                          sysdep_display_params.widthscale = 2;
                                          sysdep_display_params.yarbsize   = 2*sysdep_display_params.height;
                                        }
                                        else 
                                        {
                                          image_width  = (sysdep_display_params.width+7)&~7;
                                          image_height = (sysdep_display_params.height+1)&~1;
                                          sysdep_display_params.widthscale = 1;
                                          sysdep_display_params.yarbsize   = sysdep_display_params.height;
                                          sysdep_display_params.x_align    = 7;
                                        }
                                }
                        }
                        /* if use_xv is still set we've found a suitable format */
                        if (use_xv)
                        {
                                int count;
                                XvAttribute *attr;

                                attr = XvQueryPortAttributes(display, xv_port, &count);
                                for (i = 0; i < count; i++)
                                    if (!strcmp(attr[i].name, "XV_AUTOPAINT_COLORKEY"))
                                    {
                                                Atom atom = XInternAtom(display, "XV_AUTOPAINT_COLORKEY", False);
                                                XvSetPortAttribute(display, xv_port, atom, 1);
                                                break;
                                    }
                                
                                XSetErrorHandler (test_mit_shm);
                                xvimage = XvShmCreateImage (display,
                                        xv_port,
                                        hwscale_format,
                                        0,
                                        image_width,
                                        image_height,
                                        &shm_info);
                        }
                        /* sometimes xv gives us a smaller image then we want ! */
                        if (xvimage)
                        {
                            if ((xvimage->width  < image_width) ||
                                (xvimage->height < image_height))
                            {
                                fprintf (stderr, "\nError: XVimage is smaller then the requested size. ");
                                XFree(xvimage);
                                xvimage = NULL;
                            }
                        }
                        if (xvimage)
                        {
                        	/* this might differ from what we requested! */
                        	image_width = xvimage->width;
                        	
                                shm_info.shmid = shmget (IPC_PRIVATE,
                                                xvimage->data_size,
                                                (IPC_CREAT | 0777));
                                if (shm_info.shmid < 0)
                                {
                                        fprintf (stderr, "\nError: failed to create MITSHM block.\n");
                                        return 1;
                                }

                                /* And allocate the bitmap buffer. */
                                /* new pen color code force double buffering in every cases */
                                xvimage->data = shm_info.shmaddr =
                                        (char *) shmat (shm_info.shmid, 0, 0);

                                scaled_buffer_ptr = (unsigned char *) xvimage->data;
                                if (!scaled_buffer_ptr)
                                {
                                        fprintf (stderr, "\nError: failed to allocate MITSHM bitmap buffer.\n");
                                        return 1;
                                }

                                shm_info.readOnly = False;

                                /* Attach the MITSHM block. this will cause an exception if */
                                /* MIT-SHM is not available. so trap it and process         */
                                if (!XShmAttach (display, &shm_info))
                                {
                                        fprintf (stderr, "\nError: failed to attach MITSHM block.\n");
                                        return 1;
                                }
                                XSync (display, False);  /* be sure to get request processed */
                                /* sleep (2);          enought time to notify error if any */
                                /* Mark segment as deletable after we attach.  When all processes
                                   detach from the segment (progam exits), it will be deleted.
                                   This way it won't be left in memory if we crash or something.
                                   Grr, have todo this after X attaches too since slowlaris doesn't
                                   like it otherwise */
                                shmctl(shm_info.shmid, IPC_RMID, NULL);

                                /* HACK, GRRR sometimes this all succeeds, but the first call to
                                   XvShmPutImage to a mapped window fails with:
                                   "BadAlloc (insufficient resources for operation)" */
                                if (use_mit_shm)
                                {
                                  if (hwscale_yuv)
                                  {
                                    if (hwscale_yv12)
                                      ClearYV12();
                                    else
                                      ClearYUY2();
                                  }
                                    
                                  XvShmPutImage (display, xv_port, window, gc, xvimage,
                                    0, 0, xvimage->width, xvimage->height,
                                    0, 0, window_width, window_height, True);
                                    
                                  XSync (display, False);  /* be sure to get request processed */
                                  /* sleep (1);          enought time to notify error if any */
                                }
                                
                                /* if use_mit_shm is still set we've succeeded */
                                if (use_mit_shm)
                                {
                                        fprintf (stderr, "Success.\nUsing Xv & Shared Memory Features to speed up\n");
                                        XSetErrorHandler (None);  /* Restore error handler to default */
                                        mit_shm_attached = 1;
                                        use_hwscale = 1;
                                        break;
                                }
                                /* else we have failed clean up before retrying without XV */
                                shmdt ((char *) scaled_buffer_ptr);
                                scaled_buffer_ptr = NULL;
                                XFree(xvimage);
                                xvimage = NULL;
                        }
		        XSetErrorHandler (None);  /* Restore error handler to default */
                        sysdep_display_params.widthscale  = orig_widthscale;
                        sysdep_display_params.yarbsize    = orig_yarbsize;
                        sysdep_display_params.x_align     = 3;
                        image_width  = sysdep_display_params.widthscale  * sysdep_display_params.aligned_width;
                        image_height = sysdep_display_params.yarbsize;
			use_xv = 0;
			use_mit_shm = 1;
		        fprintf (stderr, "Failed\nReverting to MIT-SHM mode\n");
			x11_window_update_method = X11_MITSHM;
#endif
#ifdef USE_MITSHM
		case X11_MITSHM:
			/* Create a MITSHM image. */
			fprintf (stderr, "MIT-SHM Extension Available. trying to use... ");
			XSetErrorHandler (test_mit_shm);

			image = XShmCreateImage (display,
					xvisual,
					screen->root_depth,
					ZPixmap,
					NULL,
					&shm_info,
					image_width,
					image_height);
			if (image)
			{
				shm_info.shmid = shmget (IPC_PRIVATE,
						image->bytes_per_line * image->height,
						(IPC_CREAT | 0777));
				if (shm_info.shmid < 0)
				{
					fprintf (stderr, "\nError: failed to create MITSHM block.\n");
					return 1;
				}

				/* And allocate the bitmap buffer. */
				/* new pen color code force double buffering in every cases */
				image->data = shm_info.shmaddr =
					(char *) shmat (shm_info.shmid, 0, 0);

				scaled_buffer_ptr = (unsigned char *) image->data;
				if (!scaled_buffer_ptr)
				{
					fprintf (stderr, "\nError: failed to allocate MITSHM bitmap buffer.\n");
					return 1;
				}

				shm_info.readOnly = False;

				/* Attach the MITSHM block. this will cause an exception if */
				/* MIT-SHM is not available. so trap it and process         */
				if (!XShmAttach (display, &shm_info))
				{
					fprintf (stderr, "\nError: failed to attach MITSHM block.\n");
					return 1;
				}
				XSync (display, False);  /* be sure to get request processed */
				/* sleep (2);          enought time to notify error if any */
				/* Mark segment as deletable after we attach.  When all processes
				   detach from the segment (progam exits), it will be deleted.
				   This way it won't be left in memory if we crash or something.
				   Grr, have todo this after X attaches too since slowlaris doesn't
				   like it otherwise */
				shmctl(shm_info.shmid, IPC_RMID, NULL);

				/* if use_mit_shm is still set we've succeeded */
				if (use_mit_shm)
				{
					fprintf (stderr, "Success.\nUsing Shared Memory Features to speed up\n");
					XSetErrorHandler (None);  /* Restore error handler to default */
					mit_shm_attached = 1;
					break;
				}
				/* else we have failed clean up before retrying without MITSHM */
				shmdt ((char *) scaled_buffer_ptr);
				scaled_buffer_ptr = NULL;
				XDestroyImage (image);
				image = NULL;
			}
			XSetErrorHandler (None);  /* Restore error handler to default */
			use_mit_shm = 0;
			fprintf (stderr, "Failed\nReverting to normal XPutImage() mode\n");
			x11_window_update_method = X11_NORMAL;
#endif
		case X11_NORMAL:
			scaled_buffer_ptr = malloc (4 * image_width * image_height);
			if (!scaled_buffer_ptr)
			{
				fprintf (stderr, "Error: failed to allocate bitmap buffer.\n");
				return 1;
			}
			image = XCreateImage (display,
					xvisual,
					screen->root_depth,
					ZPixmap,
					0,
					(char *) scaled_buffer_ptr,
					image_width, image_height,
					32, /* image_width always is a multiple of 8 */
					0);

			if (!image)
			{
				fprintf (stderr, "OSD ERROR: could not create image.\n");
				return 1;
			}
			break;
		default:
			fprintf (stderr, "Error unknown X11 update method, this shouldn't happen\n");
			return 1;
	}

	/* Now we know if we have XIL/ Xv / MIT-SHM / Normal:
	   change the hints and size if needed */
	if (!run_in_root_window)
	{
#ifdef USE_XIL
		/*
		 *  XIL allows us to rescale the window on the fly,
		 *  so in this case, we don't prevent the user from
		 *  resizing.
		 */
		if (use_xil)
		{
			event_mask |= StructureNotifyMask;
			x11_set_window_hints(window_width, window_height, 1);
		}
#endif
#ifdef USE_HWSCALE
		if(use_hwscale)
		{
			if(!sysdep_display_params.fullscreen)
			{
			        /* set window hints to resizable */
			        x11_set_window_hints(window_width, window_height, 2);
			        /* determine window size and resize */
				if (custom_window_width)
				  mode_clip_aspect(custom_window_width, custom_window_height, &window_width, &window_height);
				else
                                  mode_stretch_aspect(window_width, window_height, &window_width, &window_height);
                                XResizeWindow(display, window, window_width, window_height);
                                /* set window hints to resizable, keep aspect */
			        x11_set_window_hints(window_width, window_height, 1);
                        }
                        else    
                        {
				/* we need to recreate the window,
				   some window managers don't
				   allow switching to fullscreen after
				   the window has been mapped */
				XDestroyWindow(display,window);
				if (x11_create_window(&window_width, &window_height, 3))
				       return 1;
			}
		}
#endif
	}
	
	/* verify the number of bits per pixel and choose the correct update method */
#ifdef USE_HWSCALE
	if (use_hwscale)
	{
		dest_bpp = hwscale_bpp;
		hwscale_visual.class = TrueColor;
		xvisual = &hwscale_visual;
        }
        else
#endif
#ifdef USE_XIL
	if (use_xil)
		/* XIL uses 16 bit visuals and does any conversion it self */
		dest_bpp = 16;
        else
#endif
	        dest_bpp = image->bits_per_pixel;

	/* setup the palette_info struct now we have the visual */
	if (x11_init_palette_info(xvisual) != 0)
		return 1;
	
	/* get a blit function */
	fprintf(stderr, "Bits per pixel = %d... ", dest_bpp);
#ifdef USE_HWSCALE
	if(use_hwscale)
	{
	  sysdep_display_properties.hwscale = 1;
	  if(hwscale_yuv)
	    sysdep_display_properties.palette_info.fourcc_format = hwscale_format;
	}
	if(use_hwscale && hwscale_yv12)
	{
	    if (sysdep_display_params.orientation)
            {
                    hwscale_yv12_rotate_buf0=malloc(
                      ((sysdep_display_params.depth+1)/8)*
                      sysdep_display_params.width);
                    hwscale_yv12_rotate_buf1=malloc(
                      ((sysdep_display_params.depth+1)/8)*
                      sysdep_display_params.width);
                    if (!hwscale_yv12_rotate_buf0 ||
                        !hwscale_yv12_rotate_buf1)
                    {
                      fprintf (stderr, "\nError: failed to allocate rotate buffer.\n");
                      return 1;
                    }
            }
            if (sysdep_display_params.depth == 32)
            {
    		if (hwscale_perfect_yuv)
    			x11_window_update_display_func
    				= x11_window_update_32_to_YV12_direct_perfect;
    		else
    			x11_window_update_display_func
    				= x11_window_update_32_to_YV12_direct;
    	    }
    	    else
    	    {
    		if (hwscale_perfect_yuv)
    			x11_window_update_display_func
    				= x11_window_update_16_to_YV12_perfect;
    		else
    			x11_window_update_display_func
    				= x11_window_update_16_to_YV12;
    	    }
	}
	else
#endif
	x11_window_update_display_func = sysdep_display_get_blitfunc(dest_bpp);

	if (x11_window_update_display_func == NULL)
	{
		fprintf(stderr, "\nError: bitmap depth %d isnot supported on %dbpp displays\n", sysdep_display_params.depth, dest_bpp);
		return 1;
	}
	fprintf(stderr, "Ok\n");

	if (effect_open())
		return 1;

#ifdef USE_HWSCALE
	xinput_open((use_hwscale && sysdep_display_params.fullscreen)? 1:0, event_mask);
#else
	xinput_open(0, event_mask);
#endif

	return 0;
}

/*
 * Shut down the display, also called by the core to clean up if any error
 * happens when creating the display.
 */
void x11_window_close_display (void)
{
   /* Restore error handler to default */
   XSetErrorHandler (None);

   /* free effect buffers */
   effect_close();

   /* ungrab keyb and mouse */
   xinput_close();

   /* now just free everything else */
   if (window)
   {
      XDestroyWindow (display, window);
      window = 0;
   }
#ifdef USE_MITSHM
   if (mit_shm_attached)
   {
      XShmDetach (display, &shm_info);
      mit_shm_attached = 0;
   }
   if (use_mit_shm && scaled_buffer_ptr)
   {
      shmdt (scaled_buffer_ptr);
      scaled_buffer_ptr = NULL;
   }
#endif
   if (image)
   {
       XDestroyImage (image);
       scaled_buffer_ptr = NULL;
       image = NULL;
   }
#ifdef USE_XV
   if(xv_port>-1)
   {
      XvUngrabPort(display,xv_port,CurrentTime);
      xv_port=-1;
   }
   if(xvimage)
   {
       XFree(xvimage);
       scaled_buffer_ptr = NULL;
       xvimage=NULL;
   }
#endif
   if (scaled_buffer_ptr)
   {
      free (scaled_buffer_ptr);
      scaled_buffer_ptr = NULL;
   }
#ifdef USE_HWSCALE
   if (hwscale_yv12_rotate_buf0)
   {
      free (hwscale_yv12_rotate_buf0);
      hwscale_yv12_rotate_buf0 = NULL;
   }

   if (hwscale_yv12_rotate_buf1)
   {
      free (hwscale_yv12_rotate_buf1);
      hwscale_yv12_rotate_buf1 = NULL;
   }
#endif

   XSync (display, True); /* send all events to sync; discard events */
}

/* invoked by main tree code to update bitmap into screen */
void x11_window_update_display(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags)
{
   x11_window_update_display_func(bitmap, vis_in_dest_out, dirty_area,
     palette, scaled_buffer_ptr, image_width);
   
   switch (x11_window_update_method)
   {
      case X11_XIL:
#ifdef USE_XIL
         refresh_xil_screen ();
#endif
         break;
      case X11_MITSHM:
#ifdef USE_MITSHM
         XShmPutImage (display, window, gc, image, vis_in_dest_out->min_x, vis_in_dest_out->min_y,
            startx+vis_in_dest_out->min_x, starty+vis_in_dest_out->min_y, ((vis_in_dest_out->max_x + 1) - vis_in_dest_out->min_x) , ((vis_in_dest_out->max_y + 1) - vis_in_dest_out->min_y),
            False);
#endif
         break;
      case X11_XVSHM:
#ifdef USE_XV
         {
            Window _dw;
            int _dint;
	    unsigned int _duint;
            int pw,ph;

            XGetGeometry(display, window, &_dw, &_dint, &_dint, &window_width, &window_height, &_duint, &_duint);
            if (sysdep_display_params.fullscreen)
            {
              mode_clip_aspect(window_width, window_height, &pw, &ph);
            }
            else
            {
              pw = window_width;
              ph = window_height;
            }
            XvShmPutImage (display, xv_port, window, gc, xvimage, 0, 0,
              sysdep_display_params.width*sysdep_display_params.widthscale,
              sysdep_display_params.yarbsize,
              (window_width-pw)/2, (window_height-ph)/2, pw, ph, True);
         }
#endif
         break;
      case X11_NORMAL:
         XPutImage (display, window, gc, image, vis_in_dest_out->min_x, vis_in_dest_out->min_y,
            startx+vis_in_dest_out->min_x, starty+vis_in_dest_out->min_y, ((vis_in_dest_out->max_x + 1) - vis_in_dest_out->min_x) , ((vis_in_dest_out->max_y + 1) - vis_in_dest_out->min_y));
         break;
   }

   /* some games "flickers" with XFlush, so command line option is provided */
   if (use_xsync)
      XSync (display, False);   /* be sure to get request processed */
   else
      XFlush (display);         /* flush buffer to server */
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0x00ff00
#define BMASK 0x0000ff

/* Hacked into place, until I integrate YV12 support into the blit core... */
static void x11_window_update_16_to_YV12(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
   int _x,_y;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned short *src; 
   unsigned short *src2;
   int u1,v1,y1,u2,v2,y2,u3,v3,y3,u4,v4,y4;     /* 12 */
   
   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area);
   
   _y = vis_in_dest_out->min_y;
   vis_in_dest_out->min_y &= ~1;
   dirty_area->min_y -= _y - vis_in_dest_out->min_y;

   for(_y=dirty_area->min_y;_y<dirty_area->max_y;_y+=2)
   {
      if (sysdep_display_params.orientation)
      {
         rotate_func(hwscale_yv12_rotate_buf0, bitmap, _y, dirty_area);
         rotate_func(hwscale_yv12_rotate_buf1, bitmap, _y+1, dirty_area);
	 src  = (unsigned short*)hwscale_yv12_rotate_buf0;
	 src2 = (unsigned short*)hwscale_yv12_rotate_buf1;
      }
      else
      {
         src=bitmap->line[_y] ;
         src+= dirty_area->min_x;
         src2=bitmap->line[_y+1];
         src2+= dirty_area->min_x;
      }

      dest_y = (xvimage->data+xvimage->offsets[0]) +  xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y)    + vis_in_dest_out->min_x;
      dest_u = (xvimage->data+xvimage->offsets[2]) + (xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y))/4 + vis_in_dest_out->min_x/2;
      dest_v = (xvimage->data+xvimage->offsets[1]) + (xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y))/4 + vis_in_dest_out->min_x/2;

      for(_x=dirty_area->min_x;_x<dirty_area->max_x;_x+=2)
      {
            v1 = palette->lookup[*src++];
            y1 = (v1)  & 0xff;
            u1 = (v1>>8) & 0xff;
            v1 = (v1>>24)     & 0xff;

            v2 = palette->lookup[*src++];
            y2 = (v2)  & 0xff;
            u2 = (v2>>8) & 0xff;
            v2 = (v2>>24)     & 0xff;

            v3 = palette->lookup[*src2++];
            y3 = (v3)  & 0xff;
            u3 = (v3>>8) & 0xff;
            v3 = (v3>>24)     & 0xff;

            v4 = palette->lookup[*src2++];
            y4 = (v4)  & 0xff;
            u4 = (v4>>8) & 0xff;
            v4 = (v4>>24)     & 0xff;

         *dest_y = y1;
         *(dest_y++ + xvimage->width) = y3;
         *dest_y = y2;
         *(dest_y++ + xvimage->width) = y4;

         *dest_u++ = (u1+u2+u3+u4)/4;
         *dest_v++ = (v1+v2+v3+v4)/4;

         /* I thought that the following would be better, but it is not
          * the case. The color gets blurred
         if (y || y2 || y3 || y4) {
                 *dest_u++ = (u*y+u2*y2+u3*y3+u4*y4)/(y+y2+y3+y4);
                 *dest_v++ = (v*y+v2*y2+v3*y3+v4*y4)/(y+y2+y3+y4);
         } else {
                 *dest_u++ =128;
                 *dest_v++ =128;
         }
         */
      }
   }
}


static void x11_window_update_16_to_YV12_perfect(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{      /* this one is used when scale==2 */
   unsigned int _x,_y;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned short *src;
   int u1,v1,y1;

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area);

   for(_y=dirty_area->min_y;_y<=dirty_area->max_y;_y++)
   {
      if (sysdep_display_params.orientation)
      {
         rotate_func(hwscale_yv12_rotate_buf0, bitmap, _y, dirty_area);
         src = (unsigned short*)hwscale_yv12_rotate_buf0;
      }
      else
      {
         src=bitmap->line[_y] ;
         src+= dirty_area->min_x;
      }

      dest_y=(xvimage->data+xvimage->offsets[0])+2*xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x;
      dest_u=(xvimage->data+xvimage->offsets[2])+ (xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x)/2;
      dest_v=(xvimage->data+xvimage->offsets[1])+ (xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x)/2;
      for(_x=dirty_area->min_x;_x<=dirty_area->max_x;_x++)
      {
            v1 = palette->lookup[*src++];
            y1 = (v1)  & 0xff;
            u1 = (v1>>8) & 0xff;
            v1 = (v1>>24)     & 0xff;

         *(dest_y+xvimage->width)=y1;
         *dest_y++=y1;
         *(dest_y+xvimage->width)=y1;
         *dest_y++=y1;
         *dest_u++ = u1;
         *dest_v++ = v1;
      }
   }
}

static void x11_window_update_32_to_YV12_direct(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
   int _x,_y,r,g,b;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned int *src;
   unsigned int *src2;
   int u1,v1,y1,u2,v2,y2,u3,v3,y3,u4,v4,y4;     /* 12  34 */

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area);

   _y = vis_in_dest_out->min_y;
   vis_in_dest_out->min_y &= ~1;
   dirty_area->min_y -= _y - vis_in_dest_out->min_y;

   for(_y=dirty_area->min_y;_y<dirty_area->max_y;_y+=2)
   {
      if (sysdep_display_params.orientation)
      {
         rotate_func(hwscale_yv12_rotate_buf0, bitmap, _y, dirty_area);
         rotate_func(hwscale_yv12_rotate_buf1, bitmap, _y+1, dirty_area);
         src  = (unsigned int*)hwscale_yv12_rotate_buf0;
         src2 = (unsigned int*)hwscale_yv12_rotate_buf1;
      }
      else
      {
         src=bitmap->line[_y] ;
         src+= dirty_area->min_x;
         src2=bitmap->line[_y+1];
         src2+= dirty_area->min_x;
      }

      dest_y = (xvimage->data+xvimage->offsets[0]) +  xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y)    + vis_in_dest_out->min_x;
      dest_u = (xvimage->data+xvimage->offsets[2]) + (xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y))/4 + vis_in_dest_out->min_x/2;
      dest_v = (xvimage->data+xvimage->offsets[1]) + (xvimage->width*((_y-dirty_area->min_y)+vis_in_dest_out->min_y))/4 + vis_in_dest_out->min_x/2;

      for(_x=dirty_area->min_x;_x<dirty_area->max_x;_x+=2)
      {
         b = *src++;
         r = (b>>16) & 0xFF;
         g = (b>>8)  & 0xFF;
         b = (b)     & 0xFF;
         RGB2YUV(r,g,b,y1,u1,v1);

         b = *src++;
         r = (b>>16) & 0xFF;
         g = (b>>8)  & 0xFF;
         b = (b)     & 0xFF;
         RGB2YUV(r,g,b,y2,u2,v2);

         b = *src2++;
         r = (b>>16) & 0xFF;
         g = (b>>8)  & 0xFF;
         b = (b)     & 0xFF;
         RGB2YUV(r,g,b,y3,u3,v3);

         b = *src2++;
         r = (b>>16) & 0xFF;
         g = (b>>8)  & 0xFF;
         b = (b)     & 0xFF;
         RGB2YUV(r,g,b,y4,u4,v4);

         *dest_y = y1;
         *(dest_y++ + xvimage->width) = y3;
         *dest_y = y2;
         *(dest_y++ + xvimage->width) = y4;

         r&=RMASK;  r>>=16;
         g&=GMASK;  g>>=8;
         b&=BMASK;  b>>=0;
         *dest_u++ = (u1+u2+u3+u4)/4;
         *dest_v++ = (v1+v2+v3+v4)/4;
      }
   }
}

static void x11_window_update_32_to_YV12_direct_perfect(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{ /* This one is used when scale == 2 */
   int _x,_y,r,g,b;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned int *src;
   int u1,v1,y1;

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area);

   for(_y=dirty_area->min_y;_y<=dirty_area->max_y;_y++)
   {
      if (sysdep_display_params.orientation)
      {
         rotate_func(hwscale_yv12_rotate_buf0, bitmap, _y, dirty_area);
         src  = (unsigned int*)hwscale_yv12_rotate_buf0;
      }
      else
      {
         src=bitmap->line[_y] ;
         src+= dirty_area->min_x;
      }

      dest_y=(xvimage->data+xvimage->offsets[0])+2*xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x;
      dest_u=(xvimage->data+xvimage->offsets[2])+ (xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x)/2;
      dest_v=(xvimage->data+xvimage->offsets[1])+ (xvimage->width*((_y-dirty_area->min_y)+(vis_in_dest_out->min_y/2))+vis_in_dest_out->min_x)/2;
      for(_x=dirty_area->min_x;_x<=dirty_area->max_x;_x++)
      {
         b = *src++;
         r = (b>>16) & 0xFF;
         g = (b>>8)  & 0xFF;
         b = (b)     & 0xFF;
         RGB2YUV(r,g,b,y1,u1,v1);

         *(dest_y+xvimage->width) = y1;
         *dest_y++ = y1;
         *(dest_y+xvimage->width) = y1;
         *dest_y++ = y1;
         *dest_u++ = u1;
         *dest_v++ = v1;
      }
   }
}

#endif

#endif /* ifdef x11 */
