/*
 * X-Mame generic video code
 *
 */
#define __VIDEO_C_
#include <math.h>
#include "xmame.h"

#ifdef xgl
	#include "video-drivers/glmame.h"
#endif
#include <stdio.h>
#include "driver.h"
#include "profiler.h"
#include "input.h"
#include "keyboard.h"
/* for uclock */
#include "sysdep/misc.h"
#include "effect.h"

#define FRAMESKIP_DRIVER_COUNT 2
static const int safety = 16;
static float beam_f;
int normal_widthscale = 1, normal_heightscale = 1;
int yarbsize = 0;
static char *vector_res = NULL;
static int use_auto_double = 1;
static int frameskipper = 0;
static int brightness = 100;
float brightness_paused_adjust = 1.0;
static int bitmap_depth;
static int rgb_direct;
static struct mame_bitmap *scrbitmap = NULL;
static int debugger_has_focus = 0;
static struct my_rectangle normal_visual;
static struct my_rectangle debug_visual;

#if (defined svgafx) || (defined xfx) 
UINT16 *color_values;
#endif


float gamma_correction = 1.0;

/* some prototypes */
static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority);
static void osd_free_colors(void);

struct rc_option video_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "bpp",		"b",			rc_int,		&options.color_depth,
     "0",		0,			0,		video_verify_bpp,
     "Specify the colordepth the core should render, one of: auto(0), 8, 16" },
   { "arbheight",	"ah",			rc_int,		&yarbsize,
     "0",		0,			4096,		NULL,
     "Scale video to exactly this height (0 = disable)" },
   { "heightscale",	"hs",			rc_int,		&normal_heightscale,
     "1",		1,			8,		NULL,
     "Set Y-Scale aspect ratio" },
   { "widthscale",	"ws",			rc_int,		&normal_widthscale,
     "1",		1,			8,		NULL,
     "Set X-Scale aspect ratio" },
   { "scale",		"s",			rc_use_function, NULL,
     NULL,		0,			0,		video_handle_scale,
     "Set X-Y Scale to the same aspect ratio. For vector games scale (and also width- and heightscale) may have value's like 1.5 and even 0.5. For scaling of regular games this will be rounded to an int" },
   { "effect",		"ef",			rc_int,		&effect,
     EFFECT_NONE,	EFFECT_NONE,		EFFECT_LAST,	NULL,
     "Video effect:\n"
	     "0 = none (default)\n"
	     "1 = scale2x (smooth scaling effect)\n"
	     "2 = scan2 (light scanlines)\n"
	     "3 = rgbstripe (3x2 rgb vertical stripes)\n"
	     "4 = rgbscan (2x3 rgb horizontal scanlines)\n"
	     "5 = scan3 (3x3 deluxe scanlines)\n" },
   { "autodouble",	"adb",			rc_bool,	&use_auto_double,
     "1",		0,			0,		NULL,
     "Enable/disable automatic scale doubling for 1:2 pixel aspect ratio games" },
   { "dirty",		"dt",			rc_bool,	&use_dirty,
     "1",		0,			0,		NULL,
     "Enable/disable use of dirty rectangles" },
   { "scanlines",	"sl",			rc_bool,	&use_scanlines,
     "0",		0,			0,		NULL,
     "Enable/disable displaying simulated scanlines" },
   { "artwork",		"a",			rc_bool,	&options.use_artwork,
     "1",		0,			0,		NULL,
     "Use/don't use artwork if available" },
   { "frameskipper",	"fsr",			rc_int,		&frameskipper,
     "0",		0,			FRAMESKIP_DRIVER_COUNT-1, NULL,
     "Select which autoframeskip and throttle routines to use. Available choices are:\n0 Dos frameskip code\n1 Enhanced frameskip code by William A. Barath" },
   { "throttle",	"th",			rc_bool,	&throttle,
     "1",		0,			0,		NULL,
     "Enable/disable throttle" },
   { "sleepidle",	"si",			rc_bool,	&sleep_idle,
     "0",		0,			0,		NULL,
     "Enable/disable sleep during idle" },
   { "autoframeskip",	"afs",			rc_bool,	&autoframeskip,
     "1",		0,			0,		NULL,
     "Enable/disable autoframeskip" },
   { "maxautoframeskip", "mafs",		rc_int,		&max_autoframeskip,
     "8",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set highest allowed frameskip for autoframeskip" },
   { "frameskip",	"fs",			rc_int,		&frameskip,
     "0",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set frameskip when not using autoframeskip" },
   { "brightness",	"brt",			rc_int,		&brightness,
     "100",		0,			100,		NULL,
     "Set the brightness (0-100%%)" },
   { "gamma-correction", "gc",			rc_float,	&gamma_correction,
     "1.0",		0.5,			2.0,		NULL,
     "Set the gamma-correction (0.5-2.0)" },
   { "norotate",	"nr",			rc_set_int,	&options.norotate,
     NULL,		1,			0,		NULL,
     "Disable rotation" },
   { "ror",		"rr",			rc_set_int,	&options.ror,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees rigth" },
   { "rol",		"rl",			rc_set_int,	&options.rol,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees left" },
   { "flipx",		"fx",			rc_set_int,	&options.flipx,
     NULL,		1,			0,		NULL,
     "Flip X axis" },
   { "flipy",		"fy",			rc_set_int,	&options.flipy,
     NULL,		1,			0,		NULL,
     "Flip Y axis" },
   { "Vector Games Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "vectorres",	"vres",			rc_string,	&vector_res,
     NULL,		0,			0,		video_verify_vectorres,
     "Always scale vectorgames to XresxYres, keeping their aspect ratio. This overrides the scale options" },
   { "beam",		"B",			rc_float,	&beam_f,
     "1.0",		1.0,			16.0,		video_verify_beam,
     "Set the beam size for vector games" },
   { "flicker",		"f",			rc_float,	&options.vector_flicker,
     "0.0",		0.0,			100.0,		video_verify_flicker,
     "Set the flicker for vector games" },
   { "antialias",	"aa",			rc_bool,	&options.antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "translucency",	"t",			rc_bool,	&options.translucency,
     "1",		0,			0,		NULL,
     "Enable/disable tranlucency" },
   { NULL,		NULL,			rc_link,	display_opts,
     NULL,		0,			0,		NULL,
     NULL },     
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority)
{
   if (rc_set_option2(video_opts, "widthscale", arg, priority))
      return -1;
   if (rc_set_option2(video_opts, "heightscale", arg, priority))
      return -1;
      
   option->priority = priority;
   
   return 0;
}

static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority)
{
   options.beam = (int)(beam_f * 0x00010000);
   if (options.beam < 0x00010000)
      options.beam = 0x00010000;
   else if (options.beam > 0x00100000)
      options.beam = 0x00100000;

   option->priority = priority;
   
   return 0;
}

static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority)
{
   if (options.vector_flicker < 0.0)
      options.vector_flicker = 0.0;
   else if (options.vector_flicker > 100.0)
      options.vector_flicker = 100.0;

   option->priority = priority;
   
   return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority)
{
   if( (options.color_depth != 0) &&
       (options.color_depth != 8) &&
       (options.color_depth != 15) &&
       (options.color_depth != 16) &&
       (options.color_depth != 32) )
   {
      options.color_depth = 0;
      fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority)
{
   if(sscanf(arg, "%dx%d", &options.vector_width, &options.vector_height) != 2)
   {
      options.vector_width = options.vector_height = 0;
      fprintf(stderr, "error: invalid value for vectorres: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

int osd_create_display(int width, int height, int depth,
   int fps, int attributes, int orientation)
{
   bitmap_depth = (depth != 15) ? depth : 16;

   rgb_direct = (depth == 15) || (depth == 32);

   current_palette = normal_palette = NULL;
   debug_visual.min_x = 0;
   debug_visual.max_x = options.debug_width - 1;
   debug_visual.min_y = 0;
   debug_visual.max_y = options.debug_height - 1;

   /* Can we do dirty? */
   if ( (attributes & VIDEO_SUPPORTS_DIRTY) == 0 )
   {
      use_dirty = FALSE;
   }
   
   if (use_auto_double)
   {
      if ( (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_1_2)
      {
         if (orientation & ORIENTATION_SWAP_XY)
            normal_widthscale  *= 2;
         else
            normal_heightscale *= 2;
      }
      if ( (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_2_1)
      {
         if (orientation & ORIENTATION_SWAP_XY)
            normal_heightscale *= 2;
         else
            normal_widthscale  *= 2;
      }
   }
  
   if (osd_dirty_init() != OSD_OK) return -1;

   visual_width     = width;
   visual_height    = height;
   widthscale       = normal_widthscale;
   heightscale      = normal_heightscale;
   use_aspect_ratio = normal_use_aspect_ratio;
   video_fps        = fps;
   
   if (sysdep_create_display(bitmap_depth) != OSD_OK)
      return -1;
   
   /* a lott of display_targets need to have the display initialised before
      initialising any input devices */
   if (osd_input_initpost()!=OSD_OK) return -1;
   
   if (use_dirty) fprintf(stderr_file,"Using dirty_buffer strategy\n");
   if (bitmap_depth==16) fprintf(stderr_file,"Using 16bpp video mode\n");
   
   return 0;
}   

void osd_close_display (void)
{
   osd_free_colors();
   sysdep_display_close();
   osd_dirty_close ();
}

static void osd_change_display_settings(struct my_rectangle *new_visual,
   struct sysdep_palette_struct *new_palette, int new_widthscale,
   int new_heightscale, int new_use_aspect_ratio)
{
   int new_visual_width, new_visual_height, palette_dirty = 0;
  
   /* always update the visual info */
   visual = *new_visual;
   
   /* calculate the new visual width / height */
   new_visual_width  = visual.max_x - visual.min_x + 1;
   new_visual_height = visual.max_y - visual.min_y + 1;
   
   if( current_palette != new_palette )
   {
      current_palette = new_palette;
      palette_dirty = 1;
   }
   
   if( (visual_width     != new_visual_width    ) ||
       (visual_height    != new_visual_height   ) ||
       (widthscale       != new_widthscale      ) ||
       (heightscale      != new_heightscale     ) ||
       (use_aspect_ratio != new_use_aspect_ratio) )
   {
      sysdep_display_close();
      
      visual_width     = new_visual_width;
      visual_height    = new_visual_height;
      widthscale       = new_widthscale;
      heightscale      = new_heightscale;
      use_aspect_ratio = new_use_aspect_ratio;
      
      if (sysdep_create_display(bitmap_depth) != OSD_OK)
      {
         /* oops this sorta sucks */
         fprintf(stderr_file, "Argh, resizing the display failed in osd_set_visible_area, aborting\n");
         exit(1);
      }
      
      /* only realloc the palette if it has been initialised */
      if (current_palette)
      {
         if (sysdep_display_alloc_palette(video_colors_used))
         {
            /* better restore the video mode before calling exit() */
            sysdep_display_close();
            /* oops this sorta sucks */
            fprintf(stderr_file, "Argh, (re)allocating the palette failed in osd_set_visible_area, aborting\n");
            exit(1);
         }
         palette_dirty = 1;
      }
      
      /* mark the current visual area of the bitmap dirty */
      osd_mark_dirty(visual.min_x, visual.min_y,
         visual.max_x, visual.max_y);
      
      /* to stop keys from getting stuck */
      xmame_keyboard_clear();
         
      /* for debugging only */
      fprintf(stderr_file, "viswidth = %d, visheight = %d,"
              "visstartx= %d, visstarty= %d\n",
               visual_width, visual_height, visual.min_x,
               visual.min_y);
   }
   
   if (palette_dirty && current_palette)
      sysdep_palette_mark_dirty(current_palette);
}

void osd_set_visible_area(int min_x,int max_x,int min_y,int max_y)
{
   normal_visual.min_x = min_x;
   normal_visual.max_x = max_x;
   normal_visual.min_y = min_y;
   normal_visual.max_y = max_y;

   /* round to 8, since the new dirty code works with 8x8 blocks,
      and we need to round to sizeof(long) for the long copies anyway */
   if (normal_visual.min_x & 7)
   {
      if((normal_visual.min_x - (normal_visual.min_x & ~7)) < 4)
         normal_visual.min_x &= ~7;
       else
         normal_visual.min_x = (normal_visual.min_x + 7) & ~7;
   }
   if ((normal_visual.max_x+1) & 7)
   {
      if(((normal_visual.max_x+1) - ((normal_visual.max_x+1) & ~7)) > 4)
         normal_visual.max_x = ((normal_visual.max_x+1 + 7) & ~7) - 1;
       else
         normal_visual.max_x = ((normal_visual.max_x+1) & ~7) - 1;
   }
   
   /* rounding of the y-coordinates is only nescesarry when we are doing dirty */
   if (use_dirty)
   {
      if (normal_visual.min_y & 7)
      {
         if((normal_visual.min_y - (normal_visual.min_y & ~7)) < 4)
            normal_visual.min_y &= ~7;
          else
            normal_visual.min_y = (normal_visual.min_y + 7) & ~7;
      }
      if ((normal_visual.max_y+1) & 7)
      {
         if(((normal_visual.max_y+1) - ((normal_visual.max_y+1) & ~7)) > 4)
            normal_visual.max_y = ((normal_visual.max_y+1 + 7) & ~7) - 1;
          else
            normal_visual.max_y = ((normal_visual.max_y+1) & ~7) - 1;
      }
   }
   
   if(!debugger_has_focus)
      osd_change_display_settings(&normal_visual, normal_palette,
         normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
   
   set_ui_visarea (normal_visual.min_x, normal_visual.min_y, normal_visual.max_x, normal_visual.max_y);
}

int osd_allocate_colors(unsigned int totalcolors, const unsigned char *palette,
   unsigned int *rgb_components, const unsigned char *debugger_palette,
   unsigned int *debug_pens)
{
   int i = 0;
   int r, g, b;
   int normal_colors;
   UINT8 rr,gg,bb;
   rr=gg=bb=0;
   /* if we have debug pens, map them 1:1 */
   if (debug_pens)
   {
      for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
      {
         debug_pens[i] = i;
      }
   }

   fprintf(stderr_file, "Game uses %d colors\n", totalcolors);

   if (rgb_direct)
   {
      if (bitmap_depth == 16)
      {
         normal_colors = 32768;

         /* create the normal palette */
         if (!(normal_palette = sysdep_palette_create(bitmap_depth,
                                                      normal_colors)))
         {
            return 1;
         }
      }
      else
      {
         normal_colors = 2;

         /* create the normal palette */
         if (!(normal_palette = sysdep_palette_create(bitmap_depth, 0)))
         {
            return 1;
         }
      }
   }
   else
   {
      normal_colors = totalcolors + 2;

      /* create the normal palette */
      if (!(normal_palette = sysdep_palette_create(bitmap_depth, 
                                                   normal_colors)))
      {
         return 1;
      }
   }

   video_colors_used = normal_colors;

   /* create the debug palette */
   if (debug_pens)
   {
      if (!(debug_palette = sysdep_palette_create(16, DEBUGGER_TOTAL_COLORS)))
      {
         osd_free_colors();
         return 1;
      }

      if (DEBUGGER_TOTAL_COLORS > video_colors_used)
      {
         video_colors_used = DEBUGGER_TOTAL_COLORS;
      }
   }

   /* now alloc the total number of colors used by both palettes */
   if (sysdep_display_alloc_palette(video_colors_used))
   {
      osd_free_colors();
      return 1;
   }
   
   sysdep_palette_set_gamma(normal_palette, gamma_correction);
   sysdep_palette_set_brightness(normal_palette, 
      brightness * brightness_paused_adjust);

   if (debug_palette)
   {
      sysdep_palette_set_gamma(debug_palette, gamma_correction);
      sysdep_palette_set_brightness(debug_palette, 
         brightness * brightness_paused_adjust);

      for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
      {
         sysdep_palette_set_pen(debug_palette, i, debugger_palette[i * 3],
            debugger_palette[i * 3 + 1], debugger_palette[i * 3 + 2]);
      }
   }
   
   /* init the palette */
   if (rgb_direct) 
   {
      if (bitmap_depth == 16)
      {
         i = 0;

         for (r = 0;r < 32;r++)
         {
            for (g = 0;g < 32;g++)
            {
               for (b = 0;b < 32;b++)
               {
                  sysdep_palette_set_pen(normal_palette, i,
                     (r << 3) | (r >> 2),
                     (g << 3) | (g >> 2),
                     (b << 3) | (b >> 2));
                  i++;
               }
            }
         }

         rgb_components[0] = 0x7c00;
         rgb_components[1] = 0x03e0;
         rgb_components[2] = 0x001f;

         Machine->uifont->colortable[0] = 0x0000;
         Machine->uifont->colortable[1] = 0x7fff;
         Machine->uifont->colortable[2] = 0x7fff;
         Machine->uifont->colortable[3] = 0x0000;
      }
      else
      {
         for (i = 0; i < totalcolors; i++)
         {
            r = palette[3 * i + 0];
            g = palette[3 * i + 1];
            b = palette[3 * i + 2];
            *rgb_components++ = (r << 16) | (g << 8) | b; 
         }

         Machine->uifont->colortable[0] = 0;
         Machine->uifont->colortable[1] = 0xffffff;
         Machine->uifont->colortable[2] = 0xffffff;
         Machine->uifont->colortable[3] = 0;
      }
   }
   else
   {
      /* normal palette */

      /* initialize the palette to black */
      for (i = 0; i < normal_colors; i++)
      {
         sysdep_palette_set_pen(normal_palette, i, 0, 0, 0);
      }

      /* mark the palette dirty to start */
      sysdep_palette_mark_dirty(normal_palette);

      /* reserve color totalcolors + 1 for the user interface text */
      if (totalcolors < 65535)
      {
         sysdep_palette_set_pen(normal_palette, totalcolors + 1, 0xFF, 0xFF, 0xFF);
         Machine->uifont->colortable[0] = totalcolors;
         Machine->uifont->colortable[1] = totalcolors + 1;
         Machine->uifont->colortable[2] = totalcolors + 1;
         Machine->uifont->colortable[3] = totalcolors;
      }
      else
      {
         Machine->uifont->colortable[0] = 0;
         Machine->uifont->colortable[1] = 65535;
         Machine->uifont->colortable[2] = 65535;
         Machine->uifont->colortable[3] = 0;
      }

      for (i = 0; i < totalcolors; i++)
      {
         sysdep_palette_set_pen(normal_palette, i, palette[i * 3], 
            palette[i * 3 + 1], palette[i * 3 + 2]);
      }
#if (defined svgafx) || (defined xfx) 
      color_values = malloc(video_colors_used * sizeof(UINT16));
#endif

      for (i = 0; i < totalcolors; i++) {
	 palette_get_color(i, (UINT8*)&rr, (UINT8*)&gg, (UINT8*)&bb);

#if (defined svgafx) || (defined xfx) 
         color_values[i] = (((rr >> 3) << 10) | ((gg >> 3) << 5) | (bb >> 3));
#endif
      }

#if (defined svgafx) || (defined xfx) 
	color_values[totalcolors] = 0x0000;
	color_values[totalcolors+1] = 0x7FFF;
#endif

   }
   
   /* set the current_palette to the normal_palette */
   current_palette = normal_palette;
   
   return 0;
}

static void osd_free_colors(void)
{
   if(normal_palette)
   {
      sysdep_palette_destroy(normal_palette);
      normal_palette = NULL;
   }
   if(debug_palette)
   {
      sysdep_palette_destroy(debug_palette);
      debug_palette = NULL;
   }
}

void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
    sysdep_palette_get_pen(normal_palette, pen, red, green, blue);
}

void osd_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue) 
{
#if (defined svgafx) || (defined xfx) 
   color_values[pen] = (((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3));
#endif

   sysdep_palette_set_pen(normal_palette, pen, red, green, blue);
}

static int skip_next_frame = 0;

typedef int (*skip_next_frame_func)(int show_fps_counter, struct mame_bitmap *bitmap);
static skip_next_frame_func skip_next_frame_functions[FRAMESKIP_DRIVER_COUNT] =
{
   dos_skip_next_frame,
   barath_skip_next_frame
};

int osd_skip_this_frame(void)
{   
   return skip_next_frame;
}

void osd_debugger_focus(int new_debugger_focus)
{
   if( (!debugger_has_focus &&  new_debugger_focus) || 
       ( debugger_has_focus && !new_debugger_focus))
   {
      if(new_debugger_focus)
         osd_change_display_settings(&debug_visual, debug_palette,
            1, 1, 0);
      else
         osd_change_display_settings(&normal_visual, normal_palette,
            normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      
      debugger_has_focus = new_debugger_focus;
   }
}

/* Update the display. */
void osd_update_video_and_audio(struct mame_bitmap *normal_bitmap,
   struct mame_bitmap *debug_bitmap, int leds_status)
{
   static int showfps=0, showfpstemp=0; 
   int skip_this_frame;
   struct mame_bitmap *current_bitmap = normal_bitmap;
   
   
/* save the active bitmap for use in osd_clearbitmap, I know this
      sucks blame the core ! */
   scrbitmap = normal_bitmap;
   
   if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = 0;
      }
      else
      {
	 if (frameskip == FRAMESKIP_LEVELS-1)
	 {
	    frameskip = 0;
	    autoframeskip = 1;
	 }
	 else frameskip++;
      }

      if (showfps == 0) showfpstemp = 2 * Machine->drv->frames_per_second;
   }

   if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = FRAMESKIP_LEVELS-1;
      }
      else
      {
	 if (frameskip == 0) autoframeskip = 1;
	 else frameskip--;
      }
      
      if (showfps == 0)	showfpstemp = 2 * Machine->drv->frames_per_second;
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_THROTTLE))
   {
      throttle ^= 1;
   }
   
   if (input_ui_pressed(IPT_UI_THROTTLE) && (keyboard_pressed(KEYCODE_RSHIFT) || keyboard_pressed(KEYCODE_LSHIFT)))
   {
      sleep_idle ^= 1;
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_SHOW_FPS))
   {
      if (showfpstemp)
      {
	 showfpstemp = 0;
	 schedule_full_refresh();
      }
      else
      {
	 showfps ^= 1;
	 if (showfps == 0)
	 {
	    schedule_full_refresh();
	 }
      }
   }
   
   if (keyboard_pressed (KEYCODE_LCONTROL))
   { 
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         frameskipper = 0;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         frameskipper = 1;
   }
   
   if (debug_bitmap)
   {
      if (input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
         osd_debugger_focus(!debugger_has_focus);
      if (debugger_has_focus)
         current_bitmap = debug_bitmap;
   }
   /* this should not happen I guess, but better safe than sorry */
   else if (debugger_has_focus)
      osd_debugger_focus(0);

   if (keyboard_pressed (KEYCODE_LSHIFT))
   {
      int widthscale_mod  = 0;
      int heightscale_mod = 0;
      
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         widthscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_DEL))
         widthscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         heightscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_END))
         heightscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_PGUP))
      {
         widthscale_mod  = 1;
         heightscale_mod = 1;
      }
      if (keyboard_pressed_memory (KEYCODE_PGDN))
      {
         widthscale_mod  = -1;
         heightscale_mod = -1;
      }
      if (widthscale_mod || heightscale_mod)
      {
         normal_widthscale  += widthscale_mod;
         normal_heightscale += heightscale_mod;
         
         if (normal_widthscale > 8)
            normal_widthscale = 8;
         else if (normal_widthscale < 1)
            normal_widthscale = 1;
         if (normal_heightscale > 8)
            normal_heightscale = 8;
         else if (normal_heightscale < 1)
            normal_heightscale = 1;
         
         if (!debugger_has_focus)
            osd_change_display_settings(&normal_visual, normal_palette,
               normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      }
   }
   
   if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
   {
      showfpstemp--;
      if (showfpstemp == 0) schedule_full_refresh();
   }

   skip_this_frame = skip_next_frame;
   skip_next_frame =
      (*skip_next_frame_functions[frameskipper])(showfps || showfpstemp,
        normal_bitmap);
   
   if (sound_stream && sound_enabled)
      sound_stream_update(sound_stream);

   if (skip_this_frame == 0)
   {
      profiler_mark(PROFILER_BLIT);
      sysdep_palette_update(current_palette);
      sysdep_update_display(current_bitmap);
      profiler_mark(PROFILER_END);
   }
   
   sysdep_set_leds(leds_status);
   osd_poll_joysticks();
}

void osd_set_gamma(float gamma)
{
   sysdep_palette_set_gamma(normal_palette, gamma);
   if(debug_palette)
      sysdep_palette_set_gamma(debug_palette, gamma);
}

float osd_get_gamma(void)
{
   return sysdep_palette_get_gamma(normal_palette);
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
   brightness = _brightness;
   sysdep_palette_set_brightness(normal_palette, brightness *
      brightness_paused_adjust);
   if (debug_palette)
      sysdep_palette_set_brightness(debug_palette, brightness);
}

int osd_get_brightness(void)
{
   return brightness;
}

#ifndef xgl
void osd_save_snapshot(struct mame_bitmap *bitmap)
{
   save_screen_snapshot(bitmap);
}
#endif

void osd_pause(int paused)
{
   if (paused) 
      brightness_paused_adjust = 0.65;
   else
      brightness_paused_adjust = 1.0;

   osd_set_brightness(brightness);
}
