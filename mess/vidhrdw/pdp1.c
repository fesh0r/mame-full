/*
 * vidhrdw/pdp1.c
 *
 * This is not a very effective emulation, but frankly, the PDP1 was so slow
 * (compared to machines running MESS), that it really doesn't matter if this
 * is not all that effecient.
 *
 * This is not the video hardware either, this is just a dot plotting routine
 * for display hardware 'emulation' look at the machine/pdp1.c file.
 *
 */

/*
	Theory of operation:

	What makes the pdp-1 CRT so odd is that there is no video processor, no scan logic,
	no refresh logic.  The beam position and intensity is controlled by the program completely:
	in order to draw an object, the program must direct the beam to each point of the object,
	and in order to refresh it, the program must redraw the object periodically.

	Since the refresh rates are highly variable (completely controlled by the program),
	I simulate CRT remanence: the intensity of each pixel on display decreases regularly.
	In order to keep this efficient, I keep a list of non-black pixels.

	However, this was a cause for additional flicker when doing frame skipping.  Basically,
	a pixel was displayed grey instead of white if it was drawn during a skipped frame.
	In order to fix this, I keep track of a pixel's maximum intensity since the last redraw,
	and each pixel is drawn at its maximum, not current, value.
*/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/pdp1.h"
#include "cpu/pdp1/pdp1.h"


static void pdp1_draw_panel(struct mame_bitmap *bitmap, int full_refresh);


static int bitmap_width;	/* width of machine bitmap */
static int bitmap_height;	/* height of machine bitmap */

typedef struct
{
	int cur_intensity;		/* current intensity of the pixel */
	int max_intensity;		/* maximum intensity since the last effective redraw */
							/* a node is not in the list when (max_intensity = -1) */
	int next;				/* index of next pixel in list */
} point;

static point *list;			/* array of (bitmap_width*bitmap_height) point */
static int list_head;		/* head of list in the array */


/*
	video init
*/
int pdp1_vh_start(void)
{
	int i;


	bitmap_width = crt_window_width;
	bitmap_height = crt_window_height;

	/* alloc bitmap for our private fun */
	tmpbitmap = bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (! tmpbitmap)
		return 1;

	/* alloc the array */
	list = malloc(bitmap_width * bitmap_height * sizeof(point));
	if (! list)
	{
		bitmap_free(tmpbitmap);
		tmpbitmap = NULL;

		return 1;
	}
	/* fill with black and set up list as empty */
	for (i=0; i<(bitmap_width * bitmap_height); i++)
	{
		list[i].cur_intensity = 0;
		list[i].max_intensity = -1;
	}

	list_head = -1;

	return 0;
}


/*
	video clean-up
*/
void pdp1_vh_stop(void)
{
	if (list)
	{
		free(list);
		list = NULL;
	}
	if (tmpbitmap)
	{
		bitmap_free(tmpbitmap);
		tmpbitmap = NULL;
	}

	return;
}

/*
	plot a pixel (this is actually deferred)
*/
void pdp1_plot(int x, int y)
{
	point *node;
	int list_index;
	int intensity;

	x = x*bitmap_width/01777;
	y = y*bitmap_height/01777;
	if (x<0) x=0;
	if (y<0) y=0;
	if ((x>(bitmap_width-1))||((y>bitmap_height-1)))
		return;
	y = (bitmap_height-1) - y;
	intensity = VIDEO_MAX_INTENSITY;

	list_index = x + y*bitmap_width;

	node = & list[list_index];

	if (node->max_intensity == -1)
	{	/* insert node in list if it is not in it */
		node->max_intensity = 0;
		node->next = list_head;
		list_head = list_index;
	}
	if (intensity > node->cur_intensity)
		node->cur_intensity = intensity;
}


/*
	decrease pixel intensity
*/
static void update_points(struct mame_bitmap *bitmap)
{
	int i, p_i=-1;

	for (i=list_head; (i != -1); i=list[i].next)
	{
		point *node = & list[i];

		/* remember maximum */
		if (node->cur_intensity > node->max_intensity)
			node->max_intensity = node->cur_intensity;

		/* reduce intensity for next update */
		if (node->cur_intensity > 0)
			node->cur_intensity--;

		p_i = i;	/* current node will be the previous node */
	}
}


