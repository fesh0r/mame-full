/* from machine\pce.c */

extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern unsigned char *pce_save_ram; /* battery backed RAM at F7 */
extern int pce_load_rom(int id);
extern int pce_id_rom (int id);
extern void pce_init_machine(void);
extern void pce_stop_machine(void);
extern WRITE_HANDLER ( pce_joystick_w );
extern READ_HANDLER ( pce_joystick_r );


