/* Sysdep display object

   Copyright 2000-2004 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "sysdep_display_priv.h"
#include <stdlib.h>
#include <string.h>

struct sysdep_display_mousedata sysdep_display_mouse_data[SYSDEP_DISPLAY_MOUSE_MAX] = {
#ifdef USE_XINPUT_DEVICES
  { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } },
  { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } },
#endif
  { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 } }
};

struct sysdep_display_properties_struct sysdep_display_properties;
struct sysdep_display_open_params sysdep_display_params;

/* keep a local copy of the params without swapxy and alignment applied to it
   for use in sysdep_display_change_params */
static struct sysdep_display_open_params current_params;

/* make the first call to sysdep_display_update_keyboard return 1 after
   sysdep_display_change_params() has closed and opened the display */
static int force_keyboard_dirty = 0;

static int sysdep_display_check_params(struct sysdep_display_open_params *params)
{
  /* these should never happen! */
  if ((params->width  > params->max_width) ||
      (params->height > params->max_height))
  {
    fprintf(stderr,
      "Error requested size (%dx%d) bigger then max size (%dx%d)\n",
      params->width, params->height, params->max_width, params->max_height);
    return 1;
  }
  switch (params->depth)
  {
    case 15:
    case 16:
    case 32:
      break;
    default:
      fprintf(stderr, "Error unsupported depth: %d\n",
        params->depth);
      return 1;
  }
  if ((params->video_mode < 0) ||
      (params->video_mode >= SYSDEP_DISPLAY_VIDEO_MODES) ||
      (!sysdep_display_properties.mode_info[params->video_mode]))
  {
    fprintf(stderr, "Error invalid video mode: %d\n",
      params->video_mode);
    return 1;
  }
  
  if (!(sysdep_display_properties.mode_info[params->video_mode] &
        SYSDEP_DISPLAY_WINDOWED))
    params->fullscreen = 1;
  
  if (!(sysdep_display_properties.mode_info[params->video_mode] &
        SYSDEP_DISPLAY_FULLSCREEN))
    params->fullscreen = 0;
  
  /* lett the effect code do its magic */
  sysdep_display_check_effect_params(params);
  
  return 0;
}  

static void sysdep_display_set_params(const struct sysdep_display_open_params *params)
{
  sysdep_display_params = current_params = *params;
  
  /* and apply swapxy to the global copy */
  if (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY)
  {
    int temp = sysdep_display_params.height;
    sysdep_display_params.height = sysdep_display_params.width;
    sysdep_display_params.width  = temp;

    temp = sysdep_display_params.max_height;
    sysdep_display_params.max_height = sysdep_display_params.max_width;
    sysdep_display_params.max_width  = temp;

    if (sysdep_display_params.aspect_ratio != 0.0)
      sysdep_display_params.aspect_ratio = 1.0 / sysdep_display_params.aspect_ratio;
  }
  
  /* The blit code sometimes needs width and x offsets to be aligned:
    -YUY2 blits blit 2 pixels at a time and thus needs an X-alignment of 2
    -packed_pixel blits blit 4 pixels to 3 longs, thus an X-alignment of 4 
    So to be on the safe size we always align x to 4.
    This alignment can be changed by display drivers if needed, this only has
    effect on sysdep_display_check_bounds, in this case the display driver
    needs to recalculate the aligned width themselves */
  sysdep_display_params.max_width += 3;
  sysdep_display_params.max_width &= ~3;
}

int sysdep_display_open (struct sysdep_display_open_params *params)
{
  if (sysdep_display_check_params(params))
    return 1;
    
  sysdep_display_set_params(params);
  
  if (sysdep_display_driver_open(0))
    return 1;

  /* update current params with and report back changes to effect made by the
     effect code, which is called from the display driver code */
  current_params.effect = params->effect = sysdep_display_params.effect;
  
  return 0;
}

int sysdep_display_change_params(
  struct sysdep_display_open_params *new_params)
{
  struct sysdep_display_properties_struct orig_properties = 
    sysdep_display_properties;
  struct sysdep_display_open_params orig_params = current_params;
  int attempt = 0;
  int retval  = 0;
  
