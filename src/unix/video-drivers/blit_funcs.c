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

#else


static void FUNC_NAME(blit_16_to_YUY2)(struct mame_bitmap *bitmap,
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
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
  struct rectangle *src_bounds, struct rectangle *dest_bounds,
  struct sysdep_palette_struct *palette, unsigned char *dest, int dest_width)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#include "blit.h"
#undef SRC_PIXEL
#undef DEST_PIXEL
}

blit_func_p FUNC_NAME(sysdep_display_get_blitfunc)(int dest_bpp)
{
  if (sysdep_display_params.depth == 32)
  {
    switch(sysdep_display_properties.palette_info.fourcc_format)
    {
      case FOURCC_YUY2:
        return FUNC_NAME(blit_32_to_YUY2_direct);
      case 0:
        switch(dest_bpp)
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
        switch(dest_bpp)
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
