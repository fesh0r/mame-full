INLINE void FUNC_NAME(blit_normal_line_1)(SRC_PIXEL *src, SRC_PIXEL *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int p,y1,y2,uv;
  
  while(src<end)
  {
    p  = GET_YUV_PIXEL(*src); src++;
    y1 = p&Y1MASK;
    uv = (p&UVMASK)>>1;
    p  = GET_YUV_PIXEL(*src); src++;
    y2 = p&Y2MASK;
    uv += (p&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
  }
}

INLINE void FUNC_NAME(blit_normal_line_2)(SRC_PIXEL *src, SRC_PIXEL *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  while(src<end)
  {
    *dst++ = GET_YUV_PIXEL(*src);
    src++;
  }
}

BLIT_BEGIN(blit_normal)
  switch(sysdep_display_params.widthscale)
  {
    case 1:
      BLIT_LOOP(blit_normal_line_1, 1);
      break;
    case 2:
      BLIT_LOOP(blit_normal_line_2, 1);
      break;
  }
BLIT_END


/* scale2x algorithm (Andrea Mazzoleni, http://advancemame.sourceforge.net):
 *
 * A 9-pixel rectangle is taken from the source bitmap:
 *
 *  a b c
 *  d e f
 *  g h i
 *
 * The central pixel e is expanded into four new pixels,
 *
 *  e0 e1
 *  e2 e3
 *
 * where
 *
 *  e0 = (d == b && b != f && d != h) ? d : e;
 *  e1 = (b == f && b != d && f != h) ? f : e;
 *  e2 = (d == h && d != b && h != f) ? d : e;
 *  e3 = (h == f && d != h && b != f) ? f : e;
 *
 */
INLINE void FUNC_NAME(blit_scale2x_border)( SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2,
  SRC_PIXEL *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,y1,y2,uv;
  while (src1 < end1) {
    if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
      p1 = GET_YUV_PIXEL(src0[0]);
    else p1 = GET_YUV_PIXEL(src1[0]);

    if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
      p2 = GET_YUV_PIXEL(src0[0]);
    else p2 = GET_YUV_PIXEL(src1[0]);

    ++src0;
    ++src1;
    ++src2;

    y1 = p1&Y1MASK;
    uv = (p1&UVMASK)>>1;
    y2 = p2&Y2MASK;
    uv += (p2&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
  }
}

INLINE void FUNC_NAME(blit_scale2x_line)(SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2, SRC_PIXEL *end1,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  FUNC_NAME(blit_scale2x_border)(src0, src1, src2, end1, dst, lookup);
  FUNC_NAME(blit_scale2x_border)(src2, src1, src0, end1, dst + dest_width/2,
    lookup);
}

INLINE void FUNC_NAME(blit_scale3x_border)( SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2,
  SRC_PIXEL *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,p3,p4,p5,p6,y1,y2,uv;
  while (src1 < end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = GET_YUV_PIXEL(src1[-1] == src0[0] ? src1[-1] : src1[0]);
    	p2 = GET_YUV_PIXEL((src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0]);
    	p3 = GET_YUV_PIXEL(src1[1] == src0[0] ? src1[1] : src1[0]);
    } else {
    	p1 = p2 = p3 = GET_YUV_PIXEL(src1[0]);
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = GET_YUV_PIXEL(src1[-1] == src0[0] ? src1[-1] : src1[0]);
    	p5 = GET_YUV_PIXEL((src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0]);
    	p6 = GET_YUV_PIXEL(src1[1] == src0[0] ? src1[1] : src1[0]);
    } else {
    	p4 = p5 = p6 = GET_YUV_PIXEL(src1[0]);
    }
    src0++;
    src1++;
    src2++;
    y1 = p1&Y1MASK;
    uv = (p1&UVMASK)>>1;
    y2 = p2&Y2MASK;
    uv += (p2&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
    y1 = p3&Y1MASK;
    uv = (p3&UVMASK)>>1;
    y2 = p4&Y2MASK;
    uv += (p4&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
    y1 = p5&Y1MASK;
    uv = (p5&UVMASK)>>1;
    y2 = p6&Y2MASK;
    uv += (p6&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
  }
}

INLINE void FUNC_NAME(blit_scale3x_center)( SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2,
  SRC_PIXEL *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,p3,p4,p5,p6,y1,y2,uv;
  while (src1 < end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = GET_YUV_PIXEL((src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0]);
    	p2 = GET_YUV_PIXEL(src1[0]);
    	p3 = GET_YUV_PIXEL((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0]);
    } else {
    	p1 = p2 = p3 = GET_YUV_PIXEL(src1[0]);
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = GET_YUV_PIXEL((src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0]);
    	p5 = GET_YUV_PIXEL(src1[0]);
    	p6 = GET_YUV_PIXEL((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0]);
    } else {
    	p4 = p5 = p6 = GET_YUV_PIXEL(src1[0]);
    }
    src0++;
    src1++;
    src2++;
    y1 = p1&Y1MASK;
    uv = (p1&UVMASK)>>1;
    y2 = p2&Y2MASK;
    uv += (p2&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
    y1 = p3&Y1MASK;
    uv = (p3&UVMASK)>>1;
    y2 = p4&Y2MASK;
    uv += (p4&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
    y1 = p5&Y1MASK;
    uv = (p5&UVMASK)>>1;
    y2 = p6&Y2MASK;
    uv += (p6&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
  }
}

INLINE void FUNC_NAME(blit_scale3x_line)(SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2, SRC_PIXEL *end1,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  FUNC_NAME(blit_scale3x_border)(src0, src1, src2, end1, dst, lookup);
  FUNC_NAME(blit_scale3x_center)(src0, src1, src2, end1, dst + dest_width/2,
    lookup);
  FUNC_NAME(blit_scale3x_border)(src2, src1, src0, end1, dst + dest_width,
    lookup);
}

BLIT_BEGIN(blit_scale2x)
  switch(sysdep_display_params.widthscale)
  {
    case 2:
      BLIT_LOOP2X(blit_scale2x_line, 2)
      break;
    case 3:
      BLIT_LOOP2X(blit_scale3x_line, 3)
      break;
  }
BLIT_END


/* lq2x yuy2 version */
#include "advance/xq2x_yuy2.h"


/* hq2x yuy2 version */
#define HQ2X
#define HQ2X_USE_YUV_LOOKUP 1
#define HQ2X_YUVLOOKUP(p) (p)
#include "advance/xq2x_yuy2.h"


/**********************************
 * scan2: light 2x2 scanlines
 **********************************/
INLINE void FUNC_NAME(blit_scan2_h_line)(SRC_PIXEL *src, SRC_PIXEL *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,uv;
  SRC_PIXEL *mysrc;
  unsigned int *mydst = dst;

  for(mysrc=src; mysrc<end; mysrc++)
    *mydst++ = GET_YUV_PIXEL(*mysrc);

  mydst = dst + dest_width/2;
  for(mysrc=src; mysrc<end; mysrc++) {
    y   = uv = GET_YUV_PIXEL(*mysrc);
    y  &= YMASK;
#ifdef LSB_FIRST
    y   = (y*3)>>2;
#else
    y   = (y>>2)*3;
#endif
    y  &= YMASK;
    uv &= UVMASK;
    *mydst++ = y|uv;
  }
}

BLIT_BEGIN(blit_scan2_h)
BLIT_LOOP(blit_scan2_h_line, 2)
BLIT_END

INLINE void FUNC_NAME(blit_scan2_v_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,yuv;

  while(src<end) {
    y    = yuv = GET_YUV_PIXEL(*src);
    y   &= Y2MASK;
    y    = (y*3) >> 2;   
    y   &= Y2MASK;
    yuv &= UVMASK|Y1MASK;
    *dst++ = y|yuv;
    src++;
  }
}

BLIT_BEGIN(blit_scan2_v)
BLIT_LOOP(blit_scan2_v_line, 1)
BLIT_END


/**********************************
 * rgbscan
 **********************************/
INLINE void FUNC_NAME(blit_rgbscan_h_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *mydst = dst;
  SRC_PIXEL *mysrc;
  int p,r,g,b,y,u,v;

  for(mysrc=src; mysrc<end; mysrc++)
  {
    p = RMASK_SEMI(GETPIXEL(*mysrc));
    r = RMASK(p);  r>>=16;
    g = GMASK(p);  g>>=8;
    b = BMASK(p);
    RGB2YUV(r,g,b,y,u,v);
    *mydst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
  }

  mydst = dst + dest_width/2;
  for(mysrc=src; mysrc<end; mysrc++)
  {
    p = GMASK_SEMI(GETPIXEL(*mysrc));
    r = RMASK(p);  r>>=16;
    g = GMASK(p);  g>>=8;
    b = BMASK(p);
    RGB2YUV(r,g,b,y,u,v);
    *mydst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
  }

  mydst = dst + dest_width;
  for(mysrc=src; mysrc<end; mysrc++)
  {
    p = BMASK_SEMI(GETPIXEL(*mysrc));
    r = RMASK(p);  r>>=16;
    g = GMASK(p);  g>>=8;
    b = BMASK(p);
    RGB2YUV(r,g,b,y,u,v);
    *mydst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
  }
}

BLIT_BEGIN(blit_rgbscan_h)
BLIT_LOOP(blit_rgbscan_h_line, 3)
BLIT_END

INLINE void FUNC_NAME(blit_rgbscan_v_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, unsigned int *dst, int dest_width, unsigned int *lookup)
{
  int r,g,b,r2,g2,b2,y,y2,u,v;

  while(src<end)
  {
    r  = g  = b  = RMASK_SEMI(GETPIXEL(*src));
    r2 = g2 = b2 = GMASK_SEMI(GETPIXEL(*src));

    r=RMASK(r);  r>>=16;
    g=GMASK(g);  g>>=8;
    b=BMASK(b);
    r2=RMASK(r2);  r2>>=16;
    g2=GMASK(g2);  g2>>=8;
    b2=BMASK(b2);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r  = g  = b  = BMASK_SEMI(GETPIXEL(*src));
    src++;
    r2 = g2 = b2 = RMASK_SEMI(GETPIXEL(*src));

    r=RMASK(r);  r>>=16;
    g=GMASK(g);  g>>=8;
    b=BMASK(b);
    r2=RMASK(r2);  r2>>=16;
    g2=GMASK(g2);  g2>>=8;
    b2=BMASK(b2);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r  = g  = b  = GMASK_SEMI(GETPIXEL(*src));
    r2 = g2 = b2 = BMASK_SEMI(GETPIXEL(*src));
    src++;

    r=RMASK(r);  r>>=16;
    g=GMASK(g);  g>>=8;
    b=BMASK(b);
    r2=RMASK(r2);  r2>>=16;
    g2=GMASK(g2);  g2>>=8;
    b2=BMASK(b2);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);
  }
}

BLIT_BEGIN(blit_rgbscan_v)
BLIT_LOOP(blit_rgbscan_v_line, 1)
BLIT_END


/**********************************
 * scan3, YUY2 version
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%. */
INLINE void FUNC_NAME(blit_scan3_h_line)(SRC_PIXEL *src, SRC_PIXEL *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,uv;
  SRC_PIXEL *mysrc;
  unsigned int *mydst = dst;

  for(mysrc=src; mysrc<end; mysrc++) {
    y   = uv = GET_YUV_PIXEL(*mysrc);
    y  &= YMASK;
#ifdef LSB_FIRST
    y   = (y*3)>>2;
#else
    y   = (y>>2)*3;
#endif
    y  &= YMASK;
    uv &= UVMASK;
    *mydst++ = y|uv;
  }

  mydst = dst + dest_width / 2;
  for(mysrc=src; mysrc<end; mysrc++)
    *mydst++ = GET_YUV_PIXEL(*mysrc);
    
  mydst = dst + dest_width;
  for(mysrc=src; mysrc<end; mysrc++) {
    y   = uv = GET_YUV_PIXEL(*mysrc);
    y  &= YMASK;
    y >>= 1;
    y  &= YMASK;
    uv &= UVMASK;
    *mydst++ = y|uv;
  }
}

BLIT_BEGIN(blit_scan3_h)
BLIT_LOOP(blit_scan3_h_line, 3)
BLIT_END

INLINE void FUNC_NAME(blit_scan3_v_line)(SRC_PIXEL *src, SRC_PIXEL *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,y2,y3,uv,uv2;

  while (src < end) {
    y = y2 = uv = GET_YUV_PIXEL(*src); src++;
    y  &= Y1MASK;
    y2 &= Y2MASK;
    uv &= UVMASK;
    y3 = y >> 1;
#ifdef LSB_FIRST
    y  = (y*3)>>2;
#else
    y  = (y>>2)*3;
    y  &= Y1MASK;
    y3 &= Y1MASK;
#endif
    *dst++ = y | y2 | uv;

    y = y2 = uv2 = GET_YUV_PIXEL(*src); src++;
    y   &= Y2MASK;
#ifdef LSB_FIRST
    y    = (y*3)>>2;
#else
    y    = (y>>2)*3;
#endif
    y   &= Y2MASK;
    uv2 &= UVMASK;
    uv >>= 1;
    uv  += (uv2>>1);
    uv  &= UVMASK;
    *dst++ = y3 | y | uv;
    
    y3   = (y2 & Y2MASK) >> 1;
    y2  &= Y1MASK;
    y3  &= Y2MASK;
    *dst++ = y2 | y3 | uv2;
  }
}

BLIT_BEGIN(blit_scan3_v)
BLIT_LOOP(blit_scan3_v_line, 1)
BLIT_END



#if 0
BLIT_BEGIN(blit_fakescan)
  switch(sysdep_display_params.widthscale)
  {
    case 1:
      BLIT_LOOP(blit_normal_line_1, 2);
      break;
    case 2:
      BLIT_LOOP(blit_normal_line_2, 2);
      break;
  }
BLIT_END
#endif
