/* from mess/machine/mbee.c */
extern MACHINE_INIT( mbee );

extern void mbee_interrupt(void);
extern int mbee_cassette_init(int id, void *fp, int open_mode);
extern int mbee_floppy_init(int id);
extern int mbee_rom_load(int id, void *fp, int open_mode);

extern READ_HANDLER ( mbee_pio_r );
extern WRITE_HANDLER ( mbee_pio_w );

extern READ_HANDLER ( mbee_fdc_status_r );
extern WRITE_HANDLER ( mbee_fdc_motor_w );


/* from mess/vidhrdw/mbee.c */
extern char mbee_frame_message[128+1];
extern int mbee_frame_counter;
extern UINT8 *pcgram;

extern READ_HANDLER ( m6545_status_r );
extern WRITE_HANDLER ( m6545_index_w );
extern READ_HANDLER ( m6545_data_r );
extern WRITE_HANDLER ( m6545_data_w );

extern READ_HANDLER ( mbee_color_bank_r );
extern WRITE_HANDLER ( mbee_color_bank_w );
extern READ_HANDLER ( mbee_video_bank_r );
extern WRITE_HANDLER ( mbee_video_bank_w );

extern READ_HANDLER ( mbee_pcg_color_latch_r );
extern WRITE_HANDLER ( mbee_pcg_color_latch_w );

extern READ_HANDLER ( mbee_videoram_r );
extern WRITE_HANDLER ( mbee_videoram_w );

extern READ_HANDLER ( mbee_pcg_color_r );
extern WRITE_HANDLER ( mbee_pcg_color_w );

extern VIDEO_START( mbee );
extern VIDEO_UPDATE( mbee );
