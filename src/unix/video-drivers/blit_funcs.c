#ifndef BLIT_FUNCS_RECURSIVE
#include <string.h>
#include "effect.h"
#include "pixel_convert.h"
#include "sysdep/sysdep_display_priv.h"
#include "blit_funcs.h"

#define DEST_WIDTH dest_width
#define DEST dest
#define BLIT_FUNCS_RECURSIVE

/* include ourselve twice once for the normal funcs and once for the
   doublebuffer variants */
#define FUNC_NAME(x) x
#include "blit_funcs.c"
#undef  FUNC_NAME

#define FUNC_NAME(x) x##_doublebuffer
#define DOUBLEBUFFER
#include "blit_funcs.c"

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
int sysdep_display_blit_dest_bitmap_equals_src_bitmap(int dest_depth)
{
  if((sysdep_display_params.depth != 16) &&         /* no palette lookup */
     (sysdep_display_params.depth == dest_depth) && /* no depth conversion */
     (!sysdep_display_params.orientation) &&        /* no rotation */
     (!sysdep_display_params.effect) &&             /* no effect */
     (sysdep_display_params.widthscale == 1) &&     /* no widthscale */
     (sysdep_display_params.yarbsize == 
      sysdep_display_params.height))                /* no heightscale */
  {
    switch(sysdep_display_params.depth)             /* check colormasks */
    {
      case 15:
        if ((sysdep_display_properties.palette_info.red_mask   == (0x1F << 10)) &&
            (sysdep_display_properties.palette_info.green_mask == (0x1F <<  5)) &&
            (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
          return 1;
      case 32:
        if ((sysdep_display_properties.palette_info.red_mask   == (0xFF << 16)) &&
            (sysdep_display_properties.palette_info.green_mask == (0xFF <<  8)) &&
            (sysdep_display_properties.palette_info.blue_mask  == (0xFF      )))
          return 1;
    }
  }
  return 0;
}

#else


static void FUNC_NAME(blit_16_to_YUY2)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned short
#define BLIT_HWSCALE_YUY2
#define INDIRECT palette->lookup
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef BLIT_HWSCALE_YUY2
#undef INDIRECT
}

static void FUNC_NAME(blit_32_to_YUY2_direct)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned short
#define BLIT_HWSCALE_YUY2
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef BLIT_HWSCALE_YUY2
}

static void FUNC_NAME(blit_16_to_16)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned short
  if (palette->lookup)
  {
#define INDIRECT palette->lookup
#include "blit.h"
#undef  INDIRECT
  }
  else
  {
#include "blit.h"
  }
#undef SRC_PIXEL
#undef DEST_PIXEL
}

static void FUNC_NAME(blit_16_to_24)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define INDIRECT palette->lookup
#define PACK_BITS
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef INDIRECT
#undef PACK_BITS
}

static void FUNC_NAME(blit_16_to_32)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define INDIRECT palette->lookup
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef INDIRECT
}

static void FUNC_NAME(blit_32_to_15)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned short
#define CONVERT_PIXEL(p) _32TO16_RGB_555(p)
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef CONVERT_PIXEL
}

static void FUNC_NAME(blit_32_to_16)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned short
#define CONVERT_PIXEL(p) _32TO16_RGB_565(p)
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef CONVERT_PIXEL
}

static void FUNC_NAME(blit_32_to_16_x)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned short
#define CONVERT_PIXEL(p) \
   sysdep_palette_make_pen(palette, p)
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef CONVERT_PIXEL
}

static void FUNC_NAME(blit_32_to_24_direct)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#define PACK_BITS
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef PACK_BITS
}

static void FUNC_NAME(blit_32_to_32_direct)(struct mame_bitmap *bitmap,
  struct rectangle *vis_in_dest_out, struct rectangle *dirty_area,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
}

/* Get a function which will blit from sysdep_display_params.depth to
   dest_depth.
   Params:
   dest_depth	not really the standard definition of depth, but, the depth
                for all modes, except for depth 24 32bpp sparse where 32 should be
                passed. This is done to differentiate depth 24 24bpp
                packed pixel and depth 24 32bpp sparse.
*/
blit_func_p FUNC_NAME(sysdep_display_get_blitfunc)(int dest_depth)
{
  if (sysdep_display_params.depth == 32)
  {
    switch(sysdep_display_properties.palette_info.fourcc_format)
    {
      case FOURCC_YUY2:
        return FUNC_NAME(blit_32_to_YUY2_direct);
      case 0:
        switch(dest_depth)
        {
          case 16:
            if ((sysdep_display_properties.palette_info.red_mask   == (0x1F << 11)) &&
                (sysdep_display_properties.palette_info.green_mask == (0x3F <<  5)) &&
                (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
              return FUNC_NAME(blit_32_to_16);
            if ((sysdep_display_properties.palette_info.red_mask   == (0x1F << 10)) &&
                (sysdep_display_properties.palette_info.green_mask == (0x1F <<  5)) &&
                (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
              return FUNC_NAME(blit_32_to_15);
            /* this is slow ! */ 
            fprintf(stderr, "\n Using generic (slow) 32 bpp to 16 bpp downsampling... ");
            return FUNC_NAME(blit_32_to_16_x);
          case 24:
            return FUNC_NAME(blit_32_to_24_direct);
          case 32:
            return FUNC_NAME(blit_32_to_32_direct);
        }
    }
  }
  else
  {
    switch(sysdep_display_properties.palette_info.fourcc_format)
    {
      case FOURCC_YUY2:
        return FUNC_NAME(blit_16_to_YUY2);
      case 0:
        switch(dest_depth)
        {
          case 16:
            return FUNC_NAME(blit_16_to_16);
          case 24:
            return FUNC_NAME(blit_16_to_24);
          case 32:
            return FUNC_NAME(blit_16_to_32);
        }
    }
  }
  return NULL;
}

#endif
