#include "interp_defs.h"

/* Some glue defines, so that we can use the advancemame lookup
   tables unmodified. */
#define MUR FUNC_NAME(is_distant)(c[1], c[5])
#define MDR FUNC_NAME(is_distant)(c[5], c[7])
#define MDL FUNC_NAME(is_distant)(c[7], c[3])
#define MUL FUNC_NAME(is_distant)(c[3], c[1])

/* 2 variants of the is_distant function and mask making code */
#if RENDER_DEPTH != 32
#include "xq2x_yuv.h"

INLINE int FUNC_NAME(is_distant)(interp_uint16 w1, interp_uint16 w2)
{
  int yuv1, yuv2;

  yuv1 = HQ2X_YUVLOOKUP(w1);
  yuv2 = HQ2X_YUVLOOKUP(w2);
  return ( ( ((yuv1 & XQ2X_YMASK) - (yuv2 & XQ2X_YMASK)) >  XQ2X_TR_Y ) ||
           ( ((yuv1 & XQ2X_YMASK) - (yuv2 & XQ2X_YMASK)) < -XQ2X_TR_Y ) ||
           ( ((yuv1 & XQ2X_UMASK) - (yuv2 & XQ2X_UMASK)) >  XQ2X_TR_U ) ||
           ( ((yuv1 & XQ2X_UMASK) - (yuv2 & XQ2X_UMASK)) < -XQ2X_TR_U ) ||
           ( ((yuv1 & XQ2X_VMASK) - (yuv2 & XQ2X_VMASK)) >  XQ2X_TR_V ) ||
           ( ((yuv1 & XQ2X_VMASK) - (yuv2 & XQ2X_VMASK)) < -XQ2X_TR_V ) );
}

