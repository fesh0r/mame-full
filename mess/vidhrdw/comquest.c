#include "driver.h"

#include "includes/comquest.h"

static struct {
	UINT8 data[128][8];
	void *timer;
	int line;
	int dma_activ;
	int state;
	int count;
} comquest_video={ { {0} } };

int comquest_vh_start(void)
{
	(void) comquest_video;
    return 0;
}

void comquest_vh_stop(void)
{
}

void comquest_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y, j;

	for (y=0; y<128;y++) {
		for (x=0, j=0; j<8;j++,x+=8*4) {
#if 0
			drawgfx(bitmap, Machine->gfx[0], studio2_video.data[y][j],0,
					0,0,x,y,
					0, TRANSPARENCY_NONE,0);
#endif
		}
	}
}
