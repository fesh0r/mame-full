/* machine/uk101.c */
extern MACHINE_INIT( uk101 );
extern READ_HANDLER( uk101_acia0_casin );
extern READ_HANDLER( uk101_acia0_statin );
extern READ_HANDLER( uk101_keyb_r );
extern WRITE_HANDLER( uk101_keyb_w );
extern int uk101_init_cassette(int id, void *fp, int open_mode);
extern void uk101_exit_cassette(int id);

/* vidhrdw/uk101.c */
extern VIDEO_UPDATE( uk101 );
extern VIDEO_UPDATE( superbrd );

