#if DEST_DEPTH == 15
# define FUNC_NAME(name) name##_15
# define PIXEL UINT16
# define HQ2XPIXEL PIXEL
# define RGB2YUV(p) rgb2yuv[p]
# define RGB2PIXEL(r,g,b) ((((r)&0xf8)<<7)|(((g)&0xf8)<<2)|(((b)&0xf8)>>3))
# define RGB32_2PIXEL(rgb) ((((rgb)&0xf80000)>>9)|(((rgb)&0xf800)>>6)|(((rgb)&0xf8)>>3))
# define PIXEL2RGB32(p) (((UINT32)RMASK15(p)<<9)|((UINT32)GMASK15(p)<<6)|((UINT32)BMASK15(p)<<3))
# define gmask 0x03E0
# define pmask 0xFC1F
# define is_distant is_distant_15
  INLINE int is_distant_15( UINT16 w1, UINT16 w2 )
  {
    int yuv1, yuv2;

    yuv1 = rgb2yuv[w1];
    yuv2 = rgb2yuv[w2];
    return ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
             ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
             ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) );
  }
#elif DEST_DEPTH == 16
# define FUNC_NAME(name) name##_16
# define PIXEL UINT16
# define HQ2XPIXEL PIXEL
# define RGB2YUV(p) rgb2yuv[p]
# define RGB2PIXEL(r,g,b) ((((r)&0xf8)<<8)|(((g)&0xfc)<<3)|(((b)&0xf8)>>3))
# define RGB32_2PIXEL(rgb) ((((rgb)&0xf80000)>>8)|(((rgb)&0x00fc00)>>5)|(((rgb)&0x0000f8)>>3))
# define PIXEL2RGB32(p) u32lookup[p]
# define gmask 0x07E0
# define pmask 0xF81F
# define is_distant is_distant_15
#elif DEST_DEPTH == 32
# define FUNC_NAME(name) name##_32
# define PIXEL UINT32
# define HQ2XPIXEL PIXEL
# define PIXEL2RGB32(p) (p)
# define rmask 0x00FF0000
# define gmask 0x0000FF00
# define bmask 0x000000FF
# define pmask 0x00FF00FF
# define is_distant is_distant_32
  INLINE int is_distant_32( UINT32 w1, UINT32 w2 )
  {
    int r1, g1, b1, r2, g2, b2;

    r1 = (w1 & rmask) >> 16;
    g1 = (w1 & gmask) >> 8;
    b1 = (w1 & bmask);
    r2 = (w2 & rmask) >> 16;
    g2 = (w2 & gmask) >> 8;
    b2 = (w2 & bmask);

    return ( ( abs( (r1+g1+b1)    - (r2+g2+b2) )    > trY32 ) ||
             ( abs( (r1-b1)       - (r2-b2)    )    > trU32 ) ||
             ( abs( (-r1+2*g1-b1) - (-r2+2*g2-b2) ) > trV32 ) );
  }
#elif DEST_DEPTH == YUY2
# define FUNC_NAME(name) name##_YUY2
# define PIXEL UINT16
# define HQ2XPIXEL UINT32 /* hq2x YUY2 hack */
# define RGB2YUV(p) (p)
# define RGB2PIXEL(r,g,b) ( \
/* y */ ((  9836*(r) + 19310*(g) +  3750*(b)) >> 15) | (((!(i&0x01))? \
/* u */ (( -5527*(r) - 10921*(g) + 16448*(b) + 4194304) >> 7): \
/* v */ (( 16448*(r) - 13783*(g) -  2665*(b) + 4194304) >> 7)) & 0xFF00))
# define RGB32_2PIXEL(rgb) RGB2PIXEL(RMASK32(rgb)>>16,GMASK32(rgb)>>8,BMASK32(rgb))
# define gmask 0x0000FF00 /* xq2x YUY2 hack */
# define pmask 0x00FF00FF /* xq2x YUY2 hack */
# define is_distant is_distant_YUY2
  INLINE int is_distant_YUY2( UINT32 yuv1, UINT32 yuv2 )
  {
    return ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
             ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
             ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) );
  }
