#include "interp_defs.h"

/* Some glue defines, so that we can use the advancemame lookup
   tables unmodified. */
#define MUR (c[1] != c[5])
#define MDR (c[5] != c[7])
#define MDL (c[7] != c[3])
#define MUL (c[3] != c[1])

#define XQ2X_LINE_LOOP_BEGIN \
  interp_uint16 c[9]; \
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
    unsigned char mask = 0; \
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
    if (c[0] != c[4]) \
    	mask |= 1 << 0; \
    if (c[1] != c[4]) \
    	mask |= 1 << 1; \
    if (c[2] != c[4]) \
    	mask |= 1 << 2; \
    if (c[3] != c[4]) \
    	mask |= 1 << 3; \
    if (c[5] != c[4]) \
    	mask |= 1 << 4; \
    if (c[6] != c[4]) \
    	mask |= 1 << 5; \
    if (c[7] != c[4]) \
    	mask |= 1 << 6; \
    if (c[8] != c[4]) \
    	mask |= 1 << 7;

#define XQ2X_LINE_LOOP_BEGIN_SWAP_XY \
  interp_uint16 c[9]; \
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
    unsigned char mask = 0; \
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
    if (c[0] != c[4]) \
    	mask |= 1 << 0; \
    if (c[1] != c[4]) \
    	mask |= 1 << 1; \
    if (c[2] != c[4]) \
    	mask |= 1 << 2; \
    if (c[3] != c[4]) \
    	mask |= 1 << 3; \
    if (c[5] != c[4]) \
    	mask |= 1 << 4; \
    if (c[6] != c[4]) \
    	mask |= 1 << 5; \
    if (c[7] != c[4]) \
    	mask |= 1 << 6; \
    if (c[8] != c[4]) \
    	mask |= 1 << 7;

#define XQ2X_LINE_LOOP_END \
    src0++; \
    src1++; \
    src2++; \
  }

#define XQ2X_NAME(x) x##_lq2x
#define XQ2X_FUNC_NAME(x) FUNC_NAME(x##_lq2x)
