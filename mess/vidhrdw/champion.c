#include "driver.h"

#include "includes/champion.h"

UINT8 mk2_led[4]= {0};

static char led[]={
	" aaaaa\r"
	"f     b\r"
	"f     b\r"
	"f     b\r"
	"f     b\r"
	"f     b\r"
    " ggggg\r"
	"e     c\r"
	"e     c\r"
	"e     c\r"
	"e     c\r"
	"e     c\r"
    " ddddd h"
};

static void mk2_draw_7segment(struct osd_bitmap *bitmap,int value, int x, int y)
{
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++) {
		mask=0;
		switch (led[i]) {
		case 'a': mask=1; break;
		case 'b': mask=2; break;
		case 'c': mask=4; break;
		case 'd': mask=8; break;
		case 'e': mask=0x10; break;
		case 'f': mask=0x20; break;
		case 'g': mask=0x40; break;
		case 'h': 
			// this is more likely wired to the separate leds
			mask=0x80; 
			break;
		}
		
		if (mask!=0) {
			color=Machine->pens[(value&mask)?1:0];
			plot_pixel(bitmap, x+xi, y+yi, color);
			osd_mark_dirty(x+xi,y,x+yi,y,0);
		}
		if (led[i]!='\r') xi++;
		else { yi++, xi=0; }
	}
}

int mk2_vh_start(void)
{
    return 0;
}

void mk2_vh_stop(void)
{
}

static struct {
	int x,y;
} mk2_led_pos[4]={
	{0,0},
	{20,0},
	{40,0},
	{60,0}
};

void mk2_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;

	for (i=0; i<4; i++) {
		mk2_draw_7segment(bitmap, mk2_led[i], mk2_led_pos[i].x, mk2_led_pos[i].y);
	}

#if 0
			drawgfx(bitmap, Machine->gfx[0], studio2_video.data[y][j],0,
					0,0,x,y,
					0, TRANSPARENCY_NONE,0);
#endif
}