  /* Skip attempting again with the original params if one of these
     obligatory params changes */
  if ((new_params->width            != orig_params.width)          ||
      (new_params->height           != orig_params.height)         ||
      (new_params->depth            != orig_params.depth)          ||
      (new_params->orientation      != orig_params.orientation)    ||
      (new_params->vec_src_bounds   != orig_params.vec_src_bounds) ||
      (new_params->vec_dest_bounds  != orig_params.vec_dest_bounds))
     attempt = 1;
  
  /* If the video mode is not available use the original video mode */
  if ((new_params->video_mode >= 0) &&
      (new_params->video_mode < SYSDEP_DISPLAY_VIDEO_MODES) &&
      !sysdep_display_properties.mode_info[new_params->video_mode])
    new_params->video_mode = orig_params.video_mode;

  /* Check and adjust the new params */
  if (sysdep_display_check_params(new_params))
  {
    /* If none of the obligatory params changed then report back we're using
       the orig params */
    if (!attempt) 
    {
      *new_params = orig_params;
      return 0;
    }
    sysdep_display_close();
    goto sysdep_display_change_params_error;
  }
  
  /* Are there any changes after checking the params? */
  if (memcmp(new_params, &orig_params, sizeof(orig_params)))
  {
    int scaled_width, scaled_height, reopen;

    /* do we need todo a full open and close, or just a reopen? */
    if ((new_params->video_mode != orig_params.video_mode) ||
        (new_params->fullscreen != orig_params.fullscreen))
    {
      sysdep_display_close();
      sysdep_display_set_params(new_params);
      force_keyboard_dirty = 1;
      reopen = 0;
    }
    else /* is this going to fit? */
    {
      sysdep_display_set_params(new_params);

      scaled_width  = sysdep_display_params.width *
        sysdep_display_params.widthscale;
      scaled_height = sysdep_display_params.yarbsize?
        sysdep_display_params.yarbsize:
        sysdep_display_params.height * sysdep_display_params.heightscale;
        
      /* to big? */
      if ((scaled_width  > sysdep_display_properties.max_width ) ||
          (scaled_height > sysdep_display_properties.max_height))
      {
        /* If none of the obligatory params changed then restore and report
           back the orig params */
        if (!attempt) 
        {
          sysdep_display_set_params(&orig_params);
          *new_params = orig_params;
          return 0;
        }
        /* Else attempt with no scaling and effects */
        current_params.widthscale  = 1;
        current_params.heightscale = 1;
        current_params.yarbsize    = 0;
        current_params.effect      = 0;
        sysdep_display_set_params(&current_params);
        retval |= SYSDEP_DISPLAY_SCALING_EFFECT_CHANGED;
        attempt = 2;
      }
      reopen = 1;
    }

    /* (re)open the display) */
    while (sysdep_display_driver_open(reopen))
    {
      sysdep_display_close();
      force_keyboard_dirty = 1;
      reopen = 0;
      switch(attempt)
      {
        case 0: /* first attempt, try again with orig params */
          sysdep_display_set_params(&orig_params);
          break;
        case 1: /* second attempt try without scaling and effects */
          current_params.widthscale  = 1;
          current_params.heightscale = 1;
          current_params.yarbsize    = 0;
          current_params.effect      = 0;
          sysdep_display_set_params(&current_params);
          retval |= SYSDEP_DISPLAY_SCALING_EFFECT_CHANGED;
          break;
        case 2: /* thirth attempt, try with video_mode 0, no fullscreen */
          current_params.video_mode  = 0;
          current_params.fullscreen  = 0;
          sysdep_display_set_params(&current_params);
          retval |= SYSDEP_DISPLAY_VIDMODE_FULLSCREEN_CHANGED;
          break;
        case 3: /* fourth attempt, give up! */
          goto sysdep_display_change_params_error;
      }
      attempt++;
    }
    /* update current params with changes to effect made by the effect code,
       which is called from the display driver code */
    current_params.effect = sysdep_display_params.effect;
    /* report back the params we're using */
    *new_params = current_params;

    if(memcmp(&sysdep_display_properties, &orig_properties,
        sizeof(struct sysdep_display_properties_struct)))
      retval |= SYSDEP_DISPLAY_PROPERTIES_CHANGED;
  }

