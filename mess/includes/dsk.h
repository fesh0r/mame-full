int dsk_floppy_load(int id);
int dsk_floppy_id(int id);
void dsk_floppy_exit(int id);
void dsk_seek_callback(int drive, int physical_track);
void dsk_get_id_callback(int drive, struct chrn_id *id, int id_index, int side);
int    dsk_get_sectors_per_track(int drive, int side);

void dsk_write_sector_data_from_buffer(int drive, int sector_index, int side, char *ptr, int length, int ddam);
void dsk_read_sector_data_into_buffer(int drive, int sector_index, int side, char *ptr, int length);
