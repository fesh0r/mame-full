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
#include "xmame.h"
#include "osd_cpu.h"
#include "effect.h"

/* for the 6tap filter, used by effect_funcs.c */
char *_6tap2x_buf0 = NULL;
char *_6tap2x_buf1 = NULL;
char *_6tap2x_buf2 = NULL;
char *_6tap2x_buf3 = NULL;
char *_6tap2x_buf4 = NULL;
char *_6tap2x_buf5 = NULL;

/* divide R, G, and B to darken pixels */
#define SHADE15_HALF(P)   (((P)>>1) & 0x3def)
#define SHADE15_FOURTH(P) (((P)>>2) & 0x1ce7)
#define SHADE16_HALF(P)   (((P)>>1) & 0x7bef)
#define SHADE16_FOURTH(P) (((P)>>2) & 0x39e7)
#define SHADE32_HALF(P)   (((P)>>1) & 0x007f7f7f)
#define SHADE32_FOURTH(P) (((P)>>2) & 0x003f3f3f)

/* inverse RGB masks */
#define RMASK16_INV(P) ((P) & 0x07ff)
#define GMASK16_INV(P) ((P) & 0xf81f)
#define BMASK16_INV(P) ((P) & 0xffe0)
#define RMASK32_INV(P) ((P) & 0x0000ffff)
#define GMASK32_INV(P) ((P) & 0x00ff00ff)
#define BMASK32_INV(P) ((P) & 0x00ffff00)

/* average two pixels */
#define MEAN15(P,Q) ( RMASK15((RMASK15(P)+RMASK15(Q))/2) | GMASK15((GMASK15(P)+GMASK15(Q))/2) | BMASK15((BMASK15(P)+BMASK15(Q))/2) )
#define MEAN16(P,Q) ( RMASK16((RMASK16(P)+RMASK16(Q))/2) | GMASK16((GMASK16(P)+GMASK16(Q))/2) | BMASK16((BMASK16(P)+BMASK16(Q))/2) )
#define MEAN32(P,Q) ( RMASK32((RMASK32(P)+RMASK32(Q))/2) | GMASK32((GMASK32(P)+GMASK32(Q))/2) | BMASK32((BMASK32(P)+BMASK32(Q))/2) )

#define FOURCC_YUY2 0x32595559
#define FOURCC_YV12 0x32315659


/* RGB16 to YUV like conversion, used for lq2x/hq2x calculations */
static int rgb2yuv[65536];
const  int Ymask    = 0x0000FF00;
const  int Umask    = 0x00FF0000;
const  int Vmask    = 0x000000FF;
const  int gmask16  = 0x07E0;
const  int pmask16  = 0xF81F;
const  int rmask32  = 0x00FF0000;
const  int gmask32  = 0x0000FF00;
const  int bmask32  = 0x000000FF;
const  int pmask32  = 0x00FF00FF;
const  int trY      = 0x00003000;
const  int trU      = 0x00070000;
const  int trV      = 0x00000006;
const  int trY32    = 0xC0;
const  int trU32    = 0x1C;
const  int trV32    = 0x18;


void init_rgb2yuv(void)
{
  int i, j, k, r, g, b, Y, u, v;

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


/* called from config.c to set scale parameters */
void effect_init1()
{
        int disable_arbscale = 0;

  init_rgb2yuv( );
  switch (effect) {
    case EFFECT_SCALE2X:
    case EFFECT_HQ2X:
    case EFFECT_LQ2X:
    case EFFECT_SCAN2:
    case EFFECT_6TAP2X:
      normal_widthscale = 2;
      normal_heightscale = 2;
                        disable_arbscale = 1;
      break;
    case EFFECT_RGBSTRIPE:
      normal_widthscale = 3;
      normal_heightscale = 2;
                        disable_arbscale = 1;
      break;
    case EFFECT_RGBSCAN:
      normal_widthscale = 2;
      normal_heightscale = 3;
                        disable_arbscale = 1;
      break;
    case EFFECT_SCAN3:
      normal_widthscale = 3;
      normal_heightscale = 3;
                        disable_arbscale = 1;
      break;
  }

  if (normal_yarbsize && disable_arbscale) {
    printf("Using effects -- disabling arbitrary scaling\n");
    normal_yarbsize = 0;
  }
}

/* Generate most effect variants automagilcy, do this before
   building the effect funcs tables, so that we don't have to declare
   prototypes for all these variants (me lazy) */

/* first the indirect versions */
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
   NULL, /* effect_lq2x_15_15_direct, */
   effect_lq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_lq2x_16_32, /* to go through the lookup anyways */
   effect_lq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   NULL, /* effect_lq2x_16_15, */
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
   NULL, /* effect_hq2x_15_15_direct, */
   effect_hq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_hq2x_16_32, /* to go through the lookup anyways */
   effect_hq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   NULL, /* effect_hq2x_16_15, */
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
   NULL, /* effect_rgbscan_15_15_direct, */
   effect_rgbscan_16_16, /* just use the 16 bpp src versions, since we need */
   effect_rgbscan_16_32, /* to go through the lookup anyways */
   effect_rgbscan_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   NULL, /* effect_rgbscan_16_15, */
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
   NULL, /* effect_scan3_15_15_direct, */
   effect_scan3_16_16, /* just use the 16 bpp src versions, since we need */
   effect_scan3_16_32, /* to go through the lookup anyways */
   effect_scan3_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   NULL, /* effect_scan3_16_15, */
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
   NULL,
   NULL
};

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
void effect_init2(int src_depth, int dst_depth, int dst_width)
{
  int i = -1;

  free(effect_dbbuf);
  effect_dbbuf = malloc(visual_width*normal_widthscale*normal_heightscale*4);
  memset(effect_dbbuf, visual_width*normal_widthscale*normal_heightscale*4, 0);

  switch(display_palette_info.fourcc_format)
  {
    case FOURCC_YUY2:
      i = 3;
      break;
    case FOURCC_YV12:
      i = 4;
      break;
    default:
      if ( (display_palette_info.red_mask   == (0x1F << 10)) &&
           (display_palette_info.green_mask == (0x1F <<  5)) &&
           (display_palette_info.blue_mask  == (0x1F      )))
        i = 0;
      if ( (display_palette_info.red_mask   == (0x1F << 11)) &&
           (display_palette_info.green_mask == (0x3F <<  5)) &&
           (display_palette_info.blue_mask  == (0x1F      )))
        i = 1;
      if ( (display_palette_info.red_mask   == (0xFF << 16)) &&
           (display_palette_info.green_mask == (0xFF <<  8)) &&
           (display_palette_info.blue_mask  == (0xFF      )))
        i = 2;
  }

  if (effect)
  {
    if (i == -1)
    {
      fprintf(stderr, "Warning your current videomode is not supported by the effect code, disabling effects\n");
      effect = 0;
    }
    else
    {
      fprintf(stderr, "Initializing video effect %d: bitmap depth = %d, display type = %d\n", effect, video_real_depth, i);
      i += (video_real_depth / 16) * DISPLAY_MODES;
    }
  }

  switch (effect)
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
      effect_scale2x_func = effect_scale2x_funcs[i+2*EFFECT_MODES];
      break;
    case EFFECT_RGBSCAN:
      effect_scale3x_func = effect_scale3x_funcs[i];
      break;
    case EFFECT_SCAN3:
      effect_scale3x_func = effect_scale3x_funcs[i+EFFECT_MODES];
      break;
    case EFFECT_6TAP2X:
      effect_6tap_addline_func = effect_6tap_addline_funcs[video_real_depth/16];
      effect_6tap_render_func  = effect_6tap_render_funcs[i%DISPLAY_MODES];
      effect_6tap_clear_func   = effect_6tap_clear;
      break;
  }
  
  /* check if we've got a valid implementation */
  i = -1;
  switch(effect)
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
    effect = 0;
  }

  if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy))
  {
    switch (video_real_depth) {
    case 15:
    case 16:
      rotate_func = rotate_16_16;
      break;
    case 32:
      rotate_func = rotate_32_32;
      break;
    }

    /* add safety of +- 16 pixels, since some effects assume that this
       is present and otherwise segfault */
    if (rotate_dbbuf0)
    {
       rotate_dbbuf0 -= 16;
       free(rotate_dbbuf0);
    }
    rotate_dbbuf0 = calloc(visual_width*video_depth/8 + 32, sizeof(char));
    rotate_dbbuf0 += 16;

    if ((effect == EFFECT_SCALE2X) ||
        (effect == EFFECT_HQ2X)    ||
        (effect == EFFECT_LQ2X)) {
      if (rotate_dbbuf1)
      {
         rotate_dbbuf1 -= 16;
         rotate_dbbuf2 -= 16;
         free(rotate_dbbuf1);
         free(rotate_dbbuf2);
      }
      rotate_dbbuf1 = calloc(visual_width*video_depth/8 + 32, sizeof(char));
      rotate_dbbuf2 = calloc(visual_width*video_depth/8 + 32, sizeof(char));
      rotate_dbbuf1 += 16;
      rotate_dbbuf2 += 16;
    }
  }

  /* I need these buffers */
  if (effect == EFFECT_6TAP2X)
  {
    free(_6tap2x_buf0);
    free(_6tap2x_buf1);
    free(_6tap2x_buf2);
    free(_6tap2x_buf3);
    free(_6tap2x_buf4);
    free(_6tap2x_buf5);
    _6tap2x_buf0 = calloc(visual_width*8, sizeof(char));
    _6tap2x_buf1 = calloc(visual_width*8, sizeof(char));
    _6tap2x_buf2 = calloc(visual_width*8, sizeof(char));
    _6tap2x_buf3 = calloc(visual_width*8, sizeof(char));
    _6tap2x_buf4 = calloc(visual_width*8, sizeof(char));
    _6tap2x_buf5 = calloc(visual_width*8, sizeof(char));
    if(video_real_depth == 16)
    {
       /* HACK: we need the palette lookup table to be 888 rgb, this means
          that the lookup table won't be usable for normal blitting anymore
          but that is not a problem, since we're not doing normal blitting */
       display_palette_info.red_mask   = 0x00FF0000;
       display_palette_info.green_mask = 0x0000FF00;
       display_palette_info.blue_mask  = 0x000000FF;
    }
  }
}


