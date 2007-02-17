/*********************************************************************

    uimenu.c

    Internal MAME menus for the user interface.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "ui.h"
#include "rendutil.h"
#include "cheat.h"
#include "uimenu.h"
#include "uitext.h"

#ifdef MESS
#include "uimess.h"
#include "inputx.h"
#endif /* MESS */



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MENU_STACK_DEPTH		8
#define MENU_STRING_POOL_SIZE	(64*1024)

#define MENU_TEXTCOLOR			ARGB_WHITE
#define MENU_SELECTCOLOR		MAKE_ARGB(0xff,0xff,0xff,0x00)
#define MENU_UNAVAILABLECOLOR	MAKE_ARGB(0xff,0x40,0x40,0x40)

#define MAX_PHYSICAL_DIPS		10

/* DIP switch rendering parameters */
#define DIP_SWITCH_HEIGHT		0.05f
#define DIP_SWITCH_SPACING		0.01
#define SPACE_BETWEEN_BOXES		0.005f
#define SINGLE_TOGGLE_SWITCH_FIELD_WIDTH 0.02f
#define SINGLE_TOGGLE_SWITCH_WIDTH 0.015f
/* make the switch 80% of the width space and 1/2 of the switch height */
#define PERCENTAGE_OF_HALF_FIELD_USED 0.80f
#define SINGLE_TOGGLE_SWITCH_HEIGHT ((DIP_SWITCH_HEIGHT / 2) * PERCENTAGE_OF_HALF_FIELD_USED)


enum
{
	INPUT_TYPE_DIGITAL = 0,
	INPUT_TYPE_ANALOG = 1,
	INPUT_TYPE_ANALOG_DEC = 2,
	INPUT_TYPE_ANALOG_INC = 3
};

