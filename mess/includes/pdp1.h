/* should go into memory.h */

#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

#define MEMORY_READ_START18(name) MEMORY_READ32_START(name)
#define MEMORY_WRITE_START18(name) MEMORY_WRITE32_START(name)

/* From machine/pdp1.c */
void init_pdp1(void);
void pdp1_init_machine(void);
READ18_HANDLER ( pdp1_read_mem );
WRITE18_HANDLER ( pdp1_write_mem );

int pdp1_tape_init(int id);
void pdp1_tape_exit(int id);

int pdp1_tape_read_binary(UINT32 *reply);

int pdp1_iot(int *io, int md);

int pdp1_keyboard(void);

extern int *pdp1_memory;

/* From vidhrdw/pdp1.c */
void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh);
void pdp1_vh_stop(void);
int pdp1_vh_start(void);

void pdp1_plot(int x, int y);

#define VIDEO_BITMAP_WIDTH  512
#define VIDEO_BITMAP_HEIGHT 512
