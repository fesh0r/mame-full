/* machine/lviv.c */
extern unsigned char * lviv_video_ram;
extern MACHINE_INIT( lviv );
extern int lviv_tape_init(int);
extern void lviv_tape_exit(int);

/* vidhrdw/lviv.c */
extern VIDEO_START( lviv );
extern VIDEO_UPDATE( lviv );
extern unsigned char lviv_palette[8*3];
extern unsigned short lviv_colortable[1][4];
extern PALETTE_INIT( lviv );
extern void lviv_update_palette (UINT8);