enum
{
	ANALOG_ITEM_KEYSPEED = 0,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY,
	ANALOG_ITEM_COUNT
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _menu_state menu_state;
struct _menu_state
{
	ui_menu_handler handler;			/* handler callback */
	UINT32			state;				/* state value */
};


typedef struct _input_item_data input_item_data;
struct _input_item_data
{
	input_seq *		seq;				/* pointer to the sequence we are operating on */
	const input_seq *defseq;			/* pointer to the sequence we are operating on */
	const char *	name;
	const char *	seqname;
	UINT16 			sortorder;			/* sorting information */
	UINT8 			type;				/* type of port */
	UINT8 			invert;				/* type of port */
};


typedef struct _dip_descriptor dip_descriptor;
struct _dip_descriptor
{
	const char * 	dip_name;
	UINT16			total_dip_mask;
	UINT16			total_dip_settings;
	UINT16 			selected_dip_feature_mask;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* menu states */
static int menu_stack_index;
static menu_state menu_stack[MENU_STACK_DEPTH];

static UINT32 menu_string_pool_offset;
static char menu_string_pool[MENU_STRING_POOL_SIZE];

static input_seq starting_seq;

static dip_descriptor dip_switch_model[MAX_PHYSICAL_DIPS];


static const char *input_format[] =
{
	"%s",
	"%s Analog",
	"%s Dec",
	"%s Inc"
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* menu handlers */
static UINT32 menu_main(UINT32 state);
static UINT32 menu_input_groups(UINT32 state);
static UINT32 menu_input(UINT32 state);
static UINT32 menu_switches(UINT32 state);
static UINT32 menu_analog(UINT32 state);
static UINT32 menu_joystick_calibrate(UINT32 state);
#ifndef MESS
static UINT32 menu_bookkeeping(UINT32 state);
#endif
static UINT32 menu_game_info(UINT32 state);
static UINT32 menu_cheat(UINT32 state);
static UINT32 menu_memory_card(UINT32 state);
static UINT32 menu_video(UINT32 state);
static UINT32 menu_reset_game(UINT32 state);
#ifdef MESS
static UINT32 menu_file_manager(UINT32 state);
static UINT32 menu_tape_control(UINT32 state);
#endif

/* menu helpers */
static int input_menu_get_items(input_item_data *itemlist, int group);
static int input_menu_get_game_items(input_item_data *itemlist);
static void input_menu_toggle_none_default(input_seq *selected_seq, input_seq *original_seq, const input_seq *selected_defseq);
static int input_menu_compare_items(const void *i1, const void *i2);
static void switches_menu_add_item(ui_menu_item *item, const input_port_entry *in, int switch_entry, void *ref);
static void switches_menu_select_previous(input_port_entry *in, int switch_entry);
static void switches_menu_select_next(input_port_entry *in, int switch_entry);
//static int switches_menu_compare_items(const void *i1, const void *i2);
static void analog_menu_add_item(ui_menu_item *item, const input_port_entry *in, int append_string, int which_item);

/* DIP switch helpers */
static void dip_switch_build_model(input_port_entry *entry, int item_is_selected);
static void dip_switch_draw_one(float dip_menu_x1, float dip_menu_y1, float dip_menu_x2, float dip_menu_y2, int model_index);
static float dip_switch_get_extra_height(void);
float dip_switch_get_extra_width(void);
static void dip_switch_augment_menu(float x1, float y1, float x2, float y2);


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    menu_string_pool_add - add a formatted string
    to the string pool
-------------------------------------------------*/

INLINE const char *CLIB_DECL menu_string_pool_add(const char *format, ...)
{
	char *result = &menu_string_pool[menu_string_pool_offset];
	va_list arg;

	/* print to the string pool */
	va_start(arg, format);
	menu_string_pool_offset += vsprintf(result, format, arg) + 1;
	va_end(arg);

	return result;
}


/*-------------------------------------------------
    get_num_dips - return the number of physical
    DIP switches that are to be drawn
-------------------------------------------------*/

INLINE int get_num_dips(void)
{
	int num = 0;

	while (dip_switch_model[num].dip_name != NULL && num++ < MAX_PHYSICAL_DIPS)
		;

	return num;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ui_menu_init - initialize the menu system
-------------------------------------------------*/

void ui_menu_init(running_machine *machine)
{
	ui_menu_stack_reset();
}


/*-------------------------------------------------
    ui_menu_draw - draw a menu
-------------------------------------------------*/

void ui_menu_draw(const ui_menu_item *items, int numitems, int selected, menu_augment_routines *augmentation_routines)
{
	const char *up_arrow = ui_getstring(UI_uparrow);
	const char *down_arrow = ui_getstring(UI_downarrow);
	const char *left_arrow = ui_getstring(UI_leftarrow);
	const char *right_arrow = ui_getstring(UI_rightarrow);
	const char *left_hilight = ui_getstring(UI_lefthilight);
	const char *right_hilight = ui_getstring(UI_righthilight);

	float left_arrow_width = ui_get_string_width(left_arrow);
	float right_arrow_width = ui_get_string_width(right_arrow);
	float left_hilight_width = ui_get_string_width(left_hilight);
	float right_hilight_width = ui_get_string_width(right_hilight);
	float line_height = ui_get_line_height();
	float gutter_width;
	float x1, y1, x2, y2;

	float effective_width, effective_left;
	float visible_width, visible_main_menu_height;
	float visible_augmented_menu_height = 0;
	float visible_top, visible_left;
	int selected_subitem_too_big = 0;
	int visible_lines;
	int top_line;
	int itemnum, linenum;

	/* the left/right gutters are the max of all stuff that might go in there */
	gutter_width = MAX(left_hilight_width, right_hilight_width);
	gutter_width = MAX(gutter_width, left_arrow_width);
	gutter_width = MAX(gutter_width, right_arrow_width);

	/* compute the width and height of the full menu */
	visible_width = 0;
	visible_main_menu_height = 0;
	for (itemnum = 0; itemnum < numitems; itemnum++)
	{
		const ui_menu_item *item = &items[itemnum];
		float total_width;

		/* compute width of left hand side */
		total_width = gutter_width + ui_get_string_width(item->text) + gutter_width;

		/* add in width of right hand side */
		if (item->subtext)
			total_width += 2.0f * gutter_width + ui_get_string_width(item->subtext);

		/* track the maximum */
		if (total_width > visible_width)
			visible_width = total_width;

		/* track the height as well */
		visible_main_menu_height += line_height;
	}

	/* if agumenting the menu, find out how much extra space is needed */
	if (augmentation_routines != NULL)
		visible_augmented_menu_height = (*augmentation_routines->get_augmentation_menu_height)();

	/* note: we should also call augmentation_routines->get_augmentation_menu_width()
     * to make sure that there is enough room to draw the DIPs in box space.
     * The method is needs to be implemented (it's just stubbed now)
     * which should not be too difficult
     * using code like that used to draw the DIP switches.  One thought is to
     * not draw the extended DIP menu at all if the width is insufficient.
     * Another thought is to draw the extended menu and just put text in it
     * indicating that "space insufficient to draw DIPs" */


	/* add a little bit of slop for rounding */
	visible_width += 0.01f;
	visible_main_menu_height += 0.01f;

	/* if we are too wide or too tall, clamp it down */
	if (visible_width + 2.0f * UI_BOX_LR_BORDER > 1.0f)
		visible_width = 1.0f - 2.0f * UI_BOX_LR_BORDER;
	/* if the menu and extra menu won't fit, take away part of the regular menu, it will scroll */
	if ((visible_main_menu_height + visible_augmented_menu_height) + 2.0f * UI_BOX_TB_BORDER > 1.0f)
		visible_main_menu_height = 1.0f - 2.0f * UI_BOX_TB_BORDER - visible_augmented_menu_height;

	visible_lines = floor(visible_main_menu_height / line_height);
	visible_main_menu_height = (float)visible_lines * line_height;

	/* compute top/left of inner menu area by centering */
	visible_left = (1.0f - visible_width) * 0.5f;
	visible_top = (1.0f - (visible_main_menu_height + visible_augmented_menu_height)) * 0.5f;

	/* first add us a box */
	x1 = visible_left - UI_BOX_LR_BORDER;
	y1 = visible_top - UI_BOX_TB_BORDER;
	x2 = visible_left + visible_width + UI_BOX_LR_BORDER;
	y2 = visible_top + visible_main_menu_height + UI_BOX_TB_BORDER;

	ui_draw_outlined_box(	x1,
							y1,
							x2,
							y2,
							UI_FILLCOLOR);

	/* determine the first visible line based on the current selection */
	top_line = selected - visible_lines / 2;
	if (top_line < 0)
		top_line = 0;
	if (top_line + visible_lines >= numitems)
		top_line = numitems - visible_lines;

	/* determine effective positions taking into account the hilighting arrows */
	effective_width = visible_width - 2.0f * gutter_width;
	effective_left = visible_left + gutter_width;

	/* loop over visible lines */
	for (linenum = 0; linenum < visible_lines; linenum++)
	{
		float line_y = visible_top + (float)linenum * line_height;
		int itemnum = top_line + linenum;
		const ui_menu_item *item = &items[itemnum];
		rgb_t itemfg = MENU_TEXTCOLOR;

		/* if we're selected, draw with a different background */
		if (itemnum == selected)
			itemfg = MENU_SELECTCOLOR;

		/* if we're on the top line, display the up arrow */
		if (linenum == 0 && top_line != 0)
			ui_draw_text_full(up_arrow, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);

		/* if we're on the bottom line, display the down arrow */
		else if (linenum == visible_lines - 1 && itemnum != numitems - 1)
			ui_draw_text_full(down_arrow, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);

		/* if we don't have a subitem, just draw the string centered */
		else if (!item->subtext)
			ui_draw_text_full(item->text, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);

		/* otherwise, draw the item on the left and the subitem text on the right */
		else
		{
			int subitem_invert = item->flags & MENU_FLAG_INVERT;
			const char *subitem_text = item->subtext;
			float item_width, subitem_width;

			/* draw the left-side text */
			ui_draw_text_full(item->text, effective_left, line_y, effective_width,
						JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, &item_width, NULL);

			/* give 2 spaces worth of padding */
			item_width += 2.0f * gutter_width;

			/* if the subitem doesn't fit here, display dots */
			if (ui_get_string_width(subitem_text) > effective_width - item_width)
			{
				subitem_text = "...";
				if (itemnum == selected)
					selected_subitem_too_big = 1;
			}

			/* draw the subitem right-justified */
			ui_draw_text_full(subitem_text, effective_left + item_width, line_y, effective_width - item_width,
						JUSTIFY_RIGHT, WRAP_TRUNCATE, subitem_invert ? DRAW_OPAQUE : DRAW_NORMAL, subitem_invert ? ARGB_BLACK : itemfg, subitem_invert ? itemfg : ARGB_BLACK, &subitem_width, NULL);

			/* apply arrows */
			if (itemnum == selected && (item->flags & MENU_FLAG_LEFT_ARROW))
				ui_draw_text_full(left_arrow, effective_left + effective_width - subitem_width - left_arrow_width, line_y, left_arrow_width,
							JUSTIFY_LEFT, WRAP_NEVER, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);
			if (itemnum == selected && (item->flags & MENU_FLAG_RIGHT_ARROW))
				ui_draw_text_full(right_arrow, visible_left, line_y, visible_width,
							JUSTIFY_RIGHT, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);
		}

		/* draw the arrows for selected items */
		if (itemnum == selected)
		{
			ui_draw_text_full(left_hilight, visible_left, line_y, visible_width,
						JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);
			if (!(item->flags & (MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW)))
				ui_draw_text_full(right_hilight, visible_left, line_y, visible_width,
							JUSTIFY_RIGHT, WRAP_TRUNCATE, DRAW_NORMAL, itemfg, ARGB_BLACK, NULL, NULL);
		}
	}

	/* if the selected subitem is too big, display it in a separate offset box */
	if (selected_subitem_too_big)
	{
		const ui_menu_item *item = &items[selected];
		int linenum = selected - top_line;
		float line_y = visible_top + (float)linenum * line_height;
		float target_width, target_height;
		float target_x, target_y;

		/* compute the multi-line target width/height */
		ui_draw_text_full(item->subtext, 0, 0, visible_width * 0.75f,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &target_width, &target_height);

		/* determine the target location */
		target_x = visible_left + visible_width - target_width - UI_BOX_LR_BORDER;
		target_y = line_y + line_height + UI_BOX_TB_BORDER;
		if (target_y + target_height + UI_BOX_TB_BORDER > visible_main_menu_height)
			target_y = line_y - target_height - UI_BOX_TB_BORDER;

		/* add a box around that */
		ui_draw_outlined_box(target_x - UI_BOX_LR_BORDER,
						 target_y - UI_BOX_TB_BORDER,
						 target_x + target_width + UI_BOX_LR_BORDER,
						 target_y + target_height + UI_BOX_TB_BORDER, UI_FILLCOLOR);
		ui_draw_text_full(item->subtext, target_x, target_y, target_width,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
	}

	/* If there is somthing special to add, do it by calling the passed routine */
	if (augmentation_routines != NULL)
		(*augmentation_routines->render_augmentation_menu)(x1, y1, x2, y2);

}


/*-------------------------------------------------
    ui_menu_generic_keys - generically handle
    keys for a menu
-------------------------------------------------*/

int ui_menu_generic_keys(UINT32 *selected, int num_items)
{
	/* hitting cancel or selecting the last item returns to the previous menu */
	if (input_ui_pressed(IPT_UI_CANCEL) || (*selected == num_items - 1 && input_ui_pressed(IPT_UI_SELECT)))
	{
		*selected = ui_menu_stack_pop();
		return 1;
	}

	/* up backs up by one item */
	if (input_ui_pressed_repeat(IPT_UI_UP, 6))
		*selected = (*selected + num_items - 1) % num_items;

	/* down advances by one item */
	if (input_ui_pressed_repeat(IPT_UI_DOWN, 6))
		*selected = (*selected + 1) % num_items;

	/* home goes to the start */
	if (input_ui_pressed(IPT_UI_HOME))
		*selected = 0;

	/* end goes to the last */
	if (input_ui_pressed(IPT_UI_END))
		*selected = num_items - 1;

	return 0;
}


/*-------------------------------------------------
    ui_menu_stack_reset - reset the menu stack
-------------------------------------------------*/

void ui_menu_stack_reset(void)
{
	menu_state *state = &menu_stack[menu_stack_index = 0];
	state->handler = NULL;
	state->state = 0;
}


/*-------------------------------------------------
    ui_menu_stack_push - push a new menu onto the
    stack
-------------------------------------------------*/

UINT32 ui_menu_stack_push(ui_menu_handler new_handler, UINT32 new_state)
{
	menu_state *state = &menu_stack[++menu_stack_index];
	assert(menu_stack_index < MENU_STACK_DEPTH);
	state->handler = new_handler;
	state->state = new_state;
	return new_state;
}


/*-------------------------------------------------
    ui_menu_stack_pop - pop a menu from the stack
-------------------------------------------------*/

UINT32 ui_menu_stack_pop(void)
{
	menu_state *state = &menu_stack[--menu_stack_index];
	assert(menu_stack_index >= 0);
	return state->state;
}



/*-------------------------------------------------
    ui_menu_ui_handler - displays the current menu
    and calls the menu handler
-------------------------------------------------*/

UINT32 ui_menu_ui_handler(UINT32 state)
{
	UINT32 newstate;

	/* if we have no menus stacked up, start with the main menu */
	if (menu_stack[menu_stack_index].handler == NULL)
		ui_menu_stack_push(menu_main, 0);

	/* update the menu state */
	newstate = (*menu_stack[menu_stack_index].handler)(menu_stack[menu_stack_index].state);
	menu_stack[menu_stack_index].state = newstate;

	/* if the menus are to be hidden, return a cancel here */
	if (input_ui_pressed(IPT_UI_CONFIGURE) || menu_stack[menu_stack_index].handler == NULL)
		return UI_HANDLER_CANCEL;

	return 0;
}



/***************************************************************************
    MENU HANDLERS
***************************************************************************/

/*-------------------------------------------------
    menu_main - main UI menu
-------------------------------------------------*/

static UINT32 menu_main(UINT32 state)
{
#define ADD_MENU(name, _handler, _param) \
do { \
	item_list[menu_items].text = ui_getstring(name); \
	handler_list[menu_items].handler = _handler; \
	handler_list[menu_items].state = _param; \
	menu_items++; \
} while (0)

	menu_state handler_list[20];
	ui_menu_item item_list[20];
	int has_categories = FALSE;
	int has_configs = FALSE;
	int has_analog = FALSE;
	int has_dips = FALSE;
	input_port_entry *in;
	int menu_items = 0;

	/* scan the input port array to see what options we need to enable */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (input_port_active(in))
		{
			if (in->type == IPT_DIPSWITCH_NAME)
				has_dips = TRUE;
			if (port_type_is_analog(in->type))
				has_analog = TRUE;
			if (in->type == IPT_CONFIG_NAME)
				has_configs = TRUE;
			if (in->category > 0)
				has_categories = TRUE;
		}

	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));

	/* add input menu items */
	ADD_MENU(UI_inputgeneral, menu_input_groups, 0);
	ADD_MENU(UI_inputspecific, menu_input, 1000 << 16);

	/* add optional input-related menus */
	if (has_dips)
		ADD_MENU(UI_dipswitches, menu_switches, (IPT_DIPSWITCH_NAME << 16) | (IPT_DIPSWITCH_SETTING << 24));
	if (has_configs)
		ADD_MENU(UI_configuration, menu_switches, (IPT_CONFIG_NAME << 16) | (IPT_CONFIG_SETTING << 24));
#ifdef MESS
	if (has_categories)
		ADD_MENU(UI_categories, menu_switches, (IPT_CATEGORY_NAME << 16) | (IPT_CATEGORY_SETTING << 24));
#endif
	if (has_analog)
		ADD_MENU(UI_analogcontrols, menu_analog, 0);
  	if (osd_joystick_needs_calibration())
		ADD_MENU(UI_calibrate, menu_joystick_calibrate, 0);

#ifndef MESS
  	/* add bookkeeping menu */
	ADD_MENU(UI_bookkeeping, menu_bookkeeping, 0);
#endif

	/* add game info menu */
	ADD_MENU(UI_gameinfo, menu_game_info, 0);

#ifdef MESS
  	/* add image info menu */
	ADD_MENU(UI_imageinfo, ui_menu_image_info, 0);

  	/* add image info menu */
	ADD_MENU(UI_filemanager, menu_file_manager, 1);

#if HAS_WAVE
  	/* add tape control menu */
	if (device_find(Machine->devices, IO_CASSETTE))
		ADD_MENU(UI_tapecontrol, menu_tape_control, 1);
#endif /* HAS_WAVE */
#endif /* MESS */

	/* add video options menu */
	ADD_MENU(UI_video, menu_video, 1000 << 16);

	/* add cheat menu */
	if (options.cheat)
		ADD_MENU(UI_cheat, menu_cheat, 1);

	/* add memory card menu */
	if (Machine->drv->memcard_handler != NULL)
		ADD_MENU(UI_memorycard, menu_memory_card, 0);

	/* add reset and exit menus */
	ADD_MENU(UI_resetgame, menu_reset_game, 0);
	ADD_MENU(UI_returntogame, NULL, 0);

	/* draw the menu */
	ui_menu_draw(item_list, menu_items, state, NULL);

	/* handle the keys */
	if (ui_menu_generic_keys(&state, menu_items))
		return state;
	if (input_ui_pressed(IPT_UI_SELECT))
		return ui_menu_stack_push(handler_list[state].handler, handler_list[state].state);

	return state;

#undef ADD_MENU
}


/*-------------------------------------------------
    menu_input_groups - menu displaying input
    groups
-------------------------------------------------*/

static UINT32 menu_input_groups(UINT32 state)
{
	ui_menu_item item_list[IPG_TOTAL_GROUPS + 2];
	int menu_items = 0;

	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));

