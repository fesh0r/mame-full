/* machine/advision.c */
extern int advision_framestart;
/*extern int advision_videoenable;*/
extern int advision_videobank;

extern MACHINE_INIT( advision );
extern int advision_load_rom (int id);
extern READ_HANDLER ( advision_MAINRAM_r);
extern WRITE_HANDLER( advision_MAINRAM_w );
extern WRITE_HANDLER( advision_putp1 );
extern WRITE_HANDLER( advision_putp2 );
extern READ_HANDLER ( advision_getp1 );
extern READ_HANDLER ( advision_getp2 );
extern READ_HANDLER ( advision_gett0 );
extern READ_HANDLER ( advision_gett1 );


/* vidhrdw/advision.c */
extern int advision_vh_hpos;

extern VIDEO_START( advision );
extern VIDEO_UPDATE( advision );
extern PALETTE_INIT( advision );
extern void advision_vh_write(int data);
extern void advision_vh_update(int data);