/*
	update the bitmap
*/
static void set_points(struct mame_bitmap *bitmap)
{
	int i, p_i=-1;

	for (i=list_head; (i != -1); i=list[i].next)
	{
		point *node = & list[i];
		int x = i % bitmap_width, y = i / bitmap_width;

		if (node->cur_intensity > node->max_intensity)
			node->max_intensity = node->cur_intensity;

		plot_pixel(bitmap, x, y, Machine->pens[node->max_intensity]);
		osd_mark_dirty(x, y, x, y);

		if (/*(node->cur_intensity > 0) ||*/ (node->max_intensity != 0))
		{
			/* reset maximum */
			node->max_intensity = 0;
			p_i = i;	/* current node will be the previous node */
		}
		else
		{	/* delete current node */
			node->max_intensity = -1;
			if (p_i != -1)
				list[p_i].next = node->next;
			else
				list_head = node->next;
		}
	}
}


/*
	pdp1_screen_update: called regularly in simulated CPU time
*/
void pdp1_screen_update(void)
{
	update_points(tmpbitmap);
}


/*
	pdp1_vh_update: effectively redraw the screen
*/
void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh)
{
	set_points(tmpbitmap);

	pdp1_draw_panel(tmpbitmap, full_refresh);
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}



/*
	Operator control panel code
*/

enum
{
	x_panel_offset = 512,
	y_panel_offset = 0,

	x_panel_col1_offset = x_panel_offset+8,
	x_panel_col2_offset = x_panel_col1_offset+144+8,
	x_panel_col3_offset = x_panel_col2_offset+96+8
};

static const struct rectangle panel_window =
{
	0+x_panel_offset,	256+x_panel_offset-1,	/* min_x, max_x */
	0+y_panel_offset,	512+y_panel_offset-1,	/* min_y, max_y */
};


/* draw a small 8*8 LED (or is this a lamp? ) */
static void pdp1_draw_led(struct mame_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;

	for (yy=1; yy<7; yy++)
		for (xx=1; xx<7; xx++)
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[state ? LIT_LAMP : UNLIT_LAMP]);
}

/* draw nb_bits leds which represent nb_bits bits in value */
static void pdp1_draw_multipleled(struct mame_bitmap *bitmap, int x, int y, int value, int nb_bits)
{
	while (nb_bits)
	{
		nb_bits--;

		pdp1_draw_led(bitmap, x, y, (value >> nb_bits) & 1);

		x += 8;
	}
}


/* draw a small 8*8 switch */
static void pdp1_draw_switch(struct mame_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;
	int i;

	/* erase area */
	for (yy=0; yy<8; yy++)
		for (xx=0; xx<8; xx++)
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[PANEL_BG]);


	/* draw circle */
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[SWITCH_BK]);
		plot_pixel(bitmap, x+2+i, y+6, Machine->pens[SWITCH_BK]);
		plot_pixel(bitmap, x+1, y+2+i, Machine->pens[SWITCH_BK]);
		plot_pixel(bitmap, x+6, y+2+i, Machine->pens[SWITCH_BK]);
	}
	plot_pixel(bitmap, x+2, y+2, Machine->pens[SWITCH_BK]);
	plot_pixel(bitmap, x+5, y+2, Machine->pens[SWITCH_BK]);
	plot_pixel(bitmap, x+2, y+5, Machine->pens[SWITCH_BK]);
	plot_pixel(bitmap, x+5, y+5, Machine->pens[SWITCH_BK]);

	/* draw button */
	if (! state)
		y += 4;
	for (i=0; i<2;i++)
	{
		plot_pixel(bitmap, x+3+i, y, Machine->pens[SWITCH_BUTTON]);
		plot_pixel(bitmap, x+3+i, y+3, Machine->pens[SWITCH_BUTTON]);
	}
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[SWITCH_BUTTON]);
		plot_pixel(bitmap, x+2+i, y+2, Machine->pens[SWITCH_BUTTON]);
	}
}


/* draw nb_bits switches which represent nb_bits bits in value */
static void pdp1_draw_multipleswitch(struct mame_bitmap *bitmap, int x, int y, int value, int nb_bits)
{
	while (nb_bits)
	{
		nb_bits--;

		pdp1_draw_switch(bitmap, x, y, (value >> nb_bits) & 1);

		x += 8;
	}
}


