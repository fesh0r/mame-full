/* machine/pmd85.c */
READ_HANDLER ( pmd85_io_r );
WRITE_HANDLER ( pmd85_io_w );
READ_HANDLER ( alfa_io_r );
WRITE_HANDLER ( alfa_io_w );
DRIVER_INIT ( pmd851 );
DRIVER_INIT ( pmd852a );
DRIVER_INIT ( alfa );
extern MACHINE_INIT( pmd85 );
extern int pmd85_tape_init(int);
extern void pmd85_tape_exit(int);

/* vidhrdw/pmd85.c */
extern VIDEO_START( pmd85 );
extern VIDEO_UPDATE( pmd85 );
extern unsigned char pmd85_palette[3*3];
extern unsigned short pmd85_colortable[1][3];
extern PALETTE_INIT( pmd85 );
