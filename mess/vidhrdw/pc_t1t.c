/***************************************************************************

  IBM PC junior
  Tandy 1000 Graphics Adapter (T1T) section

***************************************************************************/
#include "vidhrdw/generic.h"

#include "includes/crtc6845.h"
#include "includes/pc_cga.h" // cga monitor palette
#include "includes/pc_t1t.h"
#include "includes/state.h"

#define VERBOSE_T1T 1		/* T1T (Tandy 1000 Graphics Adapter) */

#if VERBOSE_T1T
#define T1T_LOG(N,M,A) \
	if(VERBOSE_T1T>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define T1T_LOG(n,m,a)
#endif

/*
  vga is used in this document for a special video gate array
  not identical to the famous graphics adapter!
*/

static struct { 
	struct _CRTC6845 *crtc;

	UINT8 mode_control, color_select;
	UINT8 status;

	int full_refresh;
	
	// used in tandy1000hx; used in pcjr???
	struct {
		UINT8 index;
		UINT8 data[0x20];
		/* see vgadoc
		   0 mode control 1
		   1 palette mask
		   2 border color
		   3 mode control 2
		   4 reset
		   0x10-0x1f palette registers 
		*/
	} reg;

	UINT8 bank;

	int pc_blink;
	int pc_framecnt;

	UINT8 *displayram;
} pcjr = { 0 };

/* crtc address line allow only decoding of 8kbyte memory
   so are in graphics mode 2 or 4 of such banks distinquished
   by line */

extern void pc_t1t_reset(void)
{
	videoram=NULL;
	pcjr.bank=0;
}


/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
void pc_t1t_blink_textcolors(int on)
{
	int i, offs, size;

	if (!videoram) return;
	if (pcjr.pc_blink == on) return;

    pcjr.pc_blink = on;
	offs = (crtc6845_get_start(pcjr.crtc)* 2) % videoram_size;
	size = crtc6845_get_char_columns(pcjr.crtc)*crtc6845_get_char_lines(pcjr.crtc);

	for (i = 0; i < size; i++)
	{
		if (videoram[offs+1] & 0x80)
			dirtybuffer[offs+1] = 1;
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

extern void pc_t1t_timer(void)
{
	if( ((++pcjr.pc_framecnt & 63) == 63) ) {
		pc_t1t_blink_textcolors(pcjr.pc_framecnt&64);
	}
}

void pc_t1t_cursor(CRTC6845_CURSOR *cursor)
{
	dirtybuffer[cursor->pos*2]=1;
}

static CRTC6845_CONFIG config= { 14318180 /*?*/, pc_t1t_cursor };


int pc_t1t_vh_start(void)
{
	videoram_size = 0x8000;

	pcjr.crtc=crtc6845;
	crtc6845_init(pcjr.crtc, &config);

    return generic_vh_start();
}

void pc_t1t_vh_stop(void)
{
    generic_vh_stop();
}

WRITE_HANDLER ( pc_t1t_videoram_w )
{
	if( !videoram )
		return;
	if( videoram[offset] == data )
		return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

READ_HANDLER ( pc_t1t_videoram_r )
{
	int data = 0xff;
	if( videoram )
		data = videoram[offset];
	return data;
}

/*
 * 3d8 rW	T1T mode control register (see #P138)
 */
void pc_t1t_mode_control_w(int data)
{
	T1T_LOG(1,"T1T_mode_control_w",("$%02x: colums %d, gfx %d, hires %d, blink %d\n", \
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
	if( (pcjr.mode_control ^ data) & 0x3b )   /* text/gfx/width change */
		pcjr.full_refresh=1;
	pcjr.mode_control = data;
}

int pc_t1t_mode_control_r(void)
{
    int data = pcjr.mode_control;
    return data;
}

/*
 * 3d9 ?W	color select register on color adapter
 */
void pc_t1t_color_select_w(int data)
{
	T1T_LOG(1,"T1T_color_select_w",("$%02x\n", data));
	if (pcjr.color_select == data)
		return;
	pcjr.color_select = data;
	pcjr.full_refresh=1;
}

int pc_t1t_color_select_r(void)
{
	int data = pcjr.color_select;
	T1T_LOG(1,"T1T_color_select_r",("$%02x\n", data));
    return data;
}

/*  Bitfields for T1T status register:
 *  Bit(s)  Description (Table P179)
 *  7-6 not used
 *  5-4 color EGA, color ET4000: diagnose video display feedback, select
 *      from color plane enable
 *  3   in vertical retrace
 *  2   (CGA,color EGA) light pen switch is off
 *      (MCGA,color ET4000) reserved (0)
 *  1   (CGA,color EGA) positive edge from light pen has set trigger
 *      (MCGA,color ET4000) reserved (0)
 *  0   horizontal retrace in progress
 *      =0  do not use memory
 *      =1  memory access without interfering with display
 *      (Genoa SuperEGA) horizontal or vertical retrace
 */
int pc_t1t_status_r(void)
{
    int data = (input_port_0_r(0) & 0x08) | pcjr.status;
    pcjr.status ^= 0x01;
    return data;
}

/*
 * 3db -W	light pen strobe reset (on any value)
 */
void pc_t1t_lightpen_strobe_w(int data)
{
	T1T_LOG(1,"T1T_lightpen_strobe_w",("$%02x\n", data));
//	pc_port[0x3db] = data;
}


/*
 * 3da -W	(mono EGA/mono VGA) feature control register
 *			(see PORT 03DAh-W for details; VGA, see PORT 03CAh-R)
 */
void pc_t1t_vga_index_w(int data)
{
	T1T_LOG(1,"T1T_vga_index_w",("$%02x\n", data));
	pcjr.reg.index = data;
}

void pc_t1t_vga_data_w(int data)
{
    pcjr.reg.data[pcjr.reg.index] = data;

	switch (pcjr.reg.index)
	{
        case 0x00: /* mode control 1 */
            T1T_LOG(1,"T1T_vga_mode_ctrl_1_w",("$%02x\n", data));
            break;
        case 0x01: /* palette mask (bits 3-0) */
            T1T_LOG(1,"T1T_vga_palette_mask_w",("$%02x\n", data));
            break;
        case 0x02: /* border color (bits 3-0) */
            T1T_LOG(1,"T1T_vga_border_color_w",("$%02x\n", data));
            break;
        case 0x03: /* mode control 2 */
            T1T_LOG(1,"T1T_vga_mode_ctrl_2_w",("$%02x\n", data));
            break;
        case 0x04: /* reset register */
            T1T_LOG(1,"T1T_vga_reset_w",("$%02x\n", data));
            break;
        /* palette array */
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			T1T_LOG(1,"T1T_vga_palette_w",("[$%02x] $%02x\n", pcjr.reg.index - 0x10, data));
			palette_change_color(pcjr.reg.index-0x10, 
								 cga_palette[data&0xf][0],
								 cga_palette[data&0xf][1],
								 cga_palette[data&0xf][2]);
            break;
    }
}

int pc_t1t_vga_data_r(void)
{
	int data = pcjr.reg.data[pcjr.reg.index];

	switch (pcjr.reg.index)
	{
        case 0x00: /* mode control 1 */
			T1T_LOG(1,"T1T_vga_mode_ctrl_1_r",("$%02x\n", data));
            break;
        case 0x01: /* palette mask (bits 3-0) */
			T1T_LOG(1,"T1T_vga_palette_mask_r",("$%02x\n", data));
            break;
        case 0x02: /* border color (bits 3-0) */
			T1T_LOG(1,"T1T_vga_border_color_r",("$%02x\n", data));
            break;
        case 0x03: /* mode control 2 */
			T1T_LOG(1,"T1T_vga_mode_ctrl_2_r",("$%02x\n", data));
            break;
        case 0x04: /* reset register */
			T1T_LOG(1,"T1T_vga_reset_r",("$%02x\n", data));
            break;
        /* palette array */
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			T1T_LOG(1,"T1T_vga_palette_r",("[$%02x] $%02x\n", pcjr.reg.index - 0x10, data));
            break;
    }
	return data;
}

/*
 * 3df RW	display bank, access bank, mode
 * bit 0-2	Identifies the page of main memory being displayed in units of 16K.
 *			0: 0K, 1: 16K...7: 112K. In 32K modes (bits 6-7 = 2) only 0,2,4 and
 *			6 are valid, as the next page will also be used.
 *	   3-5	Identifies the page of main memory that can be read/written at B8000h
 *			in units of 16K. 0: 0K, 1: 16K...7: 112K. In 32K modes (bits 6-7 = 2)
 *			only 0,2,4 and 6 are valid, as the next page will also be used.
 *	   6-7	Display mode. 0: Text, 1: 16K graphics mode (4,5,6,8)
 *			2: 32K graphics mode (9,Ah)
 */
void pc_t1t_bank_w(int data)
{
	if (pcjr.bank != data)
	{
		int dram, vram;
		pcjr.bank = data;
	/* it seems the video ram is mapped to the last 128K of main memory */
#if 1
		if ((data&0xc0)==0xc0) { /* needed for lemmings */
			dram = 0x80000 + ((data & 0x06) << 14);
			vram = 0x80000 + ((data & 0x30) << (14-3));
		} else {
			dram = 0x80000 + ((data & 0x07) << 14);
			vram = 0x80000 + ((data & 0x38) << (14-3));
		}
#else
		dram = (data & 0x07) << 14;
		vram = (data & 0x38) << (14-3);
#endif
        videoram = &memory_region(REGION_CPU1)[vram];
		pcjr.displayram = &memory_region(REGION_CPU1)[dram];
        memset(dirtybuffer, 1, videoram_size);
		T1T_LOG(1,"t1t_bank_w",("$%02x: display ram $%05x, video ram $%05x\n", data, dram, vram));
	}
}

int pc_t1t_bank_r(void)
{
	int data = pcjr.bank;
    T1T_LOG(1,"t1t_bank_r",("$%02x\n", data));
    return data;
}

/*************************************************************************
 *
 *		T1T
 *		Tandy 1000 / PCjr
 *
 *************************************************************************/

WRITE_HANDLER ( pc_T1T_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			crtc6845_port_w(pcjr.crtc,0,data);
			break;
		case 1: case 3: case 5: case 7:
			crtc6845_port_w(pcjr.crtc,1,data);
			break;
		case 8:
			pc_t1t_mode_control_w(data);
			break;
		case 9:
			pc_t1t_color_select_w(data);
			break;
		case 10:
			pc_t1t_vga_index_w(data);
            break;
        case 11:
			pc_t1t_lightpen_strobe_w(data);
			break;
		case 12:
            break;
		case 13:
            break;
        case 14:
			pc_t1t_vga_data_w(data);
            break;
        case 15:
			pc_t1t_bank_w(data);
			break;
    }
}

READ_HANDLER ( pc_T1T_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data=crtc6845_port_r(pcjr.crtc,0);
			break;
		case 1: case 3: case 5: case 7:
			data=crtc6845_port_r(pcjr.crtc,1);
			break;
		case 8:
			data = pc_t1t_mode_control_r();
			break;
		case 9:
			data = pc_t1t_color_select_r();
			break;
		case 10:
			data = pc_t1t_status_r();
			break;
		case 11:
			/* -W lightpen strobe reset */
			break;
		case 12:
			break;
		case 13:
            break;
		case 14:
			data = pc_t1t_vga_data_r();
            break;
        case 15:
			data = pc_t1t_bank_r();
    }
	return data;
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
***************************************************************************/
static void t1t_text_inten(struct osd_bitmap *bitmap)
{
	int sx, sy;
	int	offs = crtc6845_get_start(pcjr.crtc)*2;
	int lines = crtc6845_get_char_lines(pcjr.crtc);
	int height = crtc6845_get_char_height(pcjr.crtc);
	int columns = crtc6845_get_char_columns(pcjr.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(pcjr.crtc);
	crtc6845_get_cursor(pcjr.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8) {
			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
				
				drawgfx(bitmap, Machine->gfx[0], pcjr.displayram[offs], pcjr.displayram[offs+1], 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(pcjr.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
				}

				dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
***************************************************************************/
static void t1t_text_blink(struct osd_bitmap *bitmap)
{
	int sx, sy;
	int	offs = crtc6845_get_start(pcjr.crtc)*2;
	int lines = crtc6845_get_char_lines(pcjr.crtc);
	int height = crtc6845_get_char_height(pcjr.crtc);
	int columns = crtc6845_get_char_columns(pcjr.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(pcjr.crtc);
	crtc6845_get_cursor(pcjr.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8) {

			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
				
				int attr = pcjr.displayram[offs+1];
				
				if (attr & 0x80)	/* blinking ? */
				{
					if (pcjr.pc_blink)
						attr = (attr & 0x70) | ((attr & 0x70) >> 4);
					else
						attr = attr & 0x7f;
				}

				drawgfx(bitmap, Machine->gfx[0], pcjr.displayram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(pcjr.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
				}

				dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}

/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  The cell size is 2x1 (double width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Even scanlines are from T1T_base + 0x0000, odd from T1T_base + 0x2000
***************************************************************************/
static void t1t_gfx_2bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(pcjr.crtc)*2;
	int lines = crtc6845_get_char_lines(pcjr.crtc);
	int height = crtc6845_get_char_height(pcjr.crtc);
	int columns = crtc6845_get_char_columns(pcjr.crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++) { 
			switch (sh&3) {
			case 0: // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], (pcjr.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 1:
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], (pcjr.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 2:
				for (i=offs|0x4000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x4000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], (pcjr.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 3:
				for (i=offs|0x6000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x6000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], (pcjr.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			}
		}
	}
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from T1T_base + 0x0000, odd from T1T_base + 0x2000
***************************************************************************/
static void t1t_gfx_1bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(pcjr.crtc)*2;
	int lines = crtc6845_get_char_lines(pcjr.crtc);
	int height = crtc6845_get_char_height(pcjr.crtc);
	int columns = crtc6845_get_char_columns(pcjr.crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++, offs|=0x2000) { // char line 0 used as a12 line in graphic mode
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[1], pcjr.displayram[i], pcjr.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
			} else {
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[1], pcjr.displayram[i], pcjr.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}

/***************************************************************************
  Draw graphics mode with 160x200 pixels (default) and 4 bits/pixel.
  The cell size is 4x1 (quadruple width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Scanlines (scanline % 4) are from CGA_base + 0x0000,
  CGA_base + 0x2000
***************************************************************************/
static void t1t_gfx_4bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(pcjr.crtc)*2;
	int lines = crtc6845_get_char_lines(pcjr.crtc);
	int height = crtc6845_get_char_height(pcjr.crtc);
	int columns = crtc6845_get_char_columns(pcjr.crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++) { 
			switch (sh&3) {
			case 0: // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[3], pcjr.displayram[i], 0,
								0,0,sx*2,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 1:
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[3], pcjr.displayram[i], 0,
								0,0,sx*2,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 2:
				for (i=offs|0x4000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x4000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[3], pcjr.displayram[i], 0,
								0,0,sx*2,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			case 3:
				for (i=offs|0x6000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x6000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[3], pcjr.displayram[i], 0,
								0,0,sx*2,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
				break;
			}
		}
	}
}

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void pc_t1t_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	static int video_active = 0;
	static int width=0, height=0;
	int w,h;

	if (!pcjr.displayram) return;

	if( palette_recalc() )
		full_refresh = 1;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	if( crtc6845_do_full_refresh(pcjr.crtc)||pcjr.full_refresh||full_refresh )
	{
		pcjr.full_refresh=0;
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
		video_active = 0;
    }

	w=crtc6845_get_char_columns(pcjr.crtc)*8;
	h=crtc6845_get_char_height(pcjr.crtc)*crtc6845_get_char_lines(pcjr.crtc);
	switch( pcjr.mode_control & 0x3b )	/* text and gfx modes */
	{
		case 0x08:
			video_active = 10;
			t1t_text_inten(bitmap); // column
			break;
		case 0x09:
			video_active = 10;
			t1t_text_inten(bitmap);
			break;
		case 0x28:
			video_active = 10;
			t1t_text_blink(bitmap); // 40 column
			break;
		case 0x29:
			video_active = 10;
			t1t_text_blink(bitmap);
			break;
        case 0x0a: case 0x0b: case 0x2a: case 0x2b:
			video_active = 10;
			switch (pcjr.bank & 0xc0)
			{
			case 0x00:	/* hmm.. text in graphics? */
			case 0x40: t1t_gfx_2bpp(bitmap); break;
			case 0x80: t1t_gfx_4bpp(bitmap);w/=2; break; //160
			case 0xc0: t1t_gfx_4bpp(bitmap);w/=2; break; //320
			}
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
			video_active = 10;
			switch (pcjr.bank & 0xc0)
			{
			case 0x00:	/* hmm.. text in graphics? */
			case 0x40: t1t_gfx_1bpp(bitmap);w*=2; break;
			case 0x80:
			case 0xc0: t1t_gfx_2bpp(bitmap); break; //640
            }
			break;

        default:
			if( video_active && --video_active == 0 )
				fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
    }

	if ( (width!=w)||(height!=h) ) {
		width=w;
		height=h;
		if (width>Machine->visible_area.max_x) width=Machine->visible_area.max_x+1;
		if (height>Machine->visible_area.max_y) height=Machine->visible_area.max_y+1;
		if ((width>100)&&(height>100))
			osd_set_visible_area(0,width-1,0, height-1);
		else logerror("video %d %d\n",width, height);
	}

//	state_display(bitmap);
}
