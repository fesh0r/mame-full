/* from machine/vtech1.c */

//#define OLD_VIDEO

extern char vtech1_frame_message[64+1];
extern int vtech1_frame_time;

extern int vtech1_latch;

extern void init_vtech1(void);
extern void laser110_init_machine(void);
extern void laser210_init_machine(void);
extern void laser310_init_machine(void);
extern void vtech1_shutdown_machine(void);

extern int vtech1_cassette_id(int id);
extern int vtech1_cassette_init(int id);
extern void vtech1_cassette_exit(int id);

extern int vtech1_snapshot_id(int id);
extern int vtech1_snapshot_init(int id);
extern void vtech1_snapshot_exit(int id);

extern int vtech1_floppy_id(int id);
extern int vtech1_floppy_init(int id);
extern void vtech1_floppy_exit(int id);

extern READ_HANDLER ( vtech1_fdc_r );
extern WRITE_HANDLER ( vtech1_fdc_w );

extern READ_HANDLER ( vtech1_joystick_r );
extern READ_HANDLER ( vtech1_keyboard_r );
extern WRITE_HANDLER ( vtech1_latch_w );

extern int vtech1_interrupt(void);
#ifdef OLD_VIDEO
/* from vidhrdw/vtech1.c */
extern int  vtech1_vh_start(void);
extern void vtech1_vh_stop(void);
extern void vtech1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
#else
extern int  vtech1_vh_start(void);

#endif

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void vtech1_runtime_loader_init(void);
# else
	extern void vtech1_runtime_loader_init(void);
# endif
#endif
