/* should go into memory.h */

#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

#define MEMORY_READ_START18(name) MEMORY_READ32_START(name)
#define MEMORY_WRITE_START18(name) MEMORY_WRITE32_START(name)


/* From systems/pdp1.c */
/* defines for input port numbers */
enum
{
	pdp1_spacewar_controllers = 0,
	pdp1_control_switches = 1,		/* various operator control panel switches */
	pdp1_sense_switches = 2,		/* sense switches */
	pdp1_ta_switches = 3,			/* test address switches */
	pdp1_tw_switches_MSB = 4,		/* test word switches */
	pdp1_tw_switches_LSB = 5,
	pdp1_typewriter = 6,			/* typewriter keys */
	pdp1_config = 10				/* pseudo input port with config */
};

/* defines for each bit and mask in input port pdp1_control_switches */
enum
{
	/* bit numbers */
	pdp1_control_bit = 0,

	pdp1_extend_bit		= 1,
	pdp1_start_nobrk_bit= 2,
	pdp1_start_brk_bit	= 3,
	pdp1_stop_bit		= 4,
	pdp1_continue_bit	= 5,
	pdp1_examine_bit	= 6,
	pdp1_deposit_bit	= 7,
	pdp1_read_in_bit	= 8,
	pdp1_reader_bit		= 9,
	pdp1_tape_feed_bit	= 10,
	pdp1_single_step_bit= 11,
	pdp1_single_inst_bit= 12,

	/* masks */
	pdp1_control = (1 << pdp1_control_bit),
	pdp1_extend = (1 << pdp1_extend_bit),
	pdp1_start_nobrk = (1 << pdp1_start_nobrk_bit),
	pdp1_start_brk = (1 << pdp1_start_brk_bit),
	pdp1_stop = (1 << pdp1_stop_bit),
	pdp1_continue = (1 << pdp1_continue_bit),
	pdp1_examine = (1 << pdp1_examine_bit),
	pdp1_deposit = (1 << pdp1_deposit_bit),
	pdp1_read_in = (1 << pdp1_read_in_bit),
	pdp1_reader = (1 << pdp1_reader_bit),
	pdp1_tape_feed = (1 << pdp1_tape_feed_bit),
	pdp1_single_step = (1 << pdp1_single_step_bit),
	pdp1_single_inst = (1 << pdp1_single_inst_bit)
};

/* defines for each bit in input port pdp1_spacewar_controllers*/
#define ROTATE_LEFT_PLAYER1       0x01
#define ROTATE_RIGHT_PLAYER1      0x02
#define THRUST_PLAYER1            0x04
#define FIRE_PLAYER1              0x08
#define ROTATE_LEFT_PLAYER2       0x10
#define ROTATE_RIGHT_PLAYER2      0x20
#define THRUST_PLAYER2            0x40
#define FIRE_PLAYER2              0x80


/* defines for our font */
enum
{
	pdp1_charnum = /*96*/128,	/* ASCII set + ??? special characters */
									/* for whatever reason, 96 breaks some characters */

	pdp1_fontdata_size = 8 * pdp1_charnum
};

extern pdp1_reset_param_t pdp1_reset_param;


/* From machine/pdp1.c */
extern int *pdp1_memory;

void init_pdp1(void);
void pdp1_init_machine(void);
READ18_HANDLER ( pdp1_read_mem );
WRITE18_HANDLER ( pdp1_write_mem );

int pdp1_tape_init(int id);
void pdp1_tape_exit(int id);

void pdp1_tape_read_binary(void);

int pdp1_typewriter_init(int id);
void pdp1_typewriter_exit(int id);

void pdp1_io_sc_callback(void);
void pdp1_iot(int *io, int nac, int mb);

int pdp1_interrupt(void);


/* From vidhrdw/pdp1.c */
void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh);
void pdp1_vh_stop(void);
int pdp1_vh_start(void);

void pdp1_plot(int x, int y);
void pdp1_screen_update(void);

enum
{
	/* size and position of crt view */
	crt_window_width = 512,
	crt_window_height = 512,
	crt_window_offset_x = 0,
	crt_window_offset_y = 0,
	/* size and position of programmer panel view */
	panel_window_width = 384,
	panel_window_height = 128,
	panel_window_offset_x = crt_window_width,
	panel_window_offset_y = 0
};

enum
{
	total_width = crt_window_width + panel_window_width,
	total_height = crt_window_height,

	/* respect 4:3 aspect ratio */
	virtual_width_1 = ((total_width+3)/4)*4,
	virtual_height_1 = ((total_height+2)/3)*3,
	virtual_width_2 = virtual_height_1*4/3,
	virtual_height_2 = virtual_width_1*3/4,
	virtual_width = (virtual_width_1 > virtual_width_2) ? virtual_width_1 : virtual_width_2,
	virtual_height = (virtual_height_1 > virtual_height_2) ? virtual_height_1 : virtual_height_2
};

/* Color codes */
enum
{
	pen_crt_max_intensity = 15,

	pen_black = 0,
	pen_white = 15,
	/*pen_dk_gray = 8,*/
	pen_lt_gray = 12,
	pen_green = 16,
	pen_dk_green = 17,

	pen_panel_bg = pen_black,
	pen_panel_caption = pen_white,
	color_panel_caption = 0,
	pen_switch_nut = pen_lt_gray,
	pen_switch_button = pen_white,
	pen_lit_lamp = pen_green,
	pen_unlit_lamp = pen_dk_green
};
