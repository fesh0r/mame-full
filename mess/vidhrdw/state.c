#include "driver.h"
#include "includes/state.h"

typedef void (*STATE_FUNCTION)(void);
static struct {
	int count;
	STATE_FUNCTION functions[10];
	struct mame_bitmap *bitmap;

	int y;
	// this is here to generate a full refresh when needed!
	int line;
	int last_widths[20];
	int full_refresh;
} state= {0};

// call this at init time to add your state functions
void state_add_function(void (*function)(void))
{
	state.functions[state.count++]=function;
}

// call this in your state function to output text
void state_display_text(const char *text)
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
			if (state.last_widths[state.line]!=x) {
				state.full_refresh=1;
				state.last_widths[state.line]=x;
			}
			state.line++;
			y2 += Machine->uifont->height;
		}
	}
}

// call this at last after updating your frame
void state_display(struct mame_bitmap *bitmap)
{
	int i;

	state.bitmap=bitmap;
	state.line=0;
	state.full_refresh=0;
	state.y=Machine->uiymin + Machine->uiheight - Machine->uifont->height;

	for (i=0; i<state.count; i++) {
		state.functions[i]();
	}

	if (state.full_refresh||state.last_widths[state.line]!=0) {
		state.last_widths[state.line]=0;
		schedule_full_refresh();
	}

}

