/* from machine\pce.c */
extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern unsigned char *pce_save_ram; /* battery backed RAM at F7 */
int pce_cart_load(mess_image *img, mame_file *fp, int open_mode);
NVRAM_HANDLER( pce );
WRITE_HANDLER ( pce_joystick_w );
READ_HANDLER ( pce_joystick_r );
