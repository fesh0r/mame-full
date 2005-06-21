
void init_atcga(void);
#ifdef HAS_I386
void init_at386(void);
#endif

void init_at_vga(void);
void init_ps2m30286(void);

extern MACHINE_INIT( at );
extern MACHINE_INIT( at_vga );

