/*
	smc92x4.h: header file for smc92x4.c

	Raphael Nabet
*/

void smc92x4_init(int which, data8_t (*dma_read_callback)(int which, offs_t offset), void (*dma_write_callback)(int which, offs_t offset, data8_t data), void (*int_callback)(int which, int state));
void smc92x4_reset(int which);
int smc92x4_r(int which, int offset);
void smc92x4_w(int which, int offset, int data);
READ_HANDLER(smc92x4_0_r);
WRITE_HANDLER(smc92x4_0_w);
