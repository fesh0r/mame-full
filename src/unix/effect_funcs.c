#include "xmame.h"
#include "osd_cpu.h"
#include "effect.h"

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

/* below are the YUY2 versions of most effects, the rgb code is generated for
   different colordepths from the generic rgb functions in effect_funcs.h */

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

void effect_scale2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  unsigned int *u32dst0 = (unsigned int *)dst0;
  unsigned int *u32dst1 = (unsigned int *)dst1;
  UINT16 *u16src0 = (UINT16 *)src0;
  UINT16 *u16src1 = (UINT16 *)src1;
  UINT16 *u16src2 = (UINT16 *)src2;
  UINT32 *u32lookup = current_palette->lookup;
  INT32 y,y2,uv1,uv2;
  UINT32 p1,p2,p3,p4;
  while (count) {

    if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
      p1 = u32lookup[u16src0[0]];
    else  p1 = u32lookup[u16src1[0]];

    if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
      p2 = u32lookup[u16src0[0]];
    else  p2 = u32lookup[u16src1[0]];

    if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
      p3 = u32lookup[u16src2[0]];
    else  p3 = u32lookup[u16src1[0]];

    if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
      p4 = u32lookup[u16src2[0]];
    else  p4 = u32lookup[u16src1[0]];

    ++u16src0;
    ++u16src1;
    ++u16src2;

    y=p1&0x000000ff;
    uv1=(p1&0xff00ff00)>>1;
    y2=p2&0x00ff0000;
    uv2=(p2&0xff00ff00)>>1;
    uv1=(uv1+uv2)&0xff00ff00;
    *u32dst0++=y|y2|uv1;

    y=p3&0x000000ff;
    uv1=(p3&0xff00ff00)>>1;
    y2=p4&0x00ff0000;
    uv2=(p4&0xff00ff00)>>1;
    uv1=(uv1+uv2)&0xff00ff00;
    *u32dst1++=y|y2|uv1;    y=p1>>24;

    --count;
  }
}


