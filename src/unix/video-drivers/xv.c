#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if defined(__DECC) && defined(VMS)
#  include <X11/extensions/ipc.h>
#  include <X11/extensions/shm.h>
#else
#  include <sys/ipc.h>
#  include <sys/shm.h>
#endif
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include "pixel_convert.h"
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* we need MITSHM! */
#ifndef USE_MITSHM
#error "USE_XV defined but USE_MITSHM is not defined, XV needs MITSHM!"
#endif

static void xv_update_16_to_YV12 (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void xv_update_16_to_YV12_perfect (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void xv_update_32_to_YV12_direct (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);
static void xv_update_32_to_YV12_direct_perfect (struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);

static blit_func_p xv_update_display_func;
static int hwscale_force_yuv=0;
static int hwscale_perfect_yuv=1;
static char *hwscale_yv12_rotate_buf0=NULL;
static char *hwscale_yv12_rotate_buf1=NULL;
static XvImage *xvimage = NULL;
static int xv_port=-1;
static GC gc;
static int mit_shm_attached = 0;
static XShmSegmentInfo shm_info;

struct rc_option xv_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "XV Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL,  NULL },
	{ "force-yuv", NULL, rc_int, &hwscale_force_yuv, "0", 0, 2, NULL, "Force XV YUV mode:\n0 Autodetect\n1 Force YUY2\n2 Force YV12" },
	{ "perfect-yuv", NULL, rc_bool, &hwscale_perfect_yuv, "1", 0, 0, NULL, "Use perfect (slower) blitting code for XV YUV blits" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

static int FindXvPort(long format)
{
	int i,j,p,ret,num_formats;
	unsigned int num_adaptors;
	XvAdaptorInfo *ai;
	XvImageFormatValues *fo;

	ret = XvQueryAdaptors(display, DefaultRootWindow(display),
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
			fo = XvListImageFormats(display, p, &num_formats);
			for (j = 0; j < num_formats; j++)
			{
				if((fo[j].id==format))
				{
					if(XvGrabPort(display,p,CurrentTime)==Success)
					{
						xv_port=p;
						sysdep_display_properties.palette_info.fourcc_format=format;
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

static int FindRGBXvFormat(int *bpp)
{
	int i,j,p,ret,num_formats;
	unsigned int num_adaptors;
	XvAdaptorInfo *ai;
	XvImageFormatValues *fo;

	ret = XvQueryAdaptors(display, DefaultRootWindow(display),
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
			fo = XvListImageFormats(display, p, &num_formats);
			for (j = 0; j < num_formats; j++)
			{
				if((fo[j].type==XvRGB) && (fo[j].format==XvPacked))
				{
					if(XvGrabPort(display,p,CurrentTime)==Success)
					{
						xv_port=p;
						*bpp=fo[j].bits_per_pixel;
						sysdep_display_properties.palette_info.fourcc_format=fo[j].id;
						sysdep_display_properties.palette_info.red_mask  =fo[j].red_mask;
						sysdep_display_properties.palette_info.green_mask=fo[j].green_mask;
						sysdep_display_properties.palette_info.blue_mask =fo[j].blue_mask;
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

/* Since with YUV formats a field of zeros is generally
   loud green, rather than black, it makes sense
   to clear the image before use (since scanline algorithms
   leave alternate lines "black") */
static void ClearYUY2()
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

static void ClearYV12()
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

int xv_init(void)
{
  int i;
  unsigned int u;
  
  shm_info.shmaddr = NULL;
  
  if(XQueryExtension (display, "MIT-SHM", &i, &i, &i) &&
     (XvQueryExtension(display, &u, &u, &u, &u, &u)==Success))
    return 0;

  fprintf (stderr, "X-Server Doesn't support MIT-SHM or Xv extension.\n"
    "Disabling use of Xv mode.\n");
  return 1;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xv_open_display(void)
{
	XGCValues xgcv;
        XvAttribute *attr;
	unsigned int height, width;
        int i, count;
        int dest_bpp=0;

        /* set the aspect_ratio, do this here since
           this can change yarbsize */
        mode_set_aspect_ratio((double)screen->width/screen->height);

	/* create a window */
	if (x11_create_resizable_window(&window_width, &window_height))
          return 1;

        /* Xv does normal scaling for us */
        if(sysdep_display_params.effect==0)
        {
            width  = sysdep_display_params.max_width;
            height = sysdep_display_params.max_height;
            sysdep_display_params.widthscale  = 1;
            sysdep_display_params.heightscale = 1;
            sysdep_display_params.yarbsize    = 0;
        }
        else
        {
          width      = sysdep_display_params.max_width *
            sysdep_display_params.widthscale;
          /* effects don't do yarbsize */
          height     = sysdep_display_params.max_height*
            sysdep_display_params.heightscale;
        }

	/* create gc */
	gc = XCreateGC (display, window, 0, &xgcv);

	/* Initial settings of the sysdep_display_properties struct,
           the FindXvXXX fucntions will fill in the rest */
	memset(&sysdep_display_properties, 0, sizeof(sysdep_display_properties));
        sysdep_display_properties.hwscale = 1;

        fprintf (stderr, "MIT-SHM & XV Extensions Available. trying to use... ");
        /* find a suitable format */
        sysdep_display_properties.palette_info.fourcc_format = -1;
        switch(hwscale_force_yuv)
        {
          case 0: /* try normal RGB */
            if(FindRGBXvFormat(&dest_bpp))
              break;
            fprintf(stderr,"\nCan't find a suitable RGB format - trying YUY2 instead... ");
          case 1: /* force YUY2 */
            if(FindXvPort(FOURCC_YUY2))
            {
              /* Xv does normal scaling for us */
              if (!sysdep_display_params.effect && hwscale_perfect_yuv)
              {
                width = 2*sysdep_display_params.max_width;
                sysdep_display_params.widthscale = 2;
              }
              break;
            }
            fprintf(stderr,"\nYUY2 not available - trying YV12... ");
          case 2: /* forced YV12 */
          case 3:
            if(FindXvPort(FOURCC_YV12))
            {
              /* YV12 always does normal scaling, no effects! */
              if (sysdep_display_params.effect)
                fprintf(stderr, "\nWarning: YV12 doesn't do effects... ");
              /* setup the image size and scaling params for YV12:
                 -align width and x-coordinates to 8, I don't know
                  why, this is needed, but it is.
                 -align height and y-coodinates to 2.
                 Note these alignment demands are always met for
                 perfect blit. */
              if (hwscale_perfect_yuv)
              {
                width  = 2*sysdep_display_params.max_width;
                height = 2*sysdep_display_params.max_height;
                sysdep_display_params.widthscale  = 2;
                sysdep_display_params.heightscale = 2;
              }
              else
              {
                width  = (sysdep_display_params.max_width+7)&~7;
                height = (sysdep_display_params.max_height+1)&~1;
                sysdep_display_params.widthscale  = 1;
                sysdep_display_params.heightscale = 1;
              }
              break;
            }
            fprintf(stderr,"\nError: Couldn't initialise Xv port - ");
            fprintf(stderr,"\n  Either all ports are in use, or the video card");
            fprintf(stderr,"\n  doesn't provide a suitable format.\n");
            return 1;
        }

        attr = XvQueryPortAttributes(display, xv_port, &count);
        for (i = 0; i < count; i++)
        if (!strcmp(attr[i].name, "XV_AUTOPAINT_COLORKEY"))
            {
                        Atom atom = XInternAtom(display, "XV_AUTOPAINT_COLORKEY", False);
                        XvSetPortAttribute(display, xv_port, atom, 1);
                        break;
            }
        
        /* Create an XV MITSHM image. */
        x11_mit_shm_error = 0;
        XSetErrorHandler (x11_test_mit_shm);
        xvimage = XvShmCreateImage (display,
                xv_port,
                sysdep_display_properties.palette_info.fourcc_format,
                0,
                width,
                height,
                &shm_info);
        if (!xvimage)
        {
          fprintf(stderr, "\nError creating XvShmImage.\n");
          return 1;
        }

        /* sometimes xv gives us a smaller image then we want ! */
        if ((xvimage->width  < width) ||
            (xvimage->height < height))
        {
            fprintf (stderr, "\nError: XVimage is smaller then the requested size.\n");
            return 1;
        }

        shm_info.readOnly = False;
        shm_info.shmid = shmget (IPC_PRIVATE,
                        xvimage->data_size,
                        (IPC_CREAT | 0777));
        if (shm_info.shmid < 0)
        {
                fprintf (stderr, "\nError: failed to create MITSHM block.\n");
                return 1;
        }

        /* And allocate the bitmap buffer. */
        xvimage->data = shm_info.shmaddr =
                (char *) shmat (shm_info.shmid, 0, 0);
        if (!xvimage->data)
        {
                fprintf (stderr, "\nError: failed to allocate MITSHM bitmap buffer.\n");
                return 1;
        }

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

        if (x11_mit_shm_error)
        {
                fprintf (stderr, "\nError: failed to attach MITSHM block.\n");
                return 1;
        }

        mit_shm_attached = 1;
          
        /* HACK, GRRR sometimes this all succeeds, but the first call to
           XvShmPutImage to a mapped window fails with:
           "BadAlloc (insufficient resources for operation)" */
        switch(sysdep_display_properties.palette_info.fourcc_format)
        {
          case FOURCC_YUY2:
            ClearYUY2();
            break;
          case FOURCC_YV12:
            ClearYV12();
            break;
          default:
            sysdep_display_properties.palette_info.fourcc_format = 0;
        }
          
        mode_clip_aspect(window_width, window_height, &width, &height);
        XvShmPutImage (display, xv_port, window, gc, xvimage,
          0, 0, xvimage->width, xvimage->height,
          (window_width-width)/2, (window_height-height)/2, width, height,
          True);
          
        XSync (display, False);  /* be sure to get request processed */
        /* sleep (1);          enought time to notify error if any */
        
        if (x11_mit_shm_error)
        {
                fprintf(stderr, "XvShmPutImage failed, probably due to: \"BadAlloc (insufficient resources for operation)\"\n");
                return 1;
        }

        fprintf (stderr, "Success.\nUsing Xv & Shared Memory Features to speed up\n");
        XSetErrorHandler (None);  /* Restore error handler to default */

	/* get a blit function */
        if(sysdep_display_properties.palette_info.fourcc_format == FOURCC_YV12)
        {
          if (sysdep_display_params.orientation)
          {
                  hwscale_yv12_rotate_buf0=malloc(
                    ((sysdep_display_params.depth+7)/8)*
                    sysdep_display_params.width);
                  hwscale_yv12_rotate_buf1=malloc(
                    ((sysdep_display_params.depth+7)/8)*
                    sysdep_display_params.width);
                  if (!hwscale_yv12_rotate_buf0 ||
                      !hwscale_yv12_rotate_buf1)
                  {
                    fprintf (stderr, "Error: failed to allocate rotate buffer.\n");
                    return 1;
                  }
          }
          if (sysdep_display_params.depth == 32)
          {
              if (hwscale_perfect_yuv)
                      xv_update_display_func
                              = xv_update_32_to_YV12_direct_perfect;
              else
                      xv_update_display_func
                              = xv_update_32_to_YV12_direct;
          }
          else
          {
              if (hwscale_perfect_yuv)
                      xv_update_display_func
                              = xv_update_16_to_YV12_perfect;
              else
                      xv_update_display_func
                              = xv_update_16_to_YV12;
          }
        }
        else
          xv_update_display_func = sysdep_display_get_blitfunc(dest_bpp);

	if (xv_update_display_func == NULL)
	{
		fprintf(stderr, "Error: bitmap depth %d isnot supported on %dbpp displays\n", sysdep_display_params.depth, dest_bpp);
		return 1;
	}

	xinput_open(sysdep_display_params.fullscreen, 0);

	return effect_open();
}

/*
 * Shut down the display, also called by the core to clean up if any error
 * happens when creating the display.
 */
void xv_close_display (void)
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
   if (mit_shm_attached)
   {
      XShmDetach (display, &shm_info);
      mit_shm_attached = 0;
   }
   if (shm_info.shmaddr)
   {
      shmdt (shm_info.shmaddr);
      shm_info.shmaddr = NULL;
   }
   if(xv_port>-1)
   {
      XvUngrabPort(display,xv_port,CurrentTime);
      xv_port=-1;
   }
   if(xvimage)
   {
       XFree(xvimage);
       xvimage=NULL;
   }
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

   XSync (display, True); /* send all events to sync; discard events */
}

int xv_resize_display(void)
{
  /* no op */
  return 0;
}

/* invoked by main tree code to update bitmap into screen */
void xv_update_display(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags,
  const char **status_msg)
{
  Window _dw;
  int _dint;
  unsigned int _duint;
  unsigned int pw,ph;
  struct rectangle vis_area = *vis_in_dest_out;

  sysdep_display_orient_bounds(&vis_area, bitmap->width, bitmap->height);

  xv_update_display_func(bitmap, vis_in_dest_out, dirty_area,
    palette, (unsigned char *)xvimage->data, xvimage->width);

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
    ((vis_area.max_x+1)-vis_area.min_x) *
     sysdep_display_params.widthscale,
    ((vis_area.max_y+1)-vis_area.min_y) *
     sysdep_display_params.heightscale,
    (window_width-pw)/2, (window_height-ph)/2, pw, ph, True);

  /* some games "flickers" with XFlush, so command line option is provided */
  if (use_xsync)
    XSync (display, False);   /* be sure to get request processed */
  else
    XFlush (display);         /* flush buffer to server */
}

#define RMASK 0xff0000
#define GMASK 0x00ff00
#define BMASK 0x0000ff

/* Hacked into place, until I integrate YV12 support into the blit core... */
static void xv_update_16_to_YV12(struct mame_bitmap *bitmap,
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
   
   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area, 7);
   
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


static void xv_update_16_to_YV12_perfect(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{      /* this one is used when scale==2 */
   unsigned int _x,_y;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned short *src;
   int u1,v1,y1;

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area, 7);

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

static void xv_update_32_to_YV12_direct(struct mame_bitmap *bitmap,
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

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area, 7);

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

static void xv_update_32_to_YV12_direct_perfect(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{ /* This one is used when scale == 2 */
   int _x,_y,r,g,b;
   char *dest_y;
   char *dest_u;
   char *dest_v;
   unsigned int *src;
   int u1,v1,y1;

   sysdep_display_check_bounds(bitmap, vis_in_dest_out, dirty_area, 7);

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
