#include "includes/pc_video.h"

/*void pc_mda_init(void);*/

#if 0
	// cutted from some aga char rom
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("mda.chr",     0x00000, 0x01000, CRC(ac1686f3))
#endif

void pc_mda_init_video(struct _CRTC6845 *crtc);
void pc_mda_europc_init(struct _CRTC6845 *crtc);

void pc_mda_timer(void);

VIDEO_START ( pc_mda );
pc_video_update_proc pc_mda_choosevideomode(int *xfactor, int *yfactor);

WRITE_HANDLER ( pc_MDA_w );
READ_HANDLER ( pc_MDA_r );

extern unsigned char mda_palette[4][3];
extern struct GfxLayout pc_mda_charlayout;
extern struct GfxLayout pc_mda_gfxlayout_1bpp;
extern struct GfxDecodeInfo pc_mda_gfxdecodeinfo[];
extern unsigned short mda_colortable[256*2+1*2];

PALETTE_INIT( pc_mda );

//internal use
void pc_mda_cursor(CRTC6845_CURSOR *cursor);
