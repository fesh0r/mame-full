/**********************************
 * scan2: light 2x2 scanlines
 **********************************/
#define BLIT_SCAN2_H_LINE_X(NAME, DST, DST_INC) \
INLINE void FUNC_NAME(NAME)(SRC_PIXEL *src, \
  SRC_PIXEL *end, RENDER_PIXEL *dst, int dest_width, \
  unsigned int *lookup) \
{ \
  RENDER_PIXEL *mydst = dst; \
  SRC_PIXEL *mysrc = src; \
  \
  while (mysrc < end) \
  { \
    DST = GETPIXEL(*mysrc); \
    DST_INC; \
    mysrc++; \
  } \
  \
  mydst = dst + dest_width; \
  mysrc = src; \
  while (mysrc < end) \
  { \
    DST = SHADE_HALF( GETPIXEL(*mysrc) ) + \
      SHADE_FOURTH( GETPIXEL(*mysrc) ); \
    DST_INC; \
    mysrc++; \
  } \
}

BLIT_SCAN2_H_LINE_X(blit_scan2_h_line_1, *mydst, mydst++)
BLIT_SCAN2_H_LINE_X(blit_scan2_h_line_2, *(mydst+1) = *mydst, mydst+=2)
BLIT_SCAN2_H_LINE_X(blit_scan2_h_line_3, *(mydst+2) = *(mydst+1) = *mydst,
  mydst+=3)
BLIT_SCAN2_H_LINE_X(blit_scan2_h_line_4, *(mydst+3) = *(mydst+2) = *(mydst+1) =
  *mydst, mydst+=4)

BLIT_BEGIN(blit_scan2_h)
  switch(sysdep_display_params.widthscale)
  {
    case 1:
      BLIT_LOOP(blit_scan2_h_line_1, 2)
      break;
    case 2:
      BLIT_LOOP(blit_scan2_h_line_2, 2)
      break;
    case 3:
      BLIT_LOOP(blit_scan2_h_line_3, 2)
      break;
    case 4:
      BLIT_LOOP(blit_scan2_h_line_4, 2)
      break;
  }
BLIT_END

INLINE void FUNC_NAME(blit_scan2_v_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, RENDER_PIXEL *dst, unsigned int *lookup)
{
  while (src < end)
  {
    *dst++ = GETPIXEL(*src);
    *dst++ = SHADE_HALF( GETPIXEL(*src) ) +
      SHADE_FOURTH( GETPIXEL(*src) );
    src++;
  }
}

BLIT_BEGIN(blit_scan2_v)
BLIT_LOOP_YARBSIZE(blit_scan2_v_line)
BLIT_END


/**********************************
 * rgbscan
 **********************************/
#define BLIT_RGB_H_SCAN_LINE_X(NAME, DST, DST_INC) \
INLINE void FUNC_NAME(NAME)(SRC_PIXEL *src, \
  SRC_PIXEL *end, RENDER_PIXEL *dst, int dest_width, \
  unsigned int *lookup) \
{ \
  RENDER_PIXEL *mydst; \
  SRC_PIXEL *mysrc; \
  \
  mydst = dst; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    DST = RMASK_SEMI(GETPIXEL(*mysrc)); \
    DST_INC; \
  } \
  \
  mydst = dst + dest_width; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    DST = GMASK_SEMI(GETPIXEL(*mysrc)); \
    DST_INC; \
  } \
  \
  mydst = dst + 2*dest_width; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    DST = BMASK_SEMI(GETPIXEL(*mysrc)); \
    DST_INC; \
  } \
}

BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_1, *mydst, mydst++)
BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_2, *(mydst+1) = *mydst, mydst+=2)
BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_3, *(mydst+2) = *(mydst+1) = *mydst,
  mydst+=3)
BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_4, *(mydst+3) = *(mydst+2) =
  *(mydst+1) = *mydst, mydst+=4)
BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_5, *(mydst+4) = *(mydst+3) =
  *(mydst+2) = *(mydst+1) = *mydst, mydst+=5)
BLIT_RGB_H_SCAN_LINE_X(blit_rgbscan_h_line_6, *(mydst+5) = *(mydst+4) =
  *(mydst+3) = *(mydst+2) = *(mydst+1) = *mydst, mydst+=6)

