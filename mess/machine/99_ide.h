int ti99_ide_load_memcard(void);
int ti99_ide_save_memcard(void);

DEVICE_INIT( ti99_ide );
DEVICE_LOAD( ti99_ide );
DEVICE_UNLOAD( ti99_ide );

void ti99_ide_init(int in_tms9995_mode);
