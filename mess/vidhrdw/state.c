#include "driver.h"
#include "includes/state.h"

typedef void (*STATE_FUNCTION)(void);
static struct {
	int count;
	STATE_FUNCTION functions[10];
	int y;
	struct osd_bitmap *bitmap;
} state= {0};

// call this at init time to add your state functions
void state_add_function(void (*function)(void))
{
	state.functions[state.count++]=function;
}

// call this in your state function to output text
void state_display_text(const char *text)
{
	int x, x0, y2, width = (Machine->visible_area.max_x -
							Machine->visible_area.min_x) / Machine->uifont->width;


	if (text[0] != 0)
	{
		x = strlen (text);
		state.y -= Machine->uifont->height * ((x + width - 1) / width);
		y2 = state.y + Machine->uifont->height;
		x = 0;
		while (text[x])
		{
			for (x0 = Machine->visible_area.min_x;
				 text[x] && (x0 < Machine->visible_area.max_x -
							 Machine->uifont->width);
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

// call this at last after updating your frame
void state_display(struct osd_bitmap *bitmap)
{
	int i;

	state.bitmap=bitmap;
	state.y=Machine->visible_area.max_y + 1 - Machine->uifont->height;

	for (i=0; i<state.count; i++) {
		state.functions[i]();
	}
}

