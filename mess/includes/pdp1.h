/* should go into memory.h */

#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

#define MEMORY_READ_START18(name) const struct Memory_ReadAddress32 name[] = { \
	{ MEMORY_MARKER, MEMORY_DIRECTION_READ | MEMORY_TYPE_MEM | MEMORY_WIDTH_32 },
#define MEMORY_WRITE_START18(name) const struct Memory_WriteAddress32 name[] = { \
	{ MEMORY_MARKER, MEMORY_DIRECTION_WRITE | MEMORY_TYPE_MEM | MEMORY_WIDTH_32 },

/* From machine/pdp1.c */
int pdp1_load_rom (int id);
int pdp1_id_rom (int id);
void pdp1_init_machine(void);
READ18_HANDLER ( pdp1_read_mem );
WRITE18_HANDLER ( pdp1_write_mem );

int pdp1_iot(int *io, int md);
int pdp1_load_rom (int id);
int pdp1_id_rom (int id);
void pdp1_plot(int x, int y);

extern int *pdp1_memory;
