
typedef struct {         // CRTC 6845
        byte    cursor_address_lo;
        byte    cursor_address_hi;
        byte    screen_address_lo;
        byte    screen_address_hi;
        byte    cursor_bottom;
        byte    cursor_top;
        byte    scan_lines;
        byte    crt_mode;
        byte    vertical_sync_pos;
        byte    vertical_displayed;
        byte    vertical_adjust;
        byte    vertical_total;
        byte    horizontal_length;
        byte    horizontal_sync_pos;
        byte    horizontal_displayed;
        byte    horizontal_total;
        byte    idx;
        byte    cursor_visible;
        byte    cursor_phase;
} CRTC6845;

extern  int     m6845_font_offset[4];

extern  int     m6845_vh_start(void);
extern  void    m6845_vh_stop(void);

extern  int     m6845_index_r(int offset);
extern  int     m6845_register_r(int offset);

extern  void    m6845_index_w(int offset, int data);
extern  void    m6845_register_w(int offset, int data);

extern	int 	m6845_get_register(int indx);

extern  void    m6845_mode_select(int graphics);
extern  void    m6845_invalidate_range(int l, int h);

extern  void    m6845_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh);
