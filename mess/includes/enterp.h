void enterprise_init_machine(void);
void enterprise_shutdown_machine(void);
int enterprise_floppy_init(int);
int enterprise_rom_load(int);
int enterprise_rom_id(int);

void Enterprise_Initialise(void);

int enterprise_vh_start(void);
void enterprise_vh_stop(void);
void enterprise_vh_screenrefresh(struct mame_bitmap *bitmap, int fullupdate);


