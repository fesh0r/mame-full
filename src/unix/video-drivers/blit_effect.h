/* this file is used by blit.h -- don't use it directly ! */

/* For now CONVERT_PIXEL is only used to downsample 32bpp bitmaps for
   display on 15 or 16 bpp destinations, in this case the effect code renders
   in 32bpp format and we convert this afterwards in the MEMCPY macro
   The 6tap2x effect is an exception which renders directly in 16 bpp format */
#ifdef CONVERT_PIXEL
#  define MEMCPY(d, s, n) \
   {\
     SRC_PIXEL  *src = (SRC_PIXEL *)(s); \
     SRC_PIXEL  *end = src + (n); \
     DEST_PIXEL *dst = (DEST_PIXEL *)(d); \
     for(;src<end;src+=4,dst+=4) \
     { \
        *(dst  ) = CONVERT_PIXEL(*(src  )); \
        *(dst+1) = CONVERT_PIXEL(*(src+1)); \
        *(dst+2) = CONVERT_PIXEL(*(src+2)); \
        *(dst+3) = CONVERT_PIXEL(*(src+3)); \
     }\
   }
#endif

/* when packing bits the effect code renders in sparse 32bpp and we pack this
   in the MEMCPY macro */
#ifdef PACK_BITS
#  define MEMCPY(d, s, n) \
   {\
     unsigned int *src = (unsigned int *)(s); \
     unsigned int *end = src + (n); \
     DEST_PIXEL *dst = (DEST_PIXEL *)(d); \
     for(;src<end;dst+=3,src+=4) \
     { \
        *(dst  ) = ((*(src  ))    ) | ((*(src+1))<<24); \
        *(dst+1) = ((*(src+1))>> 8) | ((*(src+2))<<16); \
        *(dst+2) = ((*(src+2))>>16) | ((*(src+3))<< 8); \
     }\
   }
#endif

/* double buf / MEMCPY tricks for downsampling and packing bits or 24 bpp
   packed bits modes. WARNING this assumes that the effect code renders in
   sparse 32 bpp format for these cases ! */
#if defined CONVERT_PIXEL || defined PACK_BITS
#  define EFFECT() \
     effect_func(effect_dbbuf, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*4, line_src, bounds_width, palette->lookup); \
     MEMCPY(line_dest, effect_dbbuf, bounds_width*sysdep_display_params.widthscale); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*4, bounds_width*sysdep_display_params.widthscale)
#  define EFFECT2X() \
     effect_scale2x_func(effect_dbbuf, effect_dbbuf+bounds_width*8, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, bounds_width, palette->lookup); \
     MEMCPY(line_dest, effect_dbbuf, bounds_width*2); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+bounds_width*8, bounds_width*2)
#  define EFFECT3X() \
     effect_scale3x_func(effect_dbbuf, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*4, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*8, line_src, bounds_width, palette->lookup); \
     MEMCPY(line_dest, effect_dbbuf, bounds_width*sysdep_display_params.widthscale); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*4, bounds_width*sysdep_display_params.widthscale); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH*2, effect_dbbuf+bounds_width*sysdep_display_params.widthscale*8, bounds_width*sysdep_display_params.widthscale)
#else
#  define EFFECT() \
     effect_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_src, bounds_width, palette->lookup)
#  define EFFECT2X() \
     effect_scale2x_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, bounds_width, palette->lookup)
#  define EFFECT3X() \
     effect_scale3x_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_dest+CORRECTED_DEST_WIDTH*2, line_src, bounds_width, palette->lookup)
#endif

/* only use MEMCPY tricks for PACK_BITS, since 6tap2x can render 15/16 bpp
   directly even if the src is 32 bpp */
#ifdef PACK_BITS
#  define EFFECT_6TAP() \
     effect_6tap_render_func(effect_dbbuf, effect_dbbuf+bounds_width*8, bounds_width); \
     MEMCPY(line_dest, effect_dbbuf, bounds_width*2); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+bounds_width*8, bounds_width*2)
#else     
#  define EFFECT_6TAP() \
     effect_6tap_render_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, bounds_width)
#endif

/* start of actual effect blit code */
  case EFFECT_SCAN2:
#ifdef EFFECT_MMX_ASM
     effect_setpalette_asm(palette->lookup);
