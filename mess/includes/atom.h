#include "devices/snapquik.h"

/* machine/atom.c */
extern MACHINE_INIT( atom );
extern QUICKLOAD_LOAD( atom );
extern READ_HANDLER (atom_8255_porta_r);
extern READ_HANDLER (atom_8255_portb_r);
extern READ_HANDLER (atom_8255_portc_r);
extern WRITE_HANDLER (atom_8255_porta_w );
extern WRITE_HANDLER (atom_8255_portb_w );
extern WRITE_HANDLER (atom_8255_portc_w );

/* machine/vidhrdw.c */
extern VIDEO_START( atom );

/* for floppy disc interface */
extern READ_HANDLER (atom_8271_r);
extern WRITE_HANDLER (atom_8271_w);

extern int atom_floppy_init(mess_image *img, mame_file *fp, int open_mode);

extern READ_HANDLER(atom_via_r);
extern WRITE_HANDLER(atom_via_w);

/* for .WAV */
extern int atom_cassette_init(int, mame_file *fp, int open_mode);

extern READ_HANDLER(atom_eprom_box_r);
extern WRITE_HANDLER(atom_eprom_box_w);
extern void atom_eprom_box_init(void);

extern MACHINE_INIT( atomeb );
