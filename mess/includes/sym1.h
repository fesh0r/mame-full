extern DRIVER_INIT( sym1 );
extern MACHINE_INIT( sym1 );

#if 0
extern int kim1_cassette_init(int id);
extern void kim1_cassette_exit(int id);
#endif

extern INTERRUPT_GEN( sym1_interrupt );

extern UINT8 sym1_led[6];

extern PALETTE_INIT( sym1 );
extern VIDEO_START( sym1 );
extern VIDEO_UPDATE( sym1 );
