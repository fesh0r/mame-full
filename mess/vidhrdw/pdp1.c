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

#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"


static void pdp1_draw_panel(struct mame_bitmap *bitmap/*, int full_refresh*/);


typedef struct
{
	int cur_intensity;		/* current intensity of the pixel */
	int max_intensity;		/* maximum intensity since the last effective redraw */
							/* a node is not in the list when (max_intensity = -1) */
	int next;				/* index of next pixel in list */
} point;

static point *list;			/* array of (crt_window_width*crt_window_height) point */
static int list_head;		/* head of list in the array */

static struct mame_bitmap *typewriter_bitmap;

static const struct rectangle typewriter_window =
{
	typewriter_window_offset_x,	typewriter_window_offset_x+typewriter_window_width-1,	/* min_x, max_x */
	typewriter_window_offset_y,	typewriter_window_offset_y+typewriter_window_height-1,	/* min_y, max_y */
};

/*
	video init
*/
VIDEO_START( pdp1 )
{
	int i;

	/* alloc bitmap for our private fun */
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!tmpbitmap)
		return 1;

	typewriter_bitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height/*typewriter_window_width, typewriter_window_height*/);
	if (!typewriter_bitmap)
		return 1;

	/* alloc the array */
	list = auto_malloc(crt_window_width * crt_window_height * sizeof(point));
	if (!list)
		return 1;

	/* fill with black and set up list as empty */
	for (i=0; i<(crt_window_width * crt_window_height); i++)
	{
		list[i].cur_intensity = 0;
		list[i].max_intensity = -1;
	}

	list_head = -1;

	fillbitmap(tmpbitmap, Machine->pens[pen_typewriter_bg], &typewriter_window);

	return 0;
}


