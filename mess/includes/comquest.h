#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void comquest_runtime_loader_init(void);
# else
	extern void comquest_runtime_loader_init(void);
# endif
#endif

int comquest_vh_start(void);
void comquest_vh_stop(void);
void comquest_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh);