#else
# error Unknown DEST_DEPTH
#endif

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

INLINE void FUNC_NAME(hq2x)( HQ2XPIXEL *mydst0, HQ2XPIXEL *mydst1, HQ2XPIXEL w[9] )
{
  int    pattern = 0;
  int    flag    = 1;
  int    c;
#if DEST_DEPTH == 32
  int    r1, g1, b1, r2, g2, b2, y1, u1, v1;
  r1 = (w[4] & rmask) >> 16;
  g1 = (w[4] & gmask) >> 8;
  b1 = (w[4] & bmask);
  y1 = r1+g1+b1;
  u1 = r1-b1;
  v1 = -r1+2*g1-b1;
#else
  int    yuv1, yuv2;
  yuv1 = RGB2YUV(w[4]);
#endif

  for ( c = 0; c <= 8; c++ )
  {
    if ( c == 4 )
      continue;
#if DEST_DEPTH == 32
    r2 = (w[c] & rmask) >> 16;
    g2 = (w[c] & gmask) >> 8; 
    b2 = (w[c] & bmask);
    if ( ( abs( y1 - (r2+g2+b2) )    > trY32 ) ||
         ( abs( u1 - (r2-b2)    )    > trU32 ) ||
         ( abs( v1 - (-r2+2*g2-b2) ) > trV32 ) ) 
#else
    yuv2 = RGB2YUV(w[c]);
    if ( ( abs((yuv1 & Ymask) - (yuv2 & Ymask)) > trY ) ||
         ( abs((yuv1 & Umask) - (yuv2 & Umask)) > trU ) ||
         ( abs((yuv1 & Vmask) - (yuv2 & Vmask)) > trV ) )
#endif
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
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 2 :
    case 34 :
    case 130 :
    case 162 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 3 :
    case 35 :
    case 131 :
    case 163 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 6 :
    case 38 :
    case 134 :
    case 166 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 7 :
    case 39 :
    case 135 :
    case 167 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 8 :
    case 12 :
    case 136 :
    case 140 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 9 :
    case 13 :
    case 137 :
    case 141 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 10 :
    case 138 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 11 :
    case 139 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 15 :
    case 143 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = w[4];
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 16 :
    case 17 :
    case 48 :
    case 49 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 18 :
    case 50 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 20 :
    case 21 :
    case 52 :
    case 53 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 22 :
    case 54 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 23 :
    case 55 :
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 24 :
    case 66 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 25 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 26 :
    case 31 :
    case 95 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 27 :
    case 75 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 28 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 29 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 30 :
    case 86 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 40 :
    case 44 :
    case 168 :
    case 172 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 41 :
    case 45 :
    case 169 :
    case 173 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
        *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 43 :
    case 171 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = w[4];
        *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 46 :
    case 174 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 56 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 57 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 58 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 59 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 60 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 61 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 62 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 63 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 64 :
    case 65 :
    case 68 :
    case 69 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 67 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 70 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 71 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 72 :
    case 76 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 73 :
    case 77 :
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 74 :
    case 107 :
    case 123 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 78 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 79 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 80 :
    case 81 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 83 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 84 :
    case 85 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 87 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 89 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 90 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 91 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 92 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 93 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 94 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 96 :
    case 97 :
    case 100 :
    case 101 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 98 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 99 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 102 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 103 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 104 :
    case 108 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 105 :
    case 109 :
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *mydst1     = w[4];
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 106 :
    case 120 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 110 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 111 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 112 :
    case 113 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst1     = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 114 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 115 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 116 :
    case 117 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 118 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 119 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 121 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 122 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst1+1) = ((((w[4] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 124 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 125 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *mydst1     = w[4];
      }
      else
      {
        *mydst0     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 126 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 127 :
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[8] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[8] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 144 :
    case 145 :
    case 176 :
    case 177 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 146 :
    case 178 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 147 :
    case 179 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 148 :
    case 149 :
    case 180 :
    case 181 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 150 :
    case 182 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *(mydst0+1) = w[4];
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 151 :
    case 183 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 152 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 153 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 154 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 155 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 156 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 157 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 158 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 159 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 184 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 185 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 186 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 187 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = w[4];
        *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[4] & gmask)*5 + (w[3] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[3] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 188 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 189 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 190 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
      {
        *(mydst0+1) = w[4];
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[7] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 191 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 192 :
    case 193 :
    case 196 :
    case 197 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 194 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 195 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 198 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 199 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 200 :
    case 204 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 201 :
    case 205 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 202 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 203 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 206 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 207 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
      {
        *mydst0     = w[4];
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[1] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[1] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 208 :
    case 209 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 210 :
    case 216 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 211 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 212 :
    case 213 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = w[4];
      }
      else
      {
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 215 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 217 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 218 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 219 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 220 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 221 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = w[4];
      }
      else
      {
        *(mydst0+1) = ((((w[4] & gmask)*5 + (w[5] & gmask)*2 + (w[1] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[5] & pmask)*2 + (w[1] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 223 :
      *mydst1     = ((((w[4] & gmask)*3 + (w[6] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[6] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 224 :
    case 225 :
    case 228 :
    case 229 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 226 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 227 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 230 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 231 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 232 :
    case 236 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst1     = w[4];
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 233 :
    case 237 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 234 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      else
        *mydst0     = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 235 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 238 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
      {
        *mydst1     = w[4];
        *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      else
      {
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[5] & pmask)) & (pmask << 3))) >> 3;
      }
      break;
    case 239 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      *(mydst1+1) = ((((w[4] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 240 :
    case 241 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 242 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      else
        *(mydst0+1) = ((((w[4] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 243 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
      {
        *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[4] & gmask)*5 + (w[7] & gmask)*2 + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*5 + (w[7] & pmask)*2 + (w[3] & pmask)) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 244 :
    case 245 :
      *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 246 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 247 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      *mydst1     = ((((w[4] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 249 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 251 :
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[2] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[2] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 252 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 253 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 254 :
      *mydst0     = ((((w[4] & gmask)*3 + (w[0] & gmask)) & (gmask << 2)) |
                      (((w[4] & pmask)*3 + (w[0] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 255 :
      if ( is_distant( w[3], w[7] ) )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[5], w[7] ) )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[3] ) )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( is_distant( w[1], w[5] ) )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
  }

  return;
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

#if DEST_DEPTH != YUY2 /* this function is identical for 32 bpp and YUY2 */
INLINE void FUNC_NAME(lq2x)(PIXEL *mydst0, PIXEL *mydst1, PIXEL w[9] )
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
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
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
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      break;
    case 3 :
    case 35 :
    case 67 :
    case 99 :
    case 131 :
    case 163 :
    case 195 :
    case 227 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      break;
    case 7 :
    case 39 :
    case 71 :
    case 103 :
    case 135 :
    case 167 :
    case 199 :
    case 231 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      break;
    case 10 :
    case 138 :
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 11 :
    case 27 :
    case 75 :
    case 139 :
    case 155 :
    case 203 :
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 14 :
    case 142 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *mydst0     = w[4];
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[0] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
      }
      break;
    case 15 :
    case 143 :
    case 207 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *mydst0     = w[4];
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[4] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[4] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst0+1) = ((((w[4] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
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
      *mydst0     = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 19 :
    case 51 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *mydst0     = w[4];
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[2] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[2] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[2] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 23 :
    case 55 :
    case 119 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[1] != w[5] )
      {
        *mydst0     = w[4];
        *(mydst0+1) = w[4];
      }
      else
      {
        *mydst0     = ((((w[3] & gmask)*3 + (w[1] & gmask)) & (gmask << 2)) |
                        (((w[3] & pmask)*3 + (w[1] & pmask)) & (pmask << 2))) >> 2;
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[3] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[3] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 26 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 31 :
    case 95 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 42 :
    case 170 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *mydst0     = w[4];
        *mydst1     = w[4];
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[0] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      }
      break;
    case 43 :
    case 171 :
    case 187 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
      {
        *mydst0     = w[4];
        *mydst1     = w[4];
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)*3 + (w[2] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)*3 + (w[2] & pmask)*2) & (pmask << 3))) >> 3;
        *mydst1     = ((((w[2] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
      }
      break;
    case 46 :
    case 174 :
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 47 :
    case 175 :
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 58 :
    case 154 :
    case 186 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 59 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[2] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 63 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 72 :
    case 76 :
    case 104 :
    case 106 :
    case 108 :
    case 110 :
    case 120 :
    case 124 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 73 :
    case 77 :
    case 105 :
    case 109 :
    case 125 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
      {
        *mydst0     = w[4];
        *mydst1     = w[4];
      }
      else
      {
        *mydst0     = ((((w[1] & gmask)*3 + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*3 + (w[3] & pmask)) & (pmask << 2))) >> 2;
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[1] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[1] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 74 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 78 :
    case 202 :
    case 206 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 79 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[4] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 80 :
    case 208 :
    case 210 :
    case 216 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 81 :
    case 209 :
    case 217 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 82 :
    case 214 :
    case 222 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 83 :
    case 115 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[2] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[2] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 84 :
    case 212 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(mydst0+1) = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *(mydst0+1) = ((((w[0] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 85 :
    case 213 :
    case 221 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
      {
        *(mydst0+1) = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[1] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[1] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 87 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[3] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[3] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[3] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[3] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 88 :
    case 248 :
    case 250 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 89 :
    case 93 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[1] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[1] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 90 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 91 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[2] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[2] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[2] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 92 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 94 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 107 :
    case 123 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[2] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 111 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 112 :
    case 240 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *mydst1     = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[0] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 113 :
    case 241 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *mydst1     = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[1] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[1] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[1] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 114 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 116 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 117 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[1] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 121 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[1] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 122 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*6 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 126 :
      *mydst0     = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 127 :
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 146 :
    case 150 :
    case 178 :
    case 182 :
    case 190 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[1] != w[5] )
      {
        *(mydst0+1) = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *(mydst0+1) = ((((w[1] & gmask)*3 + (w[5] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[1] & pmask)*3 + (w[5] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[0] & gmask)*3 + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[5] & pmask)) & (pmask << 2))) >> 2;
      }
      break;
    case 147 :
    case 179 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[2] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[2] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 151 :
    case 183 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[3] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[3] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 158 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 159 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 191 :
      *mydst1     = w[4];
      *(mydst1+1) = w[4];
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 200 :
    case 204 :
    case 232 :
    case 236 :
    case 238 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
      {
        *mydst1     = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[3] & gmask)*3 + (w[7] & gmask)*3 + (w[0] & gmask)*2) & (gmask << 3)) |
                        (((w[3] & pmask)*3 + (w[7] & pmask)*3 + (w[0] & pmask)*2) & (pmask << 3))) >> 3;
        *(mydst1+1) = ((((w[0] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
      }
      break;
    case 201 :
    case 205 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[1] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 211 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[2] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 215 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[3] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[3] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[3] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[3] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 218 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 219 :
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[2] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 220 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*6 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 3))) >> 3;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 223 :
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[4] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 233 :
    case 237 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[1] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 234 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 235 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[2] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[2] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 239 :
      *(mydst0+1) = w[4];
      *(mydst1+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 242 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*6 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 3)) |
                        (((w[0] & pmask)*6 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 3))) >> 3;
      break;
    case 243 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[5] != w[7] )
      {
        *mydst1     = w[4];
        *(mydst1+1) = w[4];
      }
      else
      {
        *mydst1     = ((((w[2] & gmask)*3 + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*3 + (w[7] & pmask)) & (pmask << 2))) >> 2;
        *(mydst1+1) = ((((w[5] & gmask)*3 + (w[7] & gmask)*3 + (w[2] & gmask)*2) & (gmask << 3)) |
                        (((w[5] & pmask)*3 + (w[7] & pmask)*3 + (w[2] & pmask)*2) & (pmask << 3))) >> 3;
      }
      break;
    case 244 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[0] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 245 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[1] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 246 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[0] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 247 :
      *mydst0     = w[4];
      *mydst1     = w[4];
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[3] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[3] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[3] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[3] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 249 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[1] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[1] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 251 :
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[2] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[2] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[2] & gmask)*2 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[2] & gmask)*2 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 2)) |
                        (((w[2] & pmask)*2 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 252 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[0] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 253 :
      *mydst0     = w[4];
      *(mydst0+1) = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[1] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[1] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[1] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[1] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      break;
    case 254 :
      *mydst0     = w[4];
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[0] & gmask)*2 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 2))) >> 2;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[0] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[0] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[0] & gmask)*2 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 2)) |
                        (((w[0] & pmask)*2 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 2))) >> 2;
      break;
    case 255 :
      if ( w[7] != w[3] )
        *mydst1     = w[4];
      else
        *mydst1     = ((((w[4] & gmask)*14 + (w[3] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[3] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[5] != w[7] )
        *(mydst1+1) = w[4];
      else
        *(mydst1+1) = ((((w[4] & gmask)*14 + (w[5] & gmask) + (w[7] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[5] & pmask) + (w[7] & pmask)) & (pmask << 4))) >> 4;
      if ( w[3] != w[1] )
        *mydst0     = w[4];
      else
        *mydst0     = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[3] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[3] & pmask)) & (pmask << 4))) >> 4;
      if ( w[1] != w[5] )
        *(mydst0+1) = w[4];
      else
        *(mydst0+1) = ((((w[4] & gmask)*14 + (w[1] & gmask) + (w[5] & gmask)) & (gmask << 4)) |
                        (((w[4] & pmask)*14 + (w[1] & pmask) + (w[5] & pmask)) & (pmask << 4))) >> 4;
      break;
  }
  return;
}
#endif /* DEST_DEPTH != YUY2 */

/**********************************
 * 6tap2x: 6-tap sinc filter with light scanlines
 **********************************/

#define Clip(a) (((a) < 0) ? 0 : (((a) > 0xff) ? 0xff : (a)))

/* we also use this header file to generate the addline funcs, but those
   don't have yuv variants */
#if DEST_DEPTH != YUY2 
void FUNC_NAME(effect_6tap_addline)(const void *src0, unsigned count)
{
  PIXEL *mysrc = (PIXEL *)src0;
  UINT32 *u32dest;
  UINT8 *u8dest;
  INT32 pixel;
  UINT32 i;
  char *tmp;
#if DEST_DEPTH == 16
  UINT32 *u32lookup = current_palette->lookup;
#endif

  /* first, move the existing lines up by one */
  tmp = _6tap2x_buf0;
  _6tap2x_buf0 = _6tap2x_buf1;
  _6tap2x_buf1 = _6tap2x_buf2;
  _6tap2x_buf2 = _6tap2x_buf3;
  _6tap2x_buf3 = _6tap2x_buf4;
  _6tap2x_buf4 = _6tap2x_buf5;
  _6tap2x_buf5 = tmp;

  /* if there's no new line, clear the last one and return */
  if (!src0)
    {
	memset(_6tap2x_buf5, 0, count << 3);
	return;
    }

  /* we have a new line, so first do the palette lookup and zoom by 2 */
  u32dest = (UINT32 *) _6tap2x_buf5;
  for (i = 0; i < count; i++)
    {
    *u32dest++ = PIXEL2RGB32(*mysrc);
    mysrc++;
    u32dest++;
    }

  /* just replicate the first 2 and last 3 pixels */
  u32dest[-1] = u32dest[-2];
  u32dest[-3] = u32dest[-4];
  u32dest[-5] = u32dest[-6];
  u32dest = (UINT32 *) _6tap2x_buf5;
  u32dest[1] = u32dest[0];
  u32dest[3] = u32dest[2];

  /* finally, do the horizontal 6-tap filter for the remaining half-pixels */
  u8dest = ((UINT8 *) _6tap2x_buf5) + 20;
  for (i = 2; i < count - 3; i++)
    {
	/* first, do the blue part */
	pixel = (((INT32)  u8dest[-4] + (INT32) u8dest[4]) << 2) -
	         ((INT32) u8dest[-12] + (INT32) u8dest[12]);
	pixel += pixel << 2;
	pixel += ((INT32) u8dest[-20] + (INT32) u8dest[20]);
	pixel = (pixel + 0x10) >> 5;
	*u8dest++ = Clip(pixel);
	/* next, do the green part */
	pixel = (((INT32)  u8dest[-4] + (INT32) u8dest[4]) << 2) -
	         ((INT32) u8dest[-12] + (INT32) u8dest[12]);
	pixel += pixel << 2;
	pixel += ((INT32) u8dest[-20] + (INT32) u8dest[20]);
	pixel = (pixel + 0x10) >> 5;
	*u8dest++ = Clip(pixel);
	/* last, do the red part */
	pixel = (((INT32)  u8dest[-4] + (INT32) u8dest[4]) << 2) -
	         ((INT32) u8dest[-12] + (INT32) u8dest[12]);
	pixel += pixel << 2;
	pixel += ((INT32) u8dest[-20] + (INT32) u8dest[20]);
	pixel = (pixel + 0x10) >> 5;
	*u8dest++ = Clip(pixel);
	/* clear the last byte */
	*u8dest++ = 0;
	u8dest += 4;
    }
}
#endif /* DEST_DEPTH != YUY2  */

#if DEST_DEPTH != 32 /* we have a special optimised 32 bpp version */
void FUNC_NAME(effect_6tap_render)(void *dst0, void *dst1, unsigned count)
{
  PIXEL *mydst0 = (PIXEL *) dst0;
  PIXEL *mydst1 = (PIXEL *) dst1;
  UINT8 *src0 = (UINT8 *) _6tap2x_buf0;
  UINT8 *src1 = (UINT8 *) _6tap2x_buf1;
  UINT8 *src2 = (UINT8 *) _6tap2x_buf2;
  UINT8 *src3 = (UINT8 *) _6tap2x_buf3;
  UINT8 *src4 = (UINT8 *) _6tap2x_buf4;
  UINT8 *src5 = (UINT8 *) _6tap2x_buf5;
  UINT32 *src32 = (UINT32 *) _6tap2x_buf2;
  UINT32 i;
  INT32 red, green, blue;

  /* first we need to just copy the 3rd line into the first destination line */
  for (i = 0; i < (count << 1); i++)
    {
	*mydst0++ = RGB32_2PIXEL(*src32);
	src32++;
    }

  /* then we need to vertically filter for the second line */
  for (i = 0; i < (count << 1); i++)
    {
	/* first, do the blue part */
	blue = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	        ((INT32) *src1++ + (INT32) *src4++);
	blue += blue << 2;
	blue += ((INT32) *src0++ + (INT32) *src5++);
	blue = (blue + 0x10) >> 5;
	blue = Clip(blue);
	blue = blue - (blue >> 2);
	/* next, do the green part */
	green = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	         ((INT32) *src1++ + (INT32) *src4++);
	green += green << 2;
	green += ((INT32) *src0++ + (INT32) *src5++);
	green = (green + 0x10) >> 5;
	green = Clip(green);
	green = green - (green >> 2);
	/* last, do the red part */
	red = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	       ((INT32) *src1++ + (INT32) *src4++);
	red += red << 2;
	red += ((INT32) *src0++ + (INT32) *src5++);
	red = (red + 0x10) >> 5;
	red = Clip(red);
	red = red - (red >> 2);
	/* write the pixel */
	*mydst1++ = RGB2PIXEL(red, green, blue);
	src0++; src1++; src2++; src3++; src4++; src5++;
    }
}
#endif

#undef Clip
#undef FUNC_NAME
#undef PIXEL
#undef HQ2XPIXEL
#undef RGB2YUV
#undef RGB2PIXEL
#undef RGB32_2PIXEL
#undef PIXEL2RGB32
#undef rmask
#undef gmask
#undef bmask
#undef pmask
#undef is_distant

/* this saves from having to undef these each time in effect.c */
#undef DEST_DEPTH
