#ifndef MEKD2_H
#define MEKD2_H

/* from mess/machine/mekd2.c */
DRIVER_INIT( mekd2 );
int mekd2_cart_load (mess_image *img, mame_file *fp, int open_mode);

/* from mess/vidhrdw/mekd2.c */
PALETTE_INIT( mekd2 );
VIDEO_START( mekd2 );
VIDEO_UPDATE( mekd2 );

#endif /* MEKD2_H */
