/* should go into memory.h */

#define READ18_HANDLER(name) READ32_HANDLER(name)
#define WRITE18_HANDLER(name) WRITE32_HANDLER(name)

#define MEMORY_READ_START18(name) MEMORY_READ32_START(name)
#define MEMORY_WRITE_START18(name) MEMORY_WRITE32_START(name)

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
