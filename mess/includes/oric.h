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

int	oric_floppy_init(int id);
void oric_floppy_exit(int id);

extern int oric_cassette_init(int id);

/* Telestrat specific */
extern MACHINE_INIT( telestrat );
extern MACHINE_STOP( telestrat );
extern READ_HANDLER( telestrat_IO_r );
extern WRITE_HANDLER( telestrat_IO_w );

