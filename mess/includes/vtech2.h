/* from mame.c */
//extern int bitmap_dirty;

/* from machine/laser350.c */
extern int laser_latch;

void init_laser(void);
MACHINE_INIT( laser350 );
MACHINE_INIT( laser500 );
MACHINE_INIT( laser700 );

DEVICE_LOAD( laser_cart );
DEVICE_UNLOAD( laser_cart );

DEVICE_LOAD( laser_floppy );
DEVICE_LOAD( laser_cassette );

extern READ_HANDLER ( laser_fdc_r );
extern WRITE_HANDLER ( laser_fdc_w );
extern WRITE_HANDLER ( laser_bank_select_w );

/* from vidhrdw/laser350.c */

extern char laser_frame_message[64+1];
extern int laser_frame_time;

extern VIDEO_START( laser );
extern VIDEO_UPDATE( laser );
extern WRITE_HANDLER ( laser_bg_mode_w );
extern WRITE_HANDLER ( laser_two_color_w );
