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
    fprintf(stderr, "Error in sysdep_display:\n"
      "  requested size (%dx%d) bigger then max size (%dx%d)\n",
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
      fprintf(stderr, "Error in sysdep_display: unsupported depth: %d\n",
        params->depth);
      return 1;
  }
  if ((params->video_mode < 0) ||
      (params->video_mode >= SYSDEP_DISPLAY_VIDEO_MODES) ||
      (!sysdep_display_properties.mode[params->video_mode]))
  {
    fprintf(stderr, "Error invalid video mode: %d\n",
      params->video_mode);
    return 1;
  }
  
  /* lett the effect code do its magic */
  sysdep_display_check_effect_params(&sysdep_display_params);
  
  if (!params->effect && (sysdep_display_properties.mode[params->video_mode] &
        SYSDEP_DISPLAY_HWSCALE))
  {
    params->widthscale  = 1;
    params->heightscale = 1;
    params->yarbsize    = 0;
  }
  
  if (!(sysdep_display_properties.mode[params->video_mode] &
        SYSDEP_DISPLAY_WINDOWED))
    params->fullscreen = 1;
  
  if (!(sysdep_display_properties.mode[params->video_mode] &
        SYSDEP_DISPLAY_FULLSCREEN))
    params->fullscreen = 0;
  
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

  if ((sysdep_display_properties.mode[params->video_mode] &
       SYSDEP_DISPLAY_EFFECTS) && sysdep_display_effect_open())
    return 1;
    
  /* update current params with and report back, any changes to scaling
     and effects made by the display driver or effect code. */
  current_params.widthscale  = params->widthscale  =
    sysdep_display_params.widthscale;
  current_params.heightscale = params->heightscale =
    sysdep_display_params.heightscale;
  current_params.yarbsize = params->yarbsize = sysdep_display_params.yarbsize;
  current_params.effect   = params->effect   = sysdep_display_params.effect;
  
  return 0;
}

void sysdep_display_close(void)
{
  if (sysdep_display_properties.mode[sysdep_display_params.video_mode] &
      SYSDEP_DISPLAY_EFFECTS)
    sysdep_display_effect_close();

  sysdep_display_driver_close();
}

int sysdep_display_change_params(
  struct sysdep_display_open_params *new_params, int force)
{
  struct sysdep_display_properties_struct orig_properties = 
    sysdep_display_properties;
  struct sysdep_display_open_params orig_params = current_params;
  
  /* If the changes aren't forced and the video mode is not available
     use the current video mode */
  if (!force && (new_params->video_mode >= 0) &&
      (new_params->video_mode < SYSDEP_DISPLAY_VIDEO_MODES) &&
      !sysdep_display_properties.mode[new_params->video_mode])
    new_params->video_mode = current_params.video_mode;

  /* Check and adjust the new params */
  if (sysdep_display_check_params(new_params))
  {
    if (force)
      goto sysdep_display_change_params_error;
    
    return 0;
  }

  /* if any of these change we have to recreate the display */
  if ((new_params->depth        != current_params.depth)        ||
      ((new_params->orientation & SYSDEP_DISPLAY_SWAPXY) !=
       (current_params.orientation & SYSDEP_DISPLAY_SWAPXY))    ||
      (new_params->max_width    != current_params.max_width)    ||
      (new_params->max_height   != current_params.max_height)   ||
      (new_params->title        != current_params.title)        ||
      (new_params->video_mode   != current_params.video_mode)   ||
      (new_params->widthscale   != current_params.widthscale)   ||
      (new_params->yarbsize     != current_params.yarbsize)     ||
      (!new_params->yarbsize &&
       (new_params->heightscale != current_params.heightscale)) ||
      (new_params->fullscreen   != current_params.fullscreen)   ||
      (new_params->aspect_ratio != current_params.aspect_ratio) )
  {
    int reopen = 1;
    
    /* close effect, apply the new params */
    sysdep_display_effect_close();

    /* do we need todo a full open and close, or just a reopen? */
    if ((new_params->video_mode != orig_params.video_mode) ||
        (new_params->fullscreen != orig_params.fullscreen))
    {
      sysdep_display_driver_close();
      force_keyboard_dirty = 1;
      reopen = 0;
    }

    /* (re)open the display) */
    sysdep_display_set_params(new_params);
    if (sysdep_display_driver_open(reopen))
    {
      sysdep_display_driver_close();
      force_keyboard_dirty = 1;

      if (force)
        goto sysdep_display_change_params_error;

      /* try again with old settings */
      sysdep_display_set_params(&orig_params);
      if (sysdep_display_driver_open(0))
      {
        sysdep_display_driver_close();
        goto sysdep_display_change_params_error;
      }
      /* report back we're using the orig params */
      *new_params = orig_params;
    }

    /* open effects again if nescesarry */
    if ((sysdep_display_properties.mode[current_params.video_mode] &
        SYSDEP_DISPLAY_EFFECTS) && sysdep_display_effect_open())
      goto sysdep_display_change_params_error;
  }
  else
  {
    /* apply the new params */
    sysdep_display_set_params(new_params);
    
    /* Do we need to reinit the effect code? No need to check if the
       video_mode can handle effects because if can't effect always is 0. */
    if (new_params->effect != orig_params.effect)
    {
      sysdep_display_effect_close();
      if (sysdep_display_effect_open())
        goto sysdep_display_change_params_error;
    }
    
    /* do we need to resize? */
    if ((new_params->width           != orig_params.width)  ||
        (new_params->height          != orig_params.height) ||
        (new_params->vec_src_bounds  != orig_params.vec_src_bounds) ||
        (new_params->vec_dest_bounds != orig_params.vec_dest_bounds))
    {
      if (sysdep_display_driver_resize())
        goto sysdep_display_change_params_error;
    }
  }

  /* update current params with and report back, any changes to scaling
     and effects made by the display driver or effect code. */
  current_params.widthscale  = new_params->widthscale  =
    sysdep_display_params.widthscale;
  current_params.heightscale = new_params->heightscale =
    sysdep_display_params.heightscale;
  current_params.yarbsize    = new_params->yarbsize    = 
    sysdep_display_params.yarbsize;
  current_params.effect      = new_params->effect      = 
    sysdep_display_params.effect;
  
  /* other changes are handled 100% on the fly */
  if(memcmp(&sysdep_display_properties, &orig_properties,
      sizeof(struct sysdep_display_properties_struct)))
    return 1;
  else
    return 0;
  
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
	
	/* change vis_area to destbounds */
        vis_in_dest_out->max_x = dirty_area->max_x - vis_in_dest_out->min_x;
        vis_in_dest_out->max_y = dirty_area->max_y - vis_in_dest_out->min_y;
        vis_in_dest_out->min_x = dirty_area->min_x - vis_in_dest_out->min_x;
        vis_in_dest_out->min_y = dirty_area->min_y - vis_in_dest_out->min_y;
       
	/* apply X-alignment to destbounds min_x, apply the same change to
           dirty_area */
	old_bound = vis_in_dest_out->min_x;
	vis_in_dest_out->min_x &= ~x_align;
        dirty_area->min_x -= old_bound - vis_in_dest_out->min_x;
        
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