BLIT_BEGIN(blit_rgbscan_h)
  switch(sysdep_display_params.widthscale)
  {
    case 1:
      BLIT_LOOP(blit_rgbscan_h_line_1, 3)
      break;
    case 2:
      BLIT_LOOP(blit_rgbscan_h_line_2, 3)
      break;
    case 3:
      BLIT_LOOP(blit_rgbscan_h_line_3, 3)
      break;
    case 4:
      BLIT_LOOP(blit_rgbscan_h_line_4, 3)
      break;
    case 5:
      BLIT_LOOP(blit_rgbscan_h_line_5, 3)
      break;
    case 6:
      BLIT_LOOP(blit_rgbscan_h_line_6, 3)
      break;
  }
BLIT_END

INLINE void FUNC_NAME(blit_rgbscan_v_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, RENDER_PIXEL *dst, unsigned int *lookup)
{
  while (src < end)
  {
    *dst++ = RMASK_SEMI(GETPIXEL(*src));
    *dst++ = GMASK_SEMI(GETPIXEL(*src));
    *dst++ = BMASK_SEMI(GETPIXEL(*src));
    src++;
  }
}

BLIT_BEGIN(blit_rgbscan_v)
BLIT_LOOP_YARBSIZE(blit_rgbscan_v_line)
BLIT_END

/**********************************
 * scan3, for the horizontal version all 3 lines are horizontally blurred a
 * little (the last pixel of each three in a line is averaged with the next).
 * For both horizotal and vertical versions:
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%. */
#define BLIT_SCAN3_H_LINE_X(NAME, DST1, DST0, DST_INC) \
INLINE void FUNC_NAME(NAME)(SRC_PIXEL *src, \
  SRC_PIXEL *end, RENDER_PIXEL *dst, int dest_width, \
  unsigned int *lookup) \
{ \
  RENDER_PIXEL *mydst; \
  SRC_PIXEL *mysrc; \
  \
  mydst = dst; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    int p = MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1))); \
    DST0 = SHADE_HALF(GETPIXEL(*mysrc)) + SHADE_FOURTH(GETPIXEL(*mysrc)); \
    DST1 = SHADE_HALF(p) + SHADE_FOURTH(p); \
    DST_INC; \
  } \
  \
  mydst = dst+dest_width; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    DST0 = GETPIXEL(*mysrc); \
    DST1 = MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1))); \
    DST_INC; \
  } \
  \
  mydst = dst+2*dest_width; \
  for(mysrc=src; mysrc<end; mysrc++) { \
    DST0 = SHADE_HALF(GETPIXEL(*mysrc)); \
    DST1 = SHADE_HALF(MEAN( GETPIXEL(*mysrc), GETPIXEL(*(mysrc+1)))); \
    DST_INC; \
  } \
}

BLIT_SCAN3_H_LINE_X(blit_scan3_h_line_2, *(mydst+1), *mydst, mydst+=2)
BLIT_SCAN3_H_LINE_X(blit_scan3_h_line_3, *(mydst+2), *(mydst+1) = *mydst,
  mydst+=3)
BLIT_SCAN3_H_LINE_X(blit_scan3_h_line_4, *(mydst+3), *(mydst+2) =
  *(mydst+1) = *mydst, mydst+=4)
BLIT_SCAN3_H_LINE_X(blit_scan3_h_line_5, *(mydst+4), *(mydst+3) =
  *(mydst+2) = *(mydst+1) = *mydst, mydst+=5)
BLIT_SCAN3_H_LINE_X(blit_scan3_h_line_6, *(mydst+5), *(mydst+4) =
  *(mydst+3) = *(mydst+2) = *(mydst+1) = *mydst, mydst+=6)

BLIT_BEGIN(blit_scan3_h)
  switch(sysdep_display_params.widthscale)
  {
    case 2:
      BLIT_LOOP(blit_scan3_h_line_2, 3)
      break;
    case 3:
      BLIT_LOOP(blit_scan3_h_line_3, 3)
      break;
    case 4:
      BLIT_LOOP(blit_scan3_h_line_4, 3)
      break;
    case 5:
      BLIT_LOOP(blit_scan3_h_line_5, 3)
      break;
    case 6:
      BLIT_LOOP(blit_scan3_h_line_6, 3)
      break;
  }
BLIT_END

INLINE void FUNC_NAME(blit_scan3_v_line)(SRC_PIXEL *src,
  SRC_PIXEL *end, RENDER_PIXEL *dst, unsigned int *lookup)
{
  while(src<end) {
    *dst++ = SHADE_HALF(GETPIXEL(*src)) + SHADE_FOURTH(GETPIXEL(*src));
    *dst++ = GETPIXEL(*src);
    *dst++ = SHADE_HALF(GETPIXEL(*src));
    src++;
  }
}

BLIT_BEGIN(blit_scan3_v)
BLIT_LOOP_YARBSIZE(blit_scan3_v_line)
BLIT_END
