/***************************************************************************

  Color Graphics Adapter (CGA) section

***************************************************************************/
#include "driver.h"
//#include "vidhrdw/generic.h"
#include "includes/pc.h"

#include "includes/pc_cga.h"
#include "includes/state.h"

#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */

#if VERBOSE_CGA
#define CGA_LOG(n,m,a) LOG(VERBOSE_CGA,n,m,a)
#else
#define CGA_LOG(n,m,a)
#endif

#define CGA_HTOTAL  CGA_crtc[HTOTAL]
#define CGA_HDISP   CGA_crtc[HDISP]
#define CGA_HSYNCP  CGA_crtc[HSYNCP]
#define CGA_HSYNCW  CGA_crtc[HSYNCW]

#define CGA_VTOTAL  CGA_crtc[VTOTAL]
#define CGA_VTADJ   CGA_crtc[VTADJ]
#define CGA_VDISP   CGA_crtc[VDISP]
#define CGA_VSYNCW  CGA_crtc[VSYNCW]

#define CGA_INTLACE CGA_crtc[INTLACE]
#define CGA_SCNLINE CGA_crtc[SCNLINE]

#define CGA_CURTOP  CGA_crtc[CURTOP]
#define CGA_CURBOT  CGA_crtc[CURBOT]

#define CGA_VIDH    CGA_crtc[VIDH]
#define CGA_VIDL    CGA_crtc[VIDL]

#define CGA_CURH    CGA_crtc[CURH]
#define CGA_CURL    CGA_crtc[CURL]

#define CGA_LPENH   CGA_crtc[LPENH]
#define CGA_LPENL   CGA_crtc[LPENL]

static enum { TYPE_CGA, TYPE_PC1512 } type=TYPE_CGA;

static int CGA_reg = 0;
static int CGA_index = 0;
static int CGA_crtc[18+1] = {0, };
static int CGA_size = 0;		/* HDISP * VDISP					*/
static int CGA_base = 0;		/* (VIDH & 0x3f) * 256 + VIDL		*/
static int CGA_cursor = 0;		/* ((CURH & 0x3f) * 256 + CURL) * 2 */
static int CGA_maxscan = 16;	/* (SCNLINE & 0x1f) + 1 			*/
static int CGA_curmode = 0; 	/* CURTOP & 0x60					*/
static int CGA_curminy = 0; 	/* CURTOP & 0x1f					*/
static int CGA_curmaxy = 0; 	/* CURBOT & 0x1f					*/

static UINT8 CGA_border = 0;
static UINT8 CGA_2bpp_attr = 0;

static int pc_blink = 0;
static int pc_framecnt = 0;

/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
static void pc_cga_blink_textcolors(int on)
{
	int i, offs, size;

	if (pc_blink == on) return;

    pc_blink = on;
	offs = (CGA_base * 2) % videoram_size;
	size = CGA_size;

	for (i = 0; i < size; i++)
	{
		if (videoram[offs+1] & 0x80) dirtybuffer[offs+1] = 1;
		if ((offs += 2) == videoram_size) offs = 0;
    }
}

extern void pc_cga_timer(void)
{
	if( ((++pc_framecnt & 63) == 63) ) {
		pc_cga_blink_textcolors(pc_framecnt&64);
	}
}

int pc_cga_vh_start(void)
{
    return generic_vh_start();
}

void pc_cga_vh_stop(void)
{
    generic_vh_stop();
}

