#ifndef __BLIT_FUNCS_H
#define __BLIT_FUNCS_H

#include "sysdep/sysdep_palette.h"
#include "sysdep/sysdep_display.h"

typedef void (*blit_func_p)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width);

blit_func_p sysdep_display_get_blitfunc(int dest_bpp);
blit_func_p sysdep_display_get_blitfunc_doublebuffer(int dest_bpp);

#endif
