/*********************************************************************

	mscommon.h

	MESS specific generic functions

*********************************************************************/

#ifndef MSCOMMON_H
#define MSCOMMON_H

#include "driver.h"

/* terminal code */
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
void terminal_setcursor(struct terminal *terminal, int x, int y);
void terminal_hidecursor(struct terminal *terminal);

/* handy functions for memory pools */
void pool_init(void **pool);
void pool_exit(void **pool);
void *pool_malloc(void **pool, size_t size);
char *pool_strdup(void **pool, const char *src);

#endif /* MSCOMMON_H */
