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
	pdp1_typewriter = 6				/* typewriter keys */
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

/* defines for our font */
enum
{
	pdp1_charnum = /*96*/128,	/* ASCII set + ??? special characters */
									/* for whatever reason, 96 breaks some characters */

	pdp1_fontdata_size = 8 * pdp1_charnum
};


/* From machine/pdp1.c */
extern int *pdp1_memory;

extern int pdp1_ta;
extern int pdp1_tw;

void init_pdp1(void);
void pdp1_init_machine(void);
READ18_HANDLER ( pdp1_read_mem );
WRITE18_HANDLER ( pdp1_write_mem );

int pdp1_tape_init(int id);
void pdp1_tape_exit(int id);

int pdp1_tape_read_binary(UINT32 *reply);

int pdp1_teletyper_init(int id);
void pdp1_teletyper_exit(int id);

int pdp1_iot(int *io, int md);

int pdp1_keyboard(void);
int pdp1_interrupt(void);
int pdp1_get_test_word(void);


/* From vidhrdw/pdp1.c */
void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh);
void pdp1_vh_stop(void);
int pdp1_vh_start(void);

void pdp1_plot(int x, int y);
void pdp1_screen_update(void);

enum
{
	/* size of crt view */
	crt_window_width = 512,
	crt_window_height = 512,
	/* size of programmer panel view */
	panel_window_width = 384,
	panel_window_height = 128
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
#define VIDEO_MAX_INTENSITY 15

#define BLACK 0
#define WHITE 15
/*#define DK_GRAY 8*/
#define LT_GRAY 12
#define GREEN 16
#define DK_GREEN 17
#define PANEL_BG BLACK
#define PANEL_TEXT WHITE
#define PANEL_TEXT_IX 0
#define SWITCH_BK LT_GRAY
#define SWITCH_BUTTON WHITE
#define LIT_LAMP GREEN
#define UNLIT_LAMP DK_GREEN
