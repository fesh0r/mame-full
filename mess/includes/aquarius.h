/* machine/aquarius.c */

extern void aquarius_init_machine (void);
extern void aquarius_stop_machine (void);
extern READ_HANDLER( aquarius_port_ff_r );
extern READ_HANDLER( aquarius_port_fe_r );
extern WRITE_HANDLER( aquarius_port_fc_w );
extern WRITE_HANDLER( aquarius_port_fe_w );
extern WRITE_HANDLER( aquarius_port_ff_w );

/* vidhrdw/aquarius.c */

extern int aquarius_vh_start (void);
extern void aquarius_vh_stop (void);
extern void aquarius_vh_screenrefresh (struct osd_bitmap *bitmap,
													int full_refresh);

/* systems/aquarius.c */

