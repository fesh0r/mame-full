void amstrad_setup_machine(void);
void amstrad_shutdown_machine(void);

int amstrad_floppy_init(int, const char *);


int amstrad_snapshot_load(int, const char *);
int amstrad_snapshot_id(const char *, const char *);
void amstrad_snapshot_exit(int);

int amstrad_floppy_load(int, const char *);
int amstrad_floppy_id(const char *, const char *);
void amstrad_floppy_exit(int);

void Amstrad_Reset(void);


