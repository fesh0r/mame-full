/* from machine/vtech1.c */

/* #define OLD_VIDEO */

extern char vtech1_frame_message[64+1];
extern int vtech1_frame_time;

extern int vtech1_latch;

extern MACHINE_INIT( laser110 );
extern MACHINE_INIT( laser210 );
extern MACHINE_INIT( laser310 );
extern MACHINE_STOP( vtech1 );

extern int vtech1_cassette_init(int id, void *fp, int open_mode);

extern int vtech1_snapshot_load(void *fp);

extern int vtech1_floppy_init(int id, void *fp, int open_mode);
extern void vtech1_floppy_exit(int id);

extern READ_HANDLER ( vtech1_fdc_r );
extern WRITE_HANDLER ( vtech1_fdc_w );

extern READ_HANDLER ( vtech1_joystick_r );
extern READ_HANDLER ( vtech1_keyboard_r );
extern WRITE_HANDLER ( vtech1_latch_w );

extern void vtech1_interrupt(void);

#ifdef OLD_VIDEO
/* from vidhrdw/vtech1.c */
extern VIDEO_UPDATE( vtech1 );
#else
extern VIDEO_START( vtech1 );
#endif

