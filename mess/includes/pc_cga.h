#include "driver.h"
#include "includes/crtc6845.h"
#include "includes/pc_video.h"

#define CGA_PALETTE_SETS 2	/* one for colour, one for mono */

extern unsigned char cga_palette[CGA_PALETTE_SETS * 16][3];
extern unsigned short cga_colortable[256*2+16*2+96*4];
extern struct GfxLayout CGA_charlayout;
extern struct GfxLayout CGA_gfxlayout_1bpp;
extern struct GfxLayout CGA_gfxlayout_2bpp;

MACHINE_DRIVER_EXTERN( pcvideo_cga );

/* has a special 640x200 in 16 color mode, 4 banks at 0xb8000 */
MACHINE_DRIVER_EXTERN( pcvideo_pc1512 );

pc_video_update_proc pc_cga_choosevideomode(int *width, int *height, struct crtc6845 *crtc);

/* call this 240 times per second */
void pc_cga_timer(void);

WRITE_HANDLER ( pc_CGA_w );
READ_HANDLER ( pc_CGA_r );

VIDEO_START( pc1512 );

WRITE_HANDLER ( pc1512_w );
READ_HANDLER ( pc1512_r );
WRITE_HANDLER ( pc1512_videoram_w );

//internal use
void pc_cga_cursor(struct crtc6845_cursor *cursor);

/* CGA dipswitch settings. These have to be kept consistent over all systems
 * that use CGA. */

#define CGA_FONT		(input_port_20_r(0)&3)
#define CGA_FONT_0		0
#define CGA_FONT_1		1
#define CGA_FONT_2		2
#define CGA_FONT_3		3

#define CGA_MONITOR		(input_port_20_r(0)&0x1C)
#define CGA_MONITOR_RGB		0x00	/* Colour RGB */
#define CGA_MONITOR_MONO	0x04	/* Greyscale RGB */
#define CGA_MONITOR_COMPOSITE	0x08	/* Colour composite */
#define CGA_MONITOR_TELEVISION	0x0C	/* Television */
#define CGA_MONITOR_LCD		0x10	/* LCD, eg PPC512 */

#define CGA_CHIPSET		(input_port_20_r(0)&0xE0)
#define CGA_CHIPSET_IBM		0x00	/* Original IBM CGA */
#define CGA_CHIPSET_PC1512	0x20	/* PC1512 CGA subset */
#define CGA_CHIPSET_PC200	0x40	/* PC200 in CGA mode */
#define CGA_CHIPSET_ATI		0x60	/* ATI (supports Plantronics) */
#define CGA_CHIPSET_PARADISE	0x80	/* Paradise (used in PC1640) */ 

