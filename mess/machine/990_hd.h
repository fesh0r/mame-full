/*
	990_hd.h: include file for 990_hd.c
*/

extern int ti990_hd_init(int id, void *fp, int open_mode);
extern void ti990_hd_exit(int id);

void ti990_hdc_init(void (*interrupt_callback)(int state));

extern READ16_HANDLER(ti990_hdc_r);
extern WRITE16_HANDLER(ti990_hdc_w);

