/* machine/nascom1.c */

extern void nascom1_init_machine (void);
extern void nascom1_stop_machine (void);
extern int nascom1_init_cassette (int id);
extern void nascom1_exit_cassette (int id);
extern int nascom1_read_cassette (void);
extern int nascom1_init_cartridge (int id);
extern READ_HANDLER( nascom1_port_00_r);
extern READ_HANDLER( nascom1_port_01_r);
extern READ_HANDLER( nascom1_port_02_r);
extern WRITE_HANDLER( nascom1_port_00_w);
extern WRITE_HANDLER( nascom1_port_01_w);

/* vidhrdw/nascom1.c */

extern int nascom1_vh_start (void);
extern void nascom1_vh_stop (void);
extern void nascom1_vh_screenrefresh (struct osd_bitmap *bitmap,
												int full_refresh);
extern void nascom2_vh_screenrefresh (struct osd_bitmap *bitmap,
												int full_refresh);
/* systems/nascom1.c */

