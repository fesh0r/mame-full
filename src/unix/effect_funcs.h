#if DEST_DEPTH == 15
#  define SHADE_HALF SHADE15_HALF
#  define SHADE_FOURTH SHADE15_FOURTH
#  define RMASK_SEMI RMASK15_SEMI
#  define GMASK_SEMI GMASK15_SEMI
#  define BMASK_SEMI BMASK15_SEMI
#  define DEST_PIXEL UINT16
#elif DEST_DEPTH == 16
#  define SHADE_HALF SHADE16_HALF
#  define SHADE_FOURTH SHADE16_FOURTH
#  define RMASK_SEMI RMASK16_SEMI
#  define GMASK_SEMI GMASK16_SEMI
#  define BMASK_SEMI BMASK16_SEMI
#  define DEST_PIXEL UINT16
#elif DEST_DEPTH == 32
#  define SHADE_HALF SHADE32_HALF
#  define SHADE_FOURTH SHADE32_FOURTH
#  define RMASK_SEMI RMASK32_SEMI
#  define GMASK_SEMI GMASK32_SEMI
#  define BMASK_SEMI BMASK32_SEMI
#  define DEST_PIXEL UINT32
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
    unsigned count)
{
  DEST_PIXEL *mydst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *mydst1 = (DEST_PIXEL *)dst1;
  DEST_PIXEL *mysrc0 = (DEST_PIXEL *)src0;
  SRC_PIXEL *mysrc1 = (SRC_PIXEL *)src1;
  SRC_PIXEL *mysrc2 = (SRC_PIXEL *)src2;
#ifdef INDIRECT  
  UINT32 *u32lookup = current_palette->lookup;
#endif
  
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
void FUNC_NAME(effect_scan2) (void *dst0, void *dst1, const void *src, unsigned count)
{
  DEST_PIXEL *u16dst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *u16dst1 = (DEST_PIXEL *)dst1;
  SRC_PIXEL  *u16src  = (SRC_PIXEL  *)src;
#ifdef INDIRECT
  UINT32 *u32lookup = current_palette->lookup;
#endif

  while (count) {

    *u16dst0 = *(u16dst0+1) = GETPIXEL(*u16src);

    *u16dst1 = *(u16dst1+1) = SHADE_HALF( GETPIXEL(*u16src) ) + SHADE_FOURTH( GETPIXEL(*u16src) );

    ++u16src;
    u16dst0 += 2;
    u16dst1 += 2;
    --count;
  }
}
#endif

/**********************************
 * rgbstripe
 **********************************/

void FUNC_NAME(effect_rgbstripe) (void *dst0, void *dst1, const void *src, unsigned count)
{
  DEST_PIXEL *u16dst0 = (DEST_PIXEL *)dst0;
  DEST_PIXEL *u16dst1 = (DEST_PIXEL *)dst1;
  SRC_PIXEL  *u16src  = (SRC_PIXEL  *)src;
#ifdef INDIRECT
  UINT32 *u32lookup = current_palette->lookup;
#endif

  while (count) {

    *(u16dst0+0) = *(u16dst1+0) = RMASK_SEMI(GETPIXEL(*u16src));
    *(u16dst0+1) = *(u16dst1+1) = GMASK_SEMI(GETPIXEL(*u16src));
    *(u16dst0+2) = *(u16dst1+2) = BMASK_SEMI(GETPIXEL(*u16src));

    ++u16src;
    u16dst0 += 3;
    u16dst1 += 3;
    --count;
  }
}

#undef SHADE_HALF
#undef SHADE_FOURTH
#undef RMASK_SEMI
#undef GMASK_SEMI
#undef BMASK_SEMI
#undef DEST_PIXEL

/* this saves from having to undef these each time in effect.c */
#undef FUNC_NAME
#undef SRC_PIXEL
#undef DEST_DEPTH
