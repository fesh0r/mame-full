extern unsigned short pcjr_colortable[256*2+16*2+2*4+1*16];
extern struct GfxDecodeInfo t1t_gfxdecodeinfo[];
void pcjr_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom);


extern void pc_t1t_timer(void);
extern int	pc_t1t_vh_start(void);
extern void pc_t1t_vh_stop(void);
extern void pc_t1t_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_t1t_videoram_w );
extern READ_HANDLER ( pc_t1t_videoram_r );
extern WRITE_HANDLER ( pc_T1T_w );
extern READ_HANDLER (	pc_T1T_r );

extern void pc_t1t_reset(void);

#if 0
extern void pc_t1t_blink_textcolors(int on);
extern void pc_t1t_index_w(int data);
extern int	pc_t1t_index_r(void);
extern void pc_t1t_port_w(int data);
extern int	pc_t1t_port_r(void);
extern void pc_t1t_mode_control_w(int data);
extern int	pc_t1t_mode_control_r(void);
extern void pc_t1t_color_select_w(int data);
extern int	pc_t1t_color_select_r(void);
extern void pc_t1t_vga_index_w(int data);
extern int	pc_t1t_status_r(void);
extern void pc_t1t_lightpen_strobe_w(int data);
extern void pc_t1t_vga_data_w(int data);
extern int	pc_t1t_vga_data_r(void);
extern void pc_t1t_bank_w(int data);
extern int	pc_t1t_bank_r(void);
#endif