	/* build up the menu */
	for (menu_items = 0; menu_items < IPG_TOTAL_GROUPS; menu_items++)
		item_list[menu_items].text = ui_getstring(UI_uigroup + menu_items);

	/* add an item for the return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_menu_draw(item_list, menu_items, state, NULL);

	/* handle the keys */
	if (ui_menu_generic_keys(&state, menu_items))
		return state;
	if (input_ui_pressed(IPT_UI_SELECT))
		return ui_menu_stack_push(menu_input, state << 16);

	return state;
}


/*-------------------------------------------------
    menu_input - display a menu for inputs
-------------------------------------------------*/

static UINT32 menu_input(UINT32 state)
{
	ui_menu_item item_list[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	input_item_data item_data[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	input_item_data *selected_item_data;
	UINT32 selected = state & 0x3fff;
	int record_next = (state >> 14) & 1;
	int polling = (state >> 15) & 1;
	int group = state >> 16;
	int menu_items;
	int item;

	/* get the list of items */
	menu_string_pool_offset = 0;
	menu_items = input_menu_get_items(item_data, group);

	/* build the menu */
	memset(item_list, 0, sizeof(item_list));
	for (item = 0; item < menu_items; item++)
	{
		input_item_data *id = &item_data[item];

		/* set the item text from the precomputed data */
		item_list[item].text = id->name;
		item_list[item].subtext = id->seqname;
		if (id->invert)
			item_list[item].flags |= MENU_FLAG_INVERT;

		/* keep the sequence pointer as a ref */
		item_list[item].ref = id;
	}

	/* sort the list canonically */
	qsort(item_list, menu_items, sizeof(item_list[0]), input_menu_compare_items);

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* if we're polling, just put an empty entry and arrows for the subitem */
	if (polling)
	{
		item_list[selected].subtext = "   ";
		item_list[selected].flags = MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW;
	}

	/* draw the menu */
	ui_menu_draw(item_list, menu_items, selected, NULL);

	/* if we're polling, read the sequence */
	selected_item_data = item_list[selected].ref;
	if (polling)
	{
		int result = seq_read_async(selected_item_data->seq, !record_next);

		/* continue polling only if we get a negative result */
		polling = (result < 0);

		/* a zero result means we're done with the sequence; indicate we can append to it */
		if (result == 0)
			record_next = TRUE;

		/* a positive result means the user cancelled */
		else if (result > 0)
		{
			record_next = FALSE;
			input_menu_toggle_none_default(selected_item_data->seq, &starting_seq, selected_item_data->defseq);
		}
	}

	/* otherwise, handle the keys */
	else
	{
		int prevsel = selected;

		/* handle generic menu keys */
		if (ui_menu_generic_keys(&selected, menu_items))
			return selected;

		/* if an item was selected, start polling on it */
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			seq_read_async_start(selected_item_data->type == INPUT_TYPE_ANALOG);
			seq_copy(&starting_seq, selected_item_data->seq);
			polling = TRUE;
		}

		/* if the clear key was pressed, reset the selected item */
		if (input_ui_pressed(IPT_UI_CLEAR))
		{
			input_menu_toggle_none_default(selected_item_data->seq, selected_item_data->seq, selected_item_data->defseq);
			record_next = FALSE;
		}

		/* if the selection changed, update and reset the "record first" flag */
		if (selected != prevsel)
			record_next = FALSE;
	}

	return selected | (record_next << 14) | (polling << 15) | (group << 16);
}


/*-------------------------------------------------
    menu_switches - display a menu for DIP
    switches
-------------------------------------------------*/

static UINT32 menu_switches(UINT32 state)
{
	ui_menu_item item_list[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	int switch_entry = (state >> 24) & 0xff;
	int switch_name = (state >> 16) & 0xff;
	UINT32 selected = state & 0xffff;
	input_port_entry *selected_in = NULL;
	input_port_entry *in;
	int menu_items = 0;
	int changed = FALSE;

	menu_augment_routines augment_with_dips;
	augment_with_dips.get_augmentation_menu_width = dip_switch_get_extra_width;
	augment_with_dips.get_augmentation_menu_height = dip_switch_get_extra_height;
	augment_with_dips.render_augmentation_menu = dip_switch_augment_menu;


	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));
	/* reset the dip switch model */
	memset(dip_switch_model, 0, sizeof(dip_switch_model));

	/* loop over input ports and set up the current values */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (in->type == switch_name && input_port_active(in) && input_port_condition(in))
		{
			switches_menu_add_item(&item_list[menu_items], in, switch_entry, in);
			if (in->type == IPT_DIPSWITCH_NAME)
				dip_switch_build_model(in, menu_items == selected);
			menu_items++;
		}

	/* sort the list */
//  qsort(item_list, menu_items, sizeof(item_list[0]), switches_menu_compare_items);
	selected_in = item_list[selected].ref;

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* go through the port map and create masks that are easy to use when drawing DIP graphics. */
	/* draw the menu, augment the regular menue drawing with an additional box for DIPs */
	ui_menu_draw(item_list, menu_items, selected, (dip_switch_model[0].dip_name) ? &augment_with_dips : NULL);

	/* handle generic menu keys */
	if (ui_menu_generic_keys(&selected, menu_items))
		return selected;

	/* handle left/right arrows */
	if (input_ui_pressed(IPT_UI_LEFT) && (item_list[selected].flags & MENU_FLAG_LEFT_ARROW))
	{
		switches_menu_select_previous(selected_in, switch_entry);
		changed = TRUE;
	}
	if (input_ui_pressed(IPT_UI_RIGHT) && (item_list[selected].flags & MENU_FLAG_RIGHT_ARROW))
	{
		switches_menu_select_next(selected_in, switch_entry);
		changed = TRUE;
	}

	/* update the selection to match the existing entry in case things got shuffled */
	/* due to conditional DIPs changing things */
	if (changed)
	{
		int newsel = 0;
		input_port_update_defaults();
		for (in = Machine->input_ports; in->type != IPT_END; in++)
			if (in->type == switch_name && input_port_active(in) && input_port_condition(in))
			{
				if (selected_in == in)
				{
					selected = newsel;
					break;
				}
				newsel++;
			}
	}

	return selected | (switch_name << 16) | (switch_entry << 24);
}


/*-------------------------------------------------
    menu_analog - display a menu for analog
    control settings
-------------------------------------------------*/

static UINT32 menu_analog(UINT32 state)
{
	ui_menu_item item_list[MAX_INPUT_PORTS * 4 * ANALOG_ITEM_COUNT];
	input_port_entry *selected_in = NULL;
	input_port_entry *in;
	int menu_items = 0;
	int selected_item = 0;
	int delta = 0;
	int use_autocenter;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* loop over input ports and add the items */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (port_type_is_analog(in->type))
		{
			use_autocenter = 0;
			switch (in->type)
			{
				/* Autocenter Speed is only used for these devices */
				case IPT_POSITIONAL:
				case IPT_POSITIONAL_V:
					if (in->analog.wraps) break;

				case IPT_PEDAL:
				case IPT_PEDAL2:
				case IPT_PEDAL3:
				case IPT_PADDLE:
				case IPT_PADDLE_V:
				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
				case IPT_AD_STICK_Z:
					use_autocenter = 1;
					break;
			}

			/* track the selected item */
			if (state >= menu_items && state < menu_items + 3 + use_autocenter)
			{
				selected_in = in;
				selected_item = state - menu_items;
				// shift menu for missing Autocenter
				if (selected_item && !use_autocenter) selected_item++;
			}

			/* add the needed items for each analog input */
			analog_menu_add_item(&item_list[menu_items++], in, UI_keyjoyspeed, ANALOG_ITEM_KEYSPEED);
			if (use_autocenter)
				analog_menu_add_item(&item_list[menu_items++], in, UI_centerspeed, ANALOG_ITEM_CENTERSPEED);
			analog_menu_add_item(&item_list[menu_items++], in, UI_reverse, ANALOG_ITEM_REVERSE);
			analog_menu_add_item(&item_list[menu_items++], in, UI_sensitivity, ANALOG_ITEM_SENSITIVITY);
		}

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_menu_draw(item_list, menu_items, state, NULL);

	/* handle generic menu keys */
	if (ui_menu_generic_keys((int *) &state, menu_items))
		return state;

	/* handle left/right arrows */
	if (input_ui_pressed_repeat(IPT_UI_LEFT,6) && (item_list[state].flags & MENU_FLAG_LEFT_ARROW))
		delta = -1;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT,6) && (item_list[state].flags & MENU_FLAG_RIGHT_ARROW))
		delta = 1;
	if (code_pressed(KEYCODE_LSHIFT))
		delta *= 10;

	/* adjust the appropriate value */
	if (delta != 0 && selected_in)
		switch (selected_item)
		{
			case ANALOG_ITEM_KEYSPEED:		selected_in->analog.delta += delta;			break;
			case ANALOG_ITEM_CENTERSPEED:	selected_in->analog.centerdelta += delta;	break;
			case ANALOG_ITEM_REVERSE:		selected_in->analog.reverse += delta;		break;
			case ANALOG_ITEM_SENSITIVITY:	selected_in->analog.sensitivity += delta;	break;
		}

	return state;
}


