/*
 *  Support for the XIL imaging library.
 *
 *  Elias Mårtenson (elias-m@algonet.se)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xil/xil.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

static int use_mt_xil;

struct rc_option xil_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "XIL Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL,  NULL },
	{ "mtxil", "mtx", rc_bool, &use_mt_xil, "0", 0, 0, NULL, "Enable/disable multi threading of XIL" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

static void *redraw_thread( void * );

static XilSystemState state;
static XilImage window_image = NULL, draw_image = NULL;
static int draw_image_width, draw_image_height;
static pthread_mutex_t img_mutex;
static pthread_cond_t img_cond;
static int paintflag;
static XilImage back_image = NULL;
static blit_func_p xil_update_display_func;
static unsigned char *scaled_buffer_ptr;

int xil_init( void )
{
  if( (state = xil_open()) == NULL ) {
    fprintf( stderr, "Failed to open XIL library, disabling\n" );
    return 0;
  }

  return SYSDEP_DISPLAY_WINDOWED|SYSDEP_DISPLAY_FULLSCREEN|
    SYSDEP_DISPLAY_HWSCALE|SYSDEP_DISPLAY_EFFECTS;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xil_open_display(void)
{
        XilMemoryStorage storage_info;
        pthread_t thread_id;

        /* set the aspect_ratio, do this here since
           this can change yarbsize */
        mode_set_aspect_ratio((double)screen->width/screen->height);

	/* create a window */
	if (x11_create_resizable_window(&window_width, &window_height))
          return 1;
        
	/* create and setup the images */
        window_image = xil_create_from_window( state, display, window );
        if( use_mt_xil ) {
          printf( "initializing scaling thread\n" );
          pthread_mutex_init( &img_mutex, NULL );
          paintflag = 0;
          pthread_create( &thread_id, NULL, redraw_thread, NULL );
        }
        xil_resize_display();

	/* setup the sysdep_display_properties struct */
	sysdep_display_properties.palette_info.fourcc_format = 0;
	sysdep_display_properties.palette_info.red_mask      = 0x0000F800;
	sysdep_display_properties.palette_info.green_mask    = 0x000007E0;
	sysdep_display_properties.palette_info.blue_mask     = 0x0000001F;
        sysdep_display_properties.palette_info.depth         = 16;
        sysdep_display_properties.palette_info.bpp           = 16;
	sysdep_display_properties.vector_renderer            = NULL;
	
	if (x11_init_palette_info(xvisual) != 0)
		return 1;
	
	/* get a blit function, XIL uses 16 bit visuals and does any conversion it self */
        xil_update_display_func = sysdep_display_get_blitfunc();
	if (x11_window_update_display_func == NULL)
	{
		fprintf(stderr, "Error: bitmap depth %d isnot supported on XIL displays\n", sysdep_display_params.depth);
		return 1;
	}

        /* init the input code */
	xinput_open(sysdep_display_params.fullscreen, 0);

	return 0;
}

/*
 * Shut down the display, also called by the core to clean up if any error
 * happens when creating the display.
 */
void xil_close_display (void)
{
   /* ungrab keyb and mouse */
   xinput_close();

   /* now just free everything else */
   if (window)
   {
     XDestroyWindow (display, window);
     window = 0;
   }
   if (window_image)
   {
     xil_destroy(window_image);
     window_image = NULL;
   }
   if (draw_image)
   {
     xil_destroy(draw_image);
     draw_image = NULL;
   }
   if (back_image)
   {
     xil_destroy(back_image);
     back_image = NULL;
   }

   XSync (display, True); /* send all events to sync; discard events */
}

int xil_resize_display(void)
{
         if (draw_image)
         {
           xil_destroy(draw_image);
           draw_image = NULL;
         }
         if (back_image)
         {
           xil_destroy(back_image);
           back_image = NULL;
         }
        
        /* xil does normal scaling for us */
        if (sysdep_display_params.effect == 0)
        {
		draw_image_width  = sysdep_display_params.width;
		draw_image_height = sysdep_display_params.height;
		sysdep_display_params.widthscale  = 1;
		sysdep_display_params.heightscale = 1;
                sysdep_display_params.yarbsize    = 0;
        }
        else
        {
		draw_image_width  = sysdep_display_params.width *
		  sysdep_display_params.widthscale;
                /* effects don't do yarbsize */
		draw_image_height = sysdep_display_params.height *
		  sysdep_display_params.heightscale;
        }

        draw_image   = xil_create( state, draw_image_width, draw_image_height,
          1, XIL_SHORT );
        xil_export( draw_image );
        xil_get_memory_storage( draw_image, &storage_info );
        scaled_buffer_ptr = (char *)storage_info.byte.data;

        if( use_mt_xil ) {
          pthread_mutex_lock( &img_mutex );
          while( paintflag ) {
            pthread_cond_wait( &img_cond, &img_mutex );
          }
          back_image = xil_create( state, draw_image_width, draw_image_height, 1, XIL_BYTE );
          pthread_mutex_unlock( &img_mutex );
        }
        return 0;
}

/* invoked by main tree code to update bitmap into screen */
void xil_update_display(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned int flags,
  const char **status_msg)
{
  Window _dw;
  int _dint;
  unsigned int _duint, w, h;
  XilMemoryStorage storage_info;

  x11_window_update_display_func(bitmap, vis_in_dest_out, dirty_area,
    palette, scaled_buffer_ptr, draw_image_width);

  xil_import( draw_image, TRUE );

  XGetGeometry(display, window, &_dw, &_dint, &_dint, &w, &h, &_duint, &_duint);
  if ((window_width != w) || (window_height != h))
  {
    window_width  = w;
    window_height = h;
    xil_destroy( window_image );
    window_image = xil_create_from_window( state, display, window );
  }  

  if( use_mt_xil ) {
    pthread_mutex_lock( &img_mutex );
    while( paintflag ) {
      pthread_cond_wait( &img_cond, &img_mutex );
    }
    xil_copy( draw_image, back_image );
    paintflag = 1;
    pthread_mutex_unlock( &img_mutex );
    pthread_cond_signal( &img_cond );
  }
  else {
    xil_scale( draw_image, window_image, "nearest",
	     window_width / (float)draw_image_width,
	     window_height / (float)draw_image_height );
  }

  xil_export( draw_image );
  xil_get_memory_storage( draw_image, &storage_info );
  scaled_buffer_ptr = (char *)storage_info.byte.data;

  /* some games "flickers" with XFlush, so command line option is provided */
  if (use_xsync)
    XSync (display, False);   /* be sure to get request processed */
  else
    XFlush (display);         /* flush buffer to server */
}

static void *redraw_thread( void *arg )
{
  for( ;; ) {
    pthread_mutex_lock( &img_mutex );
    while( !paintflag ) {
      pthread_cond_wait( &img_cond, &img_mutex );
    }
    xil_scale( back_image, window_image, "nearest",
	     window_width / (float)draw_image_width,
	     window_height / (float)draw_image_height );
    paintflag = 0;
    pthread_mutex_unlock( &img_mutex );
    pthread_cond_signal( &img_cond );
  }

  return NULL;
}
