/* from mess/machine/mbee.c */
extern void mbee_init_machine(void);
extern void mbee_shutdown_machine(void);

extern int mbee_interrupt(void);
extern int mbee_cassette_init(int id);
extern void mbee_cassette_exit(int id);
extern int mbee_floppy_init(int id);
extern int mbee_rom_load(int id);
extern int mbee_rom_id(int id);

extern int mbee_pio_r(int offset);
extern void mbee_pio_w(int offset, int data);

extern int mbee_fdc_status_r(int offset);
extern void mbee_fdc_motor_w(int offset, int data);


/* from mess/vidhrdw/mbee.c */
extern char mbee_frame_message[128+1];
extern int mbee_frame_counter;
extern UINT8 *pcgram;

extern int m6545_status_r(int offs);
extern void m6545_index_w(int offs, int data);
extern int m6545_data_r(int offs);
extern void m6545_data_w(int offs, int data);

extern int mbee_color_bank_r(int offs);
extern void mbee_color_bank_w(int offs, int data);
extern int mbee_video_bank_r(int offs);
extern void mbee_video_bank_w(int offs, int data);

extern int mbee_pcg_color_latch_r(int offs);
extern void mbee_pcg_color_latch_w(int offs, int data);

extern int mbee_videoram_r(int offs);
extern void mbee_videoram_w(int offs, int data);

extern int mbee_pcg_color_r(int offs);
extern void mbee_pcg_color_w(int offs, int data);

extern int  mbee_vh_start(void);
extern void mbee_vh_stop(void);
extern void mbee_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
