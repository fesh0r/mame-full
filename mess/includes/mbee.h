/* from mess/machine/mbee.c */
MACHINE_INIT( mbee );

void mbee_interrupt(void);

DEVICE_LOAD( mbee_cart );

READ_HANDLER ( mbee_pio_r );
WRITE_HANDLER ( mbee_pio_w );

READ_HANDLER ( mbee_fdc_status_r );
WRITE_HANDLER ( mbee_fdc_motor_w );


/* from mess/vidhrdw/mbee.c */
extern char mbee_frame_message[128+1];
extern int mbee_frame_counter;
extern UINT8 *pcgram;

READ_HANDLER ( m6545_status_r );
WRITE_HANDLER ( m6545_index_w );
READ_HANDLER ( m6545_data_r );
WRITE_HANDLER ( m6545_data_w );

READ_HANDLER ( mbee_color_bank_r );
WRITE_HANDLER ( mbee_color_bank_w );
READ_HANDLER ( mbee_video_bank_r );
WRITE_HANDLER ( mbee_video_bank_w );

READ_HANDLER ( mbee_pcg_color_latch_r );
WRITE_HANDLER ( mbee_pcg_color_latch_w );

READ_HANDLER ( mbee_videoram_r );
WRITE_HANDLER ( mbee_videoram_w );

READ_HANDLER ( mbee_pcg_color_r );
WRITE_HANDLER ( mbee_pcg_color_w );

VIDEO_START( mbee );
VIDEO_UPDATE( mbee );