#endif
  case EFFECT_RGBSTRIPE:
    if (sysdep_display_params.orientation)
    {
      line_src = (SRC_PIXEL *)rotate_dbbuf0;
      for (y = src_bounds->min_y; y <= src_bounds->max_y; line_dest+=sysdep_display_params.heightscale*CORRECTED_DEST_WIDTH, y++) {
        rotate_func(rotate_dbbuf0, bitmap, y, src_bounds);
        EFFECT();
      }
    }
    else
    {
      for (;line_src < line_end; line_dest+=sysdep_display_params.heightscale*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        EFFECT();
      }
    }  
    break;

  case EFFECT_SCALE2X:
  case EFFECT_HQ2X:
  case EFFECT_LQ2X:
    if (sysdep_display_params.orientation)
    {
      char *tmp;
      /* preload the first lines for 2x effects */
      rotate_func(rotate_dbbuf1, bitmap, src_bounds->min_y, src_bounds);
      rotate_func(rotate_dbbuf2, bitmap, src_bounds->min_y, src_bounds);
      
      for (y = src_bounds->min_y; y <= src_bounds->max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) {
        /* shift lines up */
        tmp = rotate_dbbuf0;
        rotate_dbbuf0=rotate_dbbuf1;
        rotate_dbbuf1=rotate_dbbuf2;
        rotate_dbbuf2=tmp;
        rotate_func(rotate_dbbuf2, bitmap, y+1, src_bounds);
        EFFECT2X();
      } 
    } 
    else 
    { 
      /* use rotate_dbbufX for the inputs to the effect_func, to keep the EFFECT2x 
         macro ident, this should be safe since these shouldn't be allocated 
         when not rotating */ 
      for (;line_src < line_end; line_dest+=2*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        rotate_dbbuf0=(char *)(line_src-src_width); 
        rotate_dbbuf1=(char *)line_src; 
        rotate_dbbuf2=(char *)(line_src+src_width); 
        EFFECT2X(); 
      } 
      rotate_dbbuf0=NULL; 
      rotate_dbbuf1=NULL; 
      rotate_dbbuf2=NULL; 
    }  
    break;

  case EFFECT_RGBSCAN:
  case EFFECT_SCAN3:
    if (sysdep_display_params.orientation)
    {
      line_src = (SRC_PIXEL *)rotate_dbbuf0;
      for (y = src_bounds->min_y; y <= src_bounds->max_y; line_dest+=sysdep_display_params.heightscale*CORRECTED_DEST_WIDTH, y++) {
        rotate_func(rotate_dbbuf0, bitmap, y, src_bounds);
        EFFECT3X();
      }
    }
    else
    {
      for (;line_src < line_end; line_dest+=sysdep_display_params.heightscale*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        EFFECT3X();
      }
    }
    break;

  case EFFECT_6TAP2X:
#ifdef EFFECT_MMX_ASM
     effect_setpalette_asm(palette->lookup);
#endif
  {
     effect_6tap_clear_func(bounds_width);
     if (sysdep_display_params.orientation) {
       /* put in the first three lines */
       rotate_func(rotate_dbbuf0, bitmap, src_bounds->min_y, src_bounds);
       effect_6tap_addline_func(rotate_dbbuf0, bounds_width, palette->lookup);
       rotate_func(rotate_dbbuf0, bitmap, src_bounds->min_y+1, src_bounds);
       effect_6tap_addline_func(rotate_dbbuf0, bounds_width, palette->lookup);
       rotate_func(rotate_dbbuf0, bitmap, src_bounds->min_y+2, src_bounds);
       effect_6tap_addline_func(rotate_dbbuf0, bounds_width, palette->lookup);

       for (y = src_bounds->min_y; y <= src_bounds->max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) {
           if (y <= src_bounds->max_y - 3)
           {
             rotate_func(rotate_dbbuf0, bitmap, y+3, src_bounds);
             effect_6tap_addline_func(rotate_dbbuf0, bounds_width, palette->lookup);
           }
	   else
           {
             effect_6tap_addline_func(NULL, bounds_width, palette->lookup);
           }
           EFFECT_6TAP();
       }
     } else {
       effect_6tap_addline_func(line_src, bounds_width, palette->lookup);
       effect_6tap_addline_func(line_src+=src_width, bounds_width, palette->lookup);
       effect_6tap_addline_func(line_src+=src_width, bounds_width, palette->lookup);

       for (y = src_bounds->min_y; y <= src_bounds->max_y; y++, line_dest+=2*CORRECTED_DEST_WIDTH) {
           if (y <= src_bounds->max_y - 3)
           {
             effect_6tap_addline_func(line_src+=src_width, bounds_width, palette->lookup);
           }
	   else
           {
             effect_6tap_addline_func(NULL, bounds_width, palette->lookup);
           }
           EFFECT_6TAP();
       }
     }
     break;
  }

#undef MEMCPY
#undef EFFECT
#undef EFFECT2X
#undef EFFECT3X
#undef EFFECT_6TAP
