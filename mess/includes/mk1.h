extern UINT8 mk1_led[4];
extern unsigned short mk1_colortable[1][2];
extern unsigned char mk1_palette[242][3];

void mk1_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
					  const unsigned char *color_prom);

int mk1_vh_start(void);
void mk1_vh_stop(void);
void mk1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
