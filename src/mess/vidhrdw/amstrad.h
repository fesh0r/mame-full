#include "driver.h"

int amstrad_vh_start(void);
void amstrad_vh_stop(void);
void amstrad_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void amstrad_update_scanline(void);
