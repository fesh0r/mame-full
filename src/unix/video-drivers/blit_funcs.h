#ifndef __BLIT_FUNCS_H
#define __BLIT_FUNCS_H

#include "sysdep/sysdep_palette.h"
#include "sysdep/sysdep_display.h"

typedef void (*blit_func_p)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);

/* Get a function which will blit from sysdep_display_params.depth to
   dest_depth.
   Params:
   dest_depth	not really the standard definition of depth, but, the depth
                for all modes, except for depth 24 32bpp sparse where 32 should be
                passed. This is done to differentiate depth 24 24bpp
                packed pixel and depth 24 32bpp sparse.
*/
blit_func_p sysdep_display_get_blitfunc(int dest_bpp);
blit_func_p sysdep_display_get_blitfunc_doublebuffer(int dest_bpp);

/* Find out of blitting from sysdep_display_params.depth to
   dest_depth including scaling, rotation and effects will result in
   exactly the same bitmap, in this case the blitting can be skipped under
   certain circumstances.
   Params:
   dest_depth	not really the standard definition of depth, but, the depth
                for all modes, except for depth 24 32bpp sparse where 32 should be
                passed. This is done to differentiate depth 24 24bpp
                packed pixel and depth 24 32bpp sparse.
*/
int sysdep_display_blit_dest_bitmap_equals_src_bitmap(int dest_depth);

#endif
