extern MACHINE_INIT( enterprise );
int enterprise_floppy_init(int, void *fp, int open_mode);
int enterprise_rom_load(int);
int enterprise_rom_id(int);

void Enterprise_Initialise(void);

extern VIDEO_START( enterprise );
extern VIDEO_UPDATE( enterprise );


