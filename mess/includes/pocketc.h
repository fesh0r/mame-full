#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void pocketc_runtime_loader_init(void);
# else
	extern void pocketc_runtime_loader_init(void);
# endif
#endif


/* in vidhrdw/pocketc.c */

extern unsigned char pocketc_palette[248][3];
extern unsigned short pocketc_colortable[8][2];


void pocketc_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom);

int pocketc_vh_start(void);
void pocketc_vh_stop(void);

typedef char *POCKETC_FIGURE[];
void pocketc_draw_special(struct mame_bitmap *bitmap,
						  int x, int y, const POCKETC_FIGURE fig, int color);
