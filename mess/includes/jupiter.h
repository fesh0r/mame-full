/* machine/jupiter.c */

extern OPBASE_HANDLER( jupiter_opbaseoverride );
extern void jupiter_init_machine (void);
extern void jupiter_stop_machine (void);
extern void jupiter_exit_ace (int id);
extern int jupiter_load_ace (int id);
extern void jupiter_exit_tap (int id);
extern int jupiter_load_tap (int id);
extern READ_HANDLER( jupiter_port_fefe_r);
extern READ_HANDLER( jupiter_port_fdfe_r);
extern READ_HANDLER( jupiter_port_fbfe_r);
extern READ_HANDLER( jupiter_port_f7fe_r);
extern READ_HANDLER( jupiter_port_effe_r);
extern READ_HANDLER( jupiter_port_dffe_r);
extern READ_HANDLER( jupiter_port_bffe_r);
extern READ_HANDLER( jupiter_port_7ffe_r);
extern WRITE_HANDLER( jupiter_port_fe_w);

/* vidhrdw/jupiter.c */

extern int jupiter_vh_start (void);
extern void jupiter_vh_stop (void);
extern void jupiter_vh_screenrefresh (struct osd_bitmap *bitmap,
												int full_refresh);
extern WRITE_HANDLER( jupiter_vh_charram_w );
extern unsigned char *jupiter_charram;
extern size_t jupiter_charram_size;

/* systems/jupiter.c */

extern struct GfxLayout jupiter_charlayout;
												
