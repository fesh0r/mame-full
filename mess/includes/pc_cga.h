#include "driver.h"
#include "includes/crtc6845.h"

#if 0
	// cutted from some aga char rom
	// 256 8x8 thick chars
	// 256 8x8 thin chars
    ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
	// first font of above
    ROM_LOAD("cga2.chr", 0x00000, 0x800, 0xa362ffe6)
#endif


extern unsigned char cga_palette[16][3];
extern unsigned short cga_colortable[256*2+16*2+2*4];
extern struct GfxLayout CGA_charlayout;
extern struct GfxLayout CGA_gfxlayout_1bpp;
extern struct GfxLayout CGA_gfxlayout_2bpp;
extern struct GfxDecodeInfo CGA_gfxdecodeinfo[];
extern PALETTE_INIT( pc_cga );

extern void pc_cga_init_video(struct _CRTC6845 *crtc);

extern VIDEO_START( pc_cga );
extern VIDEO_UPDATE( pc_cga );

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

// has a special 640x200 in 16 color mode, 4 banks at 0xb8000
extern VIDEO_START( pc1512 );
extern VIDEO_UPDATE( pc1512 );

extern WRITE_HANDLER ( pc1512_w );
extern READ_HANDLER ( pc1512_r );
extern WRITE_HANDLER ( pc1512_videoram_w );

//internal use
void pc_cga_cursor(CRTC6845_CURSOR *cursor);
