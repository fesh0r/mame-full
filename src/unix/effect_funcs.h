#if DEST_DEPTH == 15
#  define SHADE_HALF SHADE15_HALF
#  define SHADE_FOURTH SHADE15_FOURTH
#  define RMASK_SEMI RMASK15_SEMI
#  define GMASK_SEMI GMASK15_SEMI
#  define BMASK_SEMI BMASK15_SEMI
#  define MEAN MEAN15
#  define DEST_PIXEL UINT16
#  define RFUNC_NAME(name) name##_15
#elif DEST_DEPTH == 16
#  define SHADE_HALF SHADE16_HALF
#  define SHADE_FOURTH SHADE16_FOURTH
#  define RMASK_SEMI RMASK16_SEMI
#  define GMASK_SEMI GMASK16_SEMI
#  define BMASK_SEMI BMASK16_SEMI
#  define MEAN MEAN16
#  define DEST_PIXEL UINT16
#  define RFUNC_NAME(name) name##_16
#elif DEST_DEPTH == 32
#  define SHADE_HALF SHADE32_HALF
#  define SHADE_FOURTH SHADE32_FOURTH
#  define RMASK_SEMI RMASK32_SEMI
#  define GMASK_SEMI GMASK32_SEMI
#  define BMASK_SEMI BMASK32_SEMI
#  define MEAN MEAN32
#  define DEST_PIXEL UINT32
#  define RFUNC_NAME(name) name##_32
#else
#  error Unknown DEST_DEPTH
#endif

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

void FUNC_NAME(effect_scale2x)
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  DEST_PIXEL *mysrc0 = (DEST_PIXEL *)src0;
  SRC_PIXEL *mysrc1 = (SRC_PIXEL *)src1;
  SRC_PIXEL *mysrc2 = (SRC_PIXEL *)src2;
  
  while (count) {

    if (mysrc1[-1] == mysrc0[0] && mysrc2[0] != mysrc0[0] && mysrc1[1] != mysrc0[0])
      *mydst0 = GETPIXEL(mysrc0[0]);
    else  *mydst0 = GETPIXEL(mysrc1[0]);

    if (mysrc1[1] == mysrc0[0] && mysrc2[0] != mysrc0[0] && mysrc1[-1] != mysrc0[0])
      *(mydst0+1) = GETPIXEL(mysrc0[0]);
    else  *(mydst0+1) = GETPIXEL(mysrc1[0]);

    if (mysrc1[-1] == mysrc2[0] && mysrc0[0] != mysrc2[0] && mysrc1[1] != mysrc2[0])
      *mydst1 = GETPIXEL(mysrc2[0]);
    else  *mydst1 = GETPIXEL(mysrc1[0]);

    if (mysrc1[1] == mysrc2[0] && mysrc0[0] != mysrc2[0] && mysrc1[-1] != mysrc2[0])
      *(mydst1+1) = GETPIXEL(mysrc2[0]);
    else  *(mydst1+1) = GETPIXEL(mysrc1[0]);

    ++mysrc0;
    ++mysrc1;
    ++mysrc2;
    mydst0 += 2;
    mydst1 += 2;
    --count;
  }
}

/**********************************
 * scan2: light 2x2 scanlines
 **********************************/

#ifndef EFFECT_MMX_ASM
void FUNC_NAME(effect_scan2) (void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  SRC_PIXEL  *mysrc  = (SRC_PIXEL  *)src;

  while (count) {

    *mydst0 = *(mydst0+1) = GETPIXEL(*mysrc);

    *mydst1 = *(mydst1+1) = SHADE_HALF( GETPIXEL(*mysrc) ) + SHADE_FOURTH( GETPIXEL(*mysrc) );

    ++mysrc;
    mydst0 += 2;
    mydst1 += 2;
    --count;
  }
}
#endif

/**********************************
 * rgbstripe
 **********************************/

void FUNC_NAME(effect_rgbstripe) (void *dst0, void *dst1, const void *src, unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  SRC_PIXEL  *mysrc  = (SRC_PIXEL  *)src;

  while (count) {

    *(mydst0+0) = *(mydst1+0) = RMASK_SEMI(GETPIXEL(*mysrc));
    *(mydst0+1) = *(mydst1+1) = GMASK_SEMI(GETPIXEL(*mysrc));
    *(mydst0+2) = *(mydst1+2) = BMASK_SEMI(GETPIXEL(*mysrc));

    ++mysrc;
    mydst0 += 3;
    mydst1 += 3;
    --count;
  }
}

/**********************************
 * rgbscan
 **********************************/