/**********************************
 * rgbscan
 **********************************/


void effect_rgbscan_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16dst2 = (UINT16 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u16dst0+0) = *(u16dst0+1) = RMASK16_SEMI(u32lookup[*u16src]);
    *(u16dst1+0) = *(u16dst1+1) = GMASK16_SEMI(u32lookup[*u16src]);
    *(u16dst2+0) = *(u16dst2+1) = BMASK16_SEMI(u32lookup[*u16src]);

    ++u16src;
    u16dst0 += 2;
    u16dst1 += 2;
    u16dst2 += 2;
    --count;
  }
}


void effect_rgbscan_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16dst2 = (UINT16 *)dst2;
  UINT16 *u16src = (UINT16 *)src;

  while (count) {

    *(u16dst0+0) = *(u16dst0+1) = RMASK16_SEMI(*u16src);
    *(u16dst1+0) = *(u16dst1+1) = GMASK16_SEMI(*u16src);
    *(u16dst2+0) = *(u16dst2+1) = BMASK16_SEMI(*u16src);

    ++u16src;
    u16dst0 += 2;
    u16dst1 += 2;
    u16dst2 += 2;
    --count;
  }
}


void effect_rgbscan_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(u32lookup[*u16src]);
    *(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
    *(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(u32lookup[*u16src]);

    ++u16src;
    u32dst0 += 2;
    u32dst1 += 2;
    u32dst2 += 2;
    --count;
  }
}


void effect_rgbscan_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(u32lookup[*u16src]);
    *(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
    *(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(u32lookup[*u16src]);

    ++u16src;
    u32dst0 += 2;
    u32dst1 += 2;
    u32dst2 += 2;
    --count;
  }
}


void effect_rgbscan_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT32 *u32src = (UINT32 *)src;

  while (count) {

    *(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(*u32src);
    *(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(*u32src);
    *(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(*u32src);

    ++u32src;
    u32dst0 += 2;
    u32dst1 += 2;
    u32dst2 += 2;
    --count;
  }
}


#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff

void effect_rgbscan_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 r,g,b,y,u,v;
  INT32 us,vs;

  while (count) {
    r = u32lookup[*u16src];
    y = r&255;
    u = (r>>8)&255;
    v = (r>>24);
    us = u - 128;
    vs = v - 128;
    r = ((512*y + 718*vs) >> 9);
    g = ((512*y - 176*us - 366*vs) >> 9);
    b = ((512*y + 907*us) >> 9);
    y = (( 9836*r + 19310*(g&0x7f) + 3750*(b&0x7f) ) >> 15);
    u = (( -5527*r - 10921*(g&0x7f) + 16448*(b&0x7f) ) >> 15) + 128;
    v = (( 16448*r - 13783*(g&0x7f) - 2665*(b&0x7f) ) >> 15) + 128;
    *u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    y = (( 9836*(r&0x7f) + 19310*g + 3750*(b&0x7f) ) >> 15);
    u = (( -5527*(r&0x7f) - 10921*g + 16448*(b&0x7f) ) >> 15) + 128;
    v = (( 16448*(r&0x7f) - 13783*g - 2665*(b&0x7f) ) >> 15) + 128;
    *u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    y = (( 9836*(r&0x7f) + 19310*(g&0x7f) + 3750*b ) >> 15);
    u = (( -5527*(r&0x7f) - 10921*(g&0x7f) + 16448*b ) >> 15) + 128;
    v = (( 16448*(r&0x7f) - 13783*(g&0x7f) - 2665*b ) >> 15) + 128;
    *u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    ++u16src;
    u32dst0++;
    u32dst1++;
    u32dst2++;
    --count;
  }
}


void effect_rgbscan_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT32 *u32src = (UINT32 *)src;
  UINT32 r,g,b,y,u,v;

  while (count) {
    r = RMASK32( RMASK32_SEMI(*u32src)); r>>=16;
    g = GMASK32( RMASK32_SEMI(*u32src)); g>>=8;
    b = BMASK32( RMASK32_SEMI(*u32src)); b>>=0;
    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
    v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
    *u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    r = RMASK32( GMASK32_SEMI(*u32src)); r>>=16;
    g = GMASK32( GMASK32_SEMI(*u32src)); g>>=8;
    b = BMASK32( GMASK32_SEMI(*u32src)); b>>=0;
    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
    v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
    *u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    r = RMASK32( BMASK32_SEMI(*u32src)); r>>=16;
    g = GMASK32( BMASK32_SEMI(*u32src)); g>>=8;
    b = BMASK32( BMASK32_SEMI(*u32src)); b>>=0;
    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
    v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
    *u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);

    ++u32src;
    u32dst0++;
    u32dst1++;
    u32dst2++;
    --count;
  }
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/**********************************
 * scan3
 **********************************/

/* All 3 lines are horizontally blurred a little
 * (the last pixel of each three in a line is averaged with the next pixel).
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%.
 */


