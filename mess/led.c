#include "led.h"

void draw_led(struct mame_bitmap *bitmap, const char *led, int valueorcolor, int x, int y)
{
	char c;
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++) {
		c = led[i];
		if (c == '1') {
			plot_pixel(bitmap, x+xi, y+yi, valueorcolor);
		}
		else if (c >= 'a') {
			mask = 1 << (c - 'a');
			color = Machine->pens[(valueorcolor & mask) ? 1 : 0];
			plot_pixel(bitmap, x+xi, y+yi, color);
		}
		if (c != '\r') {
			xi++;
		}
		else {
			yi++;
			xi=0;
		}
	}
}

const char *radius_2_led =
	" 111\r"
	"11111\r"
	"11111\r"
	"11111\r"
	" 111";
