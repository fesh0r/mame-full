/* from machine/zx80.c */

extern void init_zx(void);
extern void zx80_init_machine(void);
extern void zx81_init_machine(void);
extern void pc8300_init_machine(void);
extern void pow3000_init_machine(void);
extern void zx_shutdown_machine(void);

extern int zx_cassette_init(int id);
extern void zx_cassette_exit(int id);

extern READ_HANDLER ( zx_io_r );
extern WRITE_HANDLER ( zx_io_w );

extern READ_HANDLER ( pow3000_io_r );
extern WRITE_HANDLER ( pow3000_io_w );

/* from vidhrdw/zx80.c */
extern int zx_ula_scanline(void);
extern void zx_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);



extern char zx_frame_message[128];
extern int zx_frame_time;

/* from vidhrdw/zx.c */
extern void zx_ula_bkgnd(int color);
extern int zx_ula_r(int offs, int region);
extern void zx_ula_nmi(int param);
extern void zx_ula_irq(int param);

extern void *ula_nmi;
extern void *ula_irq;
extern int ula_frame_vsync;
extern int ula_scanline_count;
extern int ula_scancode_count;

