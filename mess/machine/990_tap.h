/*
	990_tap.h: include file for 990_tap.c
*/

extern int ti990_tape_load(mess_image *img, mame_file *fp, int open_mode);
extern void ti990_tape_unload(int id);

void ti990_tpc_init(void (*interrupt_callback)(int state));
void ti990_tpc_exit(void);

extern READ16_HANDLER(ti990_tpc_r);
extern WRITE16_HANDLER(ti990_tpc_w);

