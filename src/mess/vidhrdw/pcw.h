#include "driver.h"
#include "osdepend.h"
#define PCW_BORDER_HEIGHT 8
#define PCW_BORDER_WIDTH 8
#define PCW_NUM_COLOURS 2
#define PCW_DISPLAY_WIDTH 720
#define PCW_DISPLAY_HEIGHT 256

#define PCW_SCREEN_WIDTH	(PCW_DISPLAY_WIDTH + (PCW_BORDER_WIDTH<<1))
#define PCW_SCREEN_HEIGHT	(PCW_DISPLAY_HEIGHT  + (PCW_BORDER_HEIGHT<<1))

int pcw_vh_start(void);
void pcw_vh_stop(void);
void pcw_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void pcw_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);
void pcw_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
