/* machine/coleco.c */
extern unsigned char *coleco_ram;
extern unsigned char *coleco_cartridge_rom;

extern int coleco_id_rom (int id);
extern int coleco_load_rom (int id);
extern READ_HANDLER  ( coleco_ram_r );
extern WRITE_HANDLER ( coleco_ram_w );
extern READ_HANDLER  ( coleco_paddle_r );
extern WRITE_HANDLER ( coleco_paddle_toggle_1_w );
extern WRITE_HANDLER ( coleco_paddle_toggle_2_w );
extern READ_HANDLER  ( coleco_VDP_r );
extern WRITE_HANDLER ( coleco_VDP_w );


/* vidhrdw/coleco.c */
extern int coleco_vh_start(void);
extern void coleco_vh_stop(void);
extern void coleco_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);



