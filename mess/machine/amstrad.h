void amstrad_setup_machine(void);
void amstrad_shutdown_machine(void);

int amstrad_floppy_init(int);


int amstrad_snapshot_load(int);
int amstrad_snapshot_id(int);
void amstrad_snapshot_exit(int);

int amstrad_floppy_load(int);
int amstrad_floppy_id(int);
void amstrad_floppy_exit(int);

void Amstrad_Reset(void);


extern int amstrad_cassette_init(int id);
extern void amstrad_cassette_exit(int id);

