/* we use GETPIXEL to get the pixels, GETPIXEL should return a pixel
   of RENDER_DEPTH, so all operations we do should be based on RENDER_DEPTH */
#if RENDER_DEPTH == 15
#define INTERP_16_MASK_1(v) ((v) & 0x7C1F)
#define INTERP_16_MASK_2(v) ((v) & 0x03E0)
#define INTERP_16_UNMASK_1(v) ((v) & 0x7C1F)
#define INTERP_16_UNMASK_2(v) ((v) & 0x03E0)
#define INTERP_16_HNMASK (~0x4210)
#define interp_uint16 unsigned short
#elif RENDER_DEPTH == 16
#define INTERP_16_MASK_1(v) ((v) & 0xF81F)
#define INTERP_16_MASK_2(v) ((v) & 0x07E0)
#define INTERP_16_UNMASK_1(v) ((v) & 0xF81F)
#define INTERP_16_UNMASK_2(v) ((v) & 0x07E0)
#define INTERP_16_HNMASK (~0x8410)
#define interp_uint16 unsigned short
#elif RENDER_DEPTH == 32
#define INTERP_16_MASK_1(v) ((v) & 0xFF00FF)
#define INTERP_16_MASK_2(v) ((v) & 0x00FF00)
#define INTERP_16_UNMASK_1(v) ((v) & 0xFF00FF)
#define INTERP_16_UNMASK_2(v) ((v) & 0x00FF00)
#define INTERP_16_HNMASK (~0x808080)
#define interp_uint16 unsigned int
#endif

#include "interp.h"

/* Some glue defines, so that we can use the advancemame lookup
   tables unmodified. */
#define MUR FUNC_NAME(is_distant)(c[1], c[5])
#define MDR FUNC_NAME(is_distant)(c[5], c[7])
#define MDL FUNC_NAME(is_distant)(c[7], c[3])
#define MUL FUNC_NAME(is_distant)(c[3], c[1])
#define I1(p0) c[p0]
#define I2(i0, i1, p0, p1) interp_16_##i0##i1(c[p0], c[p1])
#define I3(i0, i1, i2, p0, p1, p2) interp_16_##i0##i1##i2(c[p0], c[p1], c[p2])

/* 2 variants of the is_distant function and mask making code */
#if HQ2X_USE_YUV_LOOKUP
#include "hq2x_yuv.h"

INLINE int FUNC_NAME(is_distant)(RENDER_PIXEL w1, RENDER_PIXEL w2)
{
  int yuv1, yuv2;

  yuv1 = HQ2X_YUVLOOKUP(w1);
  yuv2 = HQ2X_YUVLOOKUP(w2);
  return ( ( ((yuv1 & HQ2X_YMASK) - (yuv2 & HQ2X_YMASK)) >  HQ2X_TR_Y ) ||
           ( ((yuv1 & HQ2X_YMASK) - (yuv2 & HQ2X_YMASK)) < -HQ2X_TR_Y ) ||
           ( ((yuv1 & HQ2X_UMASK) - (yuv2 & HQ2X_UMASK)) >  HQ2X_TR_U ) ||
           ( ((yuv1 & HQ2X_UMASK) - (yuv2 & HQ2X_UMASK)) < -HQ2X_TR_U ) ||
           ( ((yuv1 & HQ2X_VMASK) - (yuv2 & HQ2X_VMASK)) >  HQ2X_TR_V ) ||
           ( ((yuv1 & HQ2X_VMASK) - (yuv2 & HQ2X_VMASK)) < -HQ2X_TR_V ) );
}

