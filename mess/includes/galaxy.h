/* machine/galaxy.c */

extern void galaxy_init_machine (void);
extern void galaxy_stop_machine (void);
extern void galaxy_exit_snap (int id);
extern int galaxy_load_snap (int id);
extern int galaxy_init_wav(int);
extern void galaxy_exit_wav(int);
extern READ_HANDLER( galaxy_kbd_r );
extern WRITE_HANDLER( galaxy_kbd_w );
extern int galaxy_interrupts_enabled;

/* vidhrdw/galaxy.c */

extern int galaxy_vh_start (void);
extern void galaxy_vh_stop (void);
extern void galaxy_vh_screenrefresh (struct mame_bitmap *bitmap,
												int full_refresh);
extern WRITE_HANDLER( galaxy_vh_charram_w );
extern unsigned char *galaxy_charram;
extern size_t galaxy_charram_size;

/* systems/galaxy.c */

extern struct GfxLayout galaxy_charlayout;
												
