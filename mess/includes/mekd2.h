/* from mess/machine/mekd2.c */
extern DRIVER_INIT( mekd2 );
extern int mekd2_rom_load (int id, void *fp, int open_mode);

/* from mess/vidhrdw/mekd2.c */
extern PALETTE_INIT( mekd2 );
extern VIDEO_START( mekd2 );
extern VIDEO_UPDATE( mekd2 );
