/***************************************************************************

  Tandy 1000 Graphics Adapter (T1T) section

***************************************************************************/
#include "mess/includes/pc.h"

#include "mess/includes/pc_t1t.h"

/* I think pc junior and the first tandy series
 have a graphics adapter with 4 digital lines to the
 monitor (16 color palette)
 dynamic palette management is not necessary here !

 vga registers???
 vga registers should be removed
 later tandys had real ega and vga cards,
 not a mixture between tandy1000 and ega,vga !??? */

/* PeT 13 May 2000
 changed osd_modify_pen to palette_change_color
 fixed a little bug in the vga color registers */

/* crtc address line allow only decoding of 8kbyte memory
   so are in graphics mode 2 or 4 of such banks distinquished
   by line */

#define T1T_HTOTAL	T1T_crtc[HTOTAL]
#define T1T_HDISP	T1T_crtc[HDISP]
#define T1T_HSYNCP	T1T_crtc[HSYNCP]
#define T1T_HSYNCW	T1T_crtc[HSYNCW]

#define T1T_VTOTAL	T1T_crtc[VTOTAL]
#define T1T_VTADJ	T1T_crtc[VTADJ]
#define T1T_VDISP	T1T_crtc[VDISP]
#define T1T_VSYNCW	T1T_crtc[VSYNCW]

#define T1T_INTLACE T1T_crtc[INTLACE]
#define T1T_SCNLINE T1T_crtc[SCNLINE]

#define T1T_CURTOP	T1T_crtc[CURTOP]
#define T1T_CURBOT	T1T_crtc[CURBOT]

#define T1T_VIDH	T1T_crtc[VIDH]
#define T1T_VIDL	T1T_crtc[VIDL]

#define T1T_CURH	T1T_crtc[CURH]
#define T1T_CURL	T1T_crtc[CURL]

#define T1T_LPENH	T1T_crtc[LPENH]
#define T1T_LPENL	T1T_crtc[LPENL]

static int T1T_reg = 0;
static int T1T_index = 0;
static int T1T_crtc[18+1] = {0, };
static int T1T_vga_index = 0;
static int T1T_vga[32] = {0, }; /* video gate array */
static int T1T_size = 0;		/* HDISP * VDISP					*/
static int T1T_base = 0;		/* (VIDH & 0x3f) * 256 + VIDL		*/
static int T1T_cursor = 0;		/* ((CURH & 0x3f) * 256 + CURL) * 2 */
static int T1T_maxscan = 8; 	/* (SCNLINE & 0x1f) + 1 			*/
static int T1T_curmode = 0; 	/* CURTOP & 0x60					*/
static int T1T_curminy = 0; 	/* CURTOP & 0x1f					*/
static int T1T_curmaxy = 0; 	/* CURBOT & 0x1f					*/

static UINT8 T1T_border = 0;
static UINT8 T1T_2bpp_attr = 0;

static int pc_blink = 0;
static int pc_framecnt = 0;

static UINT8 *displayram = 0;

/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
void pc_t1t_blink_textcolors(int on)
{
	int i, offs, size;

	if (pc_blink == on) return;

    pc_blink = on;
	offs = (T1T_base * 2) % videoram_size;
	size = T1T_size;

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
	if( ((++pc_framecnt & 63) == 63) ) {
		pc_t1t_blink_textcolors(pc_framecnt&64);
	}
}

