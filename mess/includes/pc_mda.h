extern unsigned char mda_palette[4][3];

/*void pc_mda_init(void);*/

extern void pc_mda_init_video(struct _CRTC6845 *crtc);
extern void pc_mda_europc_init(struct _CRTC6845 *crtc);

extern void pc_mda_timer(void);
extern int  pc_mda_vh_start(void);
extern void pc_mda_vh_stop(void);
extern void pc_mda_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_mda_videoram_w );

extern WRITE_HANDLER ( pc_MDA_w );
extern READ_HANDLER ( pc_MDA_r );

#if 0
extern void pc_mda_blink_textcolors(int on);
extern int	pc_mda_status_r(void);
extern void hercules_mode_control_w(int data);
extern void hercules_config_w(int data);
#endif

//internal use
void pc_mda_cursor(CRTC6845_CURSOR *cursor);
