int ti99_4_pio_load(mess_image *img, mame_file *fp, int open_mode);
void ti99_4_pio_unload(int id);
int ti99_4_rs232_load(mess_image *img, mame_file *fp, int open_mode);
void ti99_4_rs232_unload(int id);

void ti99_rs232_init(void);
void ti99_rs232_cleanup(void);
