extern void init_sym1(void);
extern void sym1_init_machine(void);

#if 0
extern int kim1_cassette_init(int id);
extern void kim1_cassette_exit(int id);
#endif

extern int sym1_interrupt(void);

extern UINT8 sym1_led[6];
extern void sym1_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern int sym1_vh_start (void);
extern void sym1_vh_stop (void);
extern void sym1_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
extern "C" void sym1_runtime_loader_init(void);
# else
extern void sym1_runtime_loader_init(void);
# endif
#endif
