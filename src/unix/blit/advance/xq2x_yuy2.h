/* Configuration defines and includes */
#define XQ2X_GETPIXEL(p) YUV_TO_XQ2X_YUV(GET_YUV_PIXEL(p))
#define HQ2X_YUVLOOKUP(p) (p)
#include "xq2x_yuv.h"
#ifdef HQ2X
#  include "hq2x_defs.h"
#else
#  include "lq2x_defs.h"
#endif

/* Pixel glue define, so that we can use the advancemame lookup
   tables unmodified. */
#define P(a, b) p_##b##_##a

INLINE void XQ2X_FUNC_NAME(blit_line_2x2) ( SRC_PIXEL *src0,
  SRC_PIXEL *src1, SRC_PIXEL *src2, SRC_PIXEL *end1,
  unsigned short *dst, int dest_width, unsigned int *lookup)
{
  unsigned int *dst0 = (unsigned int *)dst;
  unsigned int *dst1 = (unsigned int *)(dst + dest_width);
  unsigned int p_0_0 = 0, p_0_1 = 0, p_1_0 = 0, p_1_1 = 0;
  unsigned int y1,y2,uv;

  XQ2X_LINE_LOOP_BEGIN
    switch (mask) {
      #ifdef HQ2X
      #  include "hq2x.dat"
      #else
      #  include "lq2x.dat"
      #endif
    }
#ifdef LSB_FIRST
    y1 = (p_0_0>>8) & Y1MASK;
    y2 = (p_0_1<<8) & Y2MASK;
    p_0_0 &= (XQ2X_UMASK|XQ2X_VMASK);
    p_0_1 &= (XQ2X_UMASK|XQ2X_VMASK);
    uv = ((p_0_0 + p_0_1) << 7) & UVMASK;
    *dst0++ = y1 | y2 | uv;

    y1 = (p_1_0>>8) & Y1MASK;
    y2 = (p_1_1<<8) & Y2MASK;
    p_1_0 &= (XQ2X_UMASK|XQ2X_VMASK);
    p_1_1 &= (XQ2X_UMASK|XQ2X_VMASK);
    uv = ((p_1_0 + p_1_1) << 7) & UVMASK;
    *dst1++ = y1 | y2 | uv;
#else
    y1 = (p_0_0<<16) & Y1MASK;
    y2 = (p_0_1)     & Y2MASK;
    p_0_0 &= (XQ2X_UMASK|XQ2X_VMASK);
    p_0_1 &= (XQ2X_UMASK|XQ2X_VMASK);
    uv = ((p_0_0 + p_0_1) >> 1) & UVMASK;
    *dst0++ = y1 | y2 | uv;

    y1 = (p_1_0<<16) & Y1MASK;
    y2 = (p_1_1)     & Y2MASK;
    p_1_0 &= (XQ2X_UMASK|XQ2X_VMASK);
    p_1_1 &= (XQ2X_UMASK|XQ2X_VMASK);
    uv = ((p_1_0 + p_1_1) >> 1) & UVMASK;
    *dst1++ = y1 | y2 | uv;
#endif
  XQ2X_LINE_LOOP_END
}

BLIT_BEGIN(XQ2X_NAME(blit))
  switch(sysdep_display_params.widthscale)
  {
    case 2:
      BLIT_LOOP2X(XQ2X_NAME(blit_line_2x2), 2);
      break;
  }
BLIT_END

#include "xq2x_undefs.h"
