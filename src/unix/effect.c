/* Video Effect Functions
 *
 * Ben Saylor - bsaylor@macalester.edu
 *
 * Each of these functions copies one line of source pixels, applying an effect.
 * They are called through pointers from blit_core.h.
 * There's one for every bitmap depth / display depth combination,
 * and for direct/non-pallettized where needed.
 *
 * HISTORY:
 *  2004-13-09:
 *   - lots of changes, see mess CVS ChangeLog <j.w.r.degoede@hhs.nl>
 *
 *  2001-10-06:
 *   - minor changes to make -effect option trump -arbheight option <adam@gimp.org>
 *
 *  2001-10-01:
 *   - should now compile with all video drivers,
 *     though effects won't work except with
 *     aforementioned drivers
 *
 *  2001-09-21:
 *   - scan3 effect (deluxe scanlines)
 *   - small fixes & changes
 *
 *  2001-09-19:
 *   - ported to xmame-0.55.1
 *   - fixed some DGA bugs
 *   - confirmed to work with SDL
 *   - added ggi and svgalib support (untested)
 *   - added two basic RGB effects (rgbstripe3x2 and rgbscan2x3)
 *   - removed 8-bit support
 *
 *  2001-09-13:
 *   - added scan2 effect (light scanlines)
 *   - automatically scale as required by effect
 *   - works with x11_window and dga2, and hopefully dga1 and SDL (untested)
 *     (windowed mode is fastest)
 *   - use doublebuffering where requested by video driver
 *   - works on 16-bit and 32-bit displays, others untested
 *     (16-bit recommended - it's faster)
 *   - scale2x smooth scaling effect
 */

#define __EFFECT_C_
#include <stdlib.h>
#include <string.h>
#include "osd_cpu.h"
#include "sysdep/sysdep_display_priv.h"
#include "effect.h"

char *effect_dbbuf  = NULL;
char *rotate_dbbuf0 = NULL;
char *rotate_dbbuf1 = NULL;
char *rotate_dbbuf2 = NULL;
/* for the 6tap filter, used by effect_funcs.c */
char *_6tap2x_buf0 = NULL;
char *_6tap2x_buf1 = NULL;
char *_6tap2x_buf2 = NULL;
char *_6tap2x_buf3 = NULL;
char *_6tap2x_buf4 = NULL;
char *_6tap2x_buf5 = NULL;

/* inverse RGB masks */
#define RMASK16_INV(P) ((P) & 0x07ff)
#define GMASK16_INV(P) ((P) & 0xf81f)
#define BMASK16_INV(P) ((P) & 0xffe0)
#define RMASK32_INV(P) ((P) & 0x0000ffff)
#define GMASK32_INV(P) ((P) & 0x00ff00ff)
#define BMASK32_INV(P) ((P) & 0x00ffff00)

#define FOURCC_YUY2 0x32595559
#define FOURCC_YV12 0x32315659


/* RGB16 to YUV like conversion, used for lq2x/hq2x calculations */
static int rgb2yuv[65536];
const  int Ymask    = 0x0000FF00;
const  int Umask    = 0x00FF0000;
const  int Vmask    = 0x000000FF;
const  int trY      = 0x00003000;
const  int trU      = 0x00070000;
const  int trV      = 0x00000006;
const  int trY32    = 0xC0;
const  int trU32    = 0x1C;
const  int trV32    = 0x18;


static void init_rgb2yuv(int display_mode)
{
  int i, j, k, r, g, b, Y, u, v;

  if (display_mode == 0)      /* 15 bpp */
  {
    for (i=0; i<32; i++)
      for (j=0; j<32; j++)
        for (k=0; k<32; k++)
        {
          r = i << 3;
          g = j << 3;
          b = k << 3;
          Y = (r + g + b) >> 2;
          u = 128 + ((r - b) >> 2);
          v = 128 + ((-r + 2*g -b)>>3);
          rgb2yuv[ (i << 10) + (j << 5) + k ] = (v) + (u<<16) + (Y<<8);
        }
  }
  else if (display_mode == 1) /* 16 bpp */
  {
    for (i=0; i<32; i++)
      for (j=0; j<64; j++)
        for (k=0; k<32; k++)
        {
          r = i << 3;
          g = j << 2;
          b = k << 3;
          Y = (r + g + b) >> 2;
          u = 128 + ((r - b) >> 2);
          v = 128 + ((-r + 2*g -b)>>3);
          rgb2yuv[ (i << 11) + (j << 5) + k ] = (v) + (u<<16) + (Y<<8);
        }
  }
}


