#ifndef UK101_H
#define UK101_H

/* machine/uk101.c */
MACHINE_INIT( uk101 );
READ_HANDLER( uk101_acia0_casin );
READ_HANDLER( uk101_acia0_statin );
READ_HANDLER( uk101_keyb_r );
WRITE_HANDLER( uk101_keyb_w );
int uk101_cassette_load(mess_image *img, mame_file *fp, int open_mode);
void uk101_cassette_unload(int id);

/* vidhrdw/uk101.c */
VIDEO_UPDATE( uk101 );
VIDEO_UPDATE( superbrd );

#endif /* UK101_H */
