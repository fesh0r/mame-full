/* this routine is the generic blit routine used in many cases, through a
   number of defines it can be customised for specific cases.
   Currently recognised defines:
   DEST		 ptr of type DEST_PIXEL to which should be blitted
   DEST_PIXEL	 type of the buffer to which is blitted
   DEST_WIDTH    Width of the destination buffer in pixels!
   SRC_PIXEL     type of the buffer from which is blitted
   PUT_IMAGE     This function is called to update the parts of the screen
		 which need updating. This is only called if defined.
   INDIRECT      Define this if you want SRC_PIXELs to be an index to a lookup
                 table, the contents of this define is used as the lookuptable.
   CONVERT_PIXEL If defined then the blit core will call CONVERT_PIXEL on each
                 src_pixel (after looking it up in INDIRECT if defined).
                 This could be used to show for example a 32 bpp bitmap
                 (artwork) on a 16 bpp X-server by defining a CONVER_PIXEL
                 function which downgrades the direct RGB  pixels to 16 bpp.
                 When you use this you should also make sure that effect.c
                 renders in SRC_PIXEL color depth and format after lookup in
                 INDIRECT.
   PACK_BITS     If defined write to packed 24bit pixels, DEST_PIXEL must be
                 32bits and the contents of SRC_PIXEL after applying INDIRECT
                 and CONVERT_PIXEL should be 32 bits RGB 888.
   BLIT_HWSCALE_YUY2	Write to a 16 bits DEST in YUY2 format if INDIRECT is
                 set the contents of SRC_PIXEL after applying INDIRECT and
                 CONVERT_PIXEL should be a special pre calculated 32 bits
                 format, see x11_window.c for an example.
                 If indirect is not set the contents of SRC_PIXEL after
                 applying CONVERT_PIXEL should be 32 bits RGB 888.
   DOUBLEBUFFER  First copy each line to a buffer then do a memcpy to the
                 real destination. This speeds up scaling when writing directly
                 to framebuffer since it tremendously speeds up the reads done
                 to copy one line to the next.

   These routines assume visual_ height, width, min_x, min_y, max_x and max_y
   are all a multiple off 8 !

ChangeLog:

18 August 2004 (Hans de Goede):
-removed some unused vars, which totally flooded my term with warnings
-moved PUT_IMAGE to bottom, since this needs to be done exactly the same
 for each effect
-removed #ifdef DEST, we always need a DEST nowadays for rotation and effects,
 better to break compiles for targets not setting DEST
-created EFFECT, EFFECT2X and EFFECT3x macros which abstract all the
 doublebuf and indirect stuff.
-created LOOP and LOOP2X macros which abstract the loops with
 and without rotation. The LOOP2X macro uses circulating pointers instead
 of doing the rotation 3 times for each line for a small speedup
-fixed rotation in 6tap2x (broken by my previous patch)
-made the no effect code use CORRECTED_DEST_WIDTH instead of DEST_WIDTH,
 so that atleast the no effect code should work on 24 bpp packed displays,
 no complaints about this?
-BIG CLEANUP:
-removed a bunch of defines since these are no longer used
-removed 16BPP_HACK, not used by any driver, and not usefull
 since it is a HACK to speedup 8 to 16 bpp blits, which we won't be doing
 anymore, since the core doesn't do 8 bit anymore.
-made arbysize code honor use_scanlines
-removed non arb y scaling, the speed advantage is non measurable on my
 ancient computer, so on newer computers it will be even less!
-moved non-effect code from blit_core.h to a macro in blit.h
-renamed blit_core.h to blit_effect.h
-only include blit_effect.h once instead of once for each widthscale,
 since each effect has a distinct widthscale and does this scaling itself
 including it for every widthscale case makes no sense
-removed #ifdef LOW_MEM, blit.h now is so small that this should no longer be
 needed
-created a GETPIXEL macro that abstracts INDIRECT being set or not,
 removed all the not INDIRECT special cases
-added changelog
-actualised documentation in the top
-added a CONVERT_PIXEL define. See above.
*/

#ifdef PACK_BITS
/* scale destptr delta's by 3/4 since we're using 32 bits ptr's for a 24 bits
   dest */
#define DEST_PIXEL_SIZE 3
#define CORRECTED_DEST_WIDTH ((DEST_WIDTH*3)/4)
#else
#define DEST_PIXEL_SIZE sizeof(DEST_PIXEL)
#define CORRECTED_DEST_WIDTH DEST_WIDTH
#endif

