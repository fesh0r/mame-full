/* this file is used by blit.h -- don't use it directly ! */

#ifdef INDIRECT
#  ifdef DOUBLEBUFFER
#    define EFFECT(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*3)
#  else
#    define EFFECT(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_src, visual_width, INDIRECT)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT)
#  endif
#else
#  ifdef DOUBLEBUFFER
#    define EFFECT(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_direct_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width); \
       MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*3)
#  else
#    define EFFECT(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_src, visual_width)
#    define EFFECT2X(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width)
#    define EFFECT3X(FUNC) \
       effect_##FUNC##_direct_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width)
#  endif
#endif

#define LOOP(FUNC) \
if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) \
{ \
  line_src = (SRC_PIXEL *)rotate_dbbuf; \
  for (y = visual.min_y; y <= visual.max_y; line_dest+=heightscale*CORRECTED_DEST_WIDTH, y++) { \
    rotate_func(rotate_dbbuf, bitmap, y); \
    FUNC; \
  } \
} \
else \
{ \
  for (;line_src < line_end; line_dest+=heightscale*CORRECTED_DEST_WIDTH, line_src+=src_width) { \
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
  for (y = visual.min_y; y <= visual.max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) { \
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
  for (;line_src < line_end; line_dest+=2*CORRECTED_DEST_WIDTH, line_src+=src_width) { \
    rotate_dbbuf0=(char *)(line_src-src_width); \
    rotate_dbbuf1=(char *)line_src; \
    rotate_dbbuf2=(char *)(line_src+src_width); \
    EFFECT2X(FUNC); \
  } \
  rotate_dbbuf0=NULL; \
  rotate_dbbuf1=NULL; \
  rotate_dbbuf2=NULL; \
}  


/* start of actual effect blit code */

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
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       /* put in the first three lines */
#	ifdef INDIRECT
       rotate_func(rotate_dbbuf, bitmap, visual.min_y);
       effect_6tap_addline_func(rotate_dbbuf, visual_width, INDIRECT);
       rotate_func(rotate_dbbuf, bitmap, visual.min_y+1);
       effect_6tap_addline_func(rotate_dbbuf, visual_width, INDIRECT);
       rotate_func(rotate_dbbuf, bitmap, visual.min_y+2);
       effect_6tap_addline_func(rotate_dbbuf, visual_width, INDIRECT);
#	else
       rotate_func(rotate_dbbuf, bitmap, visual.min_y);
       effect_6tap_addline_direct_func(rotate_dbbuf, visual_width);
       rotate_func(rotate_dbbuf, bitmap, visual.min_y+1);
       effect_6tap_addline_direct_func(rotate_dbbuf, visual_width);
       rotate_func(rotate_dbbuf, bitmap, visual.min_y+2);
       effect_6tap_addline_direct_func(rotate_dbbuf, visual_width);
#	endif

       for (y = visual.min_y; y <= visual.max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) {

#	ifdef INDIRECT
           if (y <= visual.max_y - 3)
             {
             rotate_func(rotate_dbbuf, bitmap, y+3);
             effect_6tap_addline_func(rotate_dbbuf, visual_width, INDIRECT);
             }
		   else
             {
             effect_6tap_addline_func(NULL, visual_width, INDIRECT);
             }
#	else
           if (y <= visual.max_y - 3)
             {
             rotate_func(rotate_dbbuf, bitmap, y+3);
             effect_6tap_addline_direct_func(rotate_dbbuf, visual_width);
             }
		   else
             {
             effect_6tap_addline_direct_func(NULL, visual_width);
             }
#	endif

#		ifdef DOUBLEBUFFER
		  effect_6tap_render_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, visual_width);
		  MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_6tap_render_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, visual_width);
#		endif
       }
     } else {
#	ifdef INDIRECT
       effect_6tap_addline_func(line_src, visual_width, INDIRECT);
       effect_6tap_addline_func(line_src+=src_width, visual_width, INDIRECT);
       effect_6tap_addline_func(line_src+=src_width, visual_width, INDIRECT);
#	else
       effect_6tap_addline_direct_func(line_src, visual_width);
       effect_6tap_addline_direct_func(line_src+=src_width, visual_width);
       effect_6tap_addline_direct_func(line_src+=src_width, visual_width);
#	endif

	for (y = visual.min_y; y <= visual.max_y; y++, line_dest+=2*CORRECTED_DEST_WIDTH) {

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
		  effect_6tap_render_func(effect_dbbuf, effect_dbbuf+CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE, visual_width);
		  MEMCPY(line_dest, effect_dbbuf, CORRECTED_DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_6tap_render_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, visual_width);
#		endif
	}
     }
     break;

  case EFFECT_SCAN3:
        LOOP(EFFECT3X(scan3))
        break;

#undef EFFECT
#undef EFFECT2X
#undef EFFECT3X
#undef LOOP
#undef LOOP2X
