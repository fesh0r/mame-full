#ifndef ADAM_H
#define ADAM_H

#include "driver.h"

/* machine/adam.c */
extern int adam_lower_memory; /* Lower 32k memory Z80 address configuration */
extern int adam_upper_memory; /* Upper 32k memory Z80 address configuration */
extern int adam_joy_stat[2];
extern int adam_net_data; /* Data on AdamNet Bus */
extern int adam_pcb;

int adam_cart_verify(const UINT8 *buf, size_t size);

void clear_keyboard_buffer(void);
void exploreKeyboard(void);
void set_memory_banks(void);
void resetPCB(void);

DEVICE_LOAD( adam_cart );
DEVICE_LOAD( adam_floppy );
DEVICE_UNLOAD( adam_floppy );

READ_HANDLER  ( adamnet_r );
WRITE_HANDLER ( adamnet_w );
READ_HANDLER  ( adam_paddle_r );
WRITE_HANDLER ( adam_paddle_toggle_off );
WRITE_HANDLER ( adam_paddle_toggle_on );
WRITE_HANDLER ( adam_memory_map_controller_w );
READ_HANDLER ( adam_memory_map_controller_r );
READ_HANDLER ( adam_mem_r );
WRITE_HANDLER ( adam_mem_w );
READ_HANDLER ( adam_video_r );
WRITE_HANDLER ( adam_video_w );
WRITE_HANDLER ( common_writes_w );
READ_HANDLER  ( master6801_ram_r );
WRITE_HANDLER ( master6801_ram_w );

#endif /* ADAM_H */