/* called from sysdep_display_open to update scale parameters */
void effect_check_params(void)
{
  int disable_arbscale = 0;

  switch (sysdep_display_params.effect) {
    case EFFECT_SCALE2X:
    case EFFECT_HQ2X:
    case EFFECT_LQ2X:
    case EFFECT_SCAN2:
    case EFFECT_6TAP2X:
      sysdep_display_params.widthscale = 2;
      sysdep_display_params.heightscale = 2;
      disable_arbscale = 1;
      break;
    case EFFECT_RGBSTRIPE:
      sysdep_display_params.widthscale = 3;
      sysdep_display_params.heightscale = 2;
      disable_arbscale = 1;
      break;
    case EFFECT_RGBSCAN:
      sysdep_display_params.widthscale = 2;
      sysdep_display_params.heightscale = 3;
      disable_arbscale = 1;
      break;
    case EFFECT_SCAN3:
      sysdep_display_params.widthscale = 3;
      sysdep_display_params.heightscale = 3;
      disable_arbscale = 1;
      break;
  }

  if (sysdep_display_params.yarbsize && disable_arbscale) {
    printf("Using effects -- disabling arbitrary scaling\n");
    sysdep_display_params.yarbsize = 0;
  }
}

/* Generate most effect variants automagicly, do this before
   building the effect funcs tables, so that we don't have to declare
   prototypes for all these variants (me lazy) */

/* for effects which have 2 steps, most have 3 addline functions
   for 15, 16 and 32 bpp src's and 5 render functions for
   15, 16, 32, YUY2 and YV12 dest's */
#define DEST_DEPTH 15
#include "effect_renderers.h"

#define DEST_DEPTH 16
#include "effect_renderers.h"

#define DEST_DEPTH 32
#include "effect_renderers.h"

#define DEST_DEPTH YUY2
#include "effect_renderers.h"

/* Now the normal effect, first the indirect versions */
#define INDIRECT
#define GETPIXEL(p) u32lookup[p]

#define FUNC_NAME(name) name##_16_15
#define SRC_PIXEL  UINT16
#define DEST_DEPTH 15
#include "effect_funcs.h"

#define FUNC_NAME(name) name##_16_16
#define SRC_PIXEL  UINT16
#define DEST_DEPTH 16
#include "effect_funcs.h"

#define FUNC_NAME(name) name##_16_32
#define SRC_PIXEL  UINT16
#define DEST_DEPTH 32
#include "effect_funcs.h"

/* and now the direct versions */
#undef  INDIRECT
#undef  GETPIXEL
#define GETPIXEL(p) (p)

#define FUNC_NAME(name) name##_15_15_direct
#define SRC_PIXEL  UINT16
#define DEST_DEPTH 15
#include "effect_funcs.h"

#define FUNC_NAME(name) name##_32_32_direct
#define SRC_PIXEL  UINT32
#define DEST_DEPTH 32
#include "effect_funcs.h"

/* done */
#undef GETPIXEL

#define DISPLAY_MODES 5 /* 15,16,32,YUY2,YV12 */
#define EFFECT_MODES (DISPLAY_MODES*3) /* 15,16,32 */
/* arrays with all the effect functions:
   5x 15 to ... + 5x 16 to ... + 5x 32 to ...
   15
   16
   32
   YUY2
   YV12
   For each effect ! */
