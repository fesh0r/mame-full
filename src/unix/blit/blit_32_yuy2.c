#include "pixel_defs.h"
#include "blit.h"

/* below are the 32 to YUY2 versions of most effects */

#define FUNC_NAME(name) name##_32_YUY2_direct
#define SRC_DEPTH    32
#define DEST_DEPTH   16
#define RENDER_DEPTH 32
#include "blit_defs.h"

INLINE void FUNC_NAME(blit_normal_line_1)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  int r,g,b,r2,g2,b2,y,y2,u,v;

  while(src<end)
  {
    r=g=b=*src++;
    r=RMASK(r);  r>>=16;
    g=GMASK(g);  g>>=8;
    b=BMASK(b);
    r2=g2=b2=*src++;
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

INLINE void FUNC_NAME(blit_normal_line_2)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  int r,g,b,y,u,v;
  
  while(src<end)
  {
    r=g=b=*src++;
    r=RMASK(r);  r>>=16;
    g=GMASK(g);  g>>=8;
    b=BMASK(b);
    RGB2YUV(r,g,b,y,u,v);
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
  }
}

INLINE void FUNC_NAME(blit_scale2x_border)( const unsigned int *src0,
  const unsigned int *src1, const unsigned int *src2,
  const unsigned int *end1, unsigned int *dst, unsigned int *lookup)
{
  int p1,p2,r,g,b,r2,g2,b2,y,y2,u,v;
  while (src1< end1) {

    if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
      p1 = src0[0];
    else  p1 = src1[0];

    if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
      p2 = src0[0];
    else  p2 = src1[0];

    ++src0;
    ++src1;
    ++src2;

    r=RMASK(p1);  r>>=16;
    g=GMASK(p1);  g>>=8;
    b=BMASK(p1);

    r2=RMASK(p2);  r2>>=16;
    g2=GMASK(p2);  g2>>=8;
    b2=BMASK(p2);

    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);
  }
}

INLINE void FUNC_NAME(blit_scale3x_border)( const unsigned int *src0,
  const unsigned int *src1, const unsigned int *src2,
  const unsigned int *end1, unsigned int *dst, unsigned int *lookup)
{
  int p1,p2,p3,p4,p5,p6,r,g,b,r2,g2,b2,y,y2,u,v;

  while (src1< end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = src1[-1] == src0[0] ? src1[-1] : src1[0];
    	p2 = (src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
    	p3 = src1[1] == src0[0] ? src1[1] : src1[0];
    } else {
    	p1 = p2 = p3 = src1[0];
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = src1[-1] == src0[0] ? src1[-1] : src1[0];
    	p5 = (src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
    	p6 = src1[1] == src0[0] ? src1[1] : src1[0];
    } else {
    	p4 = p5 = p6 = src1[0];
    }
    src0++;
    src1++;
    src2++;

    r=RMASK(p1);  r>>=16;
    g=GMASK(p1);  g>>=8;
    b=BMASK(p1);
    r2=RMASK(p2);  r2>>=16;
    g2=GMASK(p2);  g2>>=8;
    b2=BMASK(p2);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r=RMASK(p3);  r>>=16;
    g=GMASK(p3);  g>>=8;
    b=BMASK(p3);
    r2=RMASK(p4);  r2>>=16;
    g2=GMASK(p4);  g2>>=8;
    b2=BMASK(p4);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r=RMASK(p5);  r>>=16;
    g=GMASK(p5);  g>>=8;
    b=BMASK(p5);
    r2=RMASK(p6);  r2>>=16;
    g2=GMASK(p6);  g2>>=8;
    b2=BMASK(p6);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);
  }
}

