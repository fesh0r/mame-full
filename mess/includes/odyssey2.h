/* machine/odyssey2.c */
extern int odyssey2_framestart;
extern int odyssey2_videobank;

extern void odyssey2_init_machine(void);
extern int odyssey2_load_rom (int id);


/* vidhrdw/odyssey2.c */
extern int odyssey2_vh_hpos;

extern int odyssey2_vh_start(void);
extern void odyssey2_vh_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom);
extern void odyssey2_vh_stop(void);
extern void odyssey2_vh_write(int data);
extern void odyssey2_vh_update(int data);
extern void odyssey2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);





/* i/o ports */
extern READ_HANDLER ( odyssey2_MAINRAM_r );
extern WRITE_HANDLER ( odyssey2_MAINRAM_w );

extern READ_HANDLER( odyssey2_getp1 );

extern WRITE_HANDLER ( odyssey2_putp1 );
