/* from src/mess/machine/mekd2.c */
extern void init_mekd2(void);
extern void mekd2_init_machine(void);

extern int mekd2_rom_load (int id);

extern int mekd2_interrupt(void);

/* from src/mess/vidhrdw/mekd2.c */
extern void mekd2_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern int mekd2_vh_start (void);
extern void mekd2_vh_stop (void);
extern void mekd2_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void mekd2_runtime_loader_init(void);
# else
	extern void mekd2_runtime_loader_init(void);
# endif
#endif
