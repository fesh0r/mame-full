#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void at_runtime_loader_init(void);
# else
	extern void at_runtime_loader_init(void);
# endif
#endif

void init_atcga(void);
#ifdef HAS_I386
void init_at386(void);
#endif

void init_at_vga(void);
void init_ps2m30286(void);

void at_machine_init(void);
void at_vga_init_machine(void);

int at_cga_frame_interrupt (void);
int at_vga_frame_interrupt (void);

