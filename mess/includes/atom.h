/* machine/atom.c */

extern void atom_init_machine (void);
extern void atom_stop_machine (void);
extern int atom_init_atm (int id);
extern READ_HANDLER (atom_8255_porta_r);
extern READ_HANDLER (atom_8255_portb_r);
extern READ_HANDLER (atom_8255_portc_r);
extern WRITE_HANDLER (void atom_8255_porta_w );
extern WRITE_HANDLER (void atom_8255_portb_w );
extern WRITE_HANDLER (void atom_8255_portc_w );

/* machine/vidhrdw.c */

extern int atom_vh_start (void);

/* for floppy disc interface */
extern READ_HANDLER (atom_8271_r);
extern WRITE_HANDLER (atom_8271_w);

extern int atom_floppy_init(int id);

extern READ_HANDLER(atom_via_r);
extern WRITE_HANDLER(atom_via_w);

/* for .WAV */
extern int atom_cassette_init(int);
extern void atom_cassette_exit(int);

extern READ_HANDLER(atom_eprom_box_r);
extern WRITE_HANDLER(atom_eprom_box_w);
extern void atom_eprom_box_init(void);

extern void	atomeb_init_machine(void);
