/* this file is used by blit.h -- don't use it directly ! */

#ifdef DEST_SCALE
#define DEST_SCALE_X(X)   (SCALE_X(X)  *DEST_SCALE)
#define DEST_SCALE_Y(Y)   (SCALE_Y(Y)  *DEST_SCALE)
#else
#define DEST_SCALE_X(X)   SCALE_X(X)
#define DEST_SCALE_Y(Y)   SCALE_Y(Y)
#endif


if (effect) {
  switch(effect) {

  default:
  case EFFECT_SCALE2X:
    {
#  ifdef DEST
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scale2x_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src-src_width, line_src, line_src+src_width, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_func(line_dest, line_dest+DEST_WIDTH, line_src-src_width, line_src, line_src+src_width, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scale2x_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src-src_width, line_src, line_src+src_width, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_direct_func(line_dest, line_dest+DEST_WIDTH, line_src-src_width, line_src, line_src+src_width, visual_width);
#		endif
#	endif
	}

#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_SCAN2:
      {
#  ifdef DEST
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan2_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan2_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
	}

#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_RGBSTRIPE:
      {
#  ifdef DEST
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
	}

#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_RGBSCAN:
      {
#  ifdef DEST
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
	}

#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

 case EFFECT_SCAN3:
  {
#  ifdef DEST
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan3_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan3_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
	}

#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
	}
      break;
    }
  }

else /* no effect */

   if (!use_dirty)
   {
      /* non dirty */
#ifdef DEST
     int y = visual.min_y;
     int src_width = (((SRC_PIXEL *)bitmap->line[1]) -
		      ((SRC_PIXEL *)bitmap->line[0]));
     SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
     SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
     DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
     
     for (;line_src < line_end;
	  line_dest+=REPS_FOR_Y(DEST_WIDTH,y,visual_height),
	    line_src+=src_width, y++)
       COPY_LINE_FOR_Y(y,visual_height,
		       line_src, line_src+visual_width, line_dest);
#endif
#ifdef PUT_IMAGE
     PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#endif
   }
   else
   {
      /* normal dirty */
      int y, max_y = (visual.max_y+1) >> 3;
#ifdef DEST
      DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST) - DEST_SCALE_X(visual.min_x);
      for (y=visual.min_y>>3; y<max_y;
	   line_dest += DEST_WIDTH * (SCALE_Y((y+1)<<3) - SCALE_Y(y<<3)),
	     y++)
#else
      for (y=visual.min_y>>3; y<max_y; y++)
#endif      
      {
         if (dirty_lines[y])
         {
            int x, max_x;
            max_x = (visual.max_x+1) >> 3;
            for(x=visual.min_x>>3; x<max_x; x++)
            {
               if (dirty_blocks[y][x])
               {
                  int min_x;
#ifdef DEST
                  int max_x, h, max_h;
                  DEST_PIXEL *block_dest = line_dest + DEST_SCALE_X(x<<3);
#endif
                  min_x = x << 3;
                  do {
                     dirty_blocks[y][x]=0;
                     x++;
                  } while (dirty_blocks[y][x]);
#ifdef DEST                  
                  if (x << 3 > visual.max_x)
                     x = (visual.max_x + 1) >> 3;
                  max_x = x << 3;
                  h     = y << 3;
                  max_h = h + 8;
                  for (; h < max_h; 
			 block_dest += REPS_FOR_Y(DEST_WIDTH,h,visual_height),
			 h++)
		    {
		      COPY_LINE_FOR_Y(h,visual_height,
				      (SRC_PIXEL *)bitmap->line[h]+min_x,
				      (SRC_PIXEL *)bitmap->line[h]+max_x,
				      block_dest);
		    }
#endif
#ifdef PUT_IMAGE
		  {
		    int put_height =
		      SCALE_Y((y<<3) - visual.min_y + 8) -
		      SCALE_Y((y<<3) - visual.min_y);

		    if (put_height > 0)
		      PUT_IMAGE(
				SCALE_X( min_x - visual.min_x),
				SCALE_Y((y<<3) - visual.min_y),
				SCALE_X((x<<3) - min_x),
				put_height
				);
		  }
#endif
               }
            }
            dirty_lines[y] = 0;
         }
      }
   }

#undef DEST_SCALE_X
#undef DEST_SCALE_Y











