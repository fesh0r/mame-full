#include "pixel_defs.h"
#include "blit.h"

/* below are the 16 to YUY2 versions of most effects */

#define FUNC_NAME(name) name##_16_YUY2
#define SRC_DEPTH    16
#define DEST_DEPTH   16
#define RENDER_DEPTH 32
#include "blit_defs.h"

INLINE void FUNC_NAME(blit_normal_line_1)(unsigned short *src, unsigned short *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int p,y1,y2,uv;
  
  while(src<end)
  {
    p  = lookup[*src++];
    y1 = p&Y1MASK;
    uv = (p&UVMASK)>>1;
    p  = lookup[*src++];
    y2 = p&Y2MASK;
    uv += (p&UVMASK)>>1;
    uv &= UVMASK;
    *dst++ = y1|y2|uv;
  }
}

INLINE void FUNC_NAME(blit_normal_line_2)(unsigned short *src, unsigned short *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  while(src<end)
    *dst++ = lookup[*src++];
}

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
INLINE void FUNC_NAME(blit_scale2x_border)( const unsigned short *src0,
  const unsigned short *src1, const unsigned short *src2,
  const unsigned short *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,y1,y2,uv;
  while (src1 < end1) {
    if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
      p1 = lookup[src0[0]];
    else p1 = lookup[src1[0]];

    if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
      p2 = lookup[src0[0]];
    else p2 = lookup[src1[0]];

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

INLINE void FUNC_NAME(blit_scale3x_border)( const unsigned short *src0,
  const unsigned short *src1, const unsigned short *src2,
  const unsigned short *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,p3,p4,p5,p6,y1,y2,uv;
  while (src1 < end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = lookup[src1[-1] == src0[0] ? src1[-1] : src1[0]];
    	p2 = lookup[(src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0]];
    	p3 = lookup[src1[1] == src0[0] ? src1[1] : src1[0]];
    } else {
    	p1 = p2 = p3 = lookup[src1[0]];
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = lookup[src1[-1] == src0[0] ? src1[-1] : src1[0]];
    	p5 = lookup[(src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0]];
    	p6 = lookup[src1[1] == src0[0] ? src1[1] : src1[0]];
    } else {
    	p4 = p5 = p6 = lookup[src1[0]];
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

INLINE void FUNC_NAME(blit_scale3x_center)( const unsigned short *src0,
  const unsigned short *src1, const unsigned short *src2,
  const unsigned short *end1, unsigned int *dst, unsigned int *lookup)
{
  unsigned int p1,p2,p3,p4,p5,p6,y1,y2,uv;
  while (src1 < end1) {
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p1 = lookup[(src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0]];
    	p2 = lookup[src1[0]];
    	p3 = lookup[(src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0]];
    } else {
    	p1 = p2 = p3 = lookup[src1[0]];
    }
    src0++;
    src1++;
    src2++;
    if (src0[0] != src2[0] && src1[-1] != src1[1]) {
    	p4 = lookup[(src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0]];
    	p5 = lookup[src1[0]];
    	p6 = lookup[(src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0]];
    } else {
    	p4 = p5 = p6 = lookup[src1[0]];
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

/**********************************
 * scan2: light 2x2 scanlines
 **********************************/
INLINE void FUNC_NAME(blit_scan2_h_line)(unsigned short *src, unsigned short *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *mydst = dst;
  unsigned short *mysrc = src;
  unsigned int y,uv;

  while(mysrc<end)
    *mydst++ = lookup[*mysrc++];

  mydst = dst + dest_width/2;
  mysrc = src;
  while(mysrc<end) {
    y   = uv = lookup[*mysrc++];
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

INLINE void FUNC_NAME(blit_scan2_v_line)(unsigned short *src,
  unsigned short *end, unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,yuv;

  while(src<end) {
    y    = yuv = lookup[*src++];
    y   &= Y2MASK;
    y    = (y*3) >> 2;   
    y   &= Y2MASK;
    yuv &= UVMASK|Y1MASK;
    *dst++ = y|yuv;
  }
}

/**********************************
 * scan3, YUY2 version
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%. */
INLINE void FUNC_NAME(blit_scan3_h_line)(unsigned short *src, unsigned short *end,
  unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *mydst = dst;
  unsigned short *mysrc = src;
  unsigned int y,uv;

  while(mysrc<end) {
    y   = uv = lookup[*mysrc++];
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
  mysrc = src;
  while (mysrc<end)
    *mydst++ = lookup[*mysrc++];
    
  mydst = dst + dest_width;
  mysrc = src;
  while(mysrc<end) {
    y   = uv = lookup[*mysrc++];
    y  &= YMASK;
    y >>= 1;
    y  &= YMASK;
    uv &= UVMASK;
    *mydst++ = y|uv;
  }
}

INLINE void FUNC_NAME(blit_scan3_v_line)(unsigned short *src,
  unsigned short *end, unsigned int *dst, int dest_width, unsigned int *lookup)
{
  unsigned int y,y2,y3,uv,uv2;

  while (src < end) {
    y = y2 = uv = lookup[*src++];
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

    y = y2 = uv2 = lookup[*src++];
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

#include "blit_yuy2.h"
#include "blit_undefs.h"
   

#if 0
/**********************************
 * 6tap2x: 6-tap sinc filter with light scanlines
 **********************************/

void effect_6tap_clear(unsigned count)
{
  memset(_6tap2x_buf0, 0, count << 3);
  memset(_6tap2x_buf1, 0, count << 3);
  memset(_6tap2x_buf2, 0, count << 3);
  memset(_6tap2x_buf3, 0, count << 3);
  memset(_6tap2x_buf4, 0, count << 3);
  memset(_6tap2x_buf5, 0, count << 3);
}

#define Clip(a) (((a) < 0) ? 0 : (((a) > 0xff) ? 0xff : (a)))

#ifndef EFFECT_MMX_ASM
void effect_6tap_render_32(void *dst0, void *dst1, unsigned count)
{
  UINT8 *u8dest = (UINT8 *) dst1;
  UINT8 *src0 = (UINT8 *) _6tap2x_buf0;
  UINT8 *src1 = (UINT8 *) _6tap2x_buf1;
  UINT8 *src2 = (UINT8 *) _6tap2x_buf2;
  UINT8 *src3 = (UINT8 *) _6tap2x_buf3;
  UINT8 *src4 = (UINT8 *) _6tap2x_buf4;
  UINT8 *src5 = (UINT8 *) _6tap2x_buf5;
  unsigned int i;
  int pixel;

  /* first we need to just copy the 3rd line into the first destination line */
  memcpy(dst0, _6tap2x_buf2, count << 3);

  /* then we need to vertically filter for the second line */
  for (i = 0; i < (count << 1); i++)
    {
	/* first, do the blue part */
	pixel = (((int) *src2++ + (int) *src3++) << 2) -
	         ((int) *src1++ + (int) *src4++);
	pixel += pixel << 2;
	pixel += ((int) *src0++ + (int) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* next, do the green part */
	pixel = (((int) *src2++ + (int) *src3++) << 2) -
	         ((int) *src1++ + (int) *src4++);
	pixel += pixel << 2;
	pixel += ((int) *src0++ + (int) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* last, do the red part */
	pixel = (((int) *src2++ + (int) *src3++) << 2) -
	         ((int) *src1++ + (int) *src4++);
	pixel += pixel << 2;
	pixel += ((int) *src0++ + (int) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* clear the last byte */
	*u8dest++ = 0;
	src0++; src1++; src2++; src3++; src4++; src5++;
    }

}
#endif
#endif