/*-------------------------------------------------
    menu_joystick_calibrate - display a menu for
    calibrating analog joysticks
-------------------------------------------------*/

static UINT32 menu_joystick_calibrate(UINT32 state)
{
	const char *msg;

	/* two states */
	switch (state)
	{
		/* state 0 is just starting */
		case 0:
			osd_joystick_start_calibration();
			state++;
			break;

		/* state 1 is where we spend our time */
		case 1:

			/* get the message; if none, we're done */
			msg = osd_joystick_calibrate_next();
			if (msg == NULL)
			{
				osd_joystick_end_calibration();
				return ui_menu_stack_pop();
			}

			/* display the message */
			ui_draw_message_window(msg);

			/* handle cancel and select */
			if (input_ui_pressed(IPT_UI_CANCEL))
				return ui_menu_stack_pop();
			if (input_ui_pressed(IPT_UI_SELECT))
				osd_joystick_calibrate();
			break;
	}

	return state;
}


/*-------------------------------------------------
    menu_bookkeeping - display a "menu" for
    bookkeeping information
-------------------------------------------------*/

#ifndef MESS
static UINT32 menu_bookkeeping(UINT32 state)
{
	char buf[2048];
	char *bufptr = buf;
	UINT32 selected = 0;
	int ctrnum;
	mame_time total_time;

	/* show total time first */
	total_time = mame_timer_get_time();
	if (total_time.seconds >= 60 * 60)
		bufptr += sprintf(bufptr, "%s: %d:%02d:%02d\n\n", ui_getstring(UI_totaltime), total_time.seconds / (60*60), (total_time.seconds / 60) % 60, total_time.seconds % 60);
	else
		bufptr += sprintf(bufptr, "%s: %d:%02d\n\n", ui_getstring(UI_totaltime), (total_time.seconds / 60) % 60, total_time.seconds % 60);

	/* show tickets at the top */
	if (dispensed_tickets)
		bufptr += sprintf(bufptr, "%s: %d\n\n", ui_getstring(UI_tickets), dispensed_tickets);

	/* loop over coin counters */
	for (ctrnum = 0; ctrnum < COIN_COUNTERS; ctrnum++)
	{
		/* display the coin counter number */
		bufptr += sprintf(bufptr, "%s %c: ", ui_getstring(UI_coin), ctrnum + 'A');

		/* display how many coins */
		if (!coin_count[ctrnum])
			bufptr += sprintf(bufptr, "%s", ui_getstring(UI_NA));
		else
			bufptr += sprintf(bufptr, "%d", coin_count[ctrnum]);

		/* display whether or not we are locked out */
		if (coinlockedout[ctrnum])
			bufptr += sprintf(bufptr, " %s", ui_getstring(UI_locked));
		*bufptr++ = '\n';
	}

	/* make it look like a menu */
	bufptr += sprintf(bufptr, "\n\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_returntomain), ui_getstring(UI_righthilight));

	/* draw the text */
	ui_draw_message_window(buf);

	/* handle the keys */
	ui_menu_generic_keys(&selected, 1);
	return selected;
}
#endif


/*-------------------------------------------------
    menu_game_info - display a "menu" for
    game information
-------------------------------------------------*/

static UINT32 menu_game_info(UINT32 state)
{
	char buf[2048];
	char *bufptr = buf;
	UINT32 selected = 0;

	/* add the game info */
	bufptr += sprintf_game_info(bufptr);

	/* make it look like a menu */
	bufptr += sprintf(bufptr, "\n\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_returntomain), ui_getstring(UI_righthilight));

	/* draw the text */
	ui_draw_message_window(buf);

	/* handle the keys */
	ui_menu_generic_keys(&selected, 1);
	return selected;
}


/*-------------------------------------------------
    menu_cheat - display a menu for cheat options
-------------------------------------------------*/

static UINT32 menu_cheat(UINT32 state)
{
	int result = cheat_menu(state);
	if (result == 0)
		return ui_menu_stack_pop();
	return result;
}


/*-------------------------------------------------
    menu_memory_card - display a menu for memory
    card options
-------------------------------------------------*/

static UINT32 menu_memory_card(UINT32 state)
{
	ui_menu_item item_list[5];
	int menu_items = 0;
	int cardnum = state >> 16;
	UINT32 selected = state & 0xffff;
	int insertindex = -1, ejectindex = -1, createindex = -1;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* add the card select menu */
	item_list[menu_items].text = ui_getstring(UI_selectcard);
	item_list[menu_items].subtext = menu_string_pool_add("%d", cardnum);
	if (cardnum > 0)
		item_list[menu_items].flags |= MENU_FLAG_LEFT_ARROW;
	if (cardnum < 1000)
		item_list[menu_items].flags |= MENU_FLAG_RIGHT_ARROW;
	menu_items++;

	/* add the remaining items */
	item_list[insertindex = menu_items++].text = ui_getstring(UI_loadcard);
	if (memcard_present() != -1)
		item_list[ejectindex = menu_items++].text = ui_getstring(UI_ejectcard);
	item_list[createindex = menu_items++].text = ui_getstring(UI_createcard);

	/* add an item for the return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_menu_draw(item_list, menu_items, selected, NULL);

	/* handle the keys */
	if (ui_menu_generic_keys(&selected, menu_items))
		return selected;
	if (selected == 0 && input_ui_pressed(IPT_UI_LEFT) && cardnum > 0)
		cardnum--;
	if (selected == 0 && input_ui_pressed(IPT_UI_RIGHT) && cardnum < 1000)
		cardnum++;

	/* handle actions */
	if (input_ui_pressed(IPT_UI_SELECT))
	{
		/* handle load */
		if (selected == insertindex)
		{
			if (memcard_insert(cardnum) == 0)
			{
				popmessage("%s", ui_getstring(UI_loadok));
				ui_menu_stack_reset();
				return 0;
			}
			else
				popmessage("%s", ui_getstring(UI_loadfailed));
		}

		/* handle eject */
		else if (selected == ejectindex)
		{
			memcard_eject(Machine);
			popmessage("%s", ui_getstring(UI_cardejected));
		}

		/* handle create */
		else if (selected == createindex)
		{
			if (memcard_create(cardnum, FALSE) == 0)
				popmessage("%s", ui_getstring(UI_cardcreated));
			else
				popmessage("%s\n%s", ui_getstring(UI_cardcreatedfailed), ui_getstring(UI_cardcreatedfailed2));
		}
	}

	return selected | (cardnum << 16);
}


/*-------------------------------------------------
    menu_video - display a menu for video options
-------------------------------------------------*/

static UINT32 menu_video(UINT32 state)
{
	ui_menu_item item_list[100];
	render_target *targetlist[16];
	int curtarget = state >> 16;
	UINT32 selected = state & 0xffff;
	int menu_items = 0;
	int targets;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* find the targets */
	for (targets = 0; targets < ARRAY_LENGTH(targetlist); targets++)
	{
		targetlist[targets] = render_target_get_indexed(targets);
		if (targetlist[targets] == NULL)
			break;
	}

	/* if we have a current target of 1000, we may need to select from multiple targets */
	if (curtarget == 1000)
	{
		/* count up the targets, creating menu items for them */
		for (; menu_items < targets; menu_items++)
			item_list[menu_items].text = menu_string_pool_add("%s%d", ui_getstring(UI_screen), menu_items);

		/* if we only ended up with one, auto-select it */
		if (menu_items == 1)
			return menu_video(0 << 16 | render_target_get_view(render_target_get_indexed(0)));

		/* add an item for moving the UI */
		item_list[menu_items++].text = "Move User Interface";

		/* add an item to return */
		item_list[menu_items++].text = ui_getstring(UI_returntomain);

		/* draw the menu */
		ui_menu_draw(item_list, menu_items, selected, NULL);

		/* handle the keys */
		if (ui_menu_generic_keys(&selected, menu_items))
			return selected;

		/* handle actions */
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			if (selected == menu_items - 2)
			{
				render_target *uitarget = render_get_ui_target();
				int targnum;

				for (targnum = 0; targnum < targets; targnum++)
					if (targetlist[targnum] == uitarget)
						break;
				targnum = (targnum + 1) % targets;
				render_set_ui_target(targetlist[targnum]);
			}
			else
				return ui_menu_stack_push(menu_video, (selected << 16) | render_target_get_view(render_target_get_indexed(selected)));
		}
	}

	/* otherwise, draw the list of layouts */
	else
	{
		render_target *target = targetlist[curtarget];
		int layermask;
		assert(target != NULL);

		/* add all the views */
		for ( ; menu_items < ARRAY_LENGTH(item_list); menu_items++)
		{
			const char *name = render_target_get_view_name(target, menu_items);
			if (name == NULL)
				break;

			/* create a string for the item */
			item_list[menu_items].text = name;
		}

		/* add an item to rotate */
		item_list[menu_items++].text = "Rotate View";

		/* add an item to enable/disable backdrops */
		layermask = render_target_get_layer_config(target);
		if (layermask & LAYER_CONFIG_ENABLE_BACKDROP)
			item_list[menu_items++].text = "Hide Backdrops";
		else
			item_list[menu_items++].text = "Show Backdrops";

		/* add an item to enable/disable overlays */
		if (layermask & LAYER_CONFIG_ENABLE_OVERLAY)
			item_list[menu_items++].text = "Hide Overlays";
		else
			item_list[menu_items++].text = "Show Overlays";

		/* add an item to enable/disable bezels */
		if (layermask & LAYER_CONFIG_ENABLE_BEZEL)
			item_list[menu_items++].text = "Hide Bezels";
		else
			item_list[menu_items++].text = "Show Bezels";

		/* add an item to enable/disable cropping */
		if (layermask & LAYER_CONFIG_ZOOM_TO_SCREEN)
			item_list[menu_items++].text = "Show Full Artwork";
		else
			item_list[menu_items++].text = "Crop to Screen";

		/* add an item to return */
		item_list[menu_items++].text = ui_getstring(UI_returntoprior);

		/* draw the menu */
		ui_menu_draw(item_list, menu_items, selected, NULL);

		/* handle the keys */
		if (ui_menu_generic_keys(&selected, menu_items))
			return selected;

		/* handle actions */
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			/* rotate */
			if (selected == menu_items - 6)
			{
				render_target_set_orientation(target, orientation_add(ROT90, render_target_get_orientation(target)));
				if (target == render_get_ui_target())
					render_container_set_orientation(render_container_get_ui(), orientation_add(ROT270, render_container_get_orientation(render_container_get_ui())));
			}

			/* show/hide backdrops */
			else if (selected == menu_items - 5)
			{
				layermask ^= LAYER_CONFIG_ENABLE_BACKDROP;
				render_target_set_layer_config(target, layermask);
			}

			/* show/hide overlays */
			else if (selected == menu_items - 4)
			{
				layermask ^= LAYER_CONFIG_ENABLE_OVERLAY;
				render_target_set_layer_config(target, layermask);
			}

			/* show/hide bezels */
			else if (selected == menu_items - 3)
			{
				layermask ^= LAYER_CONFIG_ENABLE_BEZEL;
				render_target_set_layer_config(target, layermask);
			}

			/* crop/uncrop artwork */
			else if (selected == menu_items - 2)
			{
				layermask ^= LAYER_CONFIG_ZOOM_TO_SCREEN;
				render_target_set_layer_config(target, layermask);
			}

			/* else just set the selected view */
			else
				render_target_set_view(target, selected);
		}
	}

	return selected | (curtarget << 16);
}


