/* From machine/vectrex.c */
extern unsigned char *vectrex_ram;
extern READ_HANDLER  ( vectrex_mirrorram_r );
extern WRITE_HANDLER ( vectrex_mirrorram_w );
extern int vectrex_init_cart (int id);

/* From machine/vectrex.c */
extern int vectrex_refresh_with_T2;
extern int vectrex_imager_status;
extern UINT32 vectrex_beam_color;
extern unsigned char vectrex_via_out[2];

extern void vectrex_imager_left_eye (double time_);
extern void vectrex_configuration(void);
extern READ_HANDLER (v_via_pa_r);
extern READ_HANDLER(v_via_pb_r );
extern void v_via_irq (int level);


/* From vidhrdw/vectrex.c */
extern VIDEO_START( vectrex );
extern VIDEO_UPDATE( vectrex );

extern VIDEO_START( raaspec );
extern VIDEO_UPDATE( raaspec );

extern WRITE_HANDLER  ( raaspec_led_w );

/* from vidhrdw/vectrex.c */
extern void vector_add_point_stereo (int x, int y, int color, int intensity);
extern void (*vector_add_point_function) (int, int, rgb_t, int);
