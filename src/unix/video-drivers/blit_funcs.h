#ifndef __BLIT_FUNCS_H
#define __BLIT_FUNCS_H

#include "sysdep/sysdep_palette.h"
#include "sysdep/sysdep_display.h"

typedef void (*blit_func_p)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);

blit_func_p sysdep_display_get_blitfunc(void);
blit_func_p sysdep_display_get_blitfunc_dfb(void);

/* Find out of blitting from sysdep_display_params.depth to
   dest_depth including scaling, rotation and effects will result in
   exactly the same bitmap, in this case the blitting can be skipped under
   certain circumstances. */
int sysdep_display_blit_dest_bitmap_equals_src_bitmap(void);

#endif
