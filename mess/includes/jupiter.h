/* machine/jupiter.c */

OPBASE_HANDLER( jupiter_opbaseoverride );
MACHINE_INIT( jupiter );
MACHINE_STOP( jupiter );

DEVICE_LOAD( jupiter_ace );
DEVICE_LOAD( jupiter_tap );
DEVICE_UNLOAD( jupiter_tap );

READ_HANDLER( jupiter_port_fefe_r);
READ_HANDLER( jupiter_port_fdfe_r);
READ_HANDLER( jupiter_port_fbfe_r);
READ_HANDLER( jupiter_port_f7fe_r);
READ_HANDLER( jupiter_port_effe_r);
READ_HANDLER( jupiter_port_dffe_r);
READ_HANDLER( jupiter_port_bffe_r);
READ_HANDLER( jupiter_port_7ffe_r);
WRITE_HANDLER( jupiter_port_fe_w);

/* vidhrdw/jupiter.c */
VIDEO_START( jupiter );
VIDEO_UPDATE( jupiter );
WRITE_HANDLER( jupiter_vh_charram_w );
extern unsigned char *jupiter_charram;
extern size_t jupiter_charram_size;

/* systems/jupiter.c */

extern struct GfxLayout jupiter_charlayout;
												