static effect_func_p effect_funcs[] = {
   /* scan2 */
   effect_scan2_15_15_direct,
   effect_scan2_16_16, /* just use the 16 bpp src versions, since we need */
   effect_scan2_16_32, /* to go through the lookup anyways */
   effect_scan2_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scan2_16_15,
   effect_scan2_16_16,
   effect_scan2_16_32,
   effect_scan2_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scan2_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_scan2_32_32_direct, /* use doublebuffering and do down sampling */
   effect_scan2_32_32_direct, /* blitting from the buffer to the surface */
   effect_scan2_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* rgbstripe */
   effect_rgbstripe_15_15_direct,
   effect_rgbstripe_16_16, /* just use the 16 bpp src versions, since we need */
   effect_rgbstripe_16_32, /* to go through the lookup anyways */
   effect_rgbstripe_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_rgbstripe_16_15,
   effect_rgbstripe_16_16,
   effect_rgbstripe_16_32,
   effect_rgbstripe_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_rgbstripe_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_rgbstripe_32_32_direct, /* use doublebuffering and do down sampling */
   effect_rgbstripe_32_32_direct, /* blitting from the buffer to the surface */
   effect_rgbstripe_32_YUY2_direct,
   NULL /* reserved for 32_YV12_direct */
};

static effect_scale2x_func_p effect_scale2x_funcs[] = {
   /* scale 2x */
   effect_scale2x_15_15_direct,
   effect_scale2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_scale2x_16_32, /* to go through the lookup anyways */
   effect_scale2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scale2x_16_15,
   effect_scale2x_16_16,
   effect_scale2x_16_32,
   effect_scale2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scale2x_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_scale2x_32_32_direct, /* use doublebuffering and do down sampling */
   effect_scale2x_32_32_direct, /* blitting from the buffer to the surface */
   effect_scale2x_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* lq2x */
   effect_lq2x_15_15_direct,
   effect_lq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_lq2x_16_32, /* to go through the lookup anyways */
   effect_lq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_lq2x_16_15,
   effect_lq2x_16_16,
   effect_lq2x_16_32,
   effect_lq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_lq2x_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_lq2x_32_32_direct, /* use doublebuffering and do down sampling */
   effect_lq2x_32_32_direct, /* blitting from the buffer to the surface */
   effect_lq2x_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* hq2x */
   effect_hq2x_15_15_direct,
   effect_hq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_hq2x_16_32, /* to go through the lookup anyways */
   effect_hq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_hq2x_16_15,
   effect_hq2x_16_16,
   effect_hq2x_16_32,
   effect_hq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_hq2x_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_hq2x_32_32_direct, /* use doublebuffering and do down sampling */
   effect_hq2x_32_32_direct, /* blitting from the buffer to the surface */
   effect_hq2x_32_YUY2_direct,
   NULL /* reserved for 32_YV12_direct */
};

static effect_scale3x_func_p effect_scale3x_funcs[] = {
   /* rgbscan */
   effect_rgbscan_15_15_direct,
   effect_rgbscan_16_16, /* just use the 16 bpp src versions, since we need */
   effect_rgbscan_16_32, /* to go through the lookup anyways */
   effect_rgbscan_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_rgbscan_16_15,
   effect_rgbscan_16_16,
   effect_rgbscan_16_32,
   effect_rgbscan_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_rgbscan_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_rgbscan_32_32_direct, /* use doublebuffering and do down sampling */
   effect_rgbscan_32_32_direct, /* blitting from the buffer to the surface */
   effect_rgbscan_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scan3 */
   effect_scan3_15_15_direct,
   effect_scan3_16_16, /* just use the 16 bpp src versions, since we need */
   effect_scan3_16_32, /* to go through the lookup anyways */
   effect_scan3_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scan3_16_15,
   effect_scan3_16_16,
   effect_scan3_16_32,
   effect_scan3_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_scan3_32_32_direct, /* just render in 32 bpp, the blit core will */
   effect_scan3_32_32_direct, /* use doublebuffering and do down sampling */
   effect_scan3_32_32_direct, /* blitting from the buffer to the surface */
   effect_scan3_32_YUY2_direct,
   NULL /* reserved for 32_YV12_direct */
};

static effect_6tap_addline_func_p effect_6tap_addline_funcs[] = {
   effect_6tap_addline_15,
   effect_6tap_addline_16,
   effect_6tap_addline_32
};

static effect_6tap_render_func_p effect_6tap_render_funcs[] = {
   effect_6tap_render_15,
   effect_6tap_render_16,
   effect_6tap_render_32,
   effect_6tap_render_YUY2,
   NULL
};

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering.
 *
 * The caller should call effect_close() on failure and when
 * done, to free (partly) allocated buffers */