void FUNC_NAME(effect_rgbscan)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  DEST_PIXEL *mydst2 = (DEST_PIXEL *)dst2;
  SRC_PIXEL  *mysrc  = (SRC_PIXEL  *)src;
  SRC_PIXEL  *myend  = (SRC_PIXEL  *)src + count;

  for(mysrc=(SRC_PIXEL*)src; mysrc<myend; mysrc++) {
    *(mydst0+1) = *(mydst0+0) = RMASK_SEMI(GETPIXEL(*mysrc));
    mydst0 += 2;
  }

  for(mysrc=(SRC_PIXEL*)src; mysrc<myend; mysrc++) {
    *(mydst1+1) = *(mydst1+0) = GMASK_SEMI(GETPIXEL(*mysrc));
    mydst1 += 2;
  }

  for(mysrc=(SRC_PIXEL*)src; mysrc<myend; mysrc++) {
    *(mydst2+1) = *(mydst2+0) = BMASK_SEMI(GETPIXEL(*mysrc));
    mydst2 += 2;
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

void FUNC_NAME(effect_scan3)(void *dst0, void *dst1, void *dst2, const void *src, unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  DEST_PIXEL *mydst2 = (DEST_PIXEL *)dst2;
  SRC_PIXEL *mysrc = (SRC_PIXEL *)src;

  while (count) {

    *(mydst0+1) = *(mydst0+0) =
      SHADE_HALF(GETPIXEL(*mysrc)) + SHADE_FOURTH(GETPIXEL(*mysrc));
    *(mydst0+2) =
      SHADE_HALF( MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1)) ) )
      +
      SHADE_FOURTH( MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1)) ) );

    *(mydst1+0) = *(mydst1+1) = GETPIXEL(*mysrc);
    *(mydst1+2) = MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1)) );

    *(mydst2+0) = *(mydst2+1) =
      SHADE_HALF(GETPIXEL(*mysrc));
    *(mydst2+2) =
      SHADE_HALF( MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1)) ) );

    ++mysrc;
    mydst0 += 3;
    mydst1 += 3;
    mydst2 += 3;
    --count;
  }
}

/* high quality 2x scaling effect, see effect_renderers for the
   real stuff */

void FUNC_NAME(effect_hq2x)
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0   = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1   = (DEST_PIXEL *)dst1;
  SRC_PIXEL *mysrc0   = (SRC_PIXEL *)src0;
  SRC_PIXEL *mysrc1   = (SRC_PIXEL *)src1;
  SRC_PIXEL *mysrc2   = (SRC_PIXEL *)src2;
  DEST_PIXEL  w[9];

  w[1] = GETPIXEL(mysrc0[-1]);
  w[2] = GETPIXEL(mysrc0[ 0]);
  w[4] = GETPIXEL(mysrc1[-1]);
  w[5] = GETPIXEL(mysrc1[ 0]);
  w[7] = GETPIXEL(mysrc2[-1]);
  w[8] = GETPIXEL(mysrc2[ 0]);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = GETPIXEL(mysrc0[ 1]);
    w[5] = GETPIXEL(mysrc1[ 1]);
    w[8] = GETPIXEL(mysrc2[ 1]);

    RFUNC_NAME(hq2x)( mydst0, mydst1, w );

    ++mysrc0;
    ++mysrc1;
    ++mysrc2;
    mydst0 += 2;
    mydst1 += 2;
  }
}

/* low quality 2x scaling effect, see effect_renderers for the
   real stuff */

void FUNC_NAME(effect_lq2x)
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  SRC_PIXEL *u16src0 = (SRC_PIXEL *)src0;
  SRC_PIXEL *u16src1 = (SRC_PIXEL *)src1;
  SRC_PIXEL *u16src2 = (SRC_PIXEL *)src2;
  DEST_PIXEL  w[9];

  w[1] = GETPIXEL(u16src0[-1]);
  w[2] = GETPIXEL(u16src0[ 0]);
  w[4] = GETPIXEL(u16src1[-1]);
  w[5] = GETPIXEL(u16src1[ 0]);
  w[7] = GETPIXEL(u16src2[-1]);
  w[8] = GETPIXEL(u16src2[ 0]);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[3] = w[4];
    w[4] = w[5];
    w[6] = w[7];
    w[7] = w[8];
    w[2] = GETPIXEL(u16src0[ 1]);
    w[5] = GETPIXEL(u16src1[ 1]);
    w[8] = GETPIXEL(u16src2[ 1]);

    RFUNC_NAME(lq2x)( mydst0, mydst1, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    mydst0 += 2;
    mydst1 += 2;
  }
}

#undef SHADE_HALF
#undef SHADE_FOURTH
#undef RMASK_SEMI
#undef GMASK_SEMI
#undef BMASK_SEMI
#undef MEAN
#undef DEST_PIXEL
#undef RFUNC_NAME

/* this saves from having to undef these each time in effect.c */
#undef FUNC_NAME
#undef SRC_PIXEL
#undef DEST_DEPTH
