/*********************************************************************

	mscommon.c

	MESS specific generic functions

*********************************************************************/

#include <assert.h>
#include "mscommon.h"

static struct terminal *current_terminal;

/***************************************************************************

	Terminal code

***************************************************************************/

typedef short termchar_t;

struct terminal
{
	struct tilemap *tm;
	int gfx;
	int blank_char;
	int char_bits;
	int num_cols;
	int num_rows;
	int (*getcursorcode)(int original_code);
	int cur_offset;
	termchar_t mem[1];
};

static void terminal_gettileinfo(int memory_offset)
{
	int ch, gfxfont, code, color;
	
	ch = current_terminal->mem[memory_offset];
	code = ch & ((1 << current_terminal->char_bits) - 1);
	color = ch >> current_terminal->char_bits;
	gfxfont = current_terminal->gfx;

	if ((memory_offset == current_terminal->cur_offset) && current_terminal->getcursorcode)
		code = current_terminal->getcursorcode(code);

	SET_TILE_INFO(
		gfxfont,	/* gfx */
		code,		/* character */
		color,		/* color */
		0)			/* flags */
}

struct terminal *terminal_create(
	int gfx, int blank_char, int char_bits,
	int (*getcursorcode)(int original_code),
	int num_cols, int num_rows)
{
	struct terminal *term;
	int char_width, char_height;
	int i;

	char_width = Machine->gfx[gfx]->width;
	char_height = Machine->gfx[gfx]->height;

	term = (struct terminal *) auto_malloc(sizeof(struct terminal) - sizeof(term->mem)
		+ (num_cols * num_rows * sizeof(termchar_t)));
	if (!term)
		return NULL;

	term->tm = tilemap_create(terminal_gettileinfo, tilemap_scan_rows,
		TILEMAP_OPAQUE, char_width, char_height, num_cols, num_rows);
	if (!term->tm)
		return NULL;

	term->gfx = gfx;
	term->blank_char = blank_char;
	term->char_bits = char_bits;
	term->num_cols = num_cols;
	term->num_rows = num_rows;
	term->getcursorcode = getcursorcode;
	term->cur_offset = -1;

	for (i = 0; i < num_cols * num_rows; i++)
		term->mem[i] = blank_char;
	return term;
}

void terminal_draw(struct mame_bitmap *dest, const struct rectangle *cliprect, struct terminal *terminal)
{
	current_terminal = terminal;
	tilemap_draw(dest, cliprect, terminal->tm, 0, 0);
	current_terminal = NULL;
}

void terminal_putchar(struct terminal *terminal, int x, int y, int ch)
{
	int offs;

	assert(x >= 0);
	assert(y >= 0);
	assert(x < terminal->num_cols);
	assert(y < terminal->num_rows);

	offs = y * terminal->num_cols + x;
	if (terminal->mem[offs] != ch)
	{
		terminal->mem[offs] = ch;
		tilemap_mark_tile_dirty(terminal->tm, offs);
	}
}

int terminal_getchar(struct terminal *terminal, int x, int y)
{
	int offs;
	offs = y * terminal->num_cols + x;
	return terminal->mem[offs];
}

static void terminal_dirtycursor(struct terminal *terminal)
{
	if (terminal->cur_offset >= 0)
		tilemap_mark_tile_dirty(terminal->tm, terminal->cur_offset);
}

void terminal_setcursor(struct terminal *terminal, int x, int y)
{
	terminal_dirtycursor(terminal);
	terminal->cur_offset = y * terminal->num_cols + x;
	terminal_dirtycursor(terminal);
}

void terminal_hidecursor(struct terminal *terminal)
{
	terminal_setcursor(terminal, -1, -1);
}

/***************************************************************************

	Pool code

***************************************************************************/

#ifdef DEBUG
#define GUARD_BYTES
#endif

struct pool_memory_header
{
	struct pool_memory_header *next;
#ifdef GUARD_BYTES
	size_t size;
	UINT32 canary;
#endif
};

void pool_init(void **pool)
{
	*pool = NULL;
}

void pool_exit(void **pool)
{
	struct pool_memory_header *mem;
	struct pool_memory_header *next;

	mem = (struct pool_memory_header *) *pool;
	while(mem)
	{
#ifdef GUARD_BYTES
		assert(mem->canary == 0xdeadbeef);
		assert(!memcmp(&mem->canary, ((char *) (mem+1)) + mem->size, sizeof(mem->canary)));
#endif
		next = mem->next;
		free(mem);
		mem = next;
	}
	*pool = NULL;
}

void *pool_malloc(void **pool, size_t size)
{
	struct pool_memory_header *block;
	size_t actual_size;

	actual_size = sizeof(struct pool_memory_header) + size;
#ifdef GUARD_BYTES
	actual_size += sizeof(block->canary);
#endif

	block = malloc(actual_size);
	if (!block)
		return NULL;

	block->next = (struct pool_memory_header *) *pool;
#ifdef GUARD_BYTES
	block->size = size;
	block->canary = 0xdeadbeef;
	memcpy(((char *) (block+1)) + size, &block->canary, sizeof(block->canary));
#endif

	*pool = block;
	return (void *) (block+1);
}

char *pool_strdup(void **pool, const char *src)
{
	char *dst = NULL;
	if (src)
	{
		dst = pool_malloc(pool, strlen(src) + 1);
		if (dst)
			strcpy(dst, src);
	}
	return dst;
}
