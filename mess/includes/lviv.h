/* machine/lviv.c */
extern void lviv_init_machine (void);
extern void lviv_stop_machine (void);
extern unsigned char * lviv_ram;
extern unsigned char * lviv_video_ram;

/* vidhrdw/lviv.c */
extern int lviv_vh_start (void);
extern void lviv_vh_stop (void);
extern void lviv_vh_screenrefresh (struct mame_bitmap *, int);
extern unsigned char lviv_palette[8][3];
extern unsigned short lviv_colortable[1][4];
extern void lviv_init_palette (unsigned char *, unsigned short *, const unsigned char *);
