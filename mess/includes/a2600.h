/* vidhrdw/a2600.c */
extern VIDEO_START( a2600 );
extern VIDEO_UPDATE( a2600 );
extern int a2600_scanline_interrupt(void);


/* machine/a2600.c */
extern READ_HANDLER  ( a2600_TIA_r );
extern WRITE_HANDLER ( a2600_TIA_w );
extern READ_HANDLER  ( a2600_riot_r );
extern WRITE_HANDLER ( a2600_riot_w );
extern READ_HANDLER  ( a2600_bs_r );

extern MACHINE_INIT( a2600 );
extern int  a2600_load_rom(int id);
extern READ_HANDLER ( a2600_ROM_r );
