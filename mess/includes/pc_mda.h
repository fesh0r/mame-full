void pc_mda_init(void);

extern void pc_mda_timer(void);
extern int  pc_mda_vh_start(void);
extern void pc_mda_vh_stop(void);
extern void pc_mda_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_mda_videoram_w );

extern WRITE_HANDLER ( pc_MDA_w );
extern READ_HANDLER ( pc_MDA_r );

#if 0
extern void pc_mda_blink_textcolors(int on);
extern void pc_mda_index_w(int data);
extern int	pc_mda_index_r(void);
extern void pc_mda_port_w(int data);
extern int	pc_mda_port_r(void);
extern void pc_mda_mode_control_w(int data);
extern int	pc_mda_mode_control_r(void);
extern void pc_mda_color_select_w(int data);
extern int	pc_mda_color_select_r(void);
extern void pc_mda_feature_control_w(int data);
extern int	pc_mda_status_r(void);
extern void pc_mda_lightpen_strobe_w(int data);
extern void pc_hgc_config_w(int data);
extern int	pc_hgc_config_r(void);
#endif
