extern UINT8 mk2_led[4];


int mk2_vh_start(void);
void mk2_vh_stop(void);
void mk2_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
