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

const struct sysdep_display_effect_properties_struct sysdep_display_effect_properties[] = {
  { 1, 8, 1, 8, 0, "no effect" },            /* no effect */
  { 2, 2, 2, 2, 1, "smooth scaling" },       /* scale2x */
  { 2, 2, 2, 2, 1, "light scanlines" },      /* scan2 */
  { 3, 3, 2, 2, 0, "rgb vertical stripes" }, /* rgbstripe */
  { 2, 2, 3, 3, 0, "rgb scanlines" },        /* rgbscan */
  { 3, 3, 3, 3, 1, "deluxe scanlines" },     /* scan3 */
  { 2, 2, 2, 2, 1, "low quality filter" },   /* lq2x */
  { 2, 2, 2, 2, 1, "high quality filter" },  /* hq2x */
  { 2, 2, 2, 2, 1, "6-tap filter & scanlines" }, /* 6tap2x */
  { 1, 8, 2, 8, 0, "black scanlines" }       /* fakescan */
};
 
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

/* save the original palette info to restore it on close
   as we modify it for the 6tap 2x filter code */
static struct sysdep_palette_info orig_palette_info;

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


/* check if widthscale, heightscale and yarbsize are compatible with
   the choisen effect, if not update them so that they are. */
void sysdep_display_check_effect_params(
  struct sysdep_display_open_params *params)
{
  /* warn only once about disabling yarbsize */
  static int firsttime = 1;

  /* Can we do effects? */  
  if (!(sysdep_display_properties.mode_info[params->video_mode] &
        SYSDEP_DISPLAY_EFFECTS))
    params->effect = 0;

  /* Adjust widthscale */ 
  if (params->widthscale <
      sysdep_display_effect_properties[params->effect].min_widthscale)
  {
    params->widthscale =
      sysdep_display_effect_properties[params->effect].
        min_widthscale;
  }
  else if (params->widthscale >
           sysdep_display_effect_properties[params->effect].max_widthscale)
  {
    params->widthscale =
      sysdep_display_effect_properties[params->effect].max_widthscale;
  }
  
  /* Adjust heightscale */ 
  if (params->heightscale <
      sysdep_display_effect_properties[params->effect].min_heightscale)
  {
    params->heightscale =
      sysdep_display_effect_properties[params->effect].min_heightscale;
  }
  else if(params->heightscale >
          sysdep_display_effect_properties[params->effect].max_heightscale)
  {
    params->heightscale =
      sysdep_display_effect_properties[params->effect].max_heightscale;
  }
  
  /* check if we need to lock widthscale and heightscale */
  if (sysdep_display_effect_properties[params->effect].lock_scale)
    params->heightscale = params->widthscale;

  if (params->effect && params->yarbsize)
  {
    if (firsttime)
    {
      printf("Using effects -- disabling arbitrary scaling\n");
      firsttime = 0;
    }
    params->yarbsize = 0;
  }
}

/* Generate most effect variants automagicly, do this before
   building the effect funcs tables, so that we don't have to declare
   prototypes for all these variants (me lazy) */

/* for effects which have 2 steps, most have 3 addline functions
   for 15, 16 and 32 bpp src's and 5 render functions for
   15, 16, 32, YUY2 and YV12 dest's, the templates for these functions are
   in effect_renderers.h, where the addline functions are
   surrounded by a #if DEST_DEPTH != YUY2  */
#define DEST_DEPTH 15
#include "effect_renderers.h"

#define DEST_DEPTH 16
#include "effect_renderers.h"

#define DEST_DEPTH 32
#include "effect_renderers.h"

#define DEST_DEPTH YUY2
#include "effect_renderers.h"

/* Now the normal effect for which the templates are in
   effect_funcs.h, the YUY2 versions are handcoded and are
   in effect_funcs.c. First the indirect versions */
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

#define COLOR_FORMATS 5 /* 15,16,32,YUY2,YV12 */
#define SYSDEP_DISPLAY_EFFECT_MODES (COLOR_FORMATS*3) /* 15,16,32 */
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

