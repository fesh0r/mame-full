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

static struct sysdep_display_open_params sysdep_display_orig_params;

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

static void sysdep_display_set_params(struct sysdep_display_open_params *params)
{
  sysdep_display_params = *params;
  
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
  
  sysdep_display_orig_params = *params;
  sysdep_display_set_params(params);
  
  if (sysdep_display_driver_open())
    return 1;

  if ((sysdep_display_properties.mode[params->video_mode] &
       SYSDEP_DISPLAY_EFFECTS) && sysdep_display_effect_open())
    return 1;
    
  return 0;
}

void sysdep_display_close(void)
{
  sysdep_display_effect_close();
  sysdep_display_driver_close();
}

int sysdep_display_change_settings(
  struct sysdep_display_open_params *new_params, int force)
{
  int effect_changed = 0;
  int resize = 0;
  struct sysdep_display_properties_struct old_properties = 
    sysdep_display_properties;
  
  /* If the changes aren't forced and the video mode is not available
     use the old video mode */
  if (!force && (new_params->video_mode >= 0) &&
      (new_params->video_mode < SYSDEP_DISPLAY_VIDEO_MODES) &&
      !sysdep_display_properties.mode[new_params->video_mode])
    new_params->video_mode = sysdep_display_orig_params.video_mode;

  /* Check (and adjust) the new params */   
  if (sysdep_display_check_params(new_params))
    goto sysdep_display_change_settings_error;

  /* if any of these change we have to recreate the display */    
  if ((new_params->depth        != sysdep_display_orig_params.depth)        ||
      ((new_params->orientation & SYSDEP_DISPLAY_SWAPXY) !=
       (sysdep_display_orig_params.orientation & SYSDEP_DISPLAY_SWAPXY))    ||
      (new_params->max_width    != sysdep_display_orig_params.max_width)    ||
      (new_params->max_height   != sysdep_display_orig_params.max_height)   ||
      (new_params->title        != sysdep_display_orig_params.title)        ||
      (new_params->video_mode   != sysdep_display_orig_params.video_mode)   ||
      (new_params->widthscale   != sysdep_display_orig_params.widthscale)   ||
      (new_params->yarbsize     != sysdep_display_orig_params.yarbsize)     ||
      (!new_params->yarbsize &&
       (new_params->heightscale != sysdep_display_orig_params.heightscale)) ||
      (new_params->fullscreen   != sysdep_display_orig_params.fullscreen)   ||
      (new_params->aspect_ratio != sysdep_display_orig_params.aspect_ratio) )
  {
    sysdep_display_close();
    sysdep_display_set_params(new_params);
    if (sysdep_display_driver_open())
    {
      sysdep_display_close();

      if (force)
        goto sysdep_display_change_settings_error;

      /* try again with old settings */
      sysdep_display_set_params(&sysdep_display_orig_params);
      if (sysdep_display_driver_open())
      {
        sysdep_display_close();
        goto sysdep_display_change_settings_error;
      }
      *new_params = sysdep_display_orig_params;
    }
    else
      sysdep_display_orig_params = *new_params;

    if ((sysdep_display_properties.mode[new_params->video_mode] &
         SYSDEP_DISPLAY_EFFECTS) && sysdep_display_effect_open())
      goto sysdep_display_change_settings_error;
    
    return 1;
  }
  
  /* do we need to reinit the effect code? */
  if (new_params->effect != sysdep_display_orig_params.effect)
    effect_changed = 1;
  
  /* if these change we need to resize */
  if ((new_params->width           != sysdep_display_orig_params.width)  ||
      (new_params->height          != sysdep_display_orig_params.height) ||
      (new_params->vec_src_bounds  !=
        sysdep_display_orig_params.vec_src_bounds) ||
      (new_params->vec_dest_bounds !=
        sysdep_display_orig_params.vec_dest_bounds))
    resize = 1;

  /* apply the changes */
  sysdep_display_orig_params = *new_params;
  sysdep_display_set_params(new_params);

  if (effect_changed)
  {
    sysdep_display_effect_close();
    if (sysdep_display_effect_open())
      goto sysdep_display_change_settings_error;
  }  

  if (resize && sysdep_display_driver_resize())
    goto sysdep_display_change_settings_error;

  /* other changes are handled 100% on the fly */
  if(memcmp(&sysdep_display_properties, &old_properties,
      sizeof(struct sysdep_display_properties_struct)))
    return 1;
  else
    return 0;
  
sysdep_display_change_settings_error:
  /* oops this sorta sucks, FIXME don't use exit! */
  fprintf(stderr, "Fatal error in sysdep_display_change_settings\n");
  exit(1);
  return 0; /* shut up warnings, never reached */
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