int effect_open(void)
{
  int i = -1;

  if (!(effect_dbbuf = malloc(sysdep_display_params.aligned_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4)))
    return 1;
  memset(effect_dbbuf, sysdep_display_params.aligned_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4, 0);

  switch(sysdep_display_properties.palette_info.fourcc_format)
  {
    case FOURCC_YUY2:
      i = 3;
      break;
    case FOURCC_YV12:
      i = 4;
      break;
    default:
      if ( (sysdep_display_properties.palette_info.red_mask   == (0x1F << 10)) &&
           (sysdep_display_properties.palette_info.green_mask == (0x1F <<  5)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
        i = 0;
      if ( (sysdep_display_properties.palette_info.red_mask   == (0x1F << 11)) &&
           (sysdep_display_properties.palette_info.green_mask == (0x3F <<  5)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
        i = 1;
      if ( (sysdep_display_properties.palette_info.red_mask   == (0xFF << 16)) &&
           (sysdep_display_properties.palette_info.green_mask == (0xFF <<  8)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0xFF      )))
        i = 2;
  }

  if (sysdep_display_params.effect)
  {
    if (i == -1)
    {
      fprintf(stderr, "Warning your current videomode is not supported by the effect code, disabling effects\n");
      sysdep_display_params.effect = 0;
    }
    else
    {
      fprintf(stderr, "Initializing video effect %d: bitmap depth = %d, display type = %d\n", sysdep_display_params.effect, sysdep_display_params.depth, i);
      i += (sysdep_display_params.depth / 16) * DISPLAY_MODES;
    }
  }

  switch (sysdep_display_params.effect)
  {
    case EFFECT_SCAN2:
      effect_func = effect_funcs[i];
      break;
    case EFFECT_RGBSTRIPE:
      effect_func = effect_funcs[i+EFFECT_MODES];
      break;
    case EFFECT_SCALE2X:
      effect_scale2x_func = effect_scale2x_funcs[i];
      break;
    case EFFECT_LQ2X:
      effect_scale2x_func = effect_scale2x_funcs[i+EFFECT_MODES];
      break;
    case EFFECT_HQ2X:
      /* we might need a yuv lookup table */
      init_rgb2yuv(i%DISPLAY_MODES);
      effect_scale2x_func = effect_scale2x_funcs[i+2*EFFECT_MODES];
      break;
    case EFFECT_RGBSCAN:
      effect_scale3x_func = effect_scale3x_funcs[i];
      break;
    case EFFECT_SCAN3:
      effect_scale3x_func = effect_scale3x_funcs[i+EFFECT_MODES];
      break;
    case EFFECT_6TAP2X:
      effect_6tap_addline_func = effect_6tap_addline_funcs[sysdep_display_params.depth/16];
      effect_6tap_render_func  = effect_6tap_render_funcs[i%DISPLAY_MODES];
      effect_6tap_clear_func   = effect_6tap_clear;
      break;
  }
  
  /* check if we've got a valid implementation */
  i = -1;
  switch(sysdep_display_params.effect)
  {
    case EFFECT_NONE:
      i = 0;
      break;
    case EFFECT_SCAN2:
    case EFFECT_RGBSTRIPE:
      if(effect_func) i = 0;
      break;
    case EFFECT_SCALE2X:
    case EFFECT_LQ2X:
    case EFFECT_HQ2X:
      if (effect_scale2x_func) i = 0;
      break;
    case EFFECT_RGBSCAN:
    case EFFECT_SCAN3:
      if (effect_scale3x_func) i = 0;
      break;
    case EFFECT_6TAP2X:
      if (effect_6tap_render_func) i = 0;
      break;
  }
  if (i == -1)
  {
    fprintf(stderr, "Warning the choisen effect in combination with your current videomode is not supported by the effect code, disabling effects\n");
    sysdep_display_params.effect = 0;
  }

  if (sysdep_display_params.orientation)
  {
    switch (sysdep_display_params.depth) {
    case 15:
    case 16:
      rotate_func = rotate_16_16;
      break;
    case 32:
      rotate_func = rotate_32_32;
      break;
    }

    /* add safety of +- 16 bytes, since some effects assume that this
       is present and otherwise segfault */
    if (!(rotate_dbbuf0 = calloc(sysdep_display_params.width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
      return 1;
    rotate_dbbuf0 += 16;

    if ((sysdep_display_params.effect == EFFECT_SCALE2X) ||
        (sysdep_display_params.effect == EFFECT_HQ2X)    ||
        (sysdep_display_params.effect == EFFECT_LQ2X)) {
      if (!(rotate_dbbuf1 = calloc(sysdep_display_params.width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
        return 1;
      if (!(rotate_dbbuf2 = calloc(sysdep_display_params.width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
        return 1;
      rotate_dbbuf1 += 16;
      rotate_dbbuf2 += 16;
    }
  }

  /* I need these buffers */
  if (sysdep_display_params.effect == EFFECT_6TAP2X)
  {
    if (!(_6tap2x_buf0 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf1 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf2 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf3 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf4 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf5 = calloc(sysdep_display_params.width*8, sizeof(char))))
      return 1;
    if(sysdep_display_params.depth == 16)
    {
       /* HACK: we need the palette lookup table to be 888 rgb, this means
          that the lookup table won't be usable for normal blitting anymore
          but that is not a problem, since we're not doing normal blitting */
       sysdep_display_properties.palette_info.fourcc_format = 0;
       sysdep_display_properties.palette_info.red_mask   = 0x00FF0000;
       sysdep_display_properties.palette_info.green_mask = 0x0000FF00;
       sysdep_display_properties.palette_info.blue_mask  = 0x000000FF;
    }
  }
  return 0;
}

void effect_close(void)
{
  if (effect_dbbuf)
  {
    free(effect_dbbuf);
    effect_dbbuf = NULL;
  }

  /* there is a safety of +- 16 bytes, since some effects assume that this
     is present and otherwise segfault */
  if (rotate_dbbuf0)
  {
     rotate_dbbuf0 -= 16;
     free(rotate_dbbuf0);
     rotate_dbbuf0 = NULL;
  }
  if (rotate_dbbuf1)
  {
     rotate_dbbuf1 -= 16;
     free(rotate_dbbuf1);
     rotate_dbbuf1 = NULL;
  }
  if (rotate_dbbuf2)
  {
     rotate_dbbuf2 -= 16;
     free(rotate_dbbuf2);
     rotate_dbbuf2 = NULL;
  }

  if (_6tap2x_buf0)
  {
    free(_6tap2x_buf0);
    _6tap2x_buf0 = NULL;
  }
  if (_6tap2x_buf1)
  {
    free(_6tap2x_buf1);
    _6tap2x_buf1 = NULL;
  }
  if (_6tap2x_buf2)
  {
    free(_6tap2x_buf2);
    _6tap2x_buf2 = NULL;
  }
  if (_6tap2x_buf3)
  {
    free(_6tap2x_buf3);
    _6tap2x_buf3 = NULL;
  }
  if (_6tap2x_buf4)
  {
    free(_6tap2x_buf4);
    _6tap2x_buf4 = NULL;
  }
  if (_6tap2x_buf5)
  {
    free(_6tap2x_buf5);
    _6tap2x_buf5 = NULL;
  }
}


/* These can't be moved to effect_funcs.c because they
   use inlined render functions */

/* high quality 2x scaling effect, see effect_renderers for the
   real stuff */
void effect_hq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, struct sysdep_palette_struct *palette)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = palette->lookup;
  INT32 y,y2,u,v;
  UINT32 p1[2], p2[2];
  UINT32 w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

  w[1] = (w[1]>>24) | (w[1]<<8);
  w[2] = (w[2]>>24) | (w[2]<<8);
  w[4] = (w[4]>>24) | (w[4]<<8);
  w[5] = (w[5]>>24) | (w[5]<<8);
  w[7] = (w[7]>>24) | (w[7]<<8);
  w[8] = (w[8]>>24) | (w[8]<<8);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32lookup[u16src0[ 1]];
    w[2] = (w[2]>>24) | (w[2]<<8);
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[5] = (w[5]>>24) | (w[5]<<8);
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];
    w[8] = (w[8]>>24) | (w[8]<<8);

    hq2x_YUY2( p1, p2, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}


void effect_hq2x_32_YUY2_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, struct sysdep_palette_struct *palette)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT32 *u32src0 = (UINT32 *)src0;
  UINT32 *u32src1 = (UINT32 *)src1;
  UINT32 *u32src2 = (UINT32 *)src2;
  INT32 r,g,b,y,y2,u,v;
  UINT32 p1[2],p2[2];
  UINT32 w[9];

  w[1]=u32src0[-1];
  r=RMASK32(w[1]); r>>=16; g=GMASK32(w[1]);  g>>=8; b=BMASK32(w[1]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[1]=y|u|v;

  w[2]=u32src0[0];
  r=RMASK32(w[2]); r>>=16; g=GMASK32(w[2]);  g>>=8; b=BMASK32(w[2]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[2]=y|u|v;

  w[4]=u32src1[-1];
  r=RMASK32(w[4]); r>>=16; g=GMASK32(w[4]);  g>>=8; b=BMASK32(w[4]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[4]=y|u|v;

  w[5]=u32src1[0];
  r=RMASK32(w[5]); r>>=16; g=GMASK32(w[5]);  g>>=8; b=BMASK32(w[5]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[5]=y|u|v;

  w[7]=u32src2[-1];
  r=RMASK32(w[7]); r>>=16; g=GMASK32(w[7]);  g>>=8; b=BMASK32(w[7]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[7]=y|u|v;

  w[8]=u32src2[0];
  r=RMASK32(w[8]); r>>=16; g=GMASK32(w[8]);  g>>=8; b=BMASK32(w[8]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[8]=y|u|v;

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32src0[ 1];
    r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[2]=y|u|v;

    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32src1[ 1];
    r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[5]=y|u|v;

    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32src2[ 1];
    r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[8]=y|u|v;

    hq2x_YUY2( &p1[0], &p2[0], w );

    ++u32src0;
    ++u32src1;
    ++u32src2;

    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}

/* low quality 2x scaling effect, see effect_renderers for the
   real stuff */
void effect_lq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, struct sysdep_palette_struct *palette)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = palette->lookup;
  INT32 y,y2,u,v;
  UINT32 p1[2], p2[2];
  UINT32 w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

  w[1] = (w[1]>>24) | (w[1]<<8);
  w[2] = (w[2]>>24) | (w[2]<<8);
  w[4] = (w[4]>>24) | (w[4]<<8);
  w[5] = (w[5]>>24) | (w[5]<<8);
  w[7] = (w[7]>>24) | (w[7]<<8);
  w[8] = (w[8]>>24) | (w[8]<<8);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32lookup[u16src0[ 1]];
    w[2] = (w[2]>>24) | (w[2]<<8);
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[5] = (w[5]>>24) | (w[5]<<8);
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];
    w[8] = (w[8]>>24) | (w[8]<<8);

    lq2x_32( p1, p2, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}

void effect_lq2x_32_YUY2_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, struct sysdep_palette_struct *palette)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT32 *u32src0 = (UINT32 *)src0;
  UINT32 *u32src1 = (UINT32 *)src1;
  UINT32 *u32src2 = (UINT32 *)src2;
  INT32 r,g,b,y,y2,u,v;
  UINT32 p1[2],p2[2];
  UINT32 w[9];

  w[1]=u32src0[-1];
  r = RMASK32(w[1]); r>>=16; g = GMASK32(w[1]);  g>>=8; b = BMASK32(w[1]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[1]=y|u|v;

  w[2]=u32src0[0];
  r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[2]=y|u|v;

  w[4]=u32src1[-1];
  r = RMASK32(w[4]); r>>=16; g = GMASK32(w[4]);  g>>=8; b = BMASK32(w[4]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[4]=y|u|v;

  w[5]=u32src1[0];
  r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[5]=y|u|v;

  w[7]=u32src2[-1];
  r = RMASK32(w[7]); r>>=16; g = GMASK32(w[7]);  g>>=8; b = BMASK32(w[7]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[7]=y|u|v;

  w[8]=u32src2[0];
  r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[8]=y|u|v;

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32src0[ 1];
    r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[2]=y|u|v;

    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32src1[ 1];
    r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[5]=y|u|v;

    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32src2[ 1];
    r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[8]=y|u|v;

    lq2x_32( &p1[0], &p2[0], w );

    ++u32src0;
    ++u32src1;
    ++u32src2;

    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}
