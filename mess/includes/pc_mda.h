
/*void pc_mda_init(void);*/

#if 0
	// cutted from some aga char rom
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("mda.chr",     0x00000, 0x01000, 0xac1686f3)
#endif

extern void pc_mda_init_video(struct _CRTC6845 *crtc);
extern void pc_mda_europc_init(struct _CRTC6845 *crtc);

extern void pc_mda_timer(void);
extern int  pc_mda_vh_start(void);
extern void pc_mda_vh_stop(void);
extern void pc_mda_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_mda_videoram_w );

extern WRITE_HANDLER ( pc_MDA_w );
extern READ_HANDLER ( pc_MDA_r );

extern unsigned char mda_palette[4][3];
extern struct GfxLayout pc_mda_charlayout;
extern struct GfxLayout pc_mda_gfxlayout_1bpp;
extern struct GfxDecodeInfo pc_mda_gfxdecodeinfo[];
extern unsigned short mda_colortable[256*2+1*2];
void pc_mda_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom);

//internal use
void pc_mda_cursor(CRTC6845_CURSOR *cursor);
