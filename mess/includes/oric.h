/* from machine/oric.c */

void oric_init_machine (void);
void oric_shutdown_machine (void);
int oric_interrupt(void);
READ_HANDLER ( oric_IO_r );
WRITE_HANDLER ( oric_IO_w );

/* from vidhrdw/oric.c */
void oric_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int oric_vh_start(void);
void oric_vh_stop(void);

extern WRITE_HANDLER(oric_psg_porta_write);

int	oric_floppy_init(int id);
void oric_floppy_exit(int id);

void oric_vh_stop(void);

extern int oric_cassette_init(int id);
extern void oric_cassette_exit(int id);

/* Telestrat specific */
void	telestrat_init_machine(void);
void	telestrat_shutdown_machine(void);

READ_HANDLER ( telestrat_IO_r );
WRITE_HANDLER ( telestrat_IO_w );

