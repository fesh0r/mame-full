#ifndef _VIDHRDW_PDP1
#define _VIDHRDW_PDP1 1

#define VIDEO_BITMAP_WIDTH  512
#define VIDEO_BITMAP_HEIGHT 512

#include "driver.h"
#include "vidhrdw/generic.h"

void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh);
void pdp1_vh_stop(void);
int pdp1_vh_start(void);

extern int fio_dec;
extern int concise;
extern int case_state;
extern int reader_buffer;

#endif