void effect_scan3_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16dst2 = (UINT16 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u16dst0+1) = *(u16dst0+0) =
      SHADE16_HALF(u32lookup[*u16src]) + SHADE16_FOURTH(u32lookup[*u16src]);
    *(u16dst0+2) =
      SHADE16_HALF( MEAN16( u32lookup[*u16src], u32lookup[*(u16src+1)] ) )
      +
      SHADE16_FOURTH( MEAN16( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    *(u16dst1+0) = *(u16dst1+1) = u32lookup[*u16src];
    *(u16dst1+2) = MEAN16( u32lookup[*u16src], u32lookup[*(u16src+1)] );

    *(u16dst2+0) = *(u16dst2+1) =
      SHADE16_HALF(u32lookup[*u16src]);
    *(u16dst2+2) =
      SHADE16_HALF( MEAN16( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    ++u16src;
    u16dst0 += 3;
    u16dst1 += 3;
    u16dst2 += 3;
    --count;
  }
}


void effect_scan3_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16dst2 = (UINT16 *)dst2;
  UINT16 *u16src = (UINT16 *)src;

  while (count) {

    *(u16dst0+1) = *(u16dst0+0) =
      SHADE16_HALF(*u16src) + SHADE16_FOURTH(*u16src);
    *(u16dst0+2) =
      SHADE16_HALF( MEAN16( *u16src, *(u16src+1) ) )
      +
      SHADE16_FOURTH( MEAN16( *u16src, *(u16src+1) ) );

    *(u16dst1+0) = *(u16dst1+1) = *u16src;
    *(u16dst1+2) = MEAN16( *u16src, *(u16src+1) );

    *(u16dst2+0) = *(u16dst2+1) =
      SHADE16_HALF(*u16src);
    *(u16dst2+2) =
      SHADE16_HALF( MEAN16( *u16src, *(u16src+1) ) );

    ++u16src;
    u16dst0 += 3;
    u16dst1 += 3;
    u16dst2 += 3;
    --count;
  }
}


void effect_scan3_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u32dst0+1) = *(u32dst0+0) =
      SHADE32_HALF(u32lookup[*u16src]) + SHADE32_FOURTH(u32lookup[*u16src]);
    *(u32dst0+2) =
      SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) )
      +
      SHADE32_FOURTH( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    *(u32dst1+0) = *(u32dst1+1) = u32lookup[*u16src];
    *(u32dst1+2) = MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] );

    *(u32dst2+0) = *(u32dst2+1) =
      SHADE32_HALF(u32lookup[*u16src]);
    *(u32dst2+2) =
      SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    ++u16src;
    u32dst0 += 3;
    u32dst1 += 3;
    u32dst2 += 3;
    --count;
  }
}


void effect_scan3_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;

  while (count) {

    *(u32dst0+1) = *(u32dst0+0) =
      SHADE32_HALF(u32lookup[*u16src]) + SHADE32_FOURTH(u32lookup[*u16src]);
    *(u32dst0+2) =
      SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) )
      +
      SHADE32_FOURTH( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    *(u32dst1+0) = *(u32dst1+1) = u32lookup[*u16src];
    *(u32dst1+2) = MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] );

    *(u32dst2+0) = *(u32dst2+1) =
      SHADE32_HALF(u32lookup[*u16src]);
    *(u32dst2+2) =
      SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*(u16src+1)] ) );

    ++u16src;
    u32dst0 += 3;
    u32dst1 += 3;
    u32dst2 += 3;
    --count;
  }
}


void effect_scan3_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT32 *u32src = (UINT32 *)src;

  while (count) {

    *(u32dst0+1) = *(u32dst0+0) =
      SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
    *(u32dst0+2) =
      SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
      +
      SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );

    *(u32dst1+0) = *(u32dst1+1) = *u32src;
    *(u32dst1+2) = MEAN32( *u32src, *(u32src+1) );

    *(u32dst2+0) = *(u32dst2+1) =
      SHADE32_HALF(*u32src);
    *(u32dst2+2) =
      SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );

    ++u32src;
    u32dst0 += 3;
    u32dst1 += 3;
    u32dst2 += 3;
    --count;
  }
}


#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff

void effect_scan3_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 p1,p2,y1,uv1,uv2,y2,s;

  s = 1;
  while (count) {
    if (s) {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      *u32dst0 = ((y1*3/4)&255)|(((y1*3/4)&255)<<16)|uv1;
      if (count != 1) {
        p2 = u32lookup[*(u16src+1)];
        y2 = p2&255;
        y1 = ((y1>>1) + (y2>>1))&0x000000ff;
        uv2 = p2&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        *(u32dst0+1) = ((y1*3/4)&255)|(((y2*3/4)&255)<<16)|uv1;
      }
    } else {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      p2 = u32lookup[*(u16src+1)];
      y2 = p2&255;
      y2 = ((y1>>1) + (y2>>1))&0x000000ff;
      uv2 = p2&0xff00ff00;
      uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      *u32dst0 = ((y1*3/4)&255)|(((y2*3/4)&255)<<16)|uv1;
    }
    if (s) {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      *u32dst1 = (y1&255)|((y1&255)<<16)|uv1;
      if (count != 1) {
#if 0
        p2 = u32lookup[*(u16src+1)];
        y2 = p2&255;
        y1 = ((y1>>1) + (y2>>1))&0x000000ff;
        uv2 = p2&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        *(u32dst1+1) = (y1&255)|((y2&255)<<16)|uv1;
#endif
      }
    } else {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      p2 = u32lookup[*(u16src+1)];
      y2 = p2&255;
      y2 = ((y1>>1) + (y2>>1))&0x000000ff;
      uv2 = p2&0xff00ff00;
      uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      *u32dst1 = (y1&255)|((y2&255)<<16)|uv1;
    }
    if (s) {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      *u32dst2 = ((y1/2)&255)|(((y1/2)&255)<<16)|uv1;
      if (count != 1) {
#if 0
        p2 = u32lookup[*(u16src+1)];
        y2 = p2&255;
        y1 = ((y1>>1) + (y2>>1))&0x000000ff;
        uv2 = p2&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
        *(u32dst2+1) = ((y1/2)&255)|(((y2/2)&255)<<16)|uv1;
#endif
      }
    } else {
      p1 = u32lookup[*u16src];
      y1 = p1&255;
      uv1 = p1&0xff00ff00;
      p2 = u32lookup[*(u16src+1)];
      y2 = p2&255;
      y2 = ((y1>>1) + (y2>>1))&0x000000ff;
      uv2 = p2&0xff00ff00;
      uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
      *u32dst2 = ((y1/2)&255)|(((y2/2)&255)<<16)|uv1;
    }

    ++u16src;
    if (s) {
      u32dst0 += 2;
      u32dst1 += 2;
      u32dst2 += 2;
    } else {
      u32dst0++;
      u32dst1++;
      u32dst2++;
    }
    --count;
    s = !s;
  }
}


void effect_scan3_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32dst2 = (UINT32 *)dst2;
  UINT32 *u32src = (UINT32 *)src;
  UINT32 t,r,g,b,y,u,v,y2,s;

  s = 1;
  while (count) {
    if (s) {
      t = SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      *u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
      if (count != 1) {
        t =
          SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
          +
          SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
        v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
        t = SHADE32_HALF(*(u32src+1)) + SHADE32_FOURTH(*(u32src+1));
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
        v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
        *(u32dst0+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      }
    } else {
      t = SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      t =
        SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
        +
        SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
      v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
      *u32dst0 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
    }
    if (s) {
      r = RMASK32( *u32src); r>>=16;
      g = GMASK32( *u32src); g>>=8;
      b = BMASK32( *u32src); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      *u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
      if (count != 1) {
        t = MEAN32( *u32src, *(u32src+1) );
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
        v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
        r = RMASK32( *(u32src+1)); r>>=16;
        g = GMASK32( *(u32src+1)); g>>=8;
        b = BMASK32( *(u32src+1)); b>>=0;
        y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
        v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
        *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      }
    } else {
      r = RMASK32( *u32src); r>>=16;
      g = GMASK32( *u32src); g>>=8;
      b = BMASK32( *u32src); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      t = MEAN32( *u32src, *(u32src+1) );
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
      v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
      *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
    }
    if (s) {
      t = SHADE32_HALF(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      *u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
      if (count != 1) {
        t = SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
        v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
        t = SHADE32_HALF(*(u32src+1));
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
        v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
        *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      }
    } else {
      t = SHADE32_HALF(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      t = SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
      v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
      *u32dst2 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
    }
    ++u32src;
    if (s) {
      u32dst0 += 2;
      u32dst1 += 2;
      u32dst2 += 2;
    } else {
      u32dst0++;
      u32dst1++;
      u32dst2++;
    }
    --count;
    s = !s;
  }
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/**********************************
 * rotate
 **********************************/


void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y)
{
  int x;
  UINT16 * u16dst = (UINT16 *)dst;

  if (blit_swapxy) {
    if (blit_flipx && blit_flipy)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
    else if (blit_flipx)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - x - 1])[y];
    else if (blit_flipy)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[x])[bitmap->width - y - 1];
    else
      for (x = visual.min_x; x <= visual.max_x; x++)
        u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[x])[y];
  } else if (blit_flipx && blit_flipy)
    for (x = visual.min_x; x <= visual.max_x; x++)
      u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if (blit_flipx)
         for (x = visual.min_x; x <= visual.max_x; x++)
           u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[y])[bitmap->width - x - 1];
       else if (blit_flipy)
         for (x = visual.min_x; x <= visual.max_x; x++)
           u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - y -1])[x];
}


