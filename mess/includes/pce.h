/* from machine\pce.c */

extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern unsigned char *pce_save_ram; /* battery backed RAM at F7 */
extern int pce_load_rom(int id, mame_file *fp, int open_mode);
extern MACHINE_INIT( pce );
extern NVRAM_HANDLER( pce );
extern WRITE_HANDLER ( pce_joystick_w );
extern READ_HANDLER ( pce_joystick_r );
