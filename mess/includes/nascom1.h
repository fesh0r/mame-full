/* machine/nascom1.c */
extern MACHINE_INIT( nascom1 );
extern int nascom1_cassette_load (mess_image *img, mame_file *fp, int open_mode);
extern void nascom1_cassette_unload (int id);
extern int nascom1_read_cassette (void);
extern int nascom1_init_cartridge (int id, mame_file *fp);
extern READ_HANDLER( nascom1_port_00_r);
extern READ_HANDLER( nascom1_port_01_r);
extern READ_HANDLER( nascom1_port_02_r);
extern WRITE_HANDLER( nascom1_port_00_w);
extern WRITE_HANDLER( nascom1_port_01_w);

/* vidhrdw/nascom1.c */
#define video_start_nascom1		video_start_generic
extern VIDEO_UPDATE( nascom1 );
extern VIDEO_UPDATE( nascom2 );