/*-------------------------------------------------
    menu_reset_game - handle the "menu" for
    resetting the game
-------------------------------------------------*/

static UINT32 menu_reset_game(UINT32 state)
{
	/* request a reset */
	mame_schedule_soft_reset(Machine);

	/* reset the menu stack */
	ui_menu_stack_reset();
	return 0;
}


/*-------------------------------------------------
    menu_file_manager - MESS-specific menu
-------------------------------------------------*/

#ifdef MESS
static UINT32 menu_file_manager(UINT32 state)
{
	int result = filemanager(state);
	if (result == 0)
	return ui_menu_stack_pop();
	return result;
}
#endif


/*-------------------------------------------------
    menu_tape_control - MESS-specific menu
-------------------------------------------------*/

#ifdef MESS
static UINT32 menu_tape_control(UINT32 state)
{
	int result = tapecontrol(state);
	if (result == 0)
	return ui_menu_stack_pop();
	return result;
}
#endif



/***************************************************************************
    MENU HELPERS
***************************************************************************/

/*-------------------------------------------------
    input_menu_get_items - build a list of items
    for a given group of inputs
-------------------------------------------------*/

static int input_menu_get_items(input_item_data *itemlist, int group)
{
	input_item_data *item = itemlist;
	const input_port_default_entry *indef;
	input_port_default_entry *in;
	char temp[100];

	/* an out of range group is special; it just means the game-specific inputs */
	if (group > IPG_TOTAL_GROUPS)
		return input_menu_get_game_items(itemlist);

	/* iterate over the input ports and add menu items */
	for (in = get_input_port_list(), indef = get_input_port_list_defaults(); in->type != IPT_END; in++, indef++)

		/* add if we match the group and we have a valid name */
		if (in->group == group && in->name && in->name[0] != 0)
		{
			/* build an entry for the standard sequence */
			item->seq = &in->defaultseq;
			item->defseq = &indef->defaultseq;
			item->sortorder = item - itemlist;
			item->type = port_type_is_analog(in->type) ? INPUT_TYPE_ANALOG : INPUT_TYPE_DIGITAL;
			item->name = menu_string_pool_add(input_format[item->type], in->name);
			item->seqname = menu_string_pool_add("%s", seq_name(item->seq, temp, sizeof(temp)));
			item->invert = seq_cmp(item->seq, item->defseq);
			item++;

			/* if we're analog, add more entries */
			if (item[-1].type == INPUT_TYPE_ANALOG)
			{
				/* build an entry for the decrement sequence */
				item->seq = &in->defaultdecseq;
				item->defseq = &indef->defaultdecseq;
				item->sortorder = item - itemlist;
				item->type = INPUT_TYPE_ANALOG_DEC;
				item->name = menu_string_pool_add(input_format[item->type], in->name);
				item->seqname = menu_string_pool_add("%s", seq_name(item->seq, temp, sizeof(temp)));
				item->invert = seq_cmp(item->seq, item->defseq);
				item++;

				/* build an entry for the increment sequence */
				item->seq = &in->defaultincseq;
				item->defseq = &indef->defaultincseq;
				item->sortorder = item - itemlist;
				item->type = INPUT_TYPE_ANALOG_INC;
				item->name = menu_string_pool_add(input_format[item->type], in->name);
				item->seqname = menu_string_pool_add("%s", seq_name(item->seq, temp, sizeof(temp)));
				item->invert = seq_cmp(item->seq, item->defseq);
				item++;
			}
		}

	/* return the number of items */
	return item - itemlist;
}


