extern void init_t1000hx(void);
extern void pc_t1t_init_machine(void);

WRITE_HANDLER ( pc_t1t_p37x_w );
READ_HANDLER ( pc_t1t_p37x_r );

WRITE_HANDLER ( tandy1000_pio_w );
READ_HANDLER(tandy1000_pio_r);

int tandy1000_frame_interrupt (void);

void tandy1000_nvram_handler(void* file, int write);

