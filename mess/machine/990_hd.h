/*
	990_hd.h: include file for 990_hd.c
*/

extern int ti990_hd_load(mess_image *img, mame_file *fp, int open_mode);
extern void ti990_hd_unload(int id);

void ti990_hdc_init(void (*interrupt_callback)(int state));

extern READ16_HANDLER(ti990_hdc_r);
extern WRITE16_HANDLER(ti990_hdc_w);