/* called from sysdep_display_open;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering.
 *
 * The caller should call sysdep_display_effect_close() on failure and when
 * done, to free (partly) allocated buffers */
int sysdep_display_effect_open(void)
{
  const char *display_name[COLOR_FORMATS] = {
    "RGB 555",
    "RGB 565",
    "RGB 888",
    "YUY2",
    "YV12"
  };
  int i;

  /* FIXME only allocate if needed and of the right size */
  if (!(effect_dbbuf = malloc(sysdep_display_params.max_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4)))
    return 1;
  memset(effect_dbbuf, sysdep_display_params.max_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4, 0);

  if (sysdep_display_params.effect)
  {
    i = -1;
    switch(sysdep_display_properties.palette_info.fourcc_format)
    {
      case FOURCC_YUY2:
        i = 3;
        break;
      case FOURCC_YV12:
        i = 4;
        break;
      case 0:
        if ( (sysdep_display_properties.palette_info.bpp == 16) &&
             (sysdep_display_properties.palette_info.red_mask   == (0x1F << 10)) &&
             (sysdep_display_properties.palette_info.green_mask == (0x1F <<  5)) &&
             (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
          i = 0;
        if ( (sysdep_display_properties.palette_info.bpp == 16) &&
             (sysdep_display_properties.palette_info.red_mask   == (0x1F << 11)) &&
             (sysdep_display_properties.palette_info.green_mask == (0x3F <<  5)) &&
             (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
          i = 1;
        if ( ( (sysdep_display_properties.palette_info.bpp == 24) ||
               (sysdep_display_properties.palette_info.bpp == 32) ) &&
             (sysdep_display_properties.palette_info.red_mask   == (0xFF << 16)) &&
             (sysdep_display_properties.palette_info.green_mask == (0xFF <<  8)) &&
             (sysdep_display_properties.palette_info.blue_mask  == (0xFF      )))
          i = 2;
    }
    if (i == -1)
    {
      fprintf(stderr, "Warning your current color format is not supported by the effect code, disabling effects\n");
      sysdep_display_params.effect = 0;
    }
    else
    {
      i += (sysdep_display_params.depth / 16) * COLOR_FORMATS;
      switch (sysdep_display_params.effect)
      {
        case SYSDEP_DISPLAY_EFFECT_SCAN2:
          effect_func = effect_funcs[i];
          break;
        case SYSDEP_DISPLAY_EFFECT_RGBSTRIPE:
          effect_func = effect_funcs[i+SYSDEP_DISPLAY_EFFECT_MODES];
          break;
        case SYSDEP_DISPLAY_EFFECT_SCALE2X:
          effect_scale2x_func = effect_scale2x_funcs[i];
          break;
        case SYSDEP_DISPLAY_EFFECT_LQ2X:
          effect_scale2x_func = effect_scale2x_funcs[i+SYSDEP_DISPLAY_EFFECT_MODES];
          break;
        case SYSDEP_DISPLAY_EFFECT_HQ2X:
          /* we might need a yuv lookup table */
          init_rgb2yuv(i%COLOR_FORMATS);
          effect_scale2x_func = effect_scale2x_funcs[i+2*SYSDEP_DISPLAY_EFFECT_MODES];
          break;
        case SYSDEP_DISPLAY_EFFECT_RGBSCAN:
          effect_scale3x_func = effect_scale3x_funcs[i];
          break;
        case SYSDEP_DISPLAY_EFFECT_SCAN3:
          effect_scale3x_func = effect_scale3x_funcs[i+SYSDEP_DISPLAY_EFFECT_MODES];
          break;
        case SYSDEP_DISPLAY_EFFECT_6TAP2X:
          effect_6tap_addline_func = effect_6tap_addline_funcs[sysdep_display_params.depth/16];
          effect_6tap_render_func  = effect_6tap_render_funcs[i%COLOR_FORMATS];
          effect_6tap_clear_func   = effect_6tap_clear;
          break;
      }
      
      /* check if we've got a valid implementation */
      switch(sysdep_display_params.effect)
      {
        case SYSDEP_DISPLAY_EFFECT_NONE:
          break;
        case SYSDEP_DISPLAY_EFFECT_SCAN2:
        case SYSDEP_DISPLAY_EFFECT_RGBSTRIPE:
          if(!effect_func) sysdep_display_params.effect = 0;
          break;
        case SYSDEP_DISPLAY_EFFECT_SCALE2X:
        case SYSDEP_DISPLAY_EFFECT_LQ2X:
        case SYSDEP_DISPLAY_EFFECT_HQ2X:
          if (!effect_scale2x_func) sysdep_display_params.effect = 0;
          break;
        case SYSDEP_DISPLAY_EFFECT_RGBSCAN:
        case SYSDEP_DISPLAY_EFFECT_SCAN3:
          if (!effect_scale3x_func) sysdep_display_params.effect = 0;
          break;
        case SYSDEP_DISPLAY_EFFECT_6TAP2X:
          if (!effect_6tap_render_func) sysdep_display_params.effect = 0;
          break;
        case SYSDEP_DISPLAY_EFFECT_FAKESCAN:
          /* handled by normal blitting, not supported on YV12 for now */
          if ((i%COLOR_FORMATS) == 4) 
            sysdep_display_params.effect = 0;
          else
            sysdep_display_driver_clear_buffer();
          break;
      }
      /* report our results to the user */
      if (sysdep_display_params.effect)
        fprintf(stderr,
          "Initialized %s: bitmap depth = %d, color format = %s\n",
          sysdep_display_effect_properties[sysdep_display_params.effect].name,
          sysdep_display_params.depth, display_name[i%COLOR_FORMATS]);
      else
        fprintf(stderr,
          "Warning effect %s is not supported with color format %s, disabling effects\n",
          sysdep_display_effect_properties[sysdep_display_params.effect].name,
          display_name[i%COLOR_FORMATS]);
    }
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
    if (!(rotate_dbbuf0 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
      return 1;
    rotate_dbbuf0 += 16;

    if ((sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_SCALE2X) ||
        (sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_HQ2X)    ||
        (sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_LQ2X)) {
      if (!(rotate_dbbuf1 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
        return 1;
      if (!(rotate_dbbuf2 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
        return 1;
      rotate_dbbuf1 += 16;
      rotate_dbbuf2 += 16;
    }
  }

  /* I need these buffers */
  if (sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_6TAP2X)
  {
    if (!(_6tap2x_buf0 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf1 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf2 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf3 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf4 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    if (!(_6tap2x_buf5 = calloc(sysdep_display_params.max_width*8, sizeof(char))))
      return 1;
    orig_palette_info = sysdep_display_properties.palette_info;
    if(sysdep_display_params.depth == 16)
    {
       /* HACK: we need the palette lookup table to be 888 rgb, this means
          that the lookup table won't be usable for normal blitting anymore
          but that is not a problem, since we're not doing normal blitting,
          we do need to restore it on close though! */
       sysdep_display_properties.palette_info.fourcc_format = 0;
       sysdep_display_properties.palette_info.red_mask   = 0x00FF0000;
       sysdep_display_properties.palette_info.green_mask = 0x0000FF00;
       sysdep_display_properties.palette_info.blue_mask  = 0x000000FF;
    }
  }
  return 0;
}

void sysdep_display_effect_close(void)
{
  /* if we modifified it then restore palette_info */
  if (_6tap2x_buf5)
    sysdep_display_properties.palette_info = orig_palette_info;
  
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
    unsigned count, unsigned int *u32lookup)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
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
    unsigned count, unsigned int *u32lookup)
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
    unsigned count, unsigned int *u32lookup)
{
  UINT32 *u32dst0   = (UINT32 *)dst0;
  UINT32 *u32dst1   = (UINT32 *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
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
    unsigned count, unsigned int *u32lookup)
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
