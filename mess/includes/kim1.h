/* from src/mess/machine/kim1.c */
extern void init_kim1(void);
extern void kim1_init_machine(void);

extern int kim1_cassette_init(int id);
extern void kim1_cassette_exit(int id);

extern int kim1_interrupt(void);

extern READ_HANDLER ( m6530_003_r );
extern READ_HANDLER ( m6530_002_r );
extern READ_HANDLER ( kim1_mirror_r );

extern WRITE_HANDLER ( m6530_003_w );
extern WRITE_HANDLER ( m6530_002_w );
extern WRITE_HANDLER ( kim1_mirror_w );

/* from src/mess/vidhrdw/kim1.c */
extern void kim1_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern int kim1_vh_start (void);
extern void kim1_vh_stop (void);
extern void kim1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