#define XQ2X_LINE_LOOP_BEGIN \
  interp_uint16 c[9]; \
  int i, y, u, v, yuv; \
  \
  c[1] = XQ2X_GETPIXEL(src0[-1]); \
  c[2] = XQ2X_GETPIXEL(src0[ 0]); \
  c[4] = XQ2X_GETPIXEL(src1[-1]); \
  c[5] = XQ2X_GETPIXEL(src1[ 0]); \
  c[7] = XQ2X_GETPIXEL(src2[-1]); \
  c[8] = XQ2X_GETPIXEL(src2[ 0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[1]; \
    c[1] = c[2]; \
    c[2] = XQ2X_GETPIXEL(src0[1]); \
    c[3] = c[4]; \
    c[4] = c[5]; \
    c[5] = XQ2X_GETPIXEL(src1[1]); \
    c[6] = c[7]; \
    c[7] = c[8]; \
    c[8] = XQ2X_GETPIXEL(src2[1]); \
    \
    i  = HQ2X_YUVLOOKUP(c[4]); \
    y = i & XQ2X_YMASK; \
    u = i & XQ2X_UMASK; \
    v = i & XQ2X_VMASK; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      yuv = HQ2X_YUVLOOKUP(c[i]); \
      if ( ( (y - (yuv & XQ2X_YMASK)) >  XQ2X_TR_Y ) || \
           ( (y - (yuv & XQ2X_YMASK)) < -XQ2X_TR_Y ) || \
           ( (u - (yuv & XQ2X_UMASK)) >  XQ2X_TR_U ) || \
           ( (u - (yuv & XQ2X_UMASK)) < -XQ2X_TR_U ) || \
           ( (v - (yuv & XQ2X_VMASK)) >  XQ2X_TR_V ) || \
           ( (v - (yuv & XQ2X_VMASK)) < -XQ2X_TR_V ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#define XQ2X_LINE_LOOP_BEGIN_SWAP_XY \
  interp_uint16 c[9]; \
  int i, y, u, v, yuv; \
  \
  c[3] = XQ2X_GETPIXEL(src0[-1]); \
  c[4] = XQ2X_GETPIXEL(src1[-1]); \
  c[5] = XQ2X_GETPIXEL(src2[-1]); \
  c[6] = XQ2X_GETPIXEL(src0[0]); \
  c[7] = XQ2X_GETPIXEL(src1[0]); \
  c[8] = XQ2X_GETPIXEL(src2[0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[3]; \
    c[1] = c[4]; \
    c[2] = c[5]; \
    c[3] = c[6]; \
    c[4] = c[7]; \
    c[5] = c[8]; \
    c[6] = XQ2X_GETPIXEL(src0[1]); \
    c[7] = XQ2X_GETPIXEL(src1[1]); \
    c[8] = XQ2X_GETPIXEL(src2[1]); \
    \
    i  = HQ2X_YUVLOOKUP(c[4]); \
    y = i & XQ2X_YMASK; \
    u = i & XQ2X_UMASK; \
    v = i & XQ2X_VMASK; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      yuv = HQ2X_YUVLOOKUP(c[i]); \
      if ( ( (y - (yuv & XQ2X_YMASK)) >  XQ2X_TR_Y ) || \
           ( (y - (yuv & XQ2X_YMASK)) < -XQ2X_TR_Y ) || \
           ( (u - (yuv & XQ2X_UMASK)) >  XQ2X_TR_U ) || \
           ( (u - (yuv & XQ2X_UMASK)) < -XQ2X_TR_U ) || \
           ( (v - (yuv & XQ2X_VMASK)) >  XQ2X_TR_V ) || \
           ( (v - (yuv & XQ2X_VMASK)) < -XQ2X_TR_V ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#else /* RENDER_DEPTH != 32 */

INLINE int FUNC_NAME(is_distant)(RENDER_PIXEL w1, RENDER_PIXEL w2)
{
  int r1, g1, b1, r2, g2, b2;

  r1 = RMASK(w1) >> 16;
  g1 = GMASK(w1) >> 8;
  b1 = BMASK(w1);
  r2 = RMASK(w2) >> 16;
  g2 = GMASK(w2) >> 8;
  b2 = BMASK(w2);

  return ( ( ( (r1+g1+b1) - (r2+g2+b2) )    >  0xC0 ) ||
           ( ( (r1+g1+b1) - (r2+g2+b2) )    < -0xC0 ) ||
           ( ( (r1-b1)    - (r2-b2)    )    >  0x1C ) ||
           ( ( (r1-b1)    - (r2-b2)    )    < -0x1C ) ||
           ( ( (-r1+2*g1-b1) - (-r2+2*g2-b2) ) >  0x18 ) ||
           ( ( (-r1+2*g1-b1) - (-r2+2*g2-b2) ) < -0x18 ) );
}

#define XQ2X_LINE_LOOP_BEGIN \
  RENDER_PIXEL c[9]; \
  int i, r, g, b, y, u, v; \
  \
  c[1] = XQ2X_GETPIXEL(src0[-1]); \
  c[2] = XQ2X_GETPIXEL(src0[ 0]); \
  c[4] = XQ2X_GETPIXEL(src1[-1]); \
  c[5] = XQ2X_GETPIXEL(src1[ 0]); \
  c[7] = XQ2X_GETPIXEL(src2[-1]); \
  c[8] = XQ2X_GETPIXEL(src2[ 0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[1]; \
    c[1] = c[2]; \
    c[2] = XQ2X_GETPIXEL(src0[1]); \
    c[3] = c[4]; \
    c[4] = c[5]; \
    c[5] = XQ2X_GETPIXEL(src1[1]); \
    c[6] = c[7]; \
    c[7] = c[8]; \
    c[8] = XQ2X_GETPIXEL(src2[1]); \
    \
    r = RMASK(c[4]) >> 16; \
    g = GMASK(c[4]) >> 8; \
    b = BMASK(c[4]); \
    y = r+g+b; \
    u = r-b; \
    v = -r+2*g-b; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      r = RMASK(c[i]) >> 16; \
      g = GMASK(c[i]) >> 8; \
      b = BMASK(c[i]); \
      if ( ( (y - (r+g+b) )    >  0xC0 ) || \
           ( (y - (r+g+b) )    < -0xC0 ) || \
           ( (u - (r-b)    )   >  0x1C ) || \
           ( (u - (r-b)    )   < -0x1C ) || \
           ( (v - (-r+2*g-b) ) >  0x18 ) || \
           ( (v - (-r+2*g-b) ) < -0x18 ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#define XQ2X_LINE_LOOP_BEGIN_SWAP_XY \
  RENDER_PIXEL c[9]; \
  int i, r, g, b, y, u, v; \
  \
  c[3] = XQ2X_GETPIXEL(src0[-1]); \
  c[4] = XQ2X_GETPIXEL(src1[-1]); \
  c[5] = XQ2X_GETPIXEL(src2[-1]); \
  c[6] = XQ2X_GETPIXEL(src0[0]); \
  c[7] = XQ2X_GETPIXEL(src1[0]); \
  c[8] = XQ2X_GETPIXEL(src2[0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[3]; \
    c[1] = c[4]; \
    c[2] = c[5]; \
    c[3] = c[6]; \
    c[4] = c[7]; \
    c[5] = c[8]; \
    c[6] = XQ2X_GETPIXEL(src0[1]); \
    c[7] = XQ2X_GETPIXEL(src1[1]); \
    c[8] = XQ2X_GETPIXEL(src2[1]); \
    \
    r = RMASK(c[4]) >> 16; \
    g = GMASK(c[4]) >> 8; \
    b = BMASK(c[4]); \
    y = r+g+b; \
    u = r-b; \
    v = -r+2*g-b; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      r = RMASK(c[i]) >> 16; \
      g = GMASK(c[i]) >> 8; \
      b = BMASK(c[i]); \
      if ( ( (y - (r+g+b) )    >  0xC0 ) || \
           ( (y - (r+g+b) )    < -0xC0 ) || \
           ( (u - (r-b)    )   >  0x1C ) || \
           ( (u - (r-b)    )   < -0x1C ) || \
           ( (v - (-r+2*g-b) ) >  0x18 ) || \
           ( (v - (-r+2*g-b) ) < -0x18 ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#endif /* RENDER_DEPTH != 32 */

#define XQ2X_LINE_LOOP_END \
    src0++; \
    src1++; \
    src2++; \
  }

#define XQ2X_NAME(x) x##_hq2x
#define XQ2X_FUNC_NAME(x) FUNC_NAME(x##_hq2x)
