extern UINT8 *cgenie_fontram;

extern int cgenie_cassette_init(int id);
extern int cgenie_floppy_init(int id);
extern int cgenie_rom_load(int id);

extern int cgenie_vh_start(void);
extern void cgenie_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern void cgenie_sh_sound_init(const char * gamename);
extern WRITE_HANDLER ( cgenie_sh_control_port_w );
extern WRITE_HANDLER ( cgenie_sh_data_port_w );
extern READ_HANDLER ( cgenie_sh_control_port_r );
extern READ_HANDLER ( cgenie_sh_data_port_r );

/* from src/mess/machine/cgenie.c */
extern int cgenie_tv_mode;

extern READ_HANDLER ( cgenie_psg_port_a_r);
extern READ_HANDLER ( cgenie_psg_port_b_r );
extern WRITE_HANDLER ( cgenie_psg_port_a_w );
extern WRITE_HANDLER ( cgenie_psg_port_b_w );

extern void init_cgenie(void);
extern void cgenie_init_machine(void);
extern void cgenie_stop_machine(void);

extern READ_HANDLER ( cgenie_colorram_r );
extern READ_HANDLER ( cgenie_fontram_r );

extern void cgenie_dos_rom_w(int offset, int data);
extern void cgenie_ext_rom_w(int offset, int data);
extern WRITE_HANDLER ( cgenie_colorram_w );
extern WRITE_HANDLER ( cgenie_fontram_w );

extern WRITE_HANDLER ( cgenie_port_ff_w );
extern READ_HANDLER ( cgenie_port_ff_r );
extern int cgenie_port_xx_r(int offset);

extern int cgenie_timer_interrupt(void);
extern int cgenie_frame_interrupt(void);

extern READ_HANDLER ( cgenie_status_r );
extern READ_HANDLER ( cgenie_track_r );
extern READ_HANDLER ( cgenie_sector_r );
extern READ_HANDLER ( cgenie_data_r );

extern WRITE_HANDLER ( cgenie_command_w );
extern WRITE_HANDLER ( cgenie_track_w );
extern WRITE_HANDLER ( cgenie_sector_w );
extern WRITE_HANDLER ( cgenie_data_w );

extern READ_HANDLER ( cgenie_irq_status_r );

extern WRITE_HANDLER ( cgenie_motor_w );

extern READ_HANDLER ( cgenie_keyboard_r );
extern int cgenie_videoram_r(int offset);
extern WRITE_HANDLER ( cgenie_videoram_w );

typedef struct {         // CRTC 6845
        UINT8    cursor_address_lo;
        UINT8    cursor_address_hi;
        UINT8    screen_address_lo;
        UINT8    screen_address_hi;
        UINT8    cursor_bottom;
        UINT8    cursor_top;
        UINT8    scan_lines;
        UINT8    crt_mode;
        UINT8    vertical_sync_pos;
        UINT8    vertical_displayed;
        UINT8    vertical_adjust;
        UINT8    vertical_total;
        UINT8    horizontal_length;
        UINT8    horizontal_sync_pos;
        UINT8    horizontal_displayed;
        UINT8    horizontal_total;
        UINT8    idx;
        UINT8    cursor_visible;
        UINT8    cursor_phase;
} CRTC6845;

extern	int 	cgenie_font_offset[4];

extern	int 	cgenie_vh_start(void);
extern	void	cgenie_vh_stop(void);

extern	READ_HANDLER ( cgenie_index_r );
extern	READ_HANDLER ( cgenie_register_r );

extern	WRITE_HANDLER ( cgenie_index_w );
extern	WRITE_HANDLER (	cgenie_register_w );

extern	int 	cgenie_get_register(int indx);

extern	void	cgenie_mode_select(int graphics);
extern	void	cgenie_invalidate_range(int l, int h);

extern	void	cgenie_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh);

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void cgenie_runtime_loader_init(void);
# else
	extern void cgenie_runtime_loader_init(void);
# endif
#endif
