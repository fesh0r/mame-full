#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void odyssey2_runtime_loader_init(void);
# else
	extern void odyssey2_runtime_loader_init(void);
# endif
#endif


/* machine/odyssey2.c */
extern int odyssey2_framestart;
extern int odyssey2_videobank;

extern void odyssey2_init_machine(void);
extern int odyssey2_load_rom (int id);


/* vidhrdw/odyssey2.c */
extern int odyssey2_vh_hpos;

extern UINT8 odyssey2_colors[24][3];

extern int odyssey2_vh_start(void);
extern void odyssey2_vh_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom);
extern void odyssey2_vh_stop(void);
extern void odyssey2_vh_write(int data);
extern void odyssey2_vh_update(int data);
extern void odyssey2_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
extern READ_HANDLER ( odyssey2_t1_r );
int odyssey2_line(void);

extern READ_HANDLER ( odyssey2_video_r );
extern WRITE_HANDLER ( odyssey2_video_w );




/* i/o ports */
extern READ_HANDLER ( odyssey2_bus_r );
extern WRITE_HANDLER ( odyssey2_bus_w );

extern READ_HANDLER( odyssey2_getp1 );
extern WRITE_HANDLER ( odyssey2_putp1 );

extern READ_HANDLER( odyssey2_getp2 );
extern WRITE_HANDLER ( odyssey2_putp2 );

extern READ_HANDLER( odyssey2_getbus );
extern WRITE_HANDLER ( odyssey2_putbus );