void effect_scale2x_32_YUY2_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count)
{
  unsigned char *u8dst0 = (unsigned char *)dst0;
  unsigned char *u8dst1 = (unsigned char *)dst1;
  UINT32 *u32src0 = (UINT32 *)src0;
  UINT32 *u32src1 = (UINT32 *)src1;
  UINT32 *u32src2 = (UINT32 *)src2;
  INT32 r,g,b,r2,g2,b2,y,y2,u,v;
  UINT32 p1,p2,p3,p4;
  while (count) {

    if (u32src1[-1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[1] != u32src0[0])
      p1 = u32src0[0];
    else  p1 = u32src1[0];

    if (u32src1[1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[-1] != u32src0[0])
      p2 = u32src0[0];
    else  p2 = u32src1[0];

    if (u32src1[-1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[1] != u32src2[0])
      p3 = u32src2[0];
    else  p3 = u32src1[0];

    if (u32src1[1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[-1] != u32src2[0])
      p4 = u32src2[0];
    else  p4 = u32src1[0];

    ++u32src0;
    ++u32src1;
    ++u32src2;

    r=RMASK32(p1);  r>>=16; \
    g=GMASK32(p1);  g>>=8; \
    b=BMASK32(p1);  b>>=0; \

    r2=RMASK32(p2);  r2>>=16; \
    g2=GMASK32(p2);  g2>>=8; \
    b2=BMASK32(p2);  b2>>=0; \

    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2; \
    *u8dst0++=y;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128; \
    *u8dst0++=u;
    v = (( 16448*r - 13783*g - 2665*b ) >> 16) + 128; \
    *u8dst0++=y2;
    *u8dst0++=v;

    r=RMASK32(p3);  r>>=16; \
    g=GMASK32(p3);  g>>=8; \
    b=BMASK32(p3);  b>>=0; \

    r2=RMASK32(p4);  r2>>=16; \
    g2=GMASK32(p4);  g2>>=8; \
    b2=BMASK32(p4);  b2>>=0; \

    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    y2 = (( 9836*r2 + 19310*g2 + 3750*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2; \
    *u8dst1++=y;
    u = (( -5527*r - 10921*g + 16448*b ) >> 16) + 128; \
    *u8dst1++=u;
    v = (( 16448*r - 13783*g - 2665*b ) >> 16) + 128; \
    *u8dst1++=y2;
    *u8dst1++=v;

    --count;
  }
}

/**********************************
 * scan2: light 2x2 scanlines
 **********************************/

void effect_scan2_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 r,y,u,v;

  while (count) {
    r=u32lookup[*u16src];
    y=r&255;
    u=(r>>8)&255;
    v=(r>>24);
    *u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    *u32dst1 = ((y*3/4)&255)|((u&255)<<8)|(((y*3/4)&255)<<16)|((v&255)<<24);
    ++u16src;
    u32dst0++;
    u32dst1++;
    --count;
  }
}


void effect_scan2_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32src = (UINT32 *)src;
  UINT32 r,g,b,y,u,v;

  while (count) {
    r = RMASK32(*u32src);  r>>=16;
    g = GMASK32(*u32src);  g>>=8;
    b = BMASK32(*u32src);  b>>=0;
    y = (( 9836*r + 19310*g + 3750*b ) >> 15);
    u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
    v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;

    *u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
    *u32dst1 = ((y*3/4)&255)|((u&255)<<8)|(((y*3/4)&255)<<16)|((v&255)<<24);
    ++u32src;
    u32dst0++;
    u32dst1++;
    --count;
  }
}

/**********************************
 * rgbstripe
 **********************************/

void effect_rgbstripe_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT16 *u16src = (UINT16 *)src;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 r,g,b,y,u,v,y2,u2,v2,s;
  INT32 us,vs;

  s = 1;
  while (count) {
    if (s) {
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
      y2 = (( 9836*(r&0x7f) + 19310*g + 3750*(b&0x7f) ) >> 15);
      u2 = (( -5527*(r&0x7f) - 10921*g + 16448*(b&0x7f) ) >> 15) + 128;
      v2 = (( 16448*(r&0x7f) - 13783*g - 2665*(b&0x7f) ) >> 15) + 128;
      u = (((u&255)+(u2&255))>>1);
      v = (((v&255)+(v2&255))>>1);
      *u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      if (count != 1) {
        y = (( 9836*(r&0x7f) + 19310*(g&0x7f) + 3750*b ) >> 15);
        u = (( -5527*(r&0x7f) - 10921*(g&0x7f) + 16448*b ) >> 15) + 128;
        v = (( 16448*(r&0x7f) - 13783*(g&0x7f) - 2665*b ) >> 15) + 128;
        r = u32lookup[*(u16src+1)];
        y2 = r&255;
        u2 = (r>>8)&255;
        v2 = (r>>24);
        us = u2 - 128;
        vs = v2 - 128;
        r = ((512*y2 + 718*vs) >> 9);
        g = ((512*y2 - 176*us - 366*vs) >> 9);
        b = ((512*y2 + 907*us) >> 9);
        y2 = (( 9836*r + 19310*(g&0x7f) + 3750*(b&0x7f) ) >> 15);
        u2 = (( -5527*r - 10921*(g&0x7f) + 16448*(b&0x7f) ) >> 15) + 128;
        v2 = (( 16448*r - 13783*(g&0x7f) - 2665*(b&0x7f) ) >> 15) + 128;
        u = (((u&255)+(u2&255))>>1);
        v = (((v&255)+(v2&255))>>1);
        *(u32dst0+1) = *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      }
    } else {
      r = u32lookup[*u16src];
      y = r&255;
      u = (r>>8)&255;
      v = (r>>24);
      us = u - 128;
      vs = v - 128;
      r = ((512*y + 718*vs) >> 9);
      g = ((512*y - 176*us - 366*vs) >> 9);
      b = ((512*y + 907*us) >> 9);
      y = (( 9836*(r&0x7f) + 19310*g + 3750*(b&0x7f) ) >> 15);
      u = (( -5527*(r&0x7f) - 10921*g + 16448*(b&0x7f) ) >> 15) + 128;
      v = (( 16448*(r&0x7f) - 13783*g - 2665*(b&0x7f) ) >> 15) + 128;
      y2 = (( 9836*(r&0x7f) + 19310*(g&0x7f) + 3750*b ) >> 15);
      u2 = (( -5527*(r&0x7f) - 10921*(g&0x7f) + 16448*b ) >> 15) + 128;
      v2 = (( 16448*(r&0x7f) - 13783*(g&0x7f) - 2665*b ) >> 15) + 128;
      u = (((u&255)+(u2&255))>>1);
      v = (((v&255)+(v2&255))>>1);
      *u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
    }

    ++u16src;
    if (s) {
      u32dst0 += 2;
      u32dst1 += 2;
    } else {
      u32dst0++;
      u32dst1++;
    }
    --count;
    s = !s;
  }
}


void effect_rgbstripe_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
  UINT32 *u32dst0 = (UINT32 *)dst0;
  UINT32 *u32dst1 = (UINT32 *)dst1;
  UINT32 *u32src = (UINT32 *)src;
  UINT32 t,r,g,b,y,u,v,y2,s;

  s = 1;
  while (count) {
    if (s) {
      t = RMASK32_SEMI(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      t = GMASK32_SEMI(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
      v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
      *u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      if (count != 1) {
        t = BMASK32_SEMI(*u32src);
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
        v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
        t = BMASK32_SEMI(*(u32src+1));
        r = RMASK32( t); r>>=16;
        g = GMASK32( t); g>>=8;
        b = BMASK32( t); b>>=0;
        y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
        u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
        v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
        *(u32dst0+1) = *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
      }
    } else {
      t = GMASK32_SEMI(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (( -5527*r - 10921*g + 16448*b ) >> 15) + 128;
      v = (( 16448*r - 13783*g - 2665*b ) >> 15) + 128;
      t = BMASK32_SEMI(*u32src);
      r = RMASK32( t); r>>=16;
      g = GMASK32( t); g>>=8;
      b = BMASK32( t); b>>=0;
      y2 = (( 9836*r + 19310*g + 3750*b ) >> 15);
      u = (u + (( -5527*r - 10921*g + 16448*b ) >> 15) + 128)/2;
      v = (v + (( 16448*r - 13783*g - 2665*b ) >> 15) + 128)/2;
      *u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
    }

    ++u32src;
    if (s) {
      u32dst0 += 2;
      u32dst1 += 2;
    } else {
      u32dst0++;
      u32dst1++;
    }
    --count;
    s = !s;
  }
}

/**********************************
 * rgbscan
 **********************************/

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

/**********************************
 * scan3
 **********************************/

/* All 3 lines are horizontally blurred a little
 * (the last pixel of each three in a line is averaged with the next pixel).
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%.
 */

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

void effect_6tap_render_32(void *dst0, void *dst1, unsigned count)
{
  UINT8 *u8dest = (UINT8 *) dst1;
  UINT8 *src0 = (UINT8 *) _6tap2x_buf0;
  UINT8 *src1 = (UINT8 *) _6tap2x_buf1;
  UINT8 *src2 = (UINT8 *) _6tap2x_buf2;
  UINT8 *src3 = (UINT8 *) _6tap2x_buf3;
  UINT8 *src4 = (UINT8 *) _6tap2x_buf4;
  UINT8 *src5 = (UINT8 *) _6tap2x_buf5;
  UINT32 i;
  INT32 pixel;

  /* first we need to just copy the 3rd line into the first destination line */
  memcpy(dst0, _6tap2x_buf2, count << 3);

  /* then we need to vertically filter for the second line */
  for (i = 0; i < (count << 1); i++)
    {
	/* first, do the blue part */
	pixel = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	         ((INT32) *src1++ + (INT32) *src4++);
	pixel += pixel << 2;
	pixel += ((INT32) *src0++ + (INT32) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* next, do the green part */
	pixel = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	         ((INT32) *src1++ + (INT32) *src4++);
	pixel += pixel << 2;
	pixel += ((INT32) *src0++ + (INT32) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* last, do the red part */
	pixel = (((INT32) *src2++ + (INT32) *src3++) << 2) -
	         ((INT32) *src1++ + (INT32) *src4++);
	pixel += pixel << 2;
	pixel += ((INT32) *src0++ + (INT32) *src5++);
	pixel = (pixel + 0x10) >> 5;
	pixel = Clip(pixel);
	*u8dest++ = pixel - (pixel >> 2);
	/* clear the last byte */
	*u8dest++ = 0;
	src0++; src1++; src2++; src3++; src4++; src5++;
    }

}
