extern void mc10_init_machine(void);
extern void mc10_stop_machine(void);
extern READ_HANDLER ( mc10_bfff_r );
extern WRITE_HANDLER ( mc10_bfff_w );
extern READ_HANDLER ( mc10_port1_r );
extern READ_HANDLER ( mc10_port2_r );
extern WRITE_HANDLER ( mc10_port1_w );
extern WRITE_HANDLER ( mc10_port2_w );

extern int mc10_vh_start(void);
extern void mc10_vh_stop(void);
extern void mc10_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( mc10_ram_w );

