#include "driver.h"
#include "includes/crtc6845.h"

extern unsigned char cga_palette[16][3];

extern void pc_cga_init(struct _CRTC6845 *crtc);

extern int	pc_cga_vh_start(void);
extern void pc_cga_vh_stop(void);
extern void pc_cga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

// call this with 240 times pre second
extern void pc_cga_timer(void);

extern WRITE_HANDLER ( pc_cga_videoram_w );
extern WRITE_HANDLER ( pc_CGA_w );
extern READ_HANDLER ( pc_CGA_r );

#if 0
extern void pc_cga_blink_textcolors(int on);
extern void pc_cga_mode_control_w(int data);
extern void pc_cga_color_select_w(int data);
extern int	pc_cga_status_r(void);
#endif

extern int	pc1512_vh_start(void);
extern void pc1512_vh_stop(void);
extern void pc1512_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern WRITE_HANDLER ( pc1512_w );
extern READ_HANDLER ( pc1512_r );
extern WRITE_HANDLER ( pc1512_videoram_w );


//internal use
void pc_cga_cursor(CRTC6845_CURSOR *cursor);
