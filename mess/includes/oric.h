/* from machine/oric.c */
extern MACHINE_INIT( oric );
extern MACHINE_STOP( oric );
extern INTERRUPT_GEN( oric_interrupt );
extern READ_HANDLER( oric_IO_r );
extern WRITE_HANDLER( oric_IO_w );

/* from vidhrdw/oric.c */
extern VIDEO_START( oric );
extern VIDEO_UPDATE( oric );

extern WRITE_HANDLER(oric_psg_porta_write);

int	oric_floppy_load(mess_image *img, mame_file *fp, int open_mode);
void oric_floppy_unload(int id);

extern int oric_cassette_init(mess_image *img, mame_file *fp, int open_mode);

/* Telestrat specific */
extern MACHINE_INIT( telestrat );
extern MACHINE_STOP( telestrat );
extern READ_HANDLER( telestrat_IO_r );
extern WRITE_HANDLER( telestrat_IO_w );

