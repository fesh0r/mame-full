/* from mame.c */
//extern int bitmap_dirty;

/* from machine/laser350.c */
extern int laser_latch;

extern void init_laser(void);
extern void laser350_init_machine(void);
extern void laser500_init_machine(void);
extern void laser700_init_machine(void);
extern void laser_shutdown_machine(void);

extern int laser_rom_init(int id);
extern void laser_rom_exit(int id);

extern int laser_floppy_init(int id);
extern void laser_floppy_exit(int id);

extern int laser_cassette_init(int id);
extern void laser_cassette_exit(int id);

extern READ_HANDLER ( laser_fdc_r );
extern WRITE_HANDLER ( laser_fdc_w );
extern WRITE_HANDLER ( laser_bank_select_w );

/* from vidhrdw/laser350.c */

extern char laser_frame_message[64+1];
extern int laser_frame_time;


extern int laser_vh_start(void);
extern void laser_vh_stop(void);
extern void laser_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( laser_bg_mode_w );
extern WRITE_HANDLER ( laser_two_color_w );

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void vtech2_runtime_loader_init(void);
# else
	extern void vtech2_runtime_loader_init(void);
# endif
#endif