/*
	schedule a pixel to be plotted
*/
void pdp1_plot(int x, int y)
{
	point *node;
	int list_index;
	int intensity;

	/* compute pixel coordinates */
	x = x*crt_window_width/01777;
	y = y*crt_window_height/01777;
	if (x<0) x=0;
	if (y<0) y=0;
	if ((x>(crt_window_width-1)) || ((y>crt_window_height-1)))
		return;
	y = (crt_window_height-1) - y;
	intensity = pen_crt_max_intensity;

	/* find entry in list */
	list_index = x + y*crt_window_width;

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
		int x = (i % crt_window_width) + crt_window_offset_x,
			y = (i / crt_window_width) + crt_window_offset_y;

		if (node->cur_intensity > node->max_intensity)
			node->max_intensity = node->cur_intensity;

		plot_pixel(bitmap, x, y, Machine->pens[node->max_intensity]);
		osd_mark_dirty(x, y, x, y);

		if (/*(node->cur_intensity > 0) ||*/ (node->max_intensity != 0))
		{
			/* reset maximum */
			node->max_intensity = 0;
			p_i = i;	/* current node will be next iteration's previous node */
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
VIDEO_UPDATE( pdp1 )
{
	set_points(tmpbitmap);

	pdp1_draw_panel(tmpbitmap/*, full_refresh*/);
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}



/*
	Operator control panel code
*/

enum
{
	x_panel_col1_offset = panel_window_offset_x+8,
	x_panel_col2_offset = x_panel_col1_offset+144+8,
	x_panel_col3_offset = x_panel_col2_offset+96+8
};

static const struct rectangle panel_window =
{
	panel_window_offset_x,	panel_window_offset_x+panel_window_width-1,	/* min_x, max_x */
	panel_window_offset_y,	panel_window_offset_y+panel_window_height-1,/* min_y, max_y */
};


/* draw a small 8*8 LED (or is this a lamp? ) */
static void pdp1_draw_led(struct mame_bitmap *bitmap, int x, int y, int state)
{
	int xx, yy;

	for (yy=1; yy<7; yy++)
		for (xx=1; xx<7; xx++)
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[state ? pen_lit_lamp : pen_unlit_lamp]);
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
			plot_pixel(bitmap, x+xx, y+yy, Machine->pens[pen_panel_bg]);


	/* draw nut (-> circle) */
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+2+i, y+6, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+1, y+2+i, Machine->pens[pen_switch_nut]);
		plot_pixel(bitmap, x+6, y+2+i, Machine->pens[pen_switch_nut]);
	}
	plot_pixel(bitmap, x+2, y+2, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+5, y+2, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+2, y+5, Machine->pens[pen_switch_nut]);
	plot_pixel(bitmap, x+5, y+5, Machine->pens[pen_switch_nut]);

	/* draw button (->disc) */
	if (! state)
		y += 4;
	for (i=0; i<2;i++)
	{
		plot_pixel(bitmap, x+3+i, y, Machine->pens[pen_switch_button]);
		plot_pixel(bitmap, x+3+i, y+3, Machine->pens[pen_switch_button]);
	}
	for (i=0; i<4;i++)
	{
		plot_pixel(bitmap, x+2+i, y+1, Machine->pens[pen_switch_button]);
		plot_pixel(bitmap, x+2+i, y+2, Machine->pens[pen_switch_button]);
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


/*
	draw the operator control panel
*/
static void pdp1_draw_panel(struct mame_bitmap *bitmap/*, int full_refresh*/)
{
	int y;

	//if (full_refresh)
		fillbitmap(bitmap, Machine->pens[pen_panel_bg], & panel_window);

	/* column 1: registers, test word, test address */
	y = panel_window_offset_y;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "program counter", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y, cpunum_get_reg(0, PDP1_PC), 16);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "memory address", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset+2*8, y, cpunum_get_reg(0, PDP1_MA), 16);
	y += 8;

	//if (full_refresh)
		pdp1_draw_string(bitmap, "memory buffer", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_MB), 18);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "accumulator", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_AC), 18);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "in-out", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_IO), 18);
	y += 8;

	//if (full_refresh)
		pdp1_draw_string(bitmap, "extend  address", x_panel_col1_offset-8, y, color_panel_caption);
	y += 8;
	pdp1_draw_switch(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_EXTEND_SW));
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset+2*8, y, cpunum_get_reg(0, PDP1_TA), 16);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "test word", x_panel_col1_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleswitch(bitmap, x_panel_col1_offset, y, cpunum_get_reg(0, PDP1_TW), 18);
	y += 8;

	//if (full_refresh)
		/* column separator */
		pdp1_draw_vline(bitmap, x_panel_col2_offset-4, panel_window_offset_y+8, 96, pen_panel_caption);

	/* column 2: 1-bit indicators */
	y = panel_window_offset_y+8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "run", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_RUN));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "cycle", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_CYC));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "defer", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_DEFER));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "h. s. cycle", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, 0);	/* not emulated */
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "brk. ctr. 1", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_BRK_CTR) & 1);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "brk. ctr. 2", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_BRK_CTR) & 2);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "overflow", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_OV));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "read in", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_RIM));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "seq. break", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_SBM));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "extend", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_EXD));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "i-o halt", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_IOH));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "i-o com'ds", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_IOC));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "i-o sync", x_panel_col2_offset+8, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col2_offset, y, cpunum_get_reg(0, PDP1_IOS));

	//if (full_refresh)
		/* column separator */
		pdp1_draw_vline(bitmap, x_panel_col3_offset-4, panel_window_offset_y+8, 96, pen_panel_caption);

	/* column 3: power, single step, single inst, sense, flags, instr... */
	y = panel_window_offset_y+8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "power", x_panel_col3_offset+16, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, 1);	/* always on */
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, 1);	/* always on */
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "single step", x_panel_col3_offset+16, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_SNGL_STEP));
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, cpunum_get_reg(0, PDP1_SNGL_STEP));
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "single inst.", x_panel_col3_offset+16, y, color_panel_caption);
	pdp1_draw_led(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_SNGL_INST));
	pdp1_draw_switch(bitmap, x_panel_col3_offset+8, y, cpunum_get_reg(0, PDP1_SNGL_INST));
	y += 8;
	//if (full_refresh)
		/* separator */
		pdp1_draw_hline(bitmap, x_panel_col3_offset+8, y+4, 96, pen_panel_caption);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "sense switches", x_panel_col3_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_SS), 6);
	y += 8;
	pdp1_draw_multipleswitch(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_SS), 6);
	y += 8;
	//if (full_refresh)
		/* separator */
		pdp1_draw_hline(bitmap, x_panel_col3_offset+8, y+4, 96, pen_panel_caption);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "program flags", x_panel_col3_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_PF), 6);
	y += 8;
	//if (full_refresh)
		pdp1_draw_string(bitmap, "instruction", x_panel_col3_offset, y, color_panel_caption);
	y += 8;
	pdp1_draw_multipleled(bitmap, x_panel_col3_offset, y, cpunum_get_reg(0, PDP1_IR), 5);
}


