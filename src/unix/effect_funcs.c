#include "xmame.h"
#include "osd_cpu.h"
#include "effect.h"

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
 * 6tap2x: 6-tap sinc filter with light scanlines
 **********************************/

#define Clip(a) (((a) < 0) ? 0 : (((a) > 0xff) ? 0xff : (a)))

void effect_6tap_clear(unsigned count)
{
  memset(_6tap2x_buf0, 0, count << 3);
  memset(_6tap2x_buf1, 0, count << 3);
  memset(_6tap2x_buf2, 0, count << 3);
  memset(_6tap2x_buf3, 0, count << 3);
  memset(_6tap2x_buf4, 0, count << 3);
  memset(_6tap2x_buf5, 0, count << 3);
}

INLINE void effect_6tap_addline_filter(UINT32 *u32dest, unsigned count)
{
  UINT8 *u8dest;
  INT32 pixel;
  UINT32 i;

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

void effect_6tap_addline_15(const void *src0, unsigned count)
{
  UINT16 *u16src = (UINT16 *)src0;
  UINT32 *u32dest;
  UINT32 i;
  char *tmp;

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
    *u32dest++ = ((UINT32) RMASK15(*u16src) << 9) |
                 ((UINT32) GMASK15(*u16src) << 6) |
                 ((UINT32) BMASK15(*u16src) << 3);
    u16src++;
    u32dest++;
    }

  /* and apply the filter */
  effect_6tap_addline_filter(u32dest, count);
}

void effect_6tap_addline_16(const void *src0, unsigned count)
{
  UINT16 *u16src = (UINT16 *)src0;
  UINT32 *u32lookup = current_palette->lookup;
  UINT32 *u32dest;
  UINT32 i;
  char *tmp;

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
    *u32dest++ = u32lookup[*u16src++];
    u32dest++;
    }

  /* and apply the filter */
  effect_6tap_addline_filter(u32dest, count);
}

void effect_6tap_addline_32(const void *src0, unsigned count)
{
  UINT32 *u32src = (UINT32 *)src0;
  UINT32 *u32dest;
  UINT32 i;
  char *tmp;

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

  /* we have a new line, so zoom by 2 */
  u32dest = (UINT32 *) _6tap2x_buf5;
  for (i = 0; i < count; i++)
    {
    *u32dest++ = *u32src++;
    u32dest++;
    }

  /* and apply the filter */
  effect_6tap_addline_filter(u32dest, count);
}

void effect_6tap_render_15(void *dst0, void *dst1, unsigned count)
{
  UINT16 *u16dest0 = (UINT16 *) dst0;
  UINT16 *u16dest1 = (UINT16 *) dst1;
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
	*u16dest0++ = (UINT16) ((*src32 & 0xf80000) >> 9) |
	                       ((*src32 & 0x00f800) >> 6) |
	                       ((*src32 & 0x0000f8) >> 3);
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
	/* write the 15-bit color pixel */
	*u16dest1++ = (UINT16) ((red   & 0xf8) << 7) |
	                       ((green & 0xf8) << 2) |
	                       ((blue  & 0xf8) >> 3);
	src0++; src1++; src2++; src3++; src4++; src5++;
    }

}

void effect_6tap_render_16(void *dst0, void *dst1, unsigned count)
{
  UINT16 *u16dest0 = (UINT16 *) dst0;
  UINT16 *u16dest1 = (UINT16 *) dst1;
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
	*u16dest0++ = (UINT16) ((*src32 & 0xf80000) >> 8) |
	                       ((*src32 & 0x00fc00) >> 5) |
	                       ((*src32 & 0x0000f8) >> 3);
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
	/* write the 16-bit color pixel */
	*u16dest1++ = (UINT16) ((red   & 0xf8) << 8) |
	                       ((green & 0xfc) << 3) |
	                       ((blue  & 0xf8) >> 3);
	src0++; src1++; src2++; src3++; src4++; src5++;
    }

}

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

#undef Clip
