/* machine/ay3600.c */
extern void AY3600_init(void);
extern void AY3600_interrupt(void);
extern int  AY3600_anykey_clearstrobe_r(void);
extern int  AY3600_keydata_strobe_r(void);
