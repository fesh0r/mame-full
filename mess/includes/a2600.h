#ifndef A2600_H
#define A2600_H

/* vidhrdw/a2600.c */
extern int a2600_scanline_interrupt(void);

/* machine/a2600.c */
extern READ_HANDLER  ( a2600_TIA_r );
extern WRITE_HANDLER ( a2600_TIA_w );
extern READ_HANDLER  ( a2600_riot_r );
extern WRITE_HANDLER ( a2600_riot_w );
extern READ_HANDLER  ( a2600_bs_r );

extern MACHINE_INIT( a2600 );
extern MACHINE_STOP( a2600 );
extern int a2600_cart_load(int id, mame_file *cartfile, int open_mode);
extern READ_HANDLER ( a2600_ROM_r );

#endif /* A2600_H */