void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y)
{
  int x;
  UINT32 * u32dst = (UINT32 *)dst;

  if (blit_swapxy) {
    if (blit_flipx && blit_flipy)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
    else if (blit_flipx)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - x - 1])[y];
    else if (blit_flipy)
      for (x = visual.min_x; x <= visual.max_x; x++)
        u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[x])[bitmap->width - y - 1];
    else
      for (x = visual.min_x; x <= visual.max_x; x++)
        u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[x])[y];
  } else if (blit_flipx && blit_flipy)
    for (x = visual.min_x; x <= visual.max_x; x++)
      u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if (blit_flipx)
         for (x = visual.min_x; x <= visual.max_x; x++)
           u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[y])[bitmap->width - x - 1];
       else if (blit_flipy)
         for (x = visual.min_x; x <= visual.max_x; x++)
           u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - y -1])[x];
}


/*
 * hq2x algorithm (Pieter Hulshoff)
 * (C) 2003 by Maxim Stepin (www.hiend3d.com/hq2x.html)
 *
 * hq2x is a fast, high-quality 2x magnification filter.
 *
 * The first step is an analysis of the 3x3 area of the source pixel. At
 * first, we calculate the color difference between the central pixel and
 * its 8 nearest neighbors. Then that difference is compared to a predefined
 * threshold, and these pixels are sorted into two categories: "close" and
 * "distant" colored. There are 8 neighbors, so we are getting 256 possible
 * combinations.
 *
 * For the next step, which is filtering, a lookup table with 256 entries is
 * used, one entry per each combination of close/distant colored neighbors.
 * Each entry describes how to mix the colors of the source pixels from 3x3
 * area to get interpolated pixels of the filtered image.
 *
 * The present implementation is using YUV color space to calculate color
 * differences, with more tolerance on Y (brightness) component, then on
 * color components U and V.
 *
 * Creating a lookup table was the most difficult part - for each
 * combination the most probable vector representation of the area has to be
 * determined, with the idea of edges between the different colored areas of
 * the image to be preserved, with the edge direction to be as close to a
 * correct one as possible. That vector representation is then rasterised
 * with higher (2x) resolution using anti-aliasing, and the result is stored
 * in the lookup table.
 * The filter was not designed for photographs, but for images with clear
 * sharp edges, like line graphics or cartoon sprites. It was also designed
 * to be fast enough to process 256x256 images in real-time.
 */


int is_distant_16( UINT16 w1, UINT16 w2 )
{
  int yuv1, yuv2;

  yuv1 = rgb2yuv[w1];
  yuv2 = rgb2yuv[w2];
  return ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
           ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
           ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) );
}


int is_distant_32( UINT32 w1, UINT32 w2 )
{
  int r1, g1, b1, r2, g2, b2;

  r1 = (w1 & rmask32) >> 16;
  g1 = (w1 & gmask32) >> 8;
  b1 = (w1 & bmask32);
  r2 = (w2 & rmask32) >> 16;
  g2 = (w2 & gmask32) >> 8;
  b2 = (w2 & bmask32);

  return ( ( abs( (r1+g1+b1)    - (r2+g2+b2) )    > trY32 ) ||
           ( abs( (r1-b1)       - (r2-b2)    )    > trU32 ) ||
           ( abs( (-r1+2*g1-b1) - (-r2+2*g2-b2) ) > trV32 ) );
}


int is_distant_YUY2( UINT32 yuv1, UINT32 yuv2 )
{
  return ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
           ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
           ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) );
}


