#include "driver.h"
#include "osdepend.h"
#define AVIGO_NUM_COLOURS 2

#define AVIGO_SCREEN_WIDTH        160
#define AVIGO_SCREEN_HEIGHT       240

READ_HANDLER(avigo_vid_memory_r);
WRITE_HANDLER(avigo_vid_memory_w);


int avigo_vh_start(void);
void avigo_vh_stop(void);
void avigo_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void avigo_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);

void	avigo_vh_set_stylus_marker_position(int x,int y);

