#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void ssystem3_runtime_loader_init(void);
# else
	extern void ssystem3_runtime_loader_init(void);
# endif
#endif



extern UINT8 ssystem3_led[5];
extern unsigned short ssystem3_colortable[1][2];
extern unsigned char ssystem3_palette[242][3];

void ssystem3_init_colors (unsigned char *sys_palette,
					  unsigned short *sys_colortable,
					  const unsigned char *color_prom);

int ssystem3_vh_start(void);
void ssystem3_vh_stop(void);
void ssystem3_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);
