#include "devices/snapquik.h"

/* machine/atom.c */
MACHINE_INIT( atom );
QUICKLOAD_LOAD( atom );
READ_HANDLER (atom_8255_porta_r);
READ_HANDLER (atom_8255_portb_r);
READ_HANDLER (atom_8255_portc_r);
WRITE_HANDLER (atom_8255_porta_w );
WRITE_HANDLER (atom_8255_portb_w );
WRITE_HANDLER (atom_8255_portc_w );

/* machine/vidhrdw.c */
VIDEO_START( atom );

/* for floppy disc interface */
READ_HANDLER (atom_8271_r);
WRITE_HANDLER (atom_8271_w);

DEVICE_LOAD( atom_floppy );

READ_HANDLER(atom_eprom_box_r);
WRITE_HANDLER(atom_eprom_box_w);
void atom_eprom_box_init(void);

MACHINE_INIT( atomeb );