/*-------------------------------------------------
    input_menu_get_game_items - build a list of
    items for the game-specific inputs
-------------------------------------------------*/

static int input_menu_get_game_items(input_item_data *itemlist)
{
	static const input_seq default_seq = SEQ_DEF_1(CODE_DEFAULT);
	input_item_data *item = itemlist;
	input_port_entry *in;
	char temp[100];

	/* iterate over the input ports and add menu items */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		const char *name = input_port_name(in);

		/* add if we match the group and we have a valid name */
		if (name != NULL &&
#ifdef MESS
			(in->category == 0 || input_category_active(in->category)) &&
#endif /* MESS */
			((in->type == IPT_OTHER && in->name != IP_NAME_DEFAULT) || port_type_to_group(in->type, in->player) != IPG_INVALID))
		{
			UINT16 sortorder;
			input_seq *curseq, *defseq;

			/* determine the sorting order */
			if (in->type >= IPT_START1 && in->type <= __ipt_analog_end)
				sortorder = (in->type << 2) | (in->player << 12);
			else
				sortorder = in->type | 0xf000;

			/* fetch data for the standard sequence */
			curseq = input_port_seq(in, SEQ_TYPE_STANDARD);
			defseq = input_port_default_seq(in->type, in->player, SEQ_TYPE_STANDARD);

			/* build an entry for the standard sequence */
			item->seq = &in->seq;
			item->defseq = &default_seq;
			item->sortorder = sortorder;
			item->type = port_type_is_analog(in->type) ? INPUT_TYPE_ANALOG : INPUT_TYPE_DIGITAL;
			item->name = menu_string_pool_add(input_format[item->type], name);
			item->seqname = menu_string_pool_add("%s", seq_name(curseq, temp, sizeof(temp)));
			item->invert = seq_cmp(curseq, defseq);
			item++;

			/* if we're analog, add more entries */
			if (item[-1].type == INPUT_TYPE_ANALOG)
			{
				/* fetch data for the decrement sequence */
				curseq = input_port_seq(in, SEQ_TYPE_DECREMENT);
				defseq = input_port_default_seq(in->type, in->player, SEQ_TYPE_DECREMENT);

				/* build an entry for the decrement sequence */
				item->seq = &in->analog.decseq;
				item->defseq = &default_seq;
				item->sortorder = sortorder;
				item->type = INPUT_TYPE_ANALOG_DEC;
				item->name = menu_string_pool_add(input_format[item->type], name);
				item->seqname = menu_string_pool_add("%s", seq_name(curseq, temp, sizeof(temp)));
				item->invert = seq_cmp(curseq, defseq);
				item++;

				/* fetch data for the increment sequence */
				curseq = input_port_seq(in, SEQ_TYPE_INCREMENT);
				defseq = input_port_default_seq(in->type, in->player, SEQ_TYPE_INCREMENT);

				/* build an entry for the increment sequence */
				item->seq = &in->analog.incseq;
				item->defseq = &default_seq;
				item->sortorder = sortorder;
				item->type = INPUT_TYPE_ANALOG_INC;
				item->name = menu_string_pool_add(input_format[item->type], name);
				item->seqname = menu_string_pool_add("%s", seq_name(curseq, temp, sizeof(temp)));
				item->invert = seq_cmp(curseq, defseq);
				item++;
			}
		}
	}

	/* return the number of items */
	return item - itemlist;
}


