#if !defined xgl
#include "xmame.h"
#include "driver.h"

/* hmm no more way to find out what the width and height of the screenbitmap
   are, so just define WIDTH and HEIGHT to be 2048 */

#define WIDTH  (2048 / 8)
#define HEIGHT (2048 / 8)

static int osd_dirty_malloc(unsigned char **dirty_lines, unsigned char ***dirty_blocks)
{
   int i;
   
   *dirty_lines = malloc(HEIGHT);
   if (!*dirty_lines)
   {
      fprintf(stderr_file, "Error: couldn't allocate mem\n");
      return OSD_NOT_OK;
   }
   memset(*dirty_lines, 0, HEIGHT);
	
   *dirty_blocks = malloc(HEIGHT * sizeof(char *));
   if (!*dirty_blocks)
   {
      free(*dirty_lines); *dirty_lines = NULL;
      fprintf(stderr_file, "Error: couldn't allocate mem\n");
      return OSD_NOT_OK;
   }
	   
   for (i=0; i< HEIGHT; i++)
   {
      (*dirty_blocks)[i] = malloc(WIDTH);
      if (!(*dirty_blocks)[i]) break;
      memset((*dirty_blocks)[i], 0, WIDTH);
   }
   if (i!=HEIGHT)
   { 
      fprintf(stderr_file, "Error: couldn't allocate mem\n");
      for(;i>=0;i--) free((*dirty_blocks)[i]);
      free(*dirty_blocks); *dirty_blocks = NULL;
      free(*dirty_lines);  *dirty_lines  = NULL;
      return OSD_NOT_OK;
   }
   return OSD_OK;
}

static void osd_dirty_free(unsigned char *dirty_lines, unsigned char **dirty_blocks)
{
   int i;
   
   if (!dirty_blocks) return;
   
   for (i=0; i< HEIGHT; i++) free(dirty_blocks[i]);
   free (dirty_blocks);
   free (dirty_lines);
}
      
int osd_dirty_init(void)
{
   dirty_lines      = NULL;
   dirty_blocks     = NULL;
   old_dirty_lines  = NULL;
   old_dirty_blocks = NULL;
   
   switch (use_dirty)
   {
      case 0: /* no dirty */
         /* vector games always need a dirty array */
         if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
            return osd_dirty_malloc(&dirty_lines, &dirty_blocks);
         return OSD_OK;
      case 1: /* normal dirty */
         if (osd_dirty_malloc(&dirty_lines, &dirty_blocks)!=OSD_OK)
            return OSD_NOT_OK;
         if (osd_dirty_malloc(&old_dirty_lines, &old_dirty_blocks)!=OSD_OK)
         {
            osd_dirty_free(dirty_lines, dirty_blocks);
            return OSD_NOT_OK;
         }
         return OSD_OK;
      case 2: /* vector */
         return osd_dirty_malloc(&dirty_lines, &dirty_blocks);
   }
   return OSD_NOT_OK; /* shouldn't happen */
}

void osd_dirty_close(void)
{
   switch (use_dirty)
   {
      case 0: /* no dirty */
         /* vector games always need a dirty array */
         if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
            osd_dirty_free(dirty_lines, dirty_blocks);
         break;
      case 1: /* normal dirty */
         osd_dirty_free(dirty_lines, dirty_blocks);
         osd_dirty_free(old_dirty_lines, old_dirty_blocks);
         break;
      case 2: /* vector */
         osd_dirty_free(dirty_lines, dirty_blocks);
         break;
   }
}

void osd_dirty_merge (void)
{
   int x,y;
   unsigned char *tmp_dirty_lines;
   unsigned char **tmp_dirty_blocks;
      
   /* okay, time to merge the dirty arrays. Store the result in the old arrays
      since those are not needed anymore */
   for (y = visual.min_y>>3; y < (visual.max_y >> 3); y++)
         {
      /* we only have todo something if the new line is dirty */
      if (dirty_lines[y])
           {
         long *blocks     = (long *)dirty_blocks[y];
         long *old_blocks = (long *)old_dirty_blocks[y];
            
         for (x = visual.min_x >> 3; x < (visual.max_x >> 3); x++)
            old_blocks[x] |= blocks[x];
            
         old_dirty_lines[y] = 1;
         }
      }
      
   /* now swap the arrays so the old array containing the merged
      dirty info becomes the current one */
   tmp_dirty_lines = dirty_lines;
   dirty_lines     = old_dirty_lines;
   old_dirty_lines = tmp_dirty_lines;
  
   tmp_dirty_blocks = dirty_blocks;
   dirty_blocks     = old_dirty_blocks;
   old_dirty_blocks = tmp_dirty_blocks;
}

void osd_mark_dirty(int x1, int y1, int x2, int y2, int ui)
{
	int y,x;
	if (use_dirty)
	{
	   if (x1 < visual.min_x) x1=visual.min_x;
	   if (y1 < visual.min_y) y1=visual.min_y;
	   if (x2 > visual.max_x) x2=visual.max_x;
	   if (y2 > visual.max_y) y2=visual.max_y;
	   x1 >>= 3;
	   y1 >>= 3;
	   x2 = (x2 + 8) >> 3;
	   y2 = (y2 + 8) >> 3;
 	   for (y=y1; y<y2; y++)
	   {
	      dirty_lines[y] = 1;
	      for(x=x1; x<x2; x++)
              {
	         dirty_blocks[y][x] = 1;
	      }
	   }
	}
}

#endif /* #if !defined xgl */
