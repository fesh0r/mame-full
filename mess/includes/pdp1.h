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
	pdp1_sense_switches = 1,
	pdp1_control_switches = 2,
	pdp1_ta_switches = 3,
	pdp1_tw_switches_MSB = 4,
	pdp1_tw_switches_LSB = 5,
	pdp1_typewriter = 6
};

/* defines for each bit and mask in input port pdp1_control_switches */
enum
{
	/* bit numbers */
	pdp1_read_in_bit = 0,
	pdp1_control_bit = 15,

	/* masks */
	pdp1_read_in = (1 << pdp1_read_in_bit),
	pdp1_control = (1 << pdp1_control_bit)
};


/* From machine/pdp1.c */
extern int *pdp1_memory;

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

#define VIDEO_BITMAP_WIDTH  512
#define VIDEO_BITMAP_HEIGHT 512
#define VIDEO_MAX_INTENSITY 15