/*
	Typewriter code
*/


static int pos;

static int case_shift;

static int color = color_typewriter_black;

enum
{
	typewriter_line_height = 8,
	typewriter_write_offset_y = typewriter_window_offset_y+typewriter_window_height-typewriter_line_height,
	typewriter_scroll_step = typewriter_line_height
};
static const struct rectangle typewriter_scroll_clear_window =
{
	typewriter_window_offset_x,	typewriter_window_offset_x+typewriter_window_width-1,	/* min_x, max_x */
	typewriter_window_offset_y+typewriter_window_height-typewriter_scroll_step,	typewriter_window_offset_y+typewriter_window_height-1,	/* min_y, max_y */
};
static const int var_typewriter_scroll_step = - typewriter_scroll_step;

enum
{
	tab_step = 8
};


static void pdp1_teletyper_linefeed(void)
{
	copyscrollbitmap(typewriter_bitmap, tmpbitmap, 0, NULL, 1, &var_typewriter_scroll_step,
						&Machine->visible_area, TRANSPARENCY_NONE, 0);

	fillbitmap(typewriter_bitmap, Machine->pens[pen_typewriter_bg], &typewriter_scroll_clear_window);

	copybitmap(tmpbitmap, typewriter_bitmap, 0, 0, 0, 0, &typewriter_window, TRANSPARENCY_NONE, 0);
}

void pdp1_teletyper_drawchar(int character)
{
	static const char ascii_table[2][64] =
	{	/* n-s = non-spacing */
		{	/* lower case */
			' ',				'1',				'2',				'3',
			'4',				'5',				'6',				'7',
			'8',				'9',				'*',				'*',
			'*',				'*',				'*',				'*',
			'0',				'/',				's',				't',
			'u',				'v',				'w',				'x',
			'y',				'z',				'*',				',',
			'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
			'\200',/*n-s middle dot*/'j',			'k',				'l',
			'm',				'n',				'o',				'p',
			'q',				'r',				'*',				'*',
			'-',				')',				'\201',/*n-s overstrike*/'(',
			'*',				'a',				'b',				'c',
			'd',				'e',				'f',				'g',
			'h',				'i',				'*',/*Lower Case*/	'.',
			'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
		},
		{	/* upper case */
			' ',				'"',				'\'',				'~',
			'\202',/*implies*/	'\203',/*or*/		'\204',/*and*/		'<',
			'>',				'\205',/*up arrow*/	'*',				'*',
			'*',				'*',				'*',				'*',
			'\206',/*right arrow*/'?',				'S',				'T',
			'U',				'V',				'W',				'X',
			'Y',				'Z',				'*',				'=',
			'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
			'_',/*n-s???*/		'J',				'K',				'L',
			'M',				'N',				'O',				'P',
			'Q',				'R',				'*',				'*',
			'+',				']',				'|',/*n-s???*/		'[',
			'*',				'A',				'B',				'C',
			'D',				'E',				'F',				'G',
			'H',				'I',				'*',/*Lower Case*/	'\207',/*multiply*/
			'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
		}
	};



	character &= 0x3f;

	switch (character)
	{
	case 034:
		/* Black */
		color = color_typewriter_black;
		break;

	case 035:
		/* Red */
		color = color_typewriter_red;
		break;

	case 036:
		/* Tab */
		pos = pos + tab_step - (pos % tab_step);
		break;

	case 072:
		/* Lower case */
		case_shift = 0;
		break;

	case 074:
		/* Upper case */
		case_shift = 1;
		break;

	case 075:
		/* Backspace */
		if (pos)
			pos--;
		break;

	case 077:
		/* Carriage Return */
		pos = 0;
		pdp1_teletyper_linefeed();
		break;

	default:
		/* Any printable character... */

		if (pos >= 80)
		{	/* if past right border, wrap around. (Right???) */
			pdp1_teletyper_linefeed();	/* next line */
			pos = 0;					/* return to start of line */
		}

		/* print character (lookup ASCII equivalent in table) */
		pdp1_draw_char(tmpbitmap, ascii_table[case_shift][character],
						typewriter_window_offset_x+8*pos, typewriter_write_offset_y,
						color);	/* print char */

		if ((character!= 040) && (character!= 056))	/* 040 and 056 are non-spacing characters */
			pos++;		/* step carriage forward */

		break;
	}
}
