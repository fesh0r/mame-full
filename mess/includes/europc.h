WRITE_HANDLER( europc_pio_w );
READ_HANDLER( europc_pio_r );

extern WRITE_HANDLER ( europc_jim_w );
extern READ_HANDLER ( europc_jim_r );

READ_HANDLER( europc_rtc_r );
WRITE_HANDLER( europc_rtc_w );
void europc_rtc_nvram_handler(void* file, int write);
void europc_rtc_set_time(void);
void europc_rtc_init(void);
