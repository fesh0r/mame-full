/* from mess/machine/kim1.c */
extern DRIVER_INIT( kim1 );
extern MACHINE_INIT( kim1 );

extern int kim1_cassette_init(int id, mame_file *fp, int open_mode);
extern void kim1_cassette_exit(int id);

extern INTERRUPT_GEN( kim1_interrupt );

extern READ_HANDLER ( m6530_003_r );
extern READ_HANDLER ( m6530_002_r );
extern READ_HANDLER ( kim1_mirror_r );

extern WRITE_HANDLER ( m6530_003_w );
extern WRITE_HANDLER ( m6530_002_w );
extern WRITE_HANDLER ( kim1_mirror_w );

/* from mess/vidhrdw/kim1.c */
extern PALETTE_INIT( kim1 );
extern VIDEO_START( kim1 );
extern VIDEO_UPDATE( kim1 );

