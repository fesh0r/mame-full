/*********************************************************************

	mscommon.c

	MESS specific generic functions

*********************************************************************/

#include <assert.h>
#include "mscommon.h"

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

static struct terminal *current_terminal;

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
	terminal_clear(term, blank_char);
	return term;
}

void terminal_draw(struct mame_bitmap *dest, const struct rectangle *cliprect, struct terminal *terminal)
{
	current_terminal = terminal;
	tilemap_draw(dest, cliprect, terminal->tm, 0, 0);
	current_terminal = NULL;
}

static void verify_coords(struct terminal *terminal, int x, int y)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x < terminal->num_cols);
	assert(y < terminal->num_rows);
}

void terminal_putchar(struct terminal *terminal, int x, int y, int ch)
{
	int offs;

	verify_coords(terminal, x, y);

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

	verify_coords(terminal, x, y);
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

void terminal_getcursor(struct terminal *terminal, int *x, int *y)
{
	*x = terminal->cur_offset % terminal->num_cols;
	*y = terminal->cur_offset / terminal->num_cols;
}

void terminal_clear(struct terminal *terminal, int val)
{
	int i;
	for (i = 0; i < terminal->num_cols * terminal->num_rows; i++)
		terminal->mem[i] = val;
}

/***************************************************************************

	LED code

***************************************************************************/

void draw_led(struct mame_bitmap *bitmap, const char *led, int valueorcolor, int x, int y)
{
	char c;
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++)
	{
		c = led[i];
		if (c == '1')
		{
			plot_pixel(bitmap, x+xi, y+yi, valueorcolor);
		}
		else if (c >= 'a')
		{
			mask = 1 << (c - 'a');
			color = Machine->pens[(valueorcolor & mask) ? 1 : 0];
			plot_pixel(bitmap, x+xi, y+yi, color);
		}
		if (c != '\r')
		{
			xi++;
		}
		else
		{
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

/***************************************************************************

	Pool code

***************************************************************************/

#ifdef DEBUG
#define GUARD_BYTES
#endif

struct pool_memory_header
{
	struct pool_memory_header *next;
	struct pool_memory_header **prev;
#ifdef GUARD_BYTES
	size_t size;
	UINT32 canary;
#endif
};

void pool_init(memory_pool *pool)
{
	*pool = NULL;
}

void pool_exit(memory_pool *pool)
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

void *pool_realloc(memory_pool *pool, void *ptr, size_t size)
{
	struct pool_memory_header *block;
	size_t actual_size;

	actual_size = sizeof(struct pool_memory_header) + size;
#ifdef GUARD_BYTES
	actual_size += sizeof(block->canary);
#endif

	if (ptr)
	{
		block = ((struct pool_memory_header *) ptr) - 1;
#ifdef GUARD_BYTES
		assert(block->canary == 0xdeadbeef);
#endif
		block = realloc(block, actual_size);
		if (!block)
			return NULL;
	}
	else
	{
		block = malloc(actual_size);
		if (!block)
			return NULL;
		block->next = (struct pool_memory_header *) *pool;
		block->prev = (struct pool_memory_header **) pool;
	}
	if (block->next)
		block->next->prev = &block->next;
	*(block->prev) = block;

#ifdef GUARD_BYTES
	block->size = size;
	block->canary = 0xdeadbeef;
	memcpy(((char *) (block+1)) + size, &block->canary, sizeof(block->canary));
#endif
	return (void *) (block+1);
}

void *pool_malloc(memory_pool *pool, size_t size)
{
	return pool_realloc(pool, NULL, size);
}

char *pool_strdup(memory_pool *pool, const char *src)
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

void pool_freeptr(memory_pool *pool, void *ptr)
{
	struct pool_memory_header *block;

	block = ((struct pool_memory_header *) ptr) - 1;

	if (block->next)
		block->next->prev = block->prev;
	if (block->prev)
		*(block->prev) = block->next;

	free(block);
}


/***************************************************************************

	Binary coded decimal

***************************************************************************/

int bcd_adjust(int value)
{
	if ((value & 0xf) >= 0xa)
		value = value + 0x10 - 0xa;
	if ((value & 0xf0) >= 0xa0)
		value = value - 0xa0 + 0x100;
	return value;
}

int dec_2_bcd(int a)
{
	return (a % 10) | ((a / 10) << 4);
}

int bcd_2_dec(int a)
{
	return (a & 0xf) + (a >> 4) * 10;
}

/***************************************************************************

	Gregorian calendar code

***************************************************************************/

int	gregorian_is_leap_year(int year)
{
	return ((year & 4) == 0)
		&& ((year % 100 != 0) || (year % 400 == 0));
}

/* months are one counted */
int gregorian_days_in_month(int month, int year)
{
	static int days_in_month[] =
	{
		31, 28, 31, 30, 31, 30,
		31, 31, 30, 31, 30, 31
	};

	if ((month != 2) || !gregorian_is_leap_year(year))
		return days_in_month[month-1];
	else
		return 29;
}

/***************************************************************************

	PeT's state text code

***************************************************************************/

typedef void (*STATE_FUNCTION)(void);

static struct
{
	int count;
	STATE_FUNCTION functions[10];
	struct mame_bitmap *bitmap;
	int y;
} state= {0};

/* call this at init time to add your state functions */
void statetext_add_function(void (*function)(void))
{
	state.functions[state.count++] = function;
}

/* call this in your state function to output text */
void statetext_display_text(const char *text)
{
	int x, x0, y2, width = Machine->uiwidth / Machine->uifont->width;

	if (text[0] != 0)
	{
		x = strlen (text);
		state.y -= Machine->uifont->height * ((x + width - 1) / width);
		y2 = state.y + Machine->uifont->height;
		x = 0;
		while (text[x])
		{
			for (x0 = Machine->uiymin;
				 text[x] && (x0 < Machine->uiymin + Machine->uiwidth-Machine->uifont->width);
				 x++, x0 += Machine->uifont->width)
			{
				drawgfx (state.bitmap, Machine->uifont,
						 text[x], 0, 0, 0, x0, y2, 0,
						 TRANSPARENCY_NONE, 0);
			}
			y2 += Machine->uifont->height;
		}
	}
}

/* call this at last after updating your frame */
void statetext_display(struct mame_bitmap *bitmap)
{
	int i;

	state.bitmap = bitmap;
	state.y = Machine->uiymin + Machine->uiheight - Machine->uifont->height;

	for (i=0; i<state.count; i++)
		state.functions[i]();
}
