int lisa_vh_start(void);
void lisa_vh_stop(void);
void lisa_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

int lisa_floppy_init(int id);
void lisa_floppy_exit(int id);
extern void lisa_nvram_handler(void *file, int read_or_write);

void init_lisa2(void);
void init_lisa210(void);
void init_mac_xl(void);

void lisa_init_machine(void);
void lisa_exit_machine(void);

int lisa_interrupt(void);

READ_HANDLER ( lisa_fdc_io_r );
WRITE_HANDLER ( lisa_fdc_io_w );
READ_HANDLER ( lisa_fdc_r );
READ_HANDLER ( lisa210_fdc_r );
WRITE_HANDLER ( lisa_fdc_w );
WRITE_HANDLER ( lisa210_fdc_w );
READ16_HANDLER ( lisa_r );
WRITE16_HANDLER ( lisa_w );