/* write a single char on screen */
static void pdp1_draw_char(struct mame_bitmap *bitmap, char character, int x, int y, int color)
{
	drawgfx(bitmap, Machine->gfx[0], character-32, color, 0, 0,
				x+1, y, &Machine->visible_area, TRANSPARENCY_PEN, 0);
}

/* write a string on screen */
static void pdp1_draw_string(struct mame_bitmap *bitmap, const char *buf, int x, int y, int color)
{
	while (* buf)
	{
		pdp1_draw_char(bitmap, *buf, x, y, color);

		x += 8;
		buf++;
	}
}


/* draw a vertical line */
static void pdp1_draw_vline(struct mame_bitmap *bitmap, int x, int y, int height, int color)
{
	while (height--)
		plot_pixel(bitmap, x, y++, color);
}

/* draw a horizontal line */
static void pdp1_draw_hline(struct mame_bitmap *bitmap, int x, int y, int width, int color)
{
	while (width--)
		plot_pixel(bitmap, x++, y, color);
}


static void pdp1_draw_panel(struct mame_bitmap *bitmap, int full_refresh)
{
	int y;

	if (full_refresh)
		fillbitmap(bitmap, Machine->pens[PANEL_BG], &/*Machine->visible_area*/panel_window);

	/* column 1: registers, test word, test address */
	y = y_panel_offset;
	if (full_refresh)
		pdp1_draw_string(bitmap, "program counter", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y, cpunum_get_reg(0, PDP1_PC), 16);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "memory address", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y, cpunum_get_reg(0, PDP1_MA), 16);
	y += 8;

	if (full_refresh)
		pdp1_draw_string(bitmap, "memory buffer", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_MB), 18);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "accumulator", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_AC), 18);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "in-out", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_IO), 18);
	y += 8;

	if (full_refresh)
		pdp1_draw_string(bitmap, "extend  address", x_panel_col1_offset-8, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_switch(bitmap, x_panel_col1_offset, y, 0);	/* not emulated */
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset+2*8, y, pdp1_ta, 16);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "test word", x_panel_col1_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset, y, pdp1_tw, 18);
	y += 8;

	if (full_refresh)
		/* column separator */
		pdp1_draw_vline(bitmap, x_panel_col2_offset-4, y_panel_offset+8, 96, PANEL_TEXT);

	/* column 2: 1-bit indicators */
	y = y_panel_offset+8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "run", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_RUN));
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "cycle", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_CYCLE));
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "defer", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_DEFER));
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "h. s. cycle", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "brk. ctr. 1", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "brk. ctr. 2", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "overflow", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_OV));
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "read in", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_RIM));
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "seq. break", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "extend", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "i-o halt", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "i-o com'ds", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "i-o sync", x_panel_col2_offset+8, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */

	if (full_refresh)
		/* column separator */
		pdp1_draw_vline(bitmap, x_panel_col3_offset-4, y_panel_offset+8, 96, PANEL_TEXT);

	/* column 3: power, single step, single inst, sense, flags, instr... */
	y = y_panel_offset+8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "power", x_panel_col3_offset+16, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, 1);	/* always on */
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, 1);	/* always on */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "single step", x_panel_col3_offset+16, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, 0);	/* not emulated */
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "single inst.", x_panel_col3_offset+16, y, PANEL_TEXT_IX);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, 0);	/* not emulated */
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, 0);	/* not emulated */
	y += 8;
	if (full_refresh)
		/* separator */
		pdp1_draw_hline(bitmap, x_panel_col3_offset+8, y+4, 96, PANEL_TEXT);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "sense switches", x_panel_col3_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_S), 6);
	y += 8;
	pdp1_draw_multipleswitch(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_S), 6);
	y += 8;
	if (full_refresh)
		/* separator */
		pdp1_draw_hline(bitmap, x_panel_col3_offset+8, y+4, 96, PANEL_TEXT);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "program flags", x_panel_col3_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_F), 6);
	y += 8;
	if (full_refresh)
		pdp1_draw_string(bitmap, "instruction", x_panel_col3_offset, y, PANEL_TEXT_IX);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_INSTR), 5);
}