void hq2x_16( UINT16 *u16dst0, UINT16 *u16dst1, UINT16 w[9] )
{
  int    pattern = 0;
  int    flag    = 1;
  int    c;
  int    yuv1, yuv2;

  yuv1 = rgb2yuv[w[4]];
  for ( c = 0; c <= 8; c++ )
  {
    if ( c == 4 )
      continue;
    yuv2 = rgb2yuv[w[c]];
    if ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
         ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
         ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) )
      pattern |= flag;
    flag <<= 1;
  }

  switch ( pattern )
  {
    case 0 :
    case 1 :
    case 4 :
    case 5 :
    case 32 :
    case 33 :
    case 36 :
    case 37 :
    case 128 :
    case 129 :
    case 132 :
    case 133 :
    case 160 :
    case 161 :
    case 164 :
    case 165 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 2 :
    case 34 :
    case 130 :
    case 162 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 3 :
    case 35 :
    case 131 :
    case 163 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 6 :
    case 38 :
    case 134 :
    case 166 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 7 :
    case 39 :
    case 135 :
    case 167 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 8 :
    case 12 :
    case 136 :
    case 140 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 9 :
    case 13 :
    case 137 :
    case 141 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 10 :
    case 138 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 11 :
    case 139 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 15 :
    case 143 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 16 :
    case 17 :
    case 48 :
    case 49 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 18 :
    case 50 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 20 :
    case 21 :
    case 52 :
    case 53 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 22 :
    case 54 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 23 :
    case 55 :
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 24 :
    case 66 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 25 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 26 :
    case 31 :
    case 95 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 27 :
    case 75 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 28 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 29 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 30 :
    case 86 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 40 :
    case 44 :
    case 168 :
    case 172 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 41 :
    case 45 :
    case 169 :
    case 173 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 43 :
    case 171 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = w[4];
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 46 :
    case 174 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 56 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 57 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 58 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 59 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 60 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 61 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 62 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 63 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 64 :
    case 65 :
    case 68 :
    case 69 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 67 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 70 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 71 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 72 :
    case 76 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 73 :
    case 77 :
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 74 :
    case 107 :
    case 123 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 78 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 79 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 80 :
    case 81 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 83 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 84 :
    case 85 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 87 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 89 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 90 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 91 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 92 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 93 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 94 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 96 :
    case 97 :
    case 100 :
    case 101 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 98 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 99 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 102 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 103 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 104 :
    case 108 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 105 :
    case 109 :
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *u16dst1     = w[4];
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 106 :
    case 120 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 110 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 111 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 112 :
    case 113 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 114 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 115 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 116 :
    case 117 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 118 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 119 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 121 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 122 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 124 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 125 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *u16dst1     = w[4];
      }
      else
      {
        *u16dst0     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 126 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 127 :
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[8] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[8] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 144 :
    case 145 :
    case 176 :
    case 177 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 146 :
    case 178 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 147 :
    case 179 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 148 :
    case 149 :
    case 180 :
    case 181 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 150 :
    case 182 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *(u16dst0+1) = w[4];
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 151 :
    case 183 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 152 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 153 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 154 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 155 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 156 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 157 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 158 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 159 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 184 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 185 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 186 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 187 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = w[4];
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[3] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[3] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 188 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 189 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 190 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
      {
        *(u16dst0+1) = w[4];
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 191 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 192 :
    case 193 :
    case 196 :
    case 197 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 194 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 195 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 198 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 199 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 200 :
    case 204 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 201 :
    case 205 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 202 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 203 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 206 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 207 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[1] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[1] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 208 :
    case 209 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 210 :
    case 216 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 211 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 212 :
    case 213 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = w[4];
      }
      else
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 215 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 217 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 218 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 219 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 220 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 221 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = w[4];
      }
      else
      {
        *(u16dst0+1) = ((((w[4] & gmask16)*5 + (w[5] & gmask16)*2 + (w[1] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[5] & pmask16)*2 + (w[1] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 223 :
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[6] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[6] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 224 :
    case 225 :
    case 228 :
    case 229 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 226 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 227 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 230 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 231 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 232 :
    case 236 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 233 :
    case 237 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 234 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *u16dst0     = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 235 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 238 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      else
      {
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      }
      break;
    case 239 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst1+1) = ((((w[4] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 240 :
    case 241 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 242 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 243 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
      {
        *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[4] & gmask16)*5 + (w[7] & gmask16)*2 + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*5 + (w[7] & pmask16)*2 + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 244 :
    case 245 :
      *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 246 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 247 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      *u16dst1     = ((((w[4] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 249 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 251 :
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[2] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[2] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 252 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 253 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 254 :
      *u16dst0     = ((((w[4] & gmask16)*3 + (w[0] & gmask16)) & (gmask16 << 2)) |
                      (((w[4] & pmask16)*3 + (w[0] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 255 :
      if ( is_distant_16( w[3], w[7] ) )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[5], w[7] ) )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[3] ) )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( is_distant_16( w[1], w[5] ) )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
  }

  return;
}


void hq2x_32( UINT32 *u32dst0, UINT32 *u32dst1, UINT32 w[9] )
{

  int    pattern = 0;
  int    flag    = 1;
  int    c;
  int    r1, g1, b1, r2, g2, b2, y1, u1, v1;

  r1 = (w[4] & rmask32) >> 16;
  g1 = (w[4] & gmask32) >> 8;
  b1 = (w[4] & bmask32);
  y1 = r1+g1+b1;
  u1 = r1-b1;
  v1 = -r1+2*g1-b1;
  for ( c = 0; c <= 8; c++ )
  {
    if ( c == 4 )
      continue;
    r2 = (w[c] & rmask32) >> 16;
    g2 = (w[c] & gmask32) >> 8;
    b2 = (w[c] & bmask32);
    if ( ( abs( y1 - (r2+g2+b2) )    > trY32 ) ||
         ( abs( u1 - (r2-b2)    )    > trU32 ) ||
         ( abs( v1 - (-r2+2*g2-b2) ) > trV32 ) )
      pattern |= flag;
    flag <<= 1;
  }

  switch ( pattern )
  {
    case 0 :
    case 1 :
    case 4 :
    case 5 :
    case 32 :
    case 33 :
    case 36 :
    case 37 :
    case 128 :
    case 129 :
    case 132 :
    case 133 :
    case 160 :
    case 161 :
    case 164 :
    case 165 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 2 :
    case 34 :
    case 130 :
    case 162 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 3 :
    case 35 :
    case 131 :
    case 163 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 6 :
    case 38 :
    case 134 :
    case 166 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 7 :
    case 39 :
    case 135 :
    case 167 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 8 :
    case 12 :
    case 136 :
    case 140 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 9 :
    case 13 :
    case 137 :
    case 141 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 10 :
    case 138 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 11 :
    case 139 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 15 :
    case 143 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 16 :
    case 17 :
    case 48 :
    case 49 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 18 :
    case 50 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 20 :
    case 21 :
    case 52 :
    case 53 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 22 :
    case 54 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 23 :
    case 55 :
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 24 :
    case 66 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 25 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 26 :
    case 31 :
    case 95 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 27 :
    case 75 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 28 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 29 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 30 :
    case 86 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 40 :
    case 44 :
    case 168 :
    case 172 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 41 :
    case 45 :
    case 169 :
    case 173 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 43 :
    case 171 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 46 :
    case 174 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 56 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 57 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 58 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 59 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 60 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 61 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 62 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 63 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 64 :
    case 65 :
    case 68 :
    case 69 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 67 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 70 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 71 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 72 :
    case 76 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 73 :
    case 77 :
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 74 :
    case 107 :
    case 123 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 78 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 79 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 80 :
    case 81 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 83 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 84 :
    case 85 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 87 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 89 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 90 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 91 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 92 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 93 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 94 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 96 :
    case 97 :
    case 100 :
    case 101 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 98 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 99 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 102 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 103 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 104 :
    case 108 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 105 :
    case 109 :
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 106 :
    case 120 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 110 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 111 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 112 :
    case 113 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 114 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 115 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 116 :
    case 117 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 118 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 119 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 121 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 122 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 124 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 125 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 126 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 127 :
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 144 :
    case 145 :
    case 176 :
    case 177 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 146 :
    case 178 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 147 :
    case 179 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 148 :
    case 149 :
    case 180 :
    case 181 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 150 :
    case 182 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 151 :
    case 183 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 152 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 153 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 154 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 155 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 156 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 157 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 158 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 159 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 184 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 185 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 186 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 187 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 188 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 189 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 190 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 191 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 192 :
    case 193 :
    case 196 :
    case 197 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 194 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 195 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 198 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 199 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 200 :
    case 204 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 201 :
    case 205 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 202 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 203 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 206 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 207 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 208 :
    case 209 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 210 :
    case 216 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 211 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 212 :
    case 213 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 215 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 217 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 218 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 219 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 220 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 221 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 223 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 224 :
    case 225 :
    case 228 :
    case 229 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 226 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 227 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 230 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 231 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 232 :
    case 236 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 233 :
    case 237 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 234 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 235 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 238 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 239 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 240 :
    case 241 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 242 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 243 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 244 :
    case 245 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 246 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 247 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 249 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 251 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 252 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 253 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 254 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 255 :
      if ( is_distant_32( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_32( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
  }

  return;
}


void hq2x_YUY2( UINT32 *u32dst0, UINT32 *u32dst1, UINT32 w[9] )
{
  int    pattern = 0;
  int    flag    = 1;
  int    c;
  int    yuv1,yuv2;

  yuv1 = w[4];
  for ( c = 0; c <= 8; c++ )
  {
    if ( c == 4 )
      continue;
    yuv2 = w[c];
    if ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
         ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
         ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) )
      pattern |= flag;
    flag <<= 1;
  }

  switch ( pattern )
  {
    case 0 :
    case 1 :
    case 4 :
    case 5 :
    case 32 :
    case 33 :
    case 36 :
    case 37 :
    case 128 :
    case 129 :
    case 132 :
    case 133 :
    case 160 :
    case 161 :
    case 164 :
    case 165 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 2 :
    case 34 :
    case 130 :
    case 162 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 3 :
    case 35 :
    case 131 :
    case 163 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 6 :
    case 38 :
    case 134 :
    case 166 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 7 :
    case 39 :
    case 135 :
    case 167 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 8 :
    case 12 :
    case 136 :
    case 140 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 9 :
    case 13 :
    case 137 :
    case 141 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 10 :
    case 138 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 11 :
    case 139 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 15 :
    case 143 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 16 :
    case 17 :
    case 48 :
    case 49 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 18 :
    case 50 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 20 :
    case 21 :
    case 52 :
    case 53 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 22 :
    case 54 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 23 :
    case 55 :
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 24 :
    case 66 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 25 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 26 :
    case 31 :
    case 95 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 27 :
    case 75 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 28 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 29 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 30 :
    case 86 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 40 :
    case 44 :
    case 168 :
    case 172 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 41 :
    case 45 :
    case 169 :
    case 173 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 43 :
    case 171 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 46 :
    case 174 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 56 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 57 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 58 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 59 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 60 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 61 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 62 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 63 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 64 :
    case 65 :
    case 68 :
    case 69 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 67 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 70 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 71 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 72 :
    case 76 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 73 :
    case 77 :
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 74 :
    case 107 :
    case 123 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 78 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 79 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 80 :
    case 81 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 83 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 84 :
    case 85 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 87 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 89 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 90 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 91 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 92 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 93 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 94 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 96 :
    case 97 :
    case 100 :
    case 101 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 98 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 99 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 102 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 103 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 104 :
    case 108 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 105 :
    case 109 :
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 106 :
    case 120 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 110 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 111 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 112 :
    case 113 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 114 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 115 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 116 :
    case 117 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 118 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 119 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 121 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 122 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 124 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 125 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 126 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 127 :
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[8] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[8] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 144 :
    case 145 :
    case 176 :
    case 177 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 146 :
    case 178 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 147 :
    case 179 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 148 :
    case 149 :
    case 180 :
    case 181 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 150 :
    case 182 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 151 :
    case 183 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 152 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 153 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 154 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 155 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 156 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 157 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 158 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 159 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 184 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 185 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 186 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 187 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[3] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[3] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 188 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 189 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 190 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 191 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 192 :
    case 193 :
    case 196 :
    case 197 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 194 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 195 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 198 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 199 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 200 :
    case 204 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 201 :
    case 205 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 202 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 203 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 206 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 207 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[1] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[1] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 208 :
    case 209 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 210 :
    case 216 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 211 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 212 :
    case 213 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 215 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 217 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 218 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 219 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 220 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 221 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[4] & gmask32)*5 + (w[5] & gmask32)*2 + (w[1] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[5] & pmask32)*2 + (w[1] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 223 :
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[6] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[6] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 224 :
    case 225 :
    case 228 :
    case 229 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 226 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 227 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 230 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 231 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 232 :
    case 236 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 233 :
    case 237 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 234 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *u32dst0     = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 235 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 238 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      }
      break;
    case 239 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst1+1) = ((((w[4] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 240 :
    case 241 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 242 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 243 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
      {
        *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[4] & gmask32)*5 + (w[7] & gmask32)*2 + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*5 + (w[7] & pmask32)*2 + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 244 :
    case 245 :
      *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 246 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 247 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      *u32dst1     = ((((w[4] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 249 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 251 :
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[2] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[2] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 252 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 253 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 254 :
      *u32dst0     = ((((w[4] & gmask32)*3 + (w[0] & gmask32)) & (gmask32 << 2)) |
                      (((w[4] & pmask32)*3 + (w[0] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 255 :
      if ( is_distant_YUY2( w[3], w[7] ) )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[5], w[7] ) )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[3] ) )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( is_distant_YUY2( w[1], w[5] ) )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
  }

  return;
}


void effect_hq2x_16_16
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT16 *u16dst0   = (UINT16 *)dst0;
  UINT16 *u16dst1   = (UINT16 *)dst1;
  UINT16 *u16src0   = (UINT16 *)src0;
  UINT16 *u16src1   = (UINT16 *)src1;
  UINT16 *u16src2   = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT16  w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32lookup[u16src0[ 1]];
    w[5] = u32lookup[u16src1[ 1]];
    w[8] = u32lookup[u16src2[ 1]];

    hq2x_16( u16dst0, u16dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u16dst0 += 2;
    u16dst1 += 2;
  }
}


void effect_hq2x_16_16_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT16  w[9];

  w[1] = u16src0[-1];
  w[2] = u16src0[ 0];
  w[4] = u16src1[-1];
  w[5] = u16src1[ 0];
  w[7] = u16src2[-1];
  w[8] = u16src2[ 0];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u16src0[ 1];
    w[5] = u16src1[ 1];
    w[8] = u16src2[ 1];

    hq2x_16( u16dst0, u16dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u16dst0 += 2;
    u16dst1 += 2;
  }
}


#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff
void effect_hq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
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
    unsigned count)
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

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/* FIXME: this probably doesn't work right for 24 bit */
void effect_hq2x_16_24
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0   = (UINT16 *)src0;
  UINT16 *u16src1   = (UINT16 *)src1;
  UINT16 *u16src2   = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32  w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32lookup[u16src0[ 1]];
    w[5] = u32lookup[u16src1[ 1]];
    w[8] = u32lookup[u16src2[ 1]];

    hq2x_32( u32dst0, u32dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}


void effect_hq2x_16_32
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0   = (UINT16 *)src0;
  UINT16 *u16src1   = (UINT16 *)src1;
  UINT16 *u16src2   = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32  w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32lookup[u16src0[ 1]];
    w[5] = u32lookup[u16src1[ 1]];
    w[8] = u32lookup[u16src2[ 1]];

    hq2x_32( u32dst0, u32dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}


void effect_hq2x_32_32_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32src0 = (UINT32 *)src0;
  UINT32 *u32src1 = (UINT32 *)src1;
  UINT32 *u32src2 = (UINT32 *)src2;
  UINT32  w[9];

  w[1] = u32src0[-1];
  w[2] = u32src0[ 0];
  w[4] = u32src1[-1];
  w[5] = u32src1[ 0];
  w[7] = u32src2[-1];
  w[8] = u32src2[ 0];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32src0[ 1];
    w[5] = u32src1[ 1];
    w[8] = u32src2[ 1];

    hq2x_32( u32dst0, u32dst1, w );

    ++u32src0;
    ++u32src1;
    ++u32src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}


/*
 * lq2x algorithm (Pieter Hulshoff)
 * (C) 2003 by Andrea Mazzoleni (http://advancemame.sourceforge.net)
 *
 * lq2x is a fast, low-quality 2x magnification filter.
 *
 * The first step is an analysis of the 3x3 area of the source pixel. At
 * first, we calculate the color difference between the central pixel and
 * its 8 nearest neighbors. Then that difference is compared to a predefined
 * threshold, and these pixels are sorted into two categories: "equal" and
 * "unequel" colored. There are 8 neighbors, so we are getting 256 possible
 * combinations.
 *
 * For the next step, which is filtering, a lookup table with 256 entries is
 * used, one entry per each combination of close/distant colored neighbors.
 * Each entry describes how to mix the colors of the source pixels from 3x3
 * area to get interpolated pixels of the filtered image.
 *
 * The present implementation is using YUV color space to calculate color
 * differences, with more tolerance on Y (brightness) component, then on
 * color components U and V.
 *
 * Creating a lookup table was the most difficult part - for each
 * combination the most probable vector representation of the area has to be
 * determined, with the idea of edges between the different colored areas of
 * the image to be preserved, with the edge direction to be as close to a
 * correct one as possible. That vector representation is then rasterised
 * with higher (2x) resolution using anti-aliasing, and the result is stored
 * in the lookup table.
 * The filter was not designed for photographs, but for images with clear
 * sharp edges, like line graphics or cartoon sprites. It was also designed
 * to be fast enough to process 256x256 images in real-time.
 */


void lq2x_16( UINT16 *u16dst0, UINT16 *u16dst1, UINT16 w[9] )
{
  int    pattern = 0;

  if ( w[4] != w[0] )
    pattern |= 1;
  if ( w[4] != w[1] )
    pattern |= 2;
  if ( w[4] != w[2] )
    pattern |= 4;
  if ( w[4] != w[3] )
    pattern |= 8;
  if ( w[4] != w[5] )
    pattern |= 16;
  if ( w[4] != w[6] )
    pattern |= 32;
  if ( w[4] != w[7] )
    pattern |= 64;
  if ( w[4] != w[8] )
    pattern |= 128;

  switch ( pattern )
  {
    case 0 :
    case 2 :
    case 4 :
    case 6 :
    case 8 :
    case 12 :
    case 16 :
    case 20 :
    case 24 :
    case 28 :
    case 32 :
    case 34 :
    case 36 :
    case 38 :
    case 40 :
    case 44 :
    case 48 :
    case 52 :
    case 56 :
    case 60 :
    case 64 :
    case 66 :
    case 68 :
    case 70 :
    case 96 :
    case 98 :
    case 100 :
    case 102 :
    case 128 :
    case 130 :
    case 132 :
    case 134 :
    case 136 :
    case 140 :
    case 144 :
    case 148 :
    case 152 :
    case 156 :
    case 160 :
    case 162 :
    case 164 :
    case 166 :
    case 168 :
    case 172 :
    case 176 :
    case 180 :
    case 184 :
    case 188 :
    case 192 :
    case 194 :
    case 196 :
    case 198 :
    case 224 :
    case 226 :
    case 228 :
    case 230 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      break;
    case 1 :
    case 5 :
    case 9 :
    case 13 :
    case 17 :
    case 21 :
    case 25 :
    case 29 :
    case 33 :
    case 37 :
    case 41 :
    case 45 :
    case 49 :
    case 53 :
    case 57 :
    case 61 :
    case 65 :
    case 69 :
    case 97 :
    case 101 :
    case 129 :
    case 133 :
    case 137 :
    case 141 :
    case 145 :
    case 149 :
    case 153 :
    case 157 :
    case 161 :
    case 165 :
    case 169 :
    case 173 :
    case 177 :
    case 181 :
    case 185 :
    case 189 :
    case 193 :
    case 197 :
    case 225 :
    case 229 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      break;
    case 3 :
    case 35 :
    case 67 :
    case 99 :
    case 131 :
    case 163 :
    case 195 :
    case 227 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      break;
    case 7 :
    case 39 :
    case 71 :
    case 103 :
    case 135 :
    case 167 :
    case 199 :
    case 231 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      break;
    case 10 :
    case 138 :
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 11 :
    case 27 :
    case 75 :
    case 139 :
    case 155 :
    case 203 :
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[0] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 15 :
    case 143 :
    case 207 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[4] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[4] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst0+1) = ((((w[4] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 18 :
    case 22 :
    case 30 :
    case 50 :
    case 54 :
    case 62 :
    case 86 :
    case 118 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[2] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[2] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[2] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 23 :
    case 55 :
    case 119 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *u16dst0     = w[4];
        *(u16dst0+1) = w[4];
      }
      else
      {
        *u16dst0     = ((((w[3] & gmask16)*3 + (w[1] & gmask16)) & (gmask16 << 2)) |
                        (((w[3] & pmask16)*3 + (w[1] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[3] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[3] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 26 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 31 :
    case 95 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u16dst0     = w[4];
        *u16dst1     = w[4];
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[0] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 43 :
    case 171 :
    case 187 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u16dst0     = w[4];
        *u16dst1     = w[4];
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)*3 + (w[2] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)*3 + (w[2] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *u16dst1     = ((((w[2] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 46 :
    case 174 :
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 58 :
    case 154 :
    case 186 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 59 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[2] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 63 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 72 :
    case 76 :
    case 104 :
    case 106 :
    case 108 :
    case 110 :
    case 120 :
    case 124 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 73 :
    case 77 :
    case 105 :
    case 109 :
    case 125 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
      {
        *u16dst0     = w[4];
        *u16dst1     = w[4];
      }
      else
      {
        *u16dst0     = ((((w[1] & gmask16)*3 + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*3 + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[1] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[1] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 74 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 78 :
    case 202 :
    case 206 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 79 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[4] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 80 :
    case 208 :
    case 210 :
    case 216 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 81 :
    case 209 :
    case 217 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 83 :
    case 115 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[2] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[2] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 84 :
    case 212 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(u16dst0+1) = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *(u16dst0+1) = ((((w[0] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 85 :
    case 213 :
    case 221 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(u16dst0+1) = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[1] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[1] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 87 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[3] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[3] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[3] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 89 :
    case 93 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 90 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 91 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[2] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[2] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[2] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 92 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 94 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 107 :
    case 123 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[2] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 111 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 112 :
    case 240 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[0] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 113 :
    case 241 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[1] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[1] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[1] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 114 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 116 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 117 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 121 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 122 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*6 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 126 :
      *u16dst0     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 127 :
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 146 :
    case 150 :
    case 178 :
    case 182 :
    case 190 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[1] != w[5] )
      {
        *(u16dst0+1) = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *(u16dst0+1) = ((((w[1] & gmask16)*3 + (w[5] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*3 + (w[5] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[0] & gmask16)*3 + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 147 :
    case 179 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[2] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[2] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 151 :
    case 183 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[3] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[3] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 158 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 159 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 191 :
      *u16dst1     = w[4];
      *(u16dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 200 :
    case 204 :
    case 232 :
    case 236 :
    case 238 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[3] & gmask16)*3 + (w[7] & gmask16)*3 + (w[0] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[3] & pmask16)*3 + (w[7] & pmask16)*3 + (w[0] & pmask16)*2) & (pmask16 << 3))) >> 3;
        *(u16dst1+1) = ((((w[0] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      }
      break;
    case 201 :
    case 205 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[1] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 211 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[2] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 215 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[3] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[3] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[3] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[3] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 218 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 219 :
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[2] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 220 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*6 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 223 :
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[4] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 233 :
    case 237 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[1] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 234 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 235 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[2] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[2] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 239 :
      *(u16dst0+1) = w[4];
      *(u16dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 242 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*6 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 3)) |
                        (((w[0] & pmask16)*6 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 3))) >> 3;
      break;
    case 243 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u16dst1     = w[4];
        *(u16dst1+1) = w[4];
      }
      else
      {
        *u16dst1     = ((((w[2] & gmask16)*3 + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*3 + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
        *(u16dst1+1) = ((((w[5] & gmask16)*3 + (w[7] & gmask16)*3 + (w[2] & gmask16)*2) & (gmask16 << 3)) |
                        (((w[5] & pmask16)*3 + (w[7] & pmask16)*3 + (w[2] & pmask16)*2) & (pmask16 << 3))) >> 3;
      }
      break;
    case 244 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[0] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 245 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[1] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 246 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[0] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 247 :
      *u16dst0     = w[4];
      *u16dst1     = w[4];
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[3] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[3] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[3] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[3] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 249 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[1] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[1] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 251 :
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[2] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[2] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[2] & gmask16)*2 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[2] & gmask16)*2 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 2)) |
                        (((w[2] & pmask16)*2 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 252 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[0] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 253 :
      *u16dst0     = w[4];
      *(u16dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[1] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[1] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[1] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[1] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
    case 254 :
      *u16dst0     = w[4];
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[0] & gmask16)*2 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[0] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[0] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[0] & gmask16)*2 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 2)) |
                        (((w[0] & pmask16)*2 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 2))) >> 2;
      break;
    case 255 :
      if ( w[7] != w[3] )
        *u16dst1     = w[4];
      else
        *u16dst1     = ((((w[4] & gmask16)*14 + (w[3] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[3] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u16dst1+1) = w[4];
      else
        *(u16dst1+1) = ((((w[4] & gmask16)*14 + (w[5] & gmask16) + (w[7] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[5] & pmask16) + (w[7] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u16dst0     = w[4];
      else
        *u16dst0     = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[3] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[3] & pmask16)) & (pmask16 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u16dst0+1) = w[4];
      else
        *(u16dst0+1) = ((((w[4] & gmask16)*14 + (w[1] & gmask16) + (w[5] & gmask16)) & (gmask16 << 4)) |
                        (((w[4] & pmask16)*14 + (w[1] & pmask16) + (w[5] & pmask16)) & (pmask16 << 4))) >> 4;
      break;
  }
  return;
}


void lq2x_32( UINT32 *u32dst0, UINT32 *u32dst1, UINT32 w[9] )
{
  int    pattern = 0;

  if ( w[4] != w[0] )
    pattern |= 1;
  if ( w[4] != w[1] )
    pattern |= 2;
  if ( w[4] != w[2] )
    pattern |= 4;
  if ( w[4] != w[3] )
    pattern |= 8;
  if ( w[4] != w[5] )
    pattern |= 16;
  if ( w[4] != w[6] )
    pattern |= 32;
  if ( w[4] != w[7] )
    pattern |= 64;
  if ( w[4] != w[8] )
    pattern |= 128;

  switch ( pattern )
  {
    case 0 :
    case 2 :
    case 4 :
    case 6 :
    case 8 :
    case 12 :
    case 16 :
    case 20 :
    case 24 :
    case 28 :
    case 32 :
    case 34 :
    case 36 :
    case 38 :
    case 40 :
    case 44 :
    case 48 :
    case 52 :
    case 56 :
    case 60 :
    case 64 :
    case 66 :
    case 68 :
    case 70 :
    case 96 :
    case 98 :
    case 100 :
    case 102 :
    case 128 :
    case 130 :
    case 132 :
    case 134 :
    case 136 :
    case 140 :
    case 144 :
    case 148 :
    case 152 :
    case 156 :
    case 160 :
    case 162 :
    case 164 :
    case 166 :
    case 168 :
    case 172 :
    case 176 :
    case 180 :
    case 184 :
    case 188 :
    case 192 :
    case 194 :
    case 196 :
    case 198 :
    case 224 :
    case 226 :
    case 228 :
    case 230 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      break;
    case 1 :
    case 5 :
    case 9 :
    case 13 :
    case 17 :
    case 21 :
    case 25 :
    case 29 :
    case 33 :
    case 37 :
    case 41 :
    case 45 :
    case 49 :
    case 53 :
    case 57 :
    case 61 :
    case 65 :
    case 69 :
    case 97 :
    case 101 :
    case 129 :
    case 133 :
    case 137 :
    case 141 :
    case 145 :
    case 149 :
    case 153 :
    case 157 :
    case 161 :
    case 165 :
    case 169 :
    case 173 :
    case 177 :
    case 181 :
    case 185 :
    case 189 :
    case 193 :
    case 197 :
    case 225 :
    case 229 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      break;
    case 3 :
    case 35 :
    case 67 :
    case 99 :
    case 131 :
    case 163 :
    case 195 :
    case 227 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      break;
    case 7 :
    case 39 :
    case 71 :
    case 103 :
    case 135 :
    case 167 :
    case 199 :
    case 231 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      break;
    case 10 :
    case 138 :
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 11 :
    case 27 :
    case 75 :
    case 139 :
    case 155 :
    case 203 :
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[0] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 15 :
    case 143 :
    case 207 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[4] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[4] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst0+1) = ((((w[4] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 18 :
    case 22 :
    case 30 :
    case 50 :
    case 54 :
    case 62 :
    case 86 :
    case 118 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[2] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[2] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[2] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 23 :
    case 55 :
    case 119 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *u32dst0     = w[4];
        *(u32dst0+1) = w[4];
      }
      else
      {
        *u32dst0     = ((((w[3] & gmask32)*3 + (w[1] & gmask32)) & (gmask32 << 2)) |
                        (((w[3] & pmask32)*3 + (w[1] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[3] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[3] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 26 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 31 :
    case 95 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u32dst0     = w[4];
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[0] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 43 :
    case 171 :
    case 187 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *u32dst0     = w[4];
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)*3 + (w[2] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)*3 + (w[2] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *u32dst1     = ((((w[2] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 46 :
    case 174 :
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 58 :
    case 154 :
    case 186 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 59 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[2] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 63 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 72 :
    case 76 :
    case 104 :
    case 106 :
    case 108 :
    case 110 :
    case 120 :
    case 124 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 73 :
    case 77 :
    case 105 :
    case 109 :
    case 125 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
      {
        *u32dst0     = w[4];
        *u32dst1     = w[4];
      }
      else
      {
        *u32dst0     = ((((w[1] & gmask32)*3 + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*3 + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[1] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[1] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 74 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 78 :
    case 202 :
    case 206 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 79 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[4] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 80 :
    case 208 :
    case 210 :
    case 216 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 81 :
    case 209 :
    case 217 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 83 :
    case 115 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[2] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[2] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 84 :
    case 212 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[0] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 85 :
    case 213 :
    case 221 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[1] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[1] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 87 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[3] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[3] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[3] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 89 :
    case 93 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 90 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 91 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[2] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[2] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[2] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 92 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 94 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 107 :
    case 123 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[2] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 111 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 112 :
    case 240 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[0] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 113 :
    case 241 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[1] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[1] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[1] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 114 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 116 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 117 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 121 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 122 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*6 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 126 :
      *u32dst0     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 127 :
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 146 :
    case 150 :
    case 178 :
    case 182 :
    case 190 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[1] != w[5] )
      {
        *(u32dst0+1) = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *(u32dst0+1) = ((((w[1] & gmask32)*3 + (w[5] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*3 + (w[5] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[0] & gmask32)*3 + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 147 :
    case 179 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[2] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[2] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 151 :
    case 183 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[3] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[3] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 158 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 159 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 191 :
      *u32dst1     = w[4];
      *(u32dst1+1) = w[4];
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 200 :
    case 204 :
    case 232 :
    case 236 :
    case 238 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[3] & gmask32)*3 + (w[7] & gmask32)*3 + (w[0] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[3] & pmask32)*3 + (w[7] & pmask32)*3 + (w[0] & pmask32)*2) & (pmask32 << 3))) >> 3;
        *(u32dst1+1) = ((((w[0] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      }
      break;
    case 201 :
    case 205 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[1] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 211 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[2] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 215 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[3] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[3] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[3] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[3] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 218 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 219 :
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[2] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 220 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*6 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 3))) >> 3;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 223 :
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[4] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 233 :
    case 237 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[1] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 234 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 235 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[2] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[2] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 239 :
      *(u32dst0+1) = w[4];
      *(u32dst1+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 242 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*6 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 3)) |
                        (((w[0] & pmask32)*6 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 3))) >> 3;
      break;
    case 243 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *u32dst1     = w[4];
        *(u32dst1+1) = w[4];
      }
      else
      {
        *u32dst1     = ((((w[2] & gmask32)*3 + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*3 + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
        *(u32dst1+1) = ((((w[5] & gmask32)*3 + (w[7] & gmask32)*3 + (w[2] & gmask32)*2) & (gmask32 << 3)) |
                        (((w[5] & pmask32)*3 + (w[7] & pmask32)*3 + (w[2] & pmask32)*2) & (pmask32 << 3))) >> 3;
      }
      break;
    case 244 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[0] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 245 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[1] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 246 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[0] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 247 :
      *u32dst0     = w[4];
      *u32dst1     = w[4];
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[3] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[3] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[3] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[3] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 249 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[1] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[1] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 251 :
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[2] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[2] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[2] & gmask32)*2 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[2] & gmask32)*2 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 2)) |
                        (((w[2] & pmask32)*2 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 252 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[0] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 253 :
      *u32dst0     = w[4];
      *(u32dst0+1) = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[1] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[1] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[1] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[1] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
    case 254 :
      *u32dst0     = w[4];
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[0] & gmask32)*2 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 2))) >> 2;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[0] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[0] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[0] & gmask32)*2 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 2)) |
                        (((w[0] & pmask32)*2 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 2))) >> 2;
      break;
    case 255 :
      if ( w[7] != w[3] )
        *u32dst1     = w[4];
      else
        *u32dst1     = ((((w[4] & gmask32)*14 + (w[3] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[3] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[5] != w[7] )
        *(u32dst1+1) = w[4];
      else
        *(u32dst1+1) = ((((w[4] & gmask32)*14 + (w[5] & gmask32) + (w[7] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[5] & pmask32) + (w[7] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[3] != w[1] )
        *u32dst0     = w[4];
      else
        *u32dst0     = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[3] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[3] & pmask32)) & (pmask32 << 4))) >> 4;
      if ( w[1] != w[5] )
        *(u32dst0+1) = w[4];
      else
        *(u32dst0+1) = ((((w[4] & gmask32)*14 + (w[1] & gmask32) + (w[5] & gmask32)) & (gmask32 << 4)) |
                        (((w[4] & pmask32)*14 + (w[1] & pmask32) + (w[5] & pmask32)) & (pmask32 << 4))) >> 4;
      break;
  }
  return;
}


void effect_lq2x_16_16
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT16  w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32lookup[u16src0[ 1]];
    w[5] = u32lookup[u16src1[ 1]];
    w[8] = u32lookup[u16src2[ 1]];

    lq2x_16( u16dst0, u16dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u16dst0 += 2;
    u16dst1 += 2;
  }
}


void effect_lq2x_16_16_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT16 *u16dst0 = (UINT16 *)dst0;
  UINT16 *u16dst1 = (UINT16 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT16  w[9];

  w[1] = u16src0[-1];
  w[2] = u16src0[ 0];
  w[4] = u16src1[-1];
  w[5] = u16src1[ 0];
  w[7] = u16src2[-1];
  w[8] = u16src2[ 0];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u16src0[ 1];
    w[5] = u16src1[ 1];
    w[8] = u16src2[ 1];

    lq2x_16( u16dst0, u16dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u16dst0 += 2;
    u16dst1 += 2;
  }
}


#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff
void effect_lq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
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
    unsigned count)
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

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/* FIXME: this probably doesn't work right for 24 bit */
void effect_lq2x_16_24
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0   = (UINT16 *)src0;
  UINT16 *u16src1   = (UINT16 *)src1;
  UINT16 *u16src2   = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];

    lq2x_32( u32dst0, u32dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}


void effect_lq2x_16_32
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0   = (UINT16 *)src0;
  UINT16 *u16src1   = (UINT16 *)src1;
  UINT16 *u16src2   = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

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
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];

    lq2x_32( u32dst0, u32dst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}


void effect_lq2x_32_32_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32src0 = (UINT32 *)src0;
  UINT32 *u32src1 = (UINT32 *)src1;
  UINT32 *u32src2 = (UINT32 *)src2;
  UINT32  w[9];

  w[1] = u32src0[-1];
  w[2] = u32src0[ 0];
  w[4] = u32src1[-1];
  w[5] = u32src1[ 0];
  w[7] = u32src2[-1];
  w[8] = u32src2[ 0];

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
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = u32src0[ 1];
    w[5] = u32src1[ 1];
    w[8] = u32src2[ 1];

    lq2x_32( u32dst0, u32dst1, w );

    ++u32src0;
    ++u32src1;
    ++u32src2;
    u32dst0 += 2;
    u32dst1 += 2;
  }
}
