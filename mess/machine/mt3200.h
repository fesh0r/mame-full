/*
	mt3200.h: include file for mt3200.c
*/

extern int mt3200_tape_init(int id);
extern void mt3200_tape_exit(int id);

void mt3200_reset(void);

extern READ16_HANDLER(mt3200_r);
extern WRITE16_HANDLER(mt3200_w);