INLINE void FUNC_NAME(blit_scale3x_center)( const unsigned int *src0,
  const unsigned int *src1, const unsigned int *src2,
  const unsigned int *end1, unsigned int *dst, unsigned int *lookup)
{
  int p1,p2,p3,p4,p5,p6,r,g,b,r2,g2,b2,y,y2,u,v;

  while (src1< end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
    	p2 = src1[0];
    	p3 = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
    } else {
    	p1 = p2 = p3 = src1[0];
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
    	p5 = src1[0];
    	p6 = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
    } else {
    	p4 = p5 = p6 = src1[0];
    }
    src0++;
    src1++;
    src2++;

    r=RMASK(p1);  r>>=16;
    g=GMASK(p1);  g>>=8;
    b=BMASK(p1);
    r2=RMASK(p2);  r2>>=16;
    g2=GMASK(p2);  g2>>=8;
    b2=BMASK(p2);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r=RMASK(p3);  r>>=16;
    g=GMASK(p3);  g>>=8;
    b=BMASK(p3);
    r2=RMASK(p4);  r2>>=16;
    g2=GMASK(p4);  g2>>=8;
    b2=BMASK(p4);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);

    r=RMASK(p5);  r>>=16;
    g=GMASK(p5);  g>>=8;
    b=BMASK(p5);
    r2=RMASK(p6);  r2>>=16;
    g2=GMASK(p6);  g2>>=8;
    b2=BMASK(p6);
    y  = (( 9836*r  + 19310*g  + 3750*b  ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128;
    v = (( 16448*r - 13783*g -  2665*b ) >> 16) + 128;
    *dst++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);
  }
}

INLINE void FUNC_NAME(blit_scan2_h_line)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *dst0 = dst;
  unsigned int *dst1 = dst + dest_width/2;
  int r,g,b,y,u,v;

  while (src < end) {
    r = RMASK(*src); r>>=16;
    g = GMASK(*src); g>>=8;
    b = BMASK(*src);
    RGB2YUV(r,g,b,y,u,v);
    *dst0++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
    y = (y*3) >> 2;
    *dst1++ = (y<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
    src++;
  }
}

INLINE void FUNC_NAME(blit_scan2_v_line)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  int r,g,b,y1,y2,u,v;

  while (src < end) {
    r = RMASK(*src); r>>=16;
    g = GMASK(*src); g>>=8;
    b = BMASK(*src);
    RGB2YUV(r,g,b,y1,u,v);
    y2 = (y1*3) >> 2;
    *dst++ = (y1<<Y1SHIFT) | (u<<USHIFT) | (y2<<Y2SHIFT) | (v<<VSHIFT);
    src++;
  }
}

INLINE void FUNC_NAME(blit_scan3_h_line)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *dst0 = dst;
  unsigned int *dst1 = dst + dest_width/2;
  unsigned int *dst2 = dst + dest_width;
  int r,g,b,y,u,v,_y;

  while (src < end) {
    r = RMASK(*src); r>>=16;
    g = GMASK(*src); g>>=8;
    b = BMASK(*src);
    RGB2YUV(r,g,b,y,u,v);
    _y = (y*3) >> 2;
    *dst0++ = (_y<<Y1SHIFT) | (u<<USHIFT) | (_y<<Y2SHIFT) | (v<<VSHIFT);
    *dst1++ = ( y<<Y1SHIFT) | (u<<USHIFT) | ( y<<Y2SHIFT) | (v<<VSHIFT);
    _y = y >> 1;
    *dst2++ = (_y<<Y1SHIFT) | (u<<USHIFT) | (_y<<Y2SHIFT) | (v<<VSHIFT);
    src++;
  }
}

INLINE void FUNC_NAME(blit_scan3_v_line)(unsigned int *src, unsigned int *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  int r,g,b,y,y2,u,u2,v,v2;

  while (src < end) {
    r = RMASK(*src); r>>=16;
    g = GMASK(*src); g>>=8;
    b = BMASK(*src);
    RGB2YUV(r,g,b,y,u,v);
    src++;

    r = RMASK(*src); r>>=16;
    g = GMASK(*src); g>>=8;
    b = BMASK(*src);
    RGB2YUV(r,g,b,y2,u2,v2);
    src++;

    *dst++ = (((y*3)>>2)<<Y1SHIFT) | (u<<USHIFT) | (y<<Y2SHIFT) | (v<<VSHIFT);
    u += u2;
    v += v2;
    *dst++ = ((y>>1)<<Y1SHIFT) | ((u>>1)<<USHIFT) | (((y2*3)>>2)<<Y2SHIFT) |
             ((v>>1)<<VSHIFT);
    *dst++ = (y2<<Y1SHIFT) | (u2<<USHIFT) | ((y2>>1)<<Y2SHIFT) | (v2<<VSHIFT);
  }
}

#include "blit_yuy2.h"
#include "blit_undefs.h"
