/* machine/galaxy.c */
extern MACHINE_INIT( galaxy );
extern void galaxy_exit_snap (int id);
extern int galaxy_load_snap (int id);
extern int galaxy_init_wav(int);
extern void galaxy_exit_wav(int);
extern READ_HANDLER( galaxy_kbd_r );
extern WRITE_HANDLER( galaxy_kbd_w );
extern int galaxy_interrupts_enabled;

/* vidhrdw/galaxy.c */
extern VIDEO_START( galaxy );
extern VIDEO_UPDATE( galaxy );
extern WRITE_HANDLER( galaxy_vh_charram_w );
extern unsigned char *galaxy_charram;
extern size_t galaxy_charram_size;

/* systems/galaxy.c */

extern struct GfxLayout galaxy_charlayout;
												
