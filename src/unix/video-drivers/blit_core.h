/* this file is used by blit.h don't use it directly ! */

#ifdef DEST_SCALE
#define DEST_SCALE_X(X)   (SCALE_X(X)  *DEST_SCALE)
#define DEST_SCALE_X_8(X) (SCALE_X_8(X)*DEST_SCALE)
#define DEST_SCALE_Y(Y)   (SCALE_Y(Y)  *DEST_SCALE)
#define DEST_SCALE_Y_8(Y) (SCALE_Y_8(Y)*DEST_SCALE)
#else
#define DEST_SCALE_X(X)   SCALE_X(X)
#define DEST_SCALE_X_8(X) SCALE_X_8(X)
#define DEST_SCALE_Y(Y)   SCALE_Y(Y)
#define DEST_SCALE_Y_8(Y) SCALE_Y_8(Y)
#endif

switch(use_dirty)
{
   case 0: /* non dirty */
   {
#ifdef DEST
      int src_width = (((SRC_PIXEL *)bitmap->line[1]) -
         ((SRC_PIXEL *)bitmap->line[0]));
      SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
      SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
      DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
      
      for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH),
         line_src+=src_width)
         COPY_LINE(line_src, line_src+visual_width, line_dest)
#endif
#ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#endif      
      break;
   }
   case 1: /* normal dirty */
      osd_dirty_merge();
   case 2: /* vector */
   {
      int y, max_y = (visual.max_y+1) >> 3;
#ifdef DEST
      DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST) - DEST_SCALE_X(visual.min_x);
      for (y=visual.min_y>>3; y<max_y; y++, line_dest+=DEST_SCALE_Y_8(DEST_WIDTH))
#else
      for (y=visual.min_y>>3;y<max_y; y++)
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
                  DEST_PIXEL *block_dest = line_dest + DEST_SCALE_X_8(x);
#endif
                  min_x = x << 3;
                  do {
                     dirty_blocks[y][x]=0;
                     x++;
                  } while (dirty_blocks[y][x]);
#ifdef DEST                  
                  max_x = x << 3;
                  h     = y << 3;
                  max_h = h + 8;
                  for (; h<max_h; h++, block_dest += DEST_SCALE_Y(DEST_WIDTH))
                     COPY_LINE((SRC_PIXEL *)bitmap->line[h]+min_x,
                        (SRC_PIXEL *)bitmap->line[h]+max_x, block_dest)
#endif
#ifdef PUT_IMAGE
                  PUT_IMAGE(
                     SCALE_X( min_x - visual.min_x),
                     SCALE_Y((y<<3) - visual.min_y),
                     SCALE_X((x<<3) - min_x),
                     SCALE_Y(8))
#endif
               }
            }
            dirty_lines[y] = 0;
         }
      }
      break;
   }
}

#undef DEST_SCALE_X
#undef DEST_SCALE_X_8
#undef DEST_SCALE_Y
#undef DEST_SCALE_Y_8
