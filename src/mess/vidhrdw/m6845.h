
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
