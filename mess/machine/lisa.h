int lisa_vh_start(void);
void lisa_vh_stop(void);
void lisa_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

int lisa_floppy_init(int id);
void lisa_floppy_exit(int id);

void lisa_init_machine(void);

int lisa_interrupt(void);

READ_HANDLER ( lisa_fdc_io_r );
WRITE_HANDLER ( lisa_fdc_io_w );
READ_HANDLER ( lisa_fdc_r );
WRITE_HANDLER ( lisa_fdc_w );
READ_HANDLER ( lisa_r );
WRITE_HANDLER ( lisa_w );