/* arbitrary Y-scaling (Adam D. Moss <adam@gimp.org>) */
#define REPS_FOR_Y(N,YV,YMAX) ((N)* ( (((YV)+1)*yarbsize)/(YMAX) - ((YV)*yarbsize)/(YMAX)))

#ifdef DOUBLEBUFFER

#if defined INDIRECT || defined CONVERT_PIXEL
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   if (reps >0) { \
     COPY_LINE2(SRC, END, (DEST_PIXEL *)effect_dbbuf); \
     do { memcpy((DST)+(reps*(CORRECTED_DEST_WIDTH)), effect_dbbuf, ((END)-(SRC))*DEST_PIXEL_SIZE*widthscale); \
     } while (reps-- > use_scanlines); \
   } \
}
#else /* speedup hack for GETPIXEL(src) == (src) */
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   if (reps >0) { \
     do { COPY_LINE2(SRC, END, (DST)+(reps*(CORRECTED_DEST_WIDTH))); \
     } while (reps-- > use_scanlines); \
   } \
}
#endif

#else
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   if (reps >0) { \
     COPY_LINE2(SRC, END, DST); \
     while (reps-- > (1+use_scanlines)) \
       memcpy((DST)+(reps*(CORRECTED_DEST_WIDTH)), DST, ((END)-(SRC))*DEST_PIXEL_SIZE*widthscale); \
  } \
}
#endif

#define LOOP() \
if (!blit_hardware_rotation && current_palette != debug_palette \
                 && (blit_flipx || blit_flipy || blit_swapxy)) { \
  for (y = visual.min_y; y <= visual.max_y; line_dest+=REPS_FOR_Y(CORRECTED_DEST_WIDTH,y,visual_height), y++) { \
           rotate_func(rotate_dbbuf, bitmap, y); \
           line_src = (SRC_PIXEL *)rotate_dbbuf; \
           line_end = (SRC_PIXEL *)rotate_dbbuf + visual_width; \
           COPY_LINE_FOR_Y(y, visual_height, line_src, line_end, line_dest); \
  } \
} else { \
y = visual.min_y; \
for (;line_src < line_end; \
        line_dest+=REPS_FOR_Y(CORRECTED_DEST_WIDTH,y,visual_height), \
                line_src+=src_width, y++) \
           COPY_LINE_FOR_Y(y,visual_height, \
                           line_src, line_src+visual_width, line_dest); \
}

#ifdef CONVERT_PIXEL
#  ifdef INDIRECT
#  define GETPIXEL(src) CONVERT_PIXEL(INDIRECT[src])
#  else
#  define GETPIXEL(src) CONVERT_PIXEL((src))
#  endif
#  define MEMCPY(d, s, n) \
   {\
     SRC_PIXEL  *src = (SRC_PIXEL *)(s); \
     SRC_PIXEL  *end = (SRC_PIXEL *)((char *)(s) + (n)); \
     DEST_PIXEL *dst = (DEST_PIXEL *)(d); \
     for(;src<end;src+=8,dst+=8) \
     { \
        *(dst  ) = CONVERT_PIXEL(*(src  )); \
        *(dst+1) = CONVERT_PIXEL(*(src+1)); \
        *(dst+2) = CONVERT_PIXEL(*(src+2)); \
        *(dst+3) = CONVERT_PIXEL(*(src+3)); \
        *(dst+4) = CONVERT_PIXEL(*(src+4)); \
        *(dst+5) = CONVERT_PIXEL(*(src+5)); \
        *(dst+6) = CONVERT_PIXEL(*(src+6)); \
        *(dst+7) = CONVERT_PIXEL(*(src+7)); \
     }\
   }
#else
#  ifdef INDIRECT
#  define GETPIXEL(src) INDIRECT[src]
#  else
#  define GETPIXEL(src) (src)
#  endif
#  define MEMCPY memcpy
#endif

