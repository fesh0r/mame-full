/* machine/p2000t.c */

extern void p2000t_init_machine (void);
extern void p2000t_stop_machine (void);
extern READ_HANDLER( p2000t_port_000f_r );
extern READ_HANDLER( p2000t_port_202f_r );
extern WRITE_HANDLER( p2000t_port_000f_w );
extern WRITE_HANDLER( p2000t_port_101f_w );
extern WRITE_HANDLER( p2000t_port_303f_w );
extern WRITE_HANDLER( p2000t_port_505f_w );
extern WRITE_HANDLER( p2000t_port_707f_w );
extern WRITE_HANDLER( p2000t_port_888b_w );
extern WRITE_HANDLER( p2000t_port_8c90_w );
extern WRITE_HANDLER( p2000t_port_9494_w );

/* vidhrdw/p2000t.c */

extern int p2000m_vh_start (void);
extern void p2000m_vh_stop (void);
extern void p2000m_vh_callback (void);
extern void p2000m_vh_screenrefresh (struct mame_bitmap *bitmap,
													int full_refresh);

/* systems/p2000t.c */

