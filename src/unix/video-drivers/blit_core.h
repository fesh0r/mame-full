/* this file is used by blit.h -- don't use it directly ! */

#ifdef DEST_SCALE
#define DEST_SCALE_X(X)   (SCALE_X(X) * DEST_SCALE)
#define DEST_SCALE_Y(Y)   (SCALE_Y(Y) * DEST_SCALE)
#else
#define DEST_SCALE_X(X)   SCALE_X(X)
#define DEST_SCALE_Y(Y)   SCALE_Y(Y)
#endif

#ifdef INDIRECT
#  ifdef DOUBLEBUFFER
#    define EFFECT(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3)
#  else
#    define EFFECT(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+DEST_WIDTH, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT)
#  endif
#else
#  ifdef DOUBLEBUFFER
#    define EFFECT(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width); \
       memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3)
#  else
#    define EFFECT(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+DEST_WIDTH, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width)
#  endif
#endif

#define LOOP(FUNC) \
if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) \
{ \
  line_src = (SRC_PIXEL *)rotate_dbbuf; \
  for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) { \
    rotate_func(rotate_dbbuf, bitmap, y); \
    FUNC; \
  } \
} \
else \
{ \
  for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) { \
    FUNC; \
  } \
}  

#define LOOP2X(FUNC) \
if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) \
{ \
  char *tmp; \
  \
  /* preload the first lines for 2x effects */ \
  rotate_func(rotate_dbbuf1, bitmap, visual.min_y); \
  rotate_func(rotate_dbbuf2, bitmap, visual.min_y); \
  \
  for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) { \
    /* shift lines up */ \
    tmp = rotate_dbbuf0; \
    rotate_dbbuf0=rotate_dbbuf1; \
    rotate_dbbuf1=rotate_dbbuf2; \
    rotate_dbbuf2=tmp; \
    rotate_func(rotate_dbbuf2, bitmap, y+1); \
    EFFECT2X(FUNC); \
  } \
} \
else \
{ \
  /* use rotate_dbbufX for the inputs to the effect_func, to keep the EFFECT2x \
     macro ident, this should be safe since these shouldn't be allocated \
     when not rotating and effect != 6fac2x */ \
  for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) { \
    rotate_dbbuf0=(char *)(line_src-src_width); \
    rotate_dbbuf1=(char *)line_src; \
    rotate_dbbuf2=(char *)(line_src+src_width); \
    EFFECT2X(FUNC); \
  } \
  rotate_dbbuf0=NULL; \
  rotate_dbbuf1=NULL; \
  rotate_dbbuf2=NULL; \
}  

{
  int y;
  int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
  SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y] + visual.min_x;
  SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
  DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

if (effect) {
  switch(effect) {

  default:
  case EFFECT_SCALE2X:
	LOOP2X(scale2x)
        break;

  case EFFECT_HQ2X:
        LOOP2X(hq2x)
        break;

  case EFFECT_LQ2X:
	LOOP2X(lq2x)
        break;

  case EFFECT_SCAN2:
	LOOP(EFFECT(scan2))
        break;

  case EFFECT_RGBSTRIPE:
	LOOP(EFFECT(rgbstripe))
        break;

  case EFFECT_RGBSCAN:
        LOOP(EFFECT3X(rgbscan))
        break;

  case EFFECT_6TAP2X:
       effect_6tap_clear_func(visual_width);
#	ifdef INDIRECT
       effect_6tap_addline_func(line_src, visual_width, INDIRECT);
       effect_6tap_addline_func(line_src+=src_width, visual_width, INDIRECT);
       effect_6tap_addline_func(line_src+=src_width, visual_width, INDIRECT);
#	else
       effect_6tap_addline_direct_func(line_src, visual_width);
       effect_6tap_addline_direct_func(line_src+=src_width, visual_width);
       effect_6tap_addline_direct_func(line_src+=src_width, visual_width);
#	endif

	for (y = visual.min_y; y <= visual.max_y; y++, line_dest+=DEST_SCALE_Y(DEST_WIDTH)) {

#	ifdef INDIRECT
           if (y <= visual.max_y - 3)
             {
             effect_6tap_addline_func(line_src+=src_width, visual_width, INDIRECT);
             }
		   else
             {
             effect_6tap_addline_func(NULL, visual_width, INDIRECT);
             }
#	else
           if (y <= visual.max_y - 3)
             {
             effect_6tap_addline_direct_func(line_src+=src_width, visual_width);
             }
		   else
             {
             effect_6tap_addline_direct_func(NULL, visual_width);
             }
#	endif
#		ifdef DOUBLEBUFFER
		  effect_6tap_render_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_6tap_render_func(line_dest, line_dest+DEST_WIDTH, visual_width);
#		endif
	}
        break;

  case EFFECT_SCAN3:
        LOOP(EFFECT3X(scan3))
        break;
  }
}
else /* no effect */
{
     if (!blit_hardware_rotation && current_palette != debug_palette
		     && (blit_flipx || blit_flipy || blit_swapxy)) {
       for (y = visual.min_y; y <= visual.max_y; line_dest+=REPS_FOR_Y(DEST_WIDTH,y,visual_height), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;
	       line_end = (SRC_PIXEL *)rotate_dbbuf + visual_width;
	       COPY_LINE_FOR_Y(y, visual_height, line_src, line_end, line_dest);
       }
     } else {
       y = visual.min_y;

       for (;line_src < line_end;
	    line_dest+=REPS_FOR_Y(DEST_WIDTH,y,visual_height),
		    line_src+=src_width, y++)
	       COPY_LINE_FOR_Y(y,visual_height,
			       line_src, line_src+visual_width, line_dest);
     }
}

#ifdef PUT_IMAGE
     PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#endif

}

#undef DEST_SCALE_X
#undef DEST_SCALE_Y
#undef EFFECT
#undef EFFECT2X
#undef EFFECT3X
#undef LOOP
#undef LOOP2X
