/* from machine\pce.c */
extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern unsigned char *pce_save_ram; /* battery backed RAM at F7 */
DEVICE_LOAD(pce_cart);
NVRAM_HANDLER( pce );
WRITE_HANDLER ( pce_joystick_w );
READ_HANDLER ( pce_joystick_r );
