
void init_atcga(void);
#ifdef HAS_I386
void init_at386(void);
#endif

void init_at_vga(void);

void at_machine_init(void);

int at_cga_frame_interrupt (void);
int at_vga_frame_interrupt (void);

READ_HANDLER(at_8042_r);
WRITE_HANDLER(at_8042_w);

