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

void sysdep_display_set_params(const struct sysdep_display_open_params *params)
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
  
  /* lett the effect code do its magic */
  effect_check_params();
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
