void tandy1000_init(void);
void tandy1000_close(void);

int tandy1000_read_eeprom(void);

WRITE_HANDLER ( pc_t1t_p37x_w );
READ_HANDLER ( pc_t1t_p37x_r );
