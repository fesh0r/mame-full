#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void studio2_runtime_loader_init(void);
# else
	extern void studio2_runtime_loader_init(void);
# endif
#endif


int studio2_get_vsync(void);
void studio2_video_start(void);
void studio2_video_stop(void);
void studio2_video_dma(int cycles);

int studio2_vh_start(void);
void studio2_vh_stop(void);
void studio2_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);
