#include "pixel_defs.h"
#include "blit.h"

#define FUNC_NAME(name) name##_16_YUY2
#define SRC_DEPTH    16
#define DEST_DEPTH   16
#define RENDER_DEPTH 32
#define GET_YUV_PIXEL(p) lookup[p]
#include "blit_defs.h"
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
