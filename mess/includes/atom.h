/* machine/atom.c */

extern void atom_init_machine (void);
extern void atom_stop_machine (void);
extern int atom_init_atm (int id);
extern READ_HANDLER ( atom_8255_porta_r );
extern READ_HANDLER ( atom_8255_portb_r );
extern READ_HANDLER ( atom_8255_portc_r );
extern WRITE_HANDLER ( atom_8255_porta_w );
extern WRITE_HANDLER ( atom_8255_portb_w );
extern WRITE_HANDLER ( atom_8255_portc_w );

/* machine/vidhrdw.c */

extern int atom_vh_start (void);

