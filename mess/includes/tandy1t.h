extern void init_t1000hx(void);
extern void pc_t1t_init_machine(void);

void tandy1000_init(void);
void tandy1000_close(void);

WRITE_HANDLER ( pc_t1t_p37x_w );
READ_HANDLER ( pc_t1t_p37x_r );

WRITE_HANDLER ( tandy1000_pio_w );
READ_HANDLER(tandy1000_pio_r);

int tandy1000_frame_interrupt (void);
