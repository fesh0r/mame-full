#ifdef LSB_FIRST
#define ENDIANISE(x) ((((x) >> 8) + ((x) << 8)) & 0xffff)
#else
#define ENDIANISE(x) (x)
#endif

extern char *	vdp_pattern_scroll_a;
extern char *	vdp_pattern_window;
extern char *	vdp_pattern_scroll_b;
extern unsigned char *	vdp_pattern_sprite;
extern char *	vdp_h_scroll_addr;
extern unsigned char vdp_vram[];
extern unsigned short vdp_cram[];
extern short vdp_vsram[];
extern int	vdp_h_scrollsize;
extern int	vdp_v_scrollsize;
extern int	vdp_h_scrollmode;
extern int	vdp_v_scrollmode;
extern int	vdp_h_interrupt, vdp_v_interrupt;
extern int	vdp_display_enable;
extern int	vdp_display_height;
extern int	vdp_scroll_height;
extern int	vdp_background_colour;
extern int	vdp_h_width;

/* Prototypes for video-related routines */
READ_HANDLER  ( genesis_vdp_data_r );
WRITE_HANDLER ( genesis_vdp_data_w );
READ_HANDLER  ( genesis_vdp_ctrl_r );
WRITE_HANDLER ( genesis_vdp_ctrl_w );
READ_HANDLER  ( genesis_vdp_hv_r );
WRITE_HANDLER ( genesis_vdp_hv_w );

void genesis_dma_poll (int amount);
void genesis_initialise_dma (unsigned char *src, int dest, int length, int id, int increment);

WRITE_HANDLER ( genesis_videoram1_w );

int genesis_vh_start (void);
void genesis_vh_stop (void);
void genesis_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