  return retval;
  
sysdep_display_change_params_error:
  /* oops this sorta sucks, FIXME don't use exit! */
  fprintf(stderr, "Fatal error in sysdep_display_change_params\n");
  exit(1);
  return 0; /* shut up warnings, never reached */
}

int sysdep_display_update_keyboard(void)
{
  int i = sysdep_display_driver_update_keyboard();

  if (force_keyboard_dirty)
  {
    force_keyboard_dirty = 0;
    return 1;
  }

  return i;
}

void sysdep_display_orient_bounds(struct rectangle *bounds, int width, int height)
{
        int temp;
        
	/* apply X/Y swap first */
	if (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY)
	{
		temp = bounds->min_x;
		bounds->min_x = bounds->min_y;
		bounds->min_y = temp;

		temp = bounds->max_x;
		bounds->max_x = bounds->max_y;
		bounds->max_y = temp;

		temp   = height;
		height = width;
		width  = temp;
	}

	/* apply X flip */
	if (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX)
	{
		temp = width - bounds->min_x - 1;
		bounds->min_x = width - bounds->max_x - 1;
		bounds->max_x = temp;
	}

	/* apply Y flip */
	if (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY)
	{
		temp = height - bounds->min_y - 1;
		bounds->min_y = height - bounds->max_y - 1;
		bounds->max_y = temp;
	}
}

void sysdep_display_check_bounds(struct mame_bitmap *bitmap, struct rectangle *vis_in_dest_out, struct rectangle *dirty_area, int x_align)
{	
	int old_bound;

	/* orient the bounds */	
	sysdep_display_orient_bounds(vis_in_dest_out, bitmap->width, bitmap->height);
	sysdep_display_orient_bounds(dirty_area, bitmap->width, bitmap->height);
	
	/* fprintf(stderr, "vis_area: (%d,%d) x (%d,%d)\n",
	  vis_in_dest_out->min_x, vis_in_dest_out->min_y,
	  vis_in_dest_out->max_x, vis_in_dest_out->max_y); */
	
	/* change vis_area to destbounds */
        vis_in_dest_out->max_x = dirty_area->max_x - vis_in_dest_out->min_x;
        vis_in_dest_out->max_y = dirty_area->max_y - vis_in_dest_out->min_y;
        vis_in_dest_out->min_x = dirty_area->min_x - vis_in_dest_out->min_x;
        vis_in_dest_out->min_y = dirty_area->min_y - vis_in_dest_out->min_y;
       
	/* fprintf(stderr, "dest_area: (%d,%d) x (%d,%d)\n",
	  vis_in_dest_out->min_x, vis_in_dest_out->min_y,
	  vis_in_dest_out->max_x, vis_in_dest_out->max_y); */
	
	/* apply X-alignment to destbounds min_x and max_x apply the same
	   change to dirty_area */
	old_bound = vis_in_dest_out->min_x;
	vis_in_dest_out->min_x &= ~x_align;
        dirty_area->min_x -= old_bound - vis_in_dest_out->min_x;

	old_bound = vis_in_dest_out->max_x;
	vis_in_dest_out->max_x += 1 + x_align;
	vis_in_dest_out->max_x &= ~x_align;
	vis_in_dest_out->max_x -= 1;
        dirty_area->max_x -= old_bound - vis_in_dest_out->max_x;

	/* fprintf(stderr, "aligned dest_area: (%d,%d) x (%d,%d)\n",
	  vis_in_dest_out->min_x, vis_in_dest_out->min_y,
	  vis_in_dest_out->max_x, vis_in_dest_out->max_y); */
	
        /* apply scaling to dest_bounds */
	vis_in_dest_out->min_x *= sysdep_display_params.widthscale;
	vis_in_dest_out->max_x *= sysdep_display_params.widthscale;	
	if (sysdep_display_params.yarbsize)
	{
          vis_in_dest_out->min_y = (vis_in_dest_out->min_y * sysdep_display_params.yarbsize) /
            sysdep_display_params.height;
          vis_in_dest_out->max_y = (vis_in_dest_out->max_y * sysdep_display_params.yarbsize) /
            sysdep_display_params.height;
        }
        else
	{
          vis_in_dest_out->min_y *= sysdep_display_params.heightscale;
          vis_in_dest_out->max_y *= sysdep_display_params.heightscale;
        }
}
