/* From machine/vectrex.c */
extern unsigned char *vectrex_ram;
extern READ_HANDLER  ( vectrex_mirrorram_r );
extern WRITE_HANDLER ( vectrex_mirrorram_w );
extern int vectrex_load_rom (int id);
extern int vectrex_id_rom (int id);

/* From machine/vectrex.c */
extern int vectrex_refresh_with_T2;
extern int vectrex_imager_status;
extern int vectrex_beam_color;
extern unsigned char vectrex_via_out[2];

extern void vectrex_imager_left_eye (double time);
extern void vectrex_configuration(void);
extern READ_HANDLER (v_via_pa_r);
extern READ_HANDLER(v_via_pb_r );
extern void v_via_irq (int level);


/* From vidhrdw/vectrex.c */
extern int vectrex_start(void);
extern void vectrex_stop (void);
extern void vectrex_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void vectrex_vh_update (struct osd_bitmap *bitmap, int full_refresh);

extern int raaspec_start(void);
extern WRITE_HANDLER  ( raaspec_led_w );
extern void raaspec_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void raaspec_vh_update (struct osd_bitmap *bitmap, int full_refresh);

/* from vidhrdw/vectrex.c */
extern void vector_add_point_stereo (int x, int y, int color, int intensity);
extern void (*vector_add_point_function) (int, int, int, int);
extern void vectrex_set_palette (void);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void vectrex_runtime_loader_init(void);
# else
	extern void vectrex_runtime_loader_init(void);
# endif
#endif