/* begin actual blit code */
{
  int org_yarbsize = yarbsize;
  int y;
  int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
  SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y] + visual.min_x;
  SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
  DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

  if (yarbsize==0) 
    yarbsize = heightscale * visual_height;

  switch(effect)
  {
    case 0:
      switch(widthscale)
      {

case 1:
#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
      {\
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;dst+=3,src+=4) \
   { \
      *(dst  ) = (GETPIXEL(*(src  ))    ) | (GETPIXEL(*(src+1))<<24); \
      *(dst+1) = (GETPIXEL(*(src+1))>> 8) | (GETPIXEL(*(src+2))<<16); \
      *(dst+2) = (GETPIXEL(*(src+2))>>16) | (GETPIXEL(*(src+3))<< 8); \
   }\
      }
#elif defined BLIT_HWSCALE_YUY2
/* HWSCALE_YUY2 has seperate code for direct / indirect, see above */
#ifdef INDIRECT
#ifdef LSB_FIRST /* x86 etc */
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned long *dst = (unsigned long *)DST; \
      unsigned int r,y,y2,uv1,uv2; \
      for(;src<end;) \
      { \
         r=GETPIXEL(*src++); \
         y=r&255; \
         uv1=(r&0xff00ff00)>>1; \
         r=GETPIXEL(*src++); \
         uv2=(r&0xff00ff00)>>1; \
         y2=r&0xff0000; \
         *dst++=y|y2|((uv1+uv2)&0xff00ff00);\
      } \
   }
#else /* ppc etc */
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned long *dst = (unsigned long *)DST; \
      unsigned int r,y,y2,uv1,uv2; \
      for(;src<end;) \
      { \
         r=GETPIXEL(*src++); \
         y=r&0xff000000 ; \
         uv1=(r&0x00ff00ff); \
         r=GETPIXEL(*src++); \
         uv2=(uv1+(r&0x00ff00ff))>>1; \
         y2=r&0xff00; \
         *dst++=y|y2|(uv2&0x00ff00ff); \
      } \
   }
#endif /* ifdef LSB_FIRST */
#else  /* HWSCLALE_YUY2, not indirect */
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned char *dst = (unsigned char *)DST; \
      int r,g,b,r2,g2,b2,y,y2,u,v; \
      for(;src<end;) \
      { \
         r=g=b=GETPIXEL(*src++); \
         r&=RMASK;  r>>=16; \
         g&=GMASK;  g>>=8; \
         b&=BMASK;  b>>=0; \
         r2=g2=b2=GETPIXEL(*src++); \
         r2&=RMASK;  r2>>=16; \
         g2&=GMASK;  g2>>=8; \
         b2&=BMASK;  b2>>=0; \
         y = (( 9836*r + 19310*g + 3750*b ) >> 15); \
         y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15); \
         r+=r2; g+=g2; b+=b2; \
         *dst++=y; \
         u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128; \
         *dst++=u; \
         v = (( 16448*r - 13783*g - 2665*b ) >> 16 ) + 128; \
         *dst++=y2; \
         *dst++=v; \
      }\
   }
#endif /* HWSCLALE_YUY2 direct or indirect */
#else  /* normal */
/* speedup hack for 1x widthscale and GETPIXEL(src) == (src) */
#if !defined INDIRECT && !defined CONVERT_PIXEL
#define COPY_LINE2(SRC, END, DST) \
   memcpy(DST, SRC, ((END)-(SRC))*DEST_PIXEL_SIZE);
#else /* really normal */
#define COPY_LINE2(SRC, END, DST) \
      {\
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;src+=8,dst+=8) \
   { \
      *(dst  ) = GETPIXEL(*(src  )); \
      *(dst+1) = GETPIXEL(*(src+1)); \
      *(dst+2) = GETPIXEL(*(src+2)); \
      *(dst+3) = GETPIXEL(*(src+3)); \
      *(dst+4) = GETPIXEL(*(src+4)); \
      *(dst+5) = GETPIXEL(*(src+5)); \
      *(dst+6) = GETPIXEL(*(src+6)); \
      *(dst+7) = GETPIXEL(*(src+7)); \
   }\
      }
#endif /* speedup hack for 1x widthscale and GETPIXEL(src) == (src) */
#endif /* packed / yuv / normal */

LOOP()

#undef COPY_LINE2
break;