/*-------------------------------------------------
    input_menu_toggle_none_default - toggle
    between "NONE" and the default item
-------------------------------------------------*/

static void input_menu_toggle_none_default(input_seq *selected_seq, input_seq *original_seq, const input_seq *selected_defseq)
{
	/* if we used to be "none", toggle to the default value */
	if (seq_get_1(original_seq) == CODE_NONE)
		seq_copy(selected_seq, selected_defseq);

	/* otherwise, toggle to "none" */
	else
		seq_set_1(selected_seq, CODE_NONE);
}


/*-------------------------------------------------
    input_menu_compare_items - compare two
    items for quicksort
-------------------------------------------------*/

static int input_menu_compare_items(const void *i1, const void *i2)
{
	const ui_menu_item *item1 = i1;
	const ui_menu_item *item2 = i2;
	const input_item_data *data1 = item1->ref;
	const input_item_data *data2 = item2->ref;
	if (data1->sortorder < data2->sortorder)
		return -1;
	if (data1->sortorder > data2->sortorder)
		return 1;
	if (data1->type < data2->type)
		return -1;
	if (data1->type > data2->type)
		return 1;
	return 0;
}


/*-------------------------------------------------
    switches_menu_add_item - add an item to the
    switches menu list
-------------------------------------------------*/

static void switches_menu_add_item(ui_menu_item *item, const input_port_entry *in, int switch_entry, void *ref)
{
	const input_port_entry *tin;

	/* set the text to the name and the subitem text to invalid */
	item->text = input_port_name(in);
	item->subtext = NULL;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if this is a match, set the subtext */
			if (in->default_value == tin->default_value)
				item->subtext = input_port_name(tin);

			/* else if we haven't seen a match yet, show a left arrow */
			else if (!item->subtext)
				item->flags |= MENU_FLAG_LEFT_ARROW;

			/* else if we have seen a match, show a right arrow */
			else
				item->flags |= MENU_FLAG_RIGHT_ARROW;
		}

	/* if no matches, we're invalid */
	if (!item->subtext)
		item->subtext = ui_getstring(UI_INVALID);

	/* stash our reference */
	item->ref = ref;
}


/*-------------------------------------------------
    switches_menu_select_previous - select the
    previous item in the switches list
-------------------------------------------------*/

static void switches_menu_select_previous(input_port_entry *in, int switch_entry)
{
	int last_value = in->default_value;
	const input_port_entry *tin;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if this is a match, we're done */
			if (in->default_value == tin->default_value)
			{
				in->default_value = last_value;
				return;
			}

			/* otherwise, keep track of last one found */
			else
				last_value = tin->default_value;
		}
}


/*-------------------------------------------------
    switches_menu_select_next - select the
    next item in the switches list
-------------------------------------------------*/

static void switches_menu_select_next(input_port_entry *in, int switch_entry)
{
	const input_port_entry *tin;
	int foundit = FALSE;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if we found the current selection, we pick the next one */
			if (foundit)
			{
				in->default_value = tin->default_value;
				return;
			}

			/* if this is a match, note it */
			if (in->default_value == tin->default_value)
				foundit = TRUE;
		}
}


/*-------------------------------------------------
    switches_menu_compare_items - compare two
    switches items for quicksort purposes
-------------------------------------------------*/

/*
static int switches_menu_compare_items(const void *i1, const void *i2)
{
    const ui_menu_item *item1 = i1;
    const ui_menu_item *item2 = i2;
    const input_port_entry *data1 = item1->ref;
    const input_port_entry *data2 = item2->ref;
    return strcmp(input_port_name(data1), input_port_name(data2));
}
*/


/*-------------------------------------------------
    analog_menu_add_item - add an item to the
    analog controls menu
-------------------------------------------------*/

static void analog_menu_add_item(ui_menu_item *item, const input_port_entry *in, int append_string, int which_item)
{
	int value, minval, maxval;

	/* set the item text using the formatting string provided */
	item->text = menu_string_pool_add("%s %s", input_port_name(in), ui_getstring(append_string));

	/* set the subitem text */
	switch (which_item)
	{
		default:
		case ANALOG_ITEM_KEYSPEED:
			value = in->analog.delta;
			minval = 0;
			maxval = 255;
			item->subtext = menu_string_pool_add("%d", value);
			break;

		case ANALOG_ITEM_CENTERSPEED:
			value = in->analog.centerdelta;
			minval = 0;
			maxval = 255;
			item->subtext = menu_string_pool_add("%d", value);
			break;

		case ANALOG_ITEM_REVERSE:
			value = in->analog.reverse;
			minval = 0;
			maxval = 1;
			item->subtext = value ? ui_getstring(UI_on) : ui_getstring(UI_off);
			break;

		case ANALOG_ITEM_SENSITIVITY:
			value = in->analog.sensitivity;
			minval = 1;
			maxval = 255;
			item->subtext = menu_string_pool_add("%d%%", value);
			break;
	}

	/* put on arrows */
	if (value > minval)
		item->flags |= MENU_FLAG_LEFT_ARROW;
	if (value < maxval)
		item->flags |= MENU_FLAG_RIGHT_ARROW;
}


/*-------------------------------------------------
    dip_switch_build_model - build up the model
    for DIP switches
-------------------------------------------------*/

