extern void mc10_init_machine(void);
extern void mc10_stop_machine(void);
extern READ_HANDLER ( mc10_bfff_r );
extern WRITE_HANDLER ( mc10_bfff_w );
extern READ_HANDLER ( mc10_port1_r );
extern READ_HANDLER ( mc10_port2_r );
extern WRITE_HANDLER ( mc10_port1_w );
extern WRITE_HANDLER ( mc10_port2_w );

extern int video_start_mc10(void);
extern WRITE_HANDLER ( mc10_ram_w );

