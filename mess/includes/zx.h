/* from machine/zx80.c */
extern DRIVER_INIT( zx );
extern MACHINE_INIT( zx80 );
extern MACHINE_INIT( zx81 );
extern MACHINE_INIT( pc8300 );
extern MACHINE_INIT( pow3000 );

extern  READ8_HANDLER ( zx_io_r );
extern WRITE8_HANDLER ( zx_io_w );

extern  READ8_HANDLER ( pow3000_io_r );
extern WRITE8_HANDLER ( pow3000_io_w );

/* from vidhrdw/zx80.c */
extern int zx_ula_scanline(void);
extern VIDEO_START( zx );
extern VIDEO_UPDATE( zx );

extern char zx_frame_message[128];
extern int zx_frame_time;

/* from vidhrdw/zx.c */
extern void zx_ula_bkgnd(int color);
extern int zx_ula_r(int offs, int region);

extern void *ula_nmi;
extern int ula_irq_active;
extern int ula_nmi_active;
extern int ula_frame_vsync;
extern int ula_scanline_count;
extern int ula_scancode_count;