static void dip_switch_build_model(input_port_entry *entry, int item_is_selected)
{
	int dip_declaration_index = 0;
	int value_mask_temp = entry->mask;
	int value_mask_bit = 0;
	int toggle_switch_mask;
	int model_index = 0;
	int toggle_num;

	if (entry->diploc[dip_declaration_index].swname == NULL)
		return;

	/* get the entry in the model to work with */
	do
	{
		/* use this entry if it's not used */
		if (dip_switch_model[model_index].dip_name == NULL)
		{
			dip_switch_model[model_index].dip_name = entry->diploc[dip_declaration_index].swname;
			break;
		}

		/* reuse this entry if the switch name matches */
		if (!strcmp(entry->diploc[dip_declaration_index].swname, dip_switch_model[model_index].dip_name))
			break;

		// todo: add a check here to see if we go over the max dips and throw an error.
	} while (++model_index < MAX_PHYSICAL_DIPS);

	/* Create a mask depicting the number of toggles on the physical switch */
	while (entry->diploc[dip_declaration_index].swname)
	{
		/* get the Nth DIP toggle number */
		toggle_num = entry->diploc[dip_declaration_index].swnum;

		/* get the Nth mask bit -
         * should probably put a check here to avoid bad driver definitions
         * which could put us into an infinite loop. */
		while (!(value_mask_temp & 1))
		{
			value_mask_temp >>= 1;
			++value_mask_bit;
		}
		/* clear out the lsb to keep it moving next iteration. */
		value_mask_temp &= ~1;

		toggle_switch_mask = 1 << (toggle_num - 1);

		/* indicate the toggle exists in the switch */
		dip_switch_model[model_index].total_dip_mask |= toggle_switch_mask;

		/* if issolated bit is on, set the toggle on */
		if ((1 << value_mask_bit) & entry->default_value)
			dip_switch_model[model_index].total_dip_settings |= toggle_switch_mask;

		/* indicate if the toggle is selected */
		if (item_is_selected)
			dip_switch_model[model_index].selected_dip_feature_mask |= toggle_switch_mask;

		++dip_declaration_index;
	}
}


/*-------------------------------------------------
    dip_switch_draw_one - draw a single DIP
    switch
-------------------------------------------------*/

static void dip_switch_draw_one(float dip_menu_x1, float dip_menu_y1, float dip_menu_x2, float dip_menu_y2, int model_index)
{
	int toggle_index;
	int num_toggles = 0;
	float segment_start_x;
	float dip_field_y;
	float y1_on, y2_on, y1_off, y2_off;
	float switch_toggle_gap;
	float name_width;

	/* determine the number of toggles in the DIP */
	for (toggle_index = 0; toggle_index < 16; toggle_index++)
		if (dip_switch_model[model_index].total_dip_mask & (1 << toggle_index))
			num_toggles = toggle_index + 1;

	/* calculate the starting x coordinate so that the entire switch is centered */
	dip_field_y = dip_menu_y1 + DIP_SWITCH_SPACING + (model_index * (DIP_SWITCH_SPACING + DIP_SWITCH_HEIGHT));

	switch_toggle_gap = ((DIP_SWITCH_HEIGHT/2) - SINGLE_TOGGLE_SWITCH_HEIGHT)/2;

 	segment_start_x = dip_menu_x1 + (dip_menu_x2 - dip_menu_x1 - num_toggles * SINGLE_TOGGLE_SWITCH_FIELD_WIDTH) / 2;

	y1_off = dip_field_y + UI_LINE_WIDTH + switch_toggle_gap;
	y2_off = y1_off + SINGLE_TOGGLE_SWITCH_HEIGHT;

	y1_on = dip_field_y + DIP_SWITCH_HEIGHT/2 + switch_toggle_gap;
	y2_on = y1_on + SINGLE_TOGGLE_SWITCH_HEIGHT;

	for (toggle_index = 0; toggle_index < num_toggles; toggle_index++)
	{
		int bit_mask, dip_on;
		float dip_field_x1, x1, x2;

		bit_mask = 1 << toggle_index;

		/* draw the field for a single toggle on a DIP switch */
		dip_field_x1 = segment_start_x + (SINGLE_TOGGLE_SWITCH_FIELD_WIDTH * toggle_index);

		ui_draw_outlined_box(dip_field_x1,
							 dip_field_y,
							 dip_field_x1 + SINGLE_TOGGLE_SWITCH_FIELD_WIDTH,
							 dip_field_y + DIP_SWITCH_HEIGHT,
							 UI_FILLCOLOR);

		x1 = dip_field_x1 + (SINGLE_TOGGLE_SWITCH_FIELD_WIDTH - SINGLE_TOGGLE_SWITCH_WIDTH) / 2;
		x2 = x1 + SINGLE_TOGGLE_SWITCH_WIDTH;

		/* see if the switch is actually used */
		if (dip_switch_model[model_index].total_dip_mask & bit_mask)
		{
			/* yes, draw the switch position for a single toggle switch in a switch field */
			int feature_field_selected = ((bit_mask & dip_switch_model[model_index].selected_dip_feature_mask) != 0);
			dip_on = ((bit_mask & dip_switch_model[model_index].total_dip_settings) != 0);

			render_ui_add_rect(x1, dip_on ? y1_on : y1_off,
							   x2, dip_on ? y2_on : y2_off,
							   feature_field_selected ? MENU_SELECTCOLOR : ARGB_WHITE,
							   PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		}
		else
		{
			/* no, draw it grayed out */
			render_ui_add_rect(x1, y1_off,
							   x2, y2_on,
							   MENU_UNAVAILABLECOLOR,
							   PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		}
	}

	/* add the dip switch name */
	name_width = ui_get_string_width(dip_switch_model[model_index].dip_name) + ui_get_string_width(" ") / 2;

	ui_draw_text_full(	dip_switch_model[model_index].dip_name,
						segment_start_x - name_width,
						dip_field_y + (DIP_SWITCH_HEIGHT - UI_TARGET_FONT_HEIGHT)/2,
						name_width,
						JUSTIFY_LEFT,
						WRAP_NEVER,
						DRAW_NORMAL,
						ARGB_WHITE,
						PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA),
						NULL ,
						NULL);
}


/*-------------------------------------------------
    dip_switch_get_extra_height - return the
    extra menu height needed for DIP switches
-------------------------------------------------*/

static float dip_switch_get_extra_height(void)
{
	float num_dips = get_num_dips();
	return (num_dips * (DIP_SWITCH_HEIGHT + DIP_SWITCH_SPACING)) + DIP_SWITCH_SPACING;
}


/*-------------------------------------------------
    dip_switch_get_extra_width - return the
    total menu width needed for DIP switches
-------------------------------------------------*/

/* this is currently not implemented but is left here
 * for a enhancement to make sure that the allocated width in the main menu
 * is sufficient to draw the DIPs.  The current thought is that if the
 * space isn't sufficient, then don't draw the DIPs at all. */
float dip_switch_get_extra_width(void)
{
	return 0.0f;
}


/*-------------------------------------------------
    dip_switch_augment_menu - perform our
    special rendering
-------------------------------------------------*/

static void dip_switch_augment_menu(float x1, float y1, float x2, float y2)
{
	int num_dips;
	int dip_model_index;
	float dip_menu_y1, dip_menu_y2;

	/* how many dips does this game have? */
	num_dips = get_num_dips();

	dip_menu_y1 = y2 + SPACE_BETWEEN_BOXES;
	dip_menu_y2 = dip_menu_y1 + dip_switch_get_extra_height();

	/* draw extra menu area */
	ui_draw_outlined_box(	x1,
							dip_menu_y1,
							x2,
							dip_menu_y2,
							UI_FILLCOLOR);

	/* draw all the dip switches */
	for (dip_model_index = 0; dip_model_index < num_dips; dip_model_index++)
		dip_switch_draw_one(x1, dip_menu_y1, x2, dip_menu_y2, dip_model_index);
}