#define HQ2X_LINE_LOOP_BEGIN \
  RENDER_PIXEL c[9]; \
  int i, y1, u1, v1, yuv2; \
  \
  c[1] = HQ2X_GETPIXEL(src0[-1]); \
  c[2] = HQ2X_GETPIXEL(src0[ 0]); \
  c[4] = HQ2X_GETPIXEL(src1[-1]); \
  c[5] = HQ2X_GETPIXEL(src1[ 0]); \
  c[7] = HQ2X_GETPIXEL(src2[-1]); \
  c[8] = HQ2X_GETPIXEL(src2[ 0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[1]; \
    c[1] = c[2]; \
    c[2] = HQ2X_GETPIXEL(src0[1]); \
    c[3] = c[4]; \
    c[4] = c[5]; \
    c[5] = HQ2X_GETPIXEL(src1[1]); \
    c[6] = c[7]; \
    c[7] = c[8]; \
    c[8] = HQ2X_GETPIXEL(src2[1]); \
    \
    i  = HQ2X_YUVLOOKUP(c[4]); \
    y1 = i & HQ2X_YMASK; \
    u1 = i & HQ2X_UMASK; \
    v1 = i & HQ2X_VMASK; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      yuv2 = HQ2X_YUVLOOKUP(c[i]); \
      if ( ( (y1 - (yuv2 & HQ2X_YMASK)) >  HQ2X_TR_Y ) || \
           ( (y1 - (yuv2 & HQ2X_YMASK)) < -HQ2X_TR_Y ) || \
           ( (u1 - (yuv2 & HQ2X_UMASK)) >  HQ2X_TR_U ) || \
           ( (u1 - (yuv2 & HQ2X_UMASK)) < -HQ2X_TR_U ) || \
           ( (v1 - (yuv2 & HQ2X_VMASK)) >  HQ2X_TR_V ) || \
           ( (v1 - (yuv2 & HQ2X_VMASK)) < -HQ2X_TR_V ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#else

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

#define HQ2X_LINE_LOOP_BEGIN \
  RENDER_PIXEL c[9]; \
  int i, r, g, b, y1, u1, v1; \
  \
  c[1] = HQ2X_GETPIXEL(src0[-1]); \
  c[2] = HQ2X_GETPIXEL(src0[ 0]); \
  c[4] = HQ2X_GETPIXEL(src1[-1]); \
  c[5] = HQ2X_GETPIXEL(src1[ 0]); \
  c[7] = HQ2X_GETPIXEL(src2[-1]); \
  c[8] = HQ2X_GETPIXEL(src2[ 0]); \
  \
  while(src1 < end1) \
  { \
    unsigned char mask = 0, flag = 1; \
    \
    c[0] = c[1]; \
    c[1] = c[2]; \
    c[2] = HQ2X_GETPIXEL(src0[1]); \
    c[3] = c[4]; \
    c[4] = c[5]; \
    c[5] = HQ2X_GETPIXEL(src1[1]); \
    c[6] = c[7]; \
    c[7] = c[8]; \
    c[8] = HQ2X_GETPIXEL(src2[1]); \
    \
    r = RMASK(c[4]) >> 16; \
    g = GMASK(c[4]) >> 8; \
    b = BMASK(c[4]); \
    y1 = r+g+b; \
    u1 = r-b; \
    v1 = -r+2*g-b; \
    \
    for ( i = 0; i <= 8; i++ ) \
    { \
      if ( i == 4 ) \
        continue; \
      r = RMASK(c[i]) >> 16; \
      g = GMASK(c[i]) >> 8; \
      b = BMASK(c[i]); \
      if ( ( (y1 - (r+g+b) )    >  0xC0 ) || \
           ( (y1 - (r+g+b) )    < -0xC0 ) || \
           ( (u1 - (r-b)    )   >  0x1C ) || \
           ( (u1 - (r-b)    )   < -0x1C ) || \
           ( (v1 - (-r+2*g-b) ) >  0x18 ) || \
           ( (v1 - (-r+2*g-b) ) < -0x18 ) ) \
        mask |= flag; \
      flag <<= 1; \
    }

#endif

#define HQ2X_LINE_LOOP_END \
    src0++; \
    src1++; \
    src2++; \
  }
