typedef enum {
	AT8042_STANDARD, 
	AT8042_PS2, //other timing of integrated controller!
	AT8042_AT386 // hopefully this is not really a keyboard controller variant, 
				 // but who knows; now it is needed
} AT8042_TYPE;

typedef struct {
	AT8042_TYPE type;
	void (*set_gate_a20)(int a20);
} AT8042_CONFIG;

void at_8042_init(AT8042_CONFIG *config);
void at_8042_time(void);

READ8_HANDLER(at_8042_8_r);
WRITE8_HANDLER(at_8042_8_w);
READ32_HANDLER(at_8042_32_r);
WRITE32_HANDLER(at_8042_32_w);

