extern void mc10_init_machine(void);
extern void mc10_stop_machine(void);
extern  READ8_HANDLER ( mc10_bfff_r );
extern WRITE8_HANDLER ( mc10_bfff_w );
extern  READ8_HANDLER ( mc10_port1_r );
extern  READ8_HANDLER ( mc10_port2_r );
extern WRITE8_HANDLER ( mc10_port1_w );
extern WRITE8_HANDLER ( mc10_port2_w );

extern int video_start_mc10(void);
extern WRITE8_HANDLER ( mc10_ram_w );