/* HWSCALE always uses widthscale=1 for non effect blits */
#ifndef BLIT_HWSCALE_YUY2 
case 2:
#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=2, dst+=3) \
   { \
      *(dst  ) = (GETPIXEL(*(src  ))    ) | (GETPIXEL(*(src  ))<<24); \
      *(dst+1) = (GETPIXEL(*(src  ))>> 8) | (GETPIXEL(*(src+1))<<16); \
      *(dst+2) = (GETPIXEL(*(src+1))>>16) | (GETPIXEL(*(src+1))<<8); \
   } \
}
#else /* not pack bits */
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=16) \
   { \
      *(dst   ) = *(dst+ 1) = GETPIXEL(*(src  )); \
      *(dst+ 2) = *(dst+ 3) = GETPIXEL(*(src+1)); \
      *(dst+ 4) = *(dst+ 5) = GETPIXEL(*(src+2)); \
      *(dst+ 6) = *(dst+ 7) = GETPIXEL(*(src+3)); \
      *(dst+ 8) = *(dst+ 9) = GETPIXEL(*(src+4)); \
      *(dst+10) = *(dst+11) = GETPIXEL(*(src+5)); \
      *(dst+12) = *(dst+13) = GETPIXEL(*(src+6)); \
      *(dst+14) = *(dst+15) = GETPIXEL(*(src+7)); \
   } \
}
#endif /* pack bits */

LOOP()

#undef COPY_LINE2
break;


#ifndef PACK_BITS /* no optimised 3x COPY_LINE2 for PACK_BITS */
case 3:
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=24) \
   { \
      *(dst   ) = *(dst+ 1) = *(dst+ 2) = GETPIXEL(*(src  )); \
      *(dst+ 3) = *(dst+ 4) = *(dst+ 5) = GETPIXEL(*(src+1)); \
      *(dst+ 6) = *(dst+ 7) = *(dst+ 8) = GETPIXEL(*(src+2)); \
      *(dst+ 9) = *(dst+10) = *(dst+11) = GETPIXEL(*(src+3)); \
      *(dst+12) = *(dst+13) = *(dst+14) = GETPIXEL(*(src+4)); \
      *(dst+15) = *(dst+16) = *(dst+17) = GETPIXEL(*(src+5)); \
      *(dst+18) = *(dst+19) = *(dst+20) = GETPIXEL(*(src+6)); \
      *(dst+21) = *(dst+22) = *(dst+23) = GETPIXEL(*(src+7)); \
   } \
}

LOOP()

#undef COPY_LINE2
break;
#endif /* PACK_BITS (no optimised 3x COPY_LINE2 for PACK_BITS) */


default:
#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   DEST_PIXEL pixel; \
   int i, step=0; \
   for(;src<end;src++) \
   { \
      pixel = GETPIXEL(*src); \
      for(i=0; i<widthscale; i++,step=(step+1)&3) \
      { \
         switch(step) \
         { \
            case 0: \
               *(dst  )  = pixel; \
               break; \
            case 1: \
               *(dst  ) |= pixel << 24; \
               *(dst+1)  = pixel >> 8; \
               break; \
            case 2: \
               *(dst+1) |= pixel << 16; \
               *(dst+2)  = pixel >> 16; \
               break; \
            case 3: \
               *(dst+2) |= pixel << 8; \
               dst+=3; \
               break; \
         } \
      } \
   } \
}
#else
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   int i; \
   for(;src<end;src++) \
   { \
      const DEST_PIXEL v = GETPIXEL(*(src)); \
      i=(widthscale+7)/8; \
      dst+=widthscale&7; \
      switch (widthscale&7) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = v; \
         case 7:      *(dst-7) = v; \
         case 6:      *(dst-6) = v; \
         case 5:      *(dst-5) = v; \
         case 4:      *(dst-4) = v; \
         case 3:      *(dst-3) = v; \
         case 2:      *(dst-2) = v; \
         case 1:      *(dst-1) = v; \
                 } while(--i>0); \
      } \
   } \
}
#endif

LOOP()

#undef COPY_LINE2
break;
#endif /* ifndef BLIT_HWSCALE_YUY2  */


    } /* end switch(widtscale) */
    break;

/* these aren't used by the effect code, so undef them now */
#undef REPS_FOR_Y
#undef COPY_LINE_FOR_Y
#undef LOOP
#undef GETPIXEL

/* we must double buffer when converting pixels, since the pixel conversion
   is done in the MEMCPY macro */
#if defined CONVERT_PIXEL && !defined DOUBLEBUFFER
#define DOUBLEBUFFER
#define DOUBLEBUFFER_defined
#endif
   
#include "blit_effect.h"

/* clean up */
#ifdef DOUBLEBUFFER_defined
#undef DOUBLEBUFFER
#endif

  } /* end switch(effect) */

#ifdef PUT_IMAGE
  PUT_IMAGE(0, 0, widthscale*visual_width, yarbsize)
#endif

  yarbsize=org_yarbsize;
}

/* clean up */
#undef DEST_PIXEL_SIZE
#undef CORRECTED_DEST_WIDTH
#undef MEMCPY
