extern UINT8 mk2_led[4];
extern unsigned short mk2_colortable[1][2];
extern unsigned char mk2_palette[242][3];

void mk2_init_colors (unsigned char *sys_palette,
					  unsigned short *sys_colortable,
					  const unsigned char *color_prom);

int mk2_vh_start(void);
void mk2_vh_stop(void);
void mk2_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
