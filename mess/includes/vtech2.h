/* from mame.c */
//extern int bitmap_dirty;

/* from machine/laser350.c */
extern int laser_latch;

extern void init_laser(void);
extern MACHINE_INIT( laser350 );
extern MACHINE_INIT( laser500 );
extern MACHINE_INIT( laser700 );

int laser_cart_load(mess_image *img, mame_file *fp, int open_mode);
void laser_cart_unload(int id);

int laser_floppy_load(int id, mame_file *file, int open_mode);

extern int laser_cassette_init(mess_image *img, mame_file *fp, int open_mode);

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