WRITE_HANDLER ( pc_cga_videoram_w )
{
	if (videoram[offset] == data) return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

/*	-W	CGA CRT index register	   (CGA/EGA/VGA)
 *		selects which register (0-11h) is to be accessed through 03D5h
 *		Note: this port is read/write on some VGAs
 *		bit7-6: (VGA) reserved (0)
 *		bit5  : (VGA) reserved for testing (0)
 *		bit4-0: selects which register is to be accessed through 03D5h
 */
void pc_cga_index_w(int data)
{
	CGA_LOG(3,"CGA_index_w",(errorlog,"$%02x\n",data));
	CGA_index = data;
	CGA_reg = (data > 17) ? 18 : data;
}

int pc_cga_index_r(void)
{
	int data = CGA_index;
	CGA_LOG(3,"CGA_index_r",(errorlog,"$%02x\n",data));
	return data;
}

/*	RW	CGA CRT data register  (CGA/EGA/VGA) (see #P137)
 *		selected by port 3D4. registers 0C-0F may be read
 *		There are differences in names and some bits functionality
 *		  on EGA, VGA in their native modes, but clones in their
 *		  emulation modes emulate the original 6845 at bit level. The
 *		  default values are for CGA, HGC, CGA only, if not otherwise
 *		  mentioned.
 */
void pc_cga_port_w(int data)
{
	switch (CGA_reg)
	{
		case HTOTAL:
			CGA_LOG(1,"CGA_horz_total_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_HTOTAL = data;
            break;
		case HDISP:
			CGA_LOG(1,"CGA_horz_displayed_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_HDISP = data;
			CGA_size = (int)CGA_HDISP * (int)CGA_VDISP;
            break;
		case HSYNCP:
			CGA_LOG(1,"CGA_horz_sync_pos_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_HSYNCP = data;
            break;
		case HSYNCW:
			CGA_LOG(1,"CGA_horz_sync_width_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_HSYNCW = data;
            break;

		case VTOTAL:
			CGA_LOG(1,"CGA_vert_total_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_VTOTAL = data;
            break;
		case VTADJ:
			CGA_LOG(1,"CGA_vert_total_adj_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_VTADJ = data;
            break;
		case VDISP:
			CGA_LOG(1,"CGA_vert_displayed_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_VDISP = data;
			CGA_size = (int)CGA_HDISP * (int)CGA_VDISP;
            break;
		case VSYNCW:
			CGA_LOG(1,"CGA_vert_sync_width_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_VSYNCW = data;
            break;

		case INTLACE:
			CGA_LOG(1,"CGA_interlace_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_INTLACE = data;
            break;
		case SCNLINE:
			CGA_LOG(1,"CGA_scanline_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_SCNLINE = data;
			CGA_maxscan = ((CGA_SCNLINE & 0x1f) + 1) * 2;
            break;

		case CURTOP:
			CGA_LOG(1,"CGA_cursor_top_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_CURTOP = data;
			CGA_curmode = CGA_CURTOP & 0x60;
			CGA_curminy = (CGA_CURTOP & 0x1f) * 2;
			dirtybuffer[CGA_cursor] = 1;
            break;
		case CURBOT:
			CGA_LOG(1,"CGA_cursor_bottom_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_CURBOT = data;
			CGA_curmaxy = (CGA_CURBOT & 0x1f) * 2;
			dirtybuffer[CGA_cursor] = 1;
            break;

		case VIDH:
			CGA_LOG(1,"CGA_base_high_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
			memset(dirtybuffer,1,videoram_size);
            CGA_VIDH = data;
			CGA_base = (CGA_VIDH*256+CGA_VIDL) % videoram_size;
            break;
		case VIDL:
			CGA_LOG(1,"CGA_base_low_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
			memset(dirtybuffer,1,videoram_size);
            CGA_VIDL = data;
			CGA_base = (CGA_VIDH*256+CGA_VIDL) % videoram_size;
            break;

		case CURH:
			CGA_LOG(2,"CGA_cursor_high_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            CGA_CURH = data;
			CGA_cursor = ((CGA_CURH*256+CGA_CURL)*2) % videoram_size;
			dirtybuffer[CGA_cursor] = 1;
            break;
		case CURL:
			CGA_LOG(2,"CGA_cursor_low_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
			CGA_CURL = data;
			CGA_cursor = ((CGA_CURH*256+CGA_CURL)*2) % videoram_size;
            dirtybuffer[CGA_cursor] = 1;
            break;

		case LPENH: /* this is read only */
			CGA_LOG(1,"CGA_light_pen_high_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            break;
		case LPENL: /* this is read only */
			CGA_LOG(1,"CGA_light_pen_low_w",(errorlog,"$%02x\n",data));
			if (CGA_crtc[CGA_reg] == data) return;
            break;
    }
}

int pc_cga_port_r(void)
{
	int data = 0xff;
	switch (CGA_reg)
	{
		case HTOTAL:
			CGA_LOG(1,"CGA_horz_total_r",(errorlog,"$%02x\n",data));
            break;
		case HDISP:
			CGA_LOG(1,"CGA_horz_displayed_r",(errorlog,"$%02x\n",data));
            break;
		case HSYNCP:
			CGA_LOG(1,"CGA_horz_sync_pos_r",(errorlog,"$%02x\n",data));
            break;
		case HSYNCW:
			CGA_LOG(1,"CGA_horz_sync_width_r",(errorlog,"$%02x\n",data));
            break;

		case VTOTAL:
			CGA_LOG(1,"CGA_vert_total_r",(errorlog,"$%02x\n",data));
            break;
		case VTADJ:
			CGA_LOG(1,"CGA_vert_total_adj_r",(errorlog,"$%02x\n",data));
            break;
		case VDISP:
			CGA_LOG(1,"CGA_vert_displayed_r",(errorlog,"$%02x\n",data));
            break;
		case VSYNCW:
			CGA_LOG(1,"CGA_vert_sync_width_r",(errorlog,"$%02x\n",data));
            break;

		case INTLACE:
			CGA_LOG(1,"CGA_interlace_r",(errorlog,"$%02x\n",data));
            break;
		case SCNLINE:
			CGA_LOG(1,"CGA_scanline_r",(errorlog,"$%02x\n",data));
            break;

		case CURTOP:
			CGA_LOG(1,"CGA_cursor_top_r",(errorlog,"$%02x\n",data));
            break;
		case CURBOT:
			CGA_LOG(1,"CGA_cursor_bottom_r",(errorlog,"$%02x\n",data));
            break;


		case VIDH:
			data = CGA_VIDH;
			CGA_LOG(1,"CGA_base_high_r",(errorlog,"$%02x\n",data));
            break;
		case VIDL:
			data = CGA_VIDL;
			CGA_LOG(1,"CGA_base_low_r",(errorlog,"$%02x\n",data));
            break;

		case CURH:
			data = CGA_CURH;
			CGA_LOG(2,"CGA_cursor_high_r",(errorlog,"$%02x\n",data));
            break;
		case CURL:
			data = CGA_CURL;
			CGA_LOG(2,"CGA_cursor_low_r",(errorlog,"$%02x\n",data));
            break;

		case LPENH:
			data = CGA_LPENH;
			CGA_LOG(1,"CGA_light_pen_high_r",(errorlog,"$%02x\n",data));
            break;
		case LPENL:
			data = CGA_LPENL;
			CGA_LOG(1,"CGA_light_pen_low_r",(errorlog,"$%02x\n",data));
            break;
    }
    return data;
}

/*
 *	rW	CGA mode control register (see #P138)
 */
void pc_cga_mode_control_w(int data)
{
	CGA_LOG(1,"CGA_mode_control_w",(errorlog, "$%02x: colums %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
	if ((pc_port[0x3d8] ^ data) & 0x3b)    /* text/gfx/width change */
		memset(dirtybuffer, 1, videoram_size);
    pc_port[0x3d8] = data;
}

int pc_cga_mode_control_r(void)
{
    int data = pc_port[0x3d8];
	CGA_LOG(1,"CGA_mode_control_r",(errorlog, "$%02x: colums %d, gfx %d, hires %d, blink %d\n",
        data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
    return data;
}

/*
 *	?W	reserved for color select register on color adapter
 */
void pc_cga_color_select_w(int data)
{
	UINT8 r, g, b;
	CGA_LOG(1,"CGA_color_select_w",(errorlog, "$%02x\n", data));
	if( pc_port[0x3d9] == data )
		return;
	pc_port[0x3d9] = data;
	memset(dirtybuffer, 1, videoram_size);
    CGA_2bpp_attr = (data & 0x30) >> 4;
	if( (CGA_border ^ data) & 0x0f )
	{
		CGA_border = data & 0x0f;
		b = (CGA_border & 0x01) ? 0x7f : 0;
		g = (CGA_border & 0x02) ? 0x7f : 0;
		r = (CGA_border & 0x04) ? 0x7f : 0;
		if (CGA_border & 0x08)
		{
			r <<= 1;
			g <<= 1;
			b <<= 1;
		}
		osd_modify_pen(Machine->pens[16], r, g, b);
	}
}

int pc_cga_color_select_r(void)
{
	int data = pc_port[0x3d9];
	CGA_LOG(1,"CGA_color_select_w",(errorlog, "$%02x\n", data));
	return data;
}

/*
 *	-W	(mono EGA/mono VGA) feature control register
 *		(see PORT 03DAh-W for details; VGA, see PORT 03CAh-R)
 */
void pc_cga_feature_control_w(int data)
{
	CGA_LOG(1,"CGA_feature_control_w",(errorlog, "$%02x\n", data));
	pc_port[0x3da] = data;
}

/*
 * -W  light pen strobe reset (on any value)
 */
void pc_cga_lightpen_strobe_w(int data)
{
	CGA_LOG(1,"CGA_lightpen_strobe_w",(errorlog, "$%02x\n", data));
}


/*	Bitfields for CGA status register:
 *	Bit(s)	Description (Table P179)
 *	7-6 not used
 *	5-4 color EGA, color ET4000: diagnose video display feedback, select
 *		from color plane enable
 *	3	in vertical retrace
 *	2	(CGA,color EGA) light pen switch is off
 *		(MCGA,color ET4000) reserved (0)
 *	1	(CGA,color EGA) positive edge from light pen has set trigger
 *		(MCGA,color ET4000) reserved (0)
 *	0	horizontal retrace in progress
 *		=0	do not use memory
 *		=1	memory access without interfering with display
 *		(Genoa SuperEGA) horizontal or vertical retrace
 */
int pc_cga_status_r(void)
{
	static int cga_hsync = 0;
	int data = (input_port_0_r(0) & 0x08) | cga_hsync;
	cga_hsync ^= 0x01;
	return data;
}

/*************************************************************************
 *
 *		CGA
 *		color graphics adapter
 *
 *************************************************************************/
WRITE_HANDLER ( pc_CGA_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			pc_cga_index_w(data);
			break;
		case 1: case 3: case 5: case 7:
			pc_cga_port_w(data);
			break;
		case 8:
			pc_cga_mode_control_w(data);
			break;
		case 9:
			pc_cga_color_select_w(data);
			break;
		case 10:
			pc_cga_feature_control_w(data);
			break;
		case 11:
			pc_cga_lightpen_strobe_w(data);
			break;
	}
}

READ_HANDLER ( pc_CGA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = pc_cga_index_r();
			break;
		case 1: case 3: case 5: case 7:
			data = pc_cga_port_r();
			break;
		case 8:
			data = pc_cga_mode_control_r();
			break;
		case 9:
			/* -W set lightpen flipflop */
			break;
		case 10:
			data = pc_cga_status_r();
			break;
		case 11:
			/* -W lightpen strobe reset */
			break;
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
static void cga_text_40_inten(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = CGA_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (CGA_base * 2) % videoram_size;
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = videoram[offs], attr = videoram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 16 - 1;
			r.max_y = sy + CGA_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if( offs == CGA_cursor && CGA_curmode != 0x20 )
				{
					if( CGA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + CGA_curminy < r.max_y )
							r.min_y = sy + CGA_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + CGA_curmaxy < r.max_y )
							r.max_y = sy + CGA_curmaxy;
						drawgfx(bitmap,Machine->gfx[1],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
                    dirtybuffer[offs] = 1;
                }
			}
		}
		if( (sx += 16) == (CGA_HDISP * 16) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) with high intensity bg.
  The character cell size is 8x8
***************************************************************************/
static void cga_text_80_inten(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = CGA_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
	   accordingly. */
	offs = (CGA_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = videoram[offs], attr = videoram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if( offs == CGA_cursor && CGA_curmode != 0x20 )
				{
					if( CGA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + CGA_curminy < r.max_y )
							r.min_y = sy + CGA_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + CGA_curmaxy < r.max_y )
							r.max_y = sy + CGA_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
					dirtybuffer[offs] = 1;
				}
			}
		}
		if ((sx += 8) == (CGA_HDISP * 8))
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/
static void cga_text_40_blink(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = CGA_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (CGA_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = videoram[offs], attr = videoram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			if (attr & 0x80)	/* blinking ? */
			{
				if (pc_blink)
					attr = (attr & 0x70) | ((attr & 0x70) >> 4);
				else
					attr = attr & 0x7f;
            }

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 16 - 1;
			r.max_y = sy + CGA_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if (offs == CGA_cursor && CGA_curmode != 0x20)
				{
					if( CGA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + CGA_curminy < r.max_y )
							r.min_y = sy + CGA_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + CGA_curmaxy < r.max_y )
							r.max_y = sy + CGA_curmaxy;
						drawgfx(bitmap,Machine->gfx[1],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
                    dirtybuffer[offs] = 1;
				}
			}
		}
		if( (sx += 16) == (CGA_HDISP * 16) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking colors.
  The character cell size is 8x8
***************************************************************************/
static void cga_text_80_blink(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs, size = CGA_size;

	/* for every character in the Video RAM, check if it or its
	   attribute has been modified since last time and update it
       accordingly. */
	offs = (CGA_base * 2) % videoram_size;
	for (i = 0, sx = 0, sy = 0; i < size; i++)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
            struct rectangle r;
			int code = videoram[offs], attr = videoram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			if (attr & 0x80)	/* blinking ? */
			{
				if (pc_blink)
					attr = (attr & 0x70) | ((attr & 0x70) >> 4);
				else
					attr = attr & 0x7f;
            }

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan - 1;
			if( DOCLIP(&r) )
			{
				/* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if( offs == CGA_cursor && CGA_curmode != 0x20 )
				{
					if( CGA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + CGA_curminy < r.max_y )
							r.min_y = sy + CGA_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + CGA_curmaxy < r.max_y )
							r.max_y = sy + CGA_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,7,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
                    dirtybuffer[offs] = 1;
                }
			}
		}
		if( (sx += 8) == (CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( (offs += 2) == videoram_size)
			offs = 0;
    }
}

/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  The cell size is 2x1 (double width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/
static void cga_gfx_2bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs = (CGA_base&0x1fff)*2, size = CGA_size * 2;

	/* for every code in the Video RAM, check if it been modified
	   since last time and update it accordingly. */

	/* first draw the even scanlines */
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[3], code, CGA_2bpp_attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( ++offs == 0x2000 )
			offs = 0;
    }

	/* now draw the odd scanlines */
	offs = ((CGA_base&0x1fff)*2)|0x2000;
	for( i = 0, sx = 0, sy = CGA_maxscan / 2; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[3], code, CGA_2bpp_attr, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( ++offs == 0x4000 )
			offs = 0x2000;
    }
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/
static void cga_gfx_1bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs = CGA_base, size = CGA_size * 2;

	/* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */
	/* first draw the even scanlines */
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[2], code, 0, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	/* now draw the odd scanlines */
	offs = (CGA_base + 0x2000) % videoram_size;
	for( i = 0, sx = 0, sy = CGA_maxscan / 2; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + CGA_maxscan / 2 - 1;
			if( DOCLIP(&r) )
				drawgfx(bitmap, Machine->gfx[2], code, 0, 0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }
}

// amstrad pc1512 video hardware
// mapping of the 4 planes into videoram
// (text data should be readable at videoram+0)
static const int videoram_offset[4]= { 0xc000, 0x8000, 0x4000, 0 };
INLINE void pc1512_plot_unit(struct osd_bitmap *bitmap, 
							 int x, int y, int offs)
{
	int color, values[4];
	int i;

	values[0]=videoram[offs|videoram_offset[0]]; // red
	values[1]=videoram[offs|videoram_offset[1]]<<1; // green
	values[2]=videoram[offs|videoram_offset[2]]<<2; // blue
	values[3]=videoram[offs|videoram_offset[3]]<<3; // intensity

	for (i=7; i>=0; i--) {
		color=(values[0]&1)|(values[1]&2)|(values[2]&4)|(values[3]&8);
		plot_pixel(bitmap, x+i, y, Machine->pens[color]);
		plot_pixel(bitmap, x+i, y+1, Machine->pens[color]);
		values[0]>>=1;
		values[1]>>=1;
		values[2]>>=1;
		values[3]>>=1;
	}
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/
static void pc1512_gfx_4bpp(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs = CGA_base, size = CGA_size * 2;

	/* for every code in the Video RAM, check if it been modified
       since last time and update it accordingly. */
	/* first draw the even scanlines */
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            dirtybuffer[offs] = 0;


			pc1512_plot_unit(bitmap, sx, sy, offs);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	/* now draw the odd scanlines */
	offs = (CGA_base + 0x2000) % videoram_size;
	for( i = 0, sx = 0, sy = CGA_maxscan / 2; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            dirtybuffer[offs] = 0;

			pc1512_plot_unit(bitmap, sx, sy, offs);
		}
		if( (sx += 8) == (2 * CGA_HDISP * 8) )
		{
			sx = 0;
			sy += CGA_maxscan;
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
void pc_cga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	static int video_active = 0;

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
			cga_text_40_inten(bitmap);
			break;
		case 0x09:
			video_active = 10;
			cga_text_80_inten(bitmap);
			break;
		case 0x28:
			video_active = 10;
			cga_text_40_blink(bitmap);
			break;
		case 0x29:
			video_active = 10;
			cga_text_80_blink(bitmap);
			break;
        case 0x0a: case 0x0b: case 0x2a: case 0x2b:
			video_active = 10;
			cga_gfx_2bpp(bitmap);
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
			video_active = 10;
			if (type==TYPE_PC1512) {
				pc1512_gfx_4bpp(bitmap);
			} else {
				cga_gfx_1bpp(bitmap);
			}
			break;

        default:
			if (video_active && --video_active == 0)
				fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
    }

//	state_display(bitmap);
}

static struct {
	UINT8 write, read;
} pc1512;

extern WRITE_HANDLER ( pc1512_w )
{
	switch (offset) {
	case 0xd: pc1512.write=data;break;
	case 0xe: pc1512.read=data;cpu_setbank(1,videoram+videoram_offset[data&3]);break;
	default: pc_CGA_w(offset,data);
	}
}

extern READ_HANDLER ( pc1512_r )
{
	int data;
	switch (offset) {
	case 0xd: data=pc1512.write;break;
	case 0xe: data=pc1512.read;break;
	default: data=pc_CGA_r(offset);
	}
	return data;
}


extern WRITE_HANDLER ( pc1512_videoram_w )
{
	if (pc1512.write&1) videoram[offset+videoram_offset[0]]=data; //blue plane
	if (pc1512.write&2) videoram[offset+videoram_offset[1]]=data; //green
	if (pc1512.write&4) videoram[offset+videoram_offset[2]]=data; //red
	if (pc1512.write&8) videoram[offset+videoram_offset[3]]=data; //intensity (text, 4color)
	dirtybuffer[offset]=1;
}

extern int	pc1512_vh_start(void)
{
	type=TYPE_PC1512;
	videoram=(UINT8*)malloc(0x10000);
	if (videoram==0) return 1;
	videoram_size=0x4000; //! used in cga this way, size of plain memory in 1 bank
	cpu_setbank(1,videoram+videoram_offset[0]);
	pc1512.write=0xf;
	pc1512.read=0;
	return pc_cga_vh_start();
}

extern void pc1512_vh_stop(void)
{
	free(videoram);
	pc_cga_vh_stop();
}

extern void pc1512_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	pc_cga_vh_screenrefresh(bitmap, full_refresh);
}
