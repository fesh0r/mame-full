#include "driver.h"
#include "includes/crtc6845.h"
#include "includes/pc_video.h"


extern unsigned char cga_palette[16][3];
extern unsigned short cga_colortable[256*2+16*2+2*4];
extern struct GfxLayout CGA_charlayout;
extern struct GfxLayout CGA_gfxlayout_1bpp;
extern struct GfxLayout CGA_gfxlayout_2bpp;

MACHINE_DRIVER_EXTERN( pcvideo_cga );

/* has a special 640x200 in 16 color mode, 4 banks at 0xb8000 */
MACHINE_DRIVER_EXTERN( pcvideo_pc1512 );


void pc_cga_init_video(struct _CRTC6845 *crtc);

pc_video_update_proc pc_cga_choosevideomode(int *xfactor, int *yfactor);

/* call this 240 times per second */
void pc_cga_timer(void);

WRITE_HANDLER ( pc_cga_videoram_w );
WRITE_HANDLER ( pc_CGA_w );
READ_HANDLER ( pc_CGA_r );

VIDEO_START( pc1512 );

WRITE_HANDLER ( pc1512_w );
READ_HANDLER ( pc1512_r );
WRITE_HANDLER ( pc1512_videoram_w );

//internal use
void pc_cga_cursor(CRTC6845_CURSOR *cursor);
