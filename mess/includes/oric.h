/* from machine/oric.c */

void oric_init_machine (void);
void oric_shutdown_machine (void);
int oric_load_rom (int id);
int oric_interrupt(void);
READ_HANDLER ( oric_IO_r );
WRITE_HANDLER ( oric_IO_w );

/* from vidhrdw/oric.c */
void oric_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int oric_vh_start(void);
void oric_vh_stop(void);

extern unsigned char *oric_IO;

// int oric_ram_r (int offset);
// void oric_ram_w (int offset, int data);



/* from vidhrdw */
void oric_set_powerscreen_mode (int mode);
void oric_set_flash_show (int mode);

int oric_extract_file_from_tape (int filenum);



void oric_vh_stop(void);
void oric_init_char_attrs(void);


