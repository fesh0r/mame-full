
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

extern	int 	cgenie_index_r(int offset);
extern	int 	cgenie_register_r(int offset);

extern	void	cgenie_index_w(int offset, int data);
extern	void	cgenie_register_w(int offset, int data);

extern	int 	cgenie_get_register(int indx);

extern	void	cgenie_mode_select(int graphics);
extern	void	cgenie_invalidate_range(int l, int h);

extern	void	cgenie_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh);
