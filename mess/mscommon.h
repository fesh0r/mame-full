/*********************************************************************

	mscommon.h

	MESS specific generic functions

*********************************************************************/

#ifndef MSCOMMON_H
#define MSCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver.h"

/***************************************************************************

	Terminal code

***************************************************************************/

struct terminal;

struct terminal *terminal_create(
	int gfx, int blank_char, int char_bits,
	int (*getcursorcode)(int original_code),
	int num_cols, int num_rows);

void terminal_draw(struct mame_bitmap *dest, const struct rectangle *cliprect,
	struct terminal *terminal);
void terminal_putchar(struct terminal *terminal, int x, int y, int ch);
int terminal_getchar(struct terminal *terminal, int x, int y);
void terminal_setcursor(struct terminal *terminal, int x, int y);
void terminal_hidecursor(struct terminal *terminal);
void terminal_getcursor(struct terminal *terminal, int *x, int *y);
void terminal_clear(struct terminal *terminal, int val);

/***************************************************************************

	LED code

***************************************************************************/

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

/***************************************************************************

	Pool code

***************************************************************************/

typedef struct memory_pool_header *memory_pool;

void pool_init(memory_pool *pool);
void pool_exit(memory_pool *pool);
void *pool_malloc(memory_pool *pool, size_t size);
void *pool_realloc(memory_pool *pool, void *ptr, size_t size);
char *pool_strdup(memory_pool *pool, const char *src);
void pool_freeptr(memory_pool *pool, void *ptr);

/***************************************************************************

	Binary coded decimal

***************************************************************************/

int bcd_adjust(int value);
int dec_2_bcd(int a);
int bcd_2_dec(int a);

/***************************************************************************

	Gregorian calendar code

***************************************************************************/

int	gregorian_is_leap_year(int year);

/* months are one counted */
int gregorian_days_in_month(int month, int year);

/***************************************************************************

	PeT's state text code

***************************************************************************/

/* call this at init time to add your state functions */
void statetext_add_function(void (*function)(void));

/* call this in your state function to output text */
void statetext_display_text(const char *text);

/* call this at last after updating your frame */
void statetext_display(struct mame_bitmap *bitmap);

/**************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MSCOMMON_H */
