/* machine/atom.c */

extern void atom_init_machine (void);
extern void atom_stop_machine (void);
extern int atom_init_atm (int id);
extern int atom_8255_porta_r (int offset);
extern int atom_8255_portb_r (int offset);
extern int atom_8255_portc_r (int offset);
extern void atom_8255_porta_w (int offset, int data);
extern void atom_8255_portb_w (int offset, int data);
extern void atom_8255_portc_w (int offset, int data);

/* machine/vidhrdw.c */

extern int atom_vh_start (void);

extern READ_HANDLER (atom_8271_r);
extern WRITE_HANDLER (atom_8271_w);

extern int atom_floppy_init(int id);