int pc_t1t_vh_start(void)
{
	videoram_size = 0x8000;
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

/* 3d0 -W   CRT index register
 * 3d2		selects which register (0-11h) is to be accessed through 03D5h
 * 3d4		Note: this port is read/write on some VGAs
 * 3d6		bit7-6: (VGA) reserved (0)
 *			bit5  : (VGA) reserved for testing (0)
 *			bit4-0: selects which register is to be accessed through 03D5h
 */
void pc_t1t_index_w(int data)
{
	T1T_LOG(3,"T1T_index_w",("$%02x\n",data));
	T1T_index = data;
	T1T_reg = (data > 17) ? 18 : data;
}

int pc_t1t_index_r(void)
{
	int data = T1T_index;
	T1T_LOG(3,"T1T_index_r",("$%02x\n",data));
	return data;
}
/*
 * 3d1 RW	CRT data register
 * 3d3		selected by port 3D4. registers 0C-0F may be read
 * 3d5		There are differences in names and some bits functionality
 * 3d7		on EGA, VGA in their native modes, but clones in their
 *			emulation modes emulate the original 6845 at bit level. The
 *			default values are for T1T, HGC, T1T only, if not otherwise
 *			mentioned.
 */
void pc_t1t_port_w(int data)
{
	switch( T1T_reg )
	{
		case HTOTAL:
			T1T_LOG(1,"T1T_horz_total_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_HTOTAL = data;
            break;
		case HDISP:
			T1T_LOG(1,"T1T_horz_displayed_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_HDISP = data;
			T1T_size = (int)T1T_HDISP * (int)T1T_VDISP;
            break;
		case HSYNCP:
			T1T_LOG(1,"T1T_horz_sync_pos_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_HSYNCP = data;
            break;
		case HSYNCW:
			T1T_LOG(1,"T1T_horz_sync_width_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_HSYNCW = data;
            break;

		case VTOTAL:
			T1T_LOG(1,"T1T_vert_total_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VTOTAL = data;
            break;
		case VTADJ:
			T1T_LOG(1,"T1T_vert_total_adj_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VTADJ = data;
            break;
		case VDISP:
			T1T_LOG(1,"T1T_vert_displayed_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VDISP = data;
			T1T_size = (int)T1T_HDISP * (int)T1T_VDISP;
            break;
		case VSYNCW:
			T1T_LOG(1,"T1T_vert_sync_width_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VSYNCW = data;
            break;

		case INTLACE:
			T1T_LOG(1,"T1T_interlace_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_INTLACE = data;
            break;
		case SCNLINE:
			T1T_LOG(1,"T1T_scanline_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_SCNLINE = data;
			T1T_maxscan = ((T1T_SCNLINE & 0x1f) + 1) * 2;
			/* dirty hack to avoid using a (non existent) 8x9 font */
            if (T1T_maxscan == 18) T1T_maxscan = 16;
            break;

		case CURTOP:
			T1T_LOG(1,"T1T_cursor_top_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_CURTOP = data;
			T1T_curmode = T1T_CURTOP & 0x60;
			T1T_curminy = (T1T_CURTOP & 0x1f) * 2;
			dirtybuffer[T1T_cursor] = 1;
            break;
		case CURBOT:
			T1T_LOG(1,"T1T_cursor_bottom_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_CURBOT = data;
			T1T_curmaxy = (T1T_CURBOT & 0x1f) * 2;
			dirtybuffer[T1T_cursor] = 1;
            break;

		case VIDH:
			T1T_LOG(1,"T1T_base_high_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VIDH = data;
			T1T_base = (T1T_VIDH*256+T1T_VIDL) % videoram_size;
            break;
		case VIDL:
			T1T_LOG(1,"T1T_base_low_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_VIDL = data;
			T1T_base = (T1T_VIDH*256+T1T_VIDL) % videoram_size;
            break;

		case CURH:
			T1T_LOG(2,"T1T_cursor_high_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_CURH = data;
			T1T_cursor = ((T1T_CURH*256+T1T_CURL)*2) % videoram_size;
			dirtybuffer[T1T_cursor] = 1;
            break;
		case CURL:
			T1T_LOG(2,"T1T_cursor_low_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
			T1T_CURL = data;
			T1T_cursor = ((T1T_CURH*256+T1T_CURL)*2) % videoram_size;
			dirtybuffer[T1T_cursor] = 1;
            break;

		case LPENH: /* this is read only */
			T1T_LOG(1,"T1T_light_pen_high_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
            break;
		case LPENL: /* this is read only */
			T1T_LOG(1,"T1T_light_pen_low_w",("$%02x\n",data));
			if (T1T_crtc[T1T_reg] == data) return;
            break;
    }
}

int pc_t1t_port_r(void)
{
	int data = 0xff;
	switch( T1T_reg )
	{
		case HTOTAL:
			T1T_LOG(1,"T1T_horz_total_r",("$%02x\n",data));
            break;
		case HDISP:
			T1T_LOG(1,"T1T_horz_displayed_r",("$%02x\n",data));
            break;
		case HSYNCP:
			T1T_LOG(1,"T1T_horz_sync_pos_r",("$%02x\n",data));
            break;
		case HSYNCW:
			T1T_LOG(1,"T1T_horz_sync_width_r",("$%02x\n",data));
            break;

		case VTOTAL:
			T1T_LOG(1,"T1T_vert_total_r",("$%02x\n",data));
            break;
		case VTADJ:
			T1T_LOG(1,"T1T_vert_total_adj_r",("$%02x\n",data));
            break;
		case VDISP:
			T1T_LOG(1,"T1T_vert_displayed_r",("$%02x\n",data));
            break;
		case VSYNCW:
			T1T_LOG(1,"T1T_vert_sync_width_r",("$%02x\n",data));
            break;

		case INTLACE:
			T1T_LOG(1,"T1T_interlace_r",("$%02x\n",data));
            break;
		case SCNLINE:
			T1T_LOG(1,"T1T_scanline_r",("$%02x\n",data));
            break;

		case CURTOP:
			T1T_LOG(1,"T1T_cursor_top_r",("$%02x\n",data));
            break;
		case CURBOT:
			T1T_LOG(1,"T1T_cursor_bottom_r",("$%02x\n",data));
            break;


		case VIDH:
			data = T1T_VIDH;
			T1T_LOG(1,"T1T_base_high_r",("$%02x\n",data));
            break;
		case VIDL:
			data = T1T_VIDL;
			T1T_LOG(1,"T1T_base_low_r",("$%02x\n",data));
            break;

		case CURH:
			data = T1T_CURH;
			T1T_LOG(2,"T1T_cursor_high_r",("$%02x\n",data));
            break;
		case CURL:
			data = T1T_CURL;
			T1T_LOG(2,"T1T_cursor_low_r",("$%02x\n",data));
            break;

		case LPENH:
			data = T1T_LPENH;
			T1T_LOG(1,"T1T_light_pen_high_r",("$%02x\n",data));
            break;
		case LPENL:
			data = T1T_LPENL;
			T1T_LOG(1,"T1T_light_pen_low_r",("$%02x\n",data));
            break;
    }
    return data;
}

/*
 * 3d8 rW	T1T mode control register (see #P138)
 */
void pc_t1t_mode_control_w(int data)
{
	T1T_LOG(1,"T1T_mode_control_w",("$%02x: colums %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
	if( (pc_port[0x3d8] ^ data) & 0x3b )   /* text/gfx/width change */
		memset(dirtybuffer, 1, videoram_size);
	pc_port[0x3d8] = data;
}

int pc_t1t_mode_control_r(void)
{
    int data = pc_port[0x3d8];
    return data;
}

/*
 * 3d9 ?W	color select register on color adapter
 */
void pc_t1t_color_select_w(int data)
{
	UINT8 r, g, b;
	T1T_LOG(1,"T1T_color_select_w",("$%02x\n", data));
	if (pc_port[0x3d9] == data)
		return;
	pc_port[0x3d9] = data;
	memset(dirtybuffer, 1, videoram_size);
    T1T_2bpp_attr = (data & 0x30) >> 4;
	if( (T1T_border ^ data) & 0x0f )
	{
		T1T_border = data & 0x0f;
		b = (T1T_border & 0x01) ? 0x7f : 0;
		g = (T1T_border & 0x02) ? 0x7f : 0;
		r = (T1T_border & 0x04) ? 0x7f : 0;
		if( T1T_border & 0x08 )
		{
			r <<= 1;
			g <<= 1;
			b <<= 1;
		}
#if 0
		osd_modify_pen(16, r, g, b);
//		osd_modify_pen(Machine->pens[16], r, g, b);
#else
		palette_change_color(16,r,g,b);
#endif
	}
}

int pc_t1t_color_select_r(void)
{
	int data = pc_port[0x3d9];
	T1T_LOG(1,"T1T_color_select_r",("$%02x\n", data));
    return data;
}

/*
 * 3da -W	(mono EGA/mono VGA) feature control register
 *			(see PORT 03DAh-W for details; VGA, see PORT 03CAh-R)
 */
void pc_t1t_vga_index_w(int data)
{
	T1T_LOG(1,"T1T_vga_index_w",("$%02x\n", data));
	T1T_vga_index = data;
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
    static int t1t_hsync = 0;
    int data = (input_port_0_r(0) & 0x08) | t1t_hsync;
    t1t_hsync ^= 0x01;
    return data;
}

/*
 * 3db -W	light pen strobe reset (on any value)
 */
void pc_t1t_lightpen_strobe_w(int data)
{
	T1T_LOG(1,"T1T_lightpen_strobe_w",("$%02x\n", data));
	pc_port[0x3db] = data;
}


void pc_t1t_vga_data_w(int data)
{
	UINT8 r, g, b;

    T1T_vga[T1T_vga_index] = data;

	switch (T1T_vga_index)
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
			T1T_LOG(1,"T1T_vga_palette_w",("[$%02x] $%02x\n", T1T_vga_index - 0x10, data));
			r = (data & 4) ? 0x7f : 0;
			g = (data & 2) ? 0x7f : 0;
			b = (data & 1) ? 0x7f : 0;
			if (data & 8)
			{
				r <<= 1;
				g <<= 1;
				b <<= 1;
			}
#if 0
			osd_modify_pen(T1T_vga_index - 16, r, g, b);
#else
			palette_change_color(T1T_vga_index - 16, r, g, b);
#endif
            break;
    }
}

int pc_t1t_vga_data_r(void)
{
	int data = T1T_vga[T1T_vga_index];

	switch (T1T_vga_index)
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
			T1T_LOG(1,"T1T_vga_palette_r",("[$%02x] $%02x\n", T1T_vga_index - 0x10, data));
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
	if (pc_port[0x3df] != data)
	{
		int dram, vram;
		pc_port[0x3df] = data;
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
		displayram = &memory_region(REGION_CPU1)[dram];
        memset(dirtybuffer, 1, videoram_size);
		DBG_LOG(1,"t1t_bank_w",("$%02x: display ram $%05x, video ram $%05x\n", data, dram, vram));
	}
}

int pc_t1t_bank_r(void)
{
	int data = pc_port[0x3df];
    DBG_LOG(1,"t1t_bank_r",("$%02x\n", data));
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
			pc_t1t_index_w(data);
			break;
		case 1: case 3: case 5: case 7:
			pc_t1t_port_w(data);
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
			data = pc_t1t_index_r();
			break;
		case 1: case 3: case 5: case 7:
			data = pc_t1t_port_r();
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

INLINE int DOCLIP(struct rectangle *r1)
{
    const struct rectangle *r2 = &Machine->visible_area;
    if (r1->min_x > r2->max_x) return 0;
    if (r1->max_x < r2->min_x) return 0;
    if (r1->min_y > r2->max_y) return 0;
    if (r1->max_y < r2->min_y) return 0;
    if (r1->min_x < r2->min_x) r1->min_x = r2->min_x;
    if (r1->max_x > r2->max_x) r1->max_x = r2->max_x;
    if (r1->min_y < r2->min_y) r1->min_y = r2->min_y;
    if (r1->max_y > r2->max_y) r1->max_y = r2->max_y;
    return 1;
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/
static void t1t_text_40_inten(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (T1T_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = displayram[offs], attr = displayram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 16 - 1;
			r.max_y = sy + T1T_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
				if( offs == T1T_cursor && T1T_curmode != 0x20 )
				{
					if( T1T_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + T1T_curminy < r.max_y )
							r.min_y = sy + T1T_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + T1T_curmaxy < r.max_y )
							r.max_y = sy + T1T_curmaxy;
						drawgfx(bitmap,Machine->gfx[1],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
                    dirtybuffer[offs] = 1;
                }
			}
		}
		if( (sx += 16) == (T1T_HDISP * 16) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) with high intensity bg.
  The character cell size is 8x8
***************************************************************************/
static void t1t_text_80_inten(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
	   accordingly. */
	offs = (T1T_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = displayram[offs], attr = displayram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
                if( offs == T1T_cursor && T1T_curmode != 0x20 )
				{
					if( T1T_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + T1T_curminy < r.max_y )
							r.min_y = sy + T1T_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + T1T_curmaxy < r.max_y )
							r.max_y = sy + T1T_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE, 0);
                    }
                    dirtybuffer[offs] = 1;
				}
			}
		}
		if( (sx += 8) == (T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/
static void t1t_text_40_blink(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (T1T_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = displayram[offs], attr = displayram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			if( attr & 0x80 )	/* blinking ? */
			{
				if( pc_blink )
					attr = (attr & 0x70) | ((attr & 0x70) >> 4);
				else
					attr = attr & 0x7f;
            }

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 16 - 1;
			r.max_y = sy + T1T_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
				if( offs == T1T_cursor && T1T_curmode != 0x20 )
				{
					if( T1T_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + T1T_curminy < r.max_y )
							r.min_y = sy + T1T_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + T1T_curmaxy < r.max_y )
							r.max_y = sy + T1T_curmaxy;
						drawgfx(bitmap,Machine->gfx[1],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE, 0);
                    }
                    dirtybuffer[offs] = 1;
                }
			}
		}
		if( (sx += 16) == (T1T_HDISP * 16) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking colors.
  The character cell size is 8x8
***************************************************************************/
static void t1t_text_80_blink(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (T1T_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = displayram[offs], attr = displayram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			if( attr & 0x80 )	/* blinking ? */
			{
				if( pc_blink )
					attr = (attr & 0x70) | ((attr & 0x70) >> 4);
				else
					attr = attr & 0x7f;
            }

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
                if( offs == T1T_cursor && T1T_curmode != 0x20 )
				{
					if( T1T_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + T1T_curminy < r.max_y )
							r.min_y = sy + T1T_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + T1T_curmaxy < r.max_y )
							r.max_y = sy + T1T_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE, 0);
                    }
                    dirtybuffer[offs] = 1;
                }
			}
		}
		if( (sx += 8) == (T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
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
	int i, sx, sy, offs, size = T1T_size * 2;

	/* for every code in the Video RAM, check if it been modified
	   since last time and update it accordingly. */

	/* first draw the even scanlines */
	offs = T1T_base % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[3], code, T1T_2bpp_attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
		}
		if( (sx += 8) == (2 * T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	/* now draw the odd scanlines */
	offs = (T1T_base + 0x2000) % videoram_size;
	for (i = 0, sx = 0, sy = T1T_maxscan / 2; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[3], code, T1T_2bpp_attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
		}
		if( (sx += 8) == (2 * T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from T1T_base + 0x0000, odd from T1T_base + 0x2000
***************************************************************************/
static void t1t_gfx_1bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size * 2;

	/* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */
	/* first draw the even scanlines */
	offs = T1T_base % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[2], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
		}
		if( (sx += 8) == (2 * T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	/* now draw the odd scanlines */
	offs = (T1T_base + 0x2000) % videoram_size;
	for (i = 0, sx = 0, sy = T1T_maxscan / 2; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[2], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 8) == (2 * T1T_HDISP * 8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw graphics mode with 160x200 pixels (default) and 4 bits/pixel.
  The cell size is 4x1 (quadruple width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Scanlines (scanline % 4) are from CGA_base + 0x0000,
  CGA_base + 0x2000
***************************************************************************/
static void t1t_gfx_4bpp_160(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size * 2;

    /* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */

	offs = T1T_base % videoram_size;
	for (i = 0, sx = 0, sy = 0 * T1T_maxscan/2; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + T1T_maxscan/2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 8) == (T1T_HDISP*8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size ) offs = 0;
    }

	offs = (T1T_base + 0x2000) % videoram_size;
	for (i = 0, sx = 0, sy = 1 * T1T_maxscan/2; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 8) == (T1T_HDISP*8) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) and 4 bits/pixel.
  The cell size is 2x1 (double width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Scanlines (scanline % 4) are from CGA_base + 0x0000,
  CGA_base + 0x2000, CGA_base + 0x4000 resp. CGA_base + 0x6000
***************************************************************************/
static void t1t_gfx_4bpp_320(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size * 2;

    /* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */

	offs = T1T_base &0x1fff;
	for (i = 0, sx = 0, sy = 0 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (2 * T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == 0x2000 ) offs = 0;
    }

	offs = (T1T_base&0x1fff) | 0x2000;
	for (i = 0, sx = 0, sy = 1 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (2 * T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == 0x4000 )
			offs = 0x2000;
    }

	offs = (T1T_base&0x1fff) | 0x4000;
	for (i = 0, sx = 0, sy = 2 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (2 * T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == 0x6000 )
			offs = 0x4000;
    }

	offs = (T1T_base&0x1fff) |0x6000 ;
	for (i = 0, sx = 0, sy = 3 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[5], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (2 * T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == 0x8000 )
			offs = 0x6000;
    }
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default) and 2 bits/pixel.
  The cell size is 1x1 (normal pixels, 1 scanline is the real
  default but up to 32 are possible).
  Scanlines (scanline % 4) are from CGA_base + 0x0000,
  CGA_base + 0x2000, CGA_base + 0x4000 resp. CGA_base + 0x6000
***************************************************************************/
static void t1t_gfx_2bpp_640(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = T1T_size * 2;

    /* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */

	offs = T1T_base % videoram_size;
	for (i = 0, sx = 0, sy = 0 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
			r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[6], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	offs = (T1T_base + 0x2000) % videoram_size;
	for (i = 0, sx = 0, sy = 1 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[6], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	offs = (T1T_base + 0x4000) % videoram_size;
	for (i = 0, sx = 0, sy = 2 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[6], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	offs = (T1T_base + 0x6000) % videoram_size;
	for (i = 0, sx = 0, sy = 3 * T1T_maxscan/4; i < size; i++)
	{
		if (dirtybuffer[offs])
		{
            struct rectangle r;
			int code = displayram[offs];

            dirtybuffer[offs] = 0;

            r.min_x = sx;
            r.min_y = sy;
            r.max_x = sx + 4 - 1;
			r.max_y = sy + T1T_maxscan/4 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[6], code, 0, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
        }
		if( (sx += 4) == (T1T_HDISP*4) )
		{
			sx = 0;
			sy += T1T_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
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

	if (!displayram) return;

	if( palette_recalc() )
		full_refresh = 1;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	if( full_refresh )
	{
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
		video_active = 0;
    }

	switch( pc_port[0x03d8] & 0x3b )	/* text and gfx modes */
	{
		case 0x08:
			video_active = 10;
			t1t_text_40_inten(bitmap);
			break;
		case 0x09:
			video_active = 10;
			t1t_text_80_inten(bitmap);
			break;
		case 0x28:
			video_active = 10;
			t1t_text_40_blink(bitmap);
			break;
		case 0x29:
			video_active = 10;
			t1t_text_80_blink(bitmap);
			break;
        case 0x0a: case 0x0b: case 0x2a: case 0x2b:
			video_active = 10;
			switch (pc_port[0x3df] & 0xc0)
			{
				case 0x00:	/* hmm.. text in graphics? */
				case 0x40: t1t_gfx_2bpp(bitmap); break;
				case 0x80: t1t_gfx_4bpp_160(bitmap); break;
				case 0xc0: t1t_gfx_4bpp_320(bitmap); break;
			}
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
			video_active = 10;
			switch (pc_port[0x3df] & 0xc0)
			{
				case 0x00:	/* hmm.. text in graphics? */
				case 0x40: t1t_gfx_1bpp(bitmap); break;
				case 0x80:
				case 0xc0: t1t_gfx_2bpp_640(bitmap); break;
            }
			break;

        default:
			if( video_active && --video_active == 0 )
				fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
    }
}
