#include "mame.h"
#include "common.h"

/* draw_led() will both draw led (where the pixels are identified by '1' or
 * x-segment displays (where the pixels are masked with lowercase letters)
 *
 * the value of 'valueorcolor' is a mask when lowercase letters are in the led
 * string or is a color when '1' characters are in the led string
 */
void draw_led(struct mame_bitmap *bitmap, const char *led, int valueorcolor, int x, int y);

/* a radius two led:
 *
 *	" 111\r"
 *	"11111\r"
 *	"11111\r"
 *	"11111\r"
 *	" 111";
 */
extern const char *radius_2_led;
