/* this file is used by blit.h -- don't use it directly ! */
#ifdef DOUBLEBUFFER
#  define EFFECT() \
     effect_func(effect_dbbuf, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE, line_src, visual_width); \
     MEMCPY(line_dest, effect_dbbuf, visual_width*widthscale*DEST_PIXEL_SIZE); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE, visual_width*widthscale*DEST_PIXEL_SIZE)
#  define EFFECT2X() \
     effect_scale2x_func(effect_dbbuf, effect_dbbuf+visual_width*2*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width); \
     MEMCPY(line_dest, effect_dbbuf, visual_width*2*DEST_PIXEL_SIZE); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+visual_width*2*DEST_PIXEL_SIZE, visual_width*2*DEST_PIXEL_SIZE)
#  define EFFECT3X() \
     effect_scale3x_func(effect_dbbuf, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE*2, line_src, visual_width); \
     MEMCPY(line_dest, effect_dbbuf, visual_width*widthscale*DEST_PIXEL_SIZE); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE, visual_width*widthscale*DEST_PIXEL_SIZE); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH*2, effect_dbbuf+visual_width*widthscale*DEST_PIXEL_SIZE*2, visual_width*widthscale*DEST_PIXEL_SIZE)
#else
#  define EFFECT() \
     effect_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_src, visual_width)
#  define EFFECT2X() \
     effect_scale2x_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width)
#  define EFFECT3X() \
     effect_scale3x_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, line_dest+CORRECTED_DEST_WIDTH*2, line_src, visual_width)
#endif

/* no double buffering for 6tap2x, since it doesn't do any reads from DEST */
#if 0 
#  define EFFECT_6TAP() \
     effect_6tap_render_func(effect_dbbuf, effect_dbbuf+visual_width*2*DEST_PIXEL_SIZE, visual_width); \
     MEMCPY(line_dest, effect_dbbuf, visual_width*2*DEST_PIXEL_SIZE); \
     MEMCPY(line_dest+CORRECTED_DEST_WIDTH, effect_dbbuf+visual_width*2*DEST_PIXEL_SIZE, visual_width*2*DEST_PIXEL_SIZE)
#else     
#  define EFFECT_6TAP() \
     effect_6tap_render_func(line_dest, line_dest+CORRECTED_DEST_WIDTH, visual_width)
#endif

/* start of actual effect blit code */
  case EFFECT_SCAN2:
  case EFFECT_RGBSTRIPE:
    if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy))
    {
      line_src = (SRC_PIXEL *)rotate_dbbuf0;
      for (y = visual.min_y; y <= visual.max_y; line_dest+=heightscale*CORRECTED_DEST_WIDTH, y++) {
        rotate_func(rotate_dbbuf0, bitmap, y);
        EFFECT();
      }
    }
    else
    {
      for (;line_src < line_end; line_dest+=heightscale*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        EFFECT();
      }
    }  
    break;

  case EFFECT_SCALE2X:
  case EFFECT_HQ2X:
  case EFFECT_LQ2X:
    if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy))
    {
      char *tmp;
      /* preload the first lines for 2x effects */
      rotate_func(rotate_dbbuf1, bitmap, visual.min_y);
      rotate_func(rotate_dbbuf2, bitmap, visual.min_y);
      
      for (y = visual.min_y; y <= visual.max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) {
        /* shift lines up */
        tmp = rotate_dbbuf0;
        rotate_dbbuf0=rotate_dbbuf1;
        rotate_dbbuf1=rotate_dbbuf2;
        rotate_dbbuf2=tmp;
        rotate_func(rotate_dbbuf2, bitmap, y+1);
        EFFECT2X();
      } 
    } 
    else 
    { 
      /* use rotate_dbbufX for the inputs to the effect_func, to keep the EFFECT2x 
         macro ident, this should be safe since these shouldn't be allocated 
         when not rotating */ 
      for (;line_src < line_end; line_dest+=2*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        rotate_dbbuf0=(char *)(line_src-src_width); 
        rotate_dbbuf1=(char *)line_src; 
        rotate_dbbuf2=(char *)(line_src+src_width); 
        EFFECT2X(); 
      } 
      rotate_dbbuf0=NULL; 
      rotate_dbbuf1=NULL; 
      rotate_dbbuf2=NULL; 
    }  
    break;

  case EFFECT_RGBSCAN:
  case EFFECT_SCAN3:
    if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy))
    {
      line_src = (SRC_PIXEL *)rotate_dbbuf0;
      for (y = visual.min_y; y <= visual.max_y; line_dest+=heightscale*CORRECTED_DEST_WIDTH, y++) {
        rotate_func(rotate_dbbuf0, bitmap, y);
        EFFECT3X();
      }
    }
    else
    {
      for (;line_src < line_end; line_dest+=heightscale*CORRECTED_DEST_WIDTH, line_src+=src_width) {
        EFFECT3X();
      }
    }
    break;

  case EFFECT_6TAP2X:
     effect_6tap_clear_func(visual_width);
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       /* put in the first three lines */
       rotate_func(rotate_dbbuf0, bitmap, visual.min_y);
       effect_6tap_addline_func(rotate_dbbuf0, visual_width);
       rotate_func(rotate_dbbuf0, bitmap, visual.min_y+1);
       effect_6tap_addline_func(rotate_dbbuf0, visual_width);
       rotate_func(rotate_dbbuf0, bitmap, visual.min_y+2);
       effect_6tap_addline_func(rotate_dbbuf0, visual_width);

       for (y = visual.min_y; y <= visual.max_y; line_dest+=2*CORRECTED_DEST_WIDTH, y++) {
           if (y <= visual.max_y - 3)
           {
             rotate_func(rotate_dbbuf0, bitmap, y+3);
             effect_6tap_addline_func(rotate_dbbuf0, visual_width);
           }
	   else
           {
             effect_6tap_addline_func(NULL, visual_width);
           }
           EFFECT_6TAP();
       }
     } else {
       effect_6tap_addline_func(line_src, visual_width);
       effect_6tap_addline_func(line_src+=src_width, visual_width);
       effect_6tap_addline_func(line_src+=src_width, visual_width);

       for (y = visual.min_y; y <= visual.max_y; y++, line_dest+=2*CORRECTED_DEST_WIDTH) {
           if (y <= visual.max_y - 3)
           {
             effect_6tap_addline_func(line_src+=src_width, visual_width);
           }
	   else
           {
             effect_6tap_addline_func(NULL, visual_width);
           }
           EFFECT_6TAP();
       }
     }
     break;

#undef EFFECT
#undef EFFECT2X
#undef EFFECT3X
#undef EFFECT_6TAP
