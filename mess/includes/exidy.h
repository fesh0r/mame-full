#include "driver.h"
#include "osdepend.h"
#define EXIDY_NUM_COLOURS 2

/* 64 chars wide, 30 chars tall */
#define EXIDY_SCREEN_WIDTH        (64*8)
#define EXIDY_SCREEN_HEIGHT       (30*8)

int exidy_vh_start(void);
void exidy_vh_stop(void);
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void exidy_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);

