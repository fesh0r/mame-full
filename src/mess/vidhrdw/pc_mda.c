/***************************************************************************

  Monochrom Display Adapter (MDA) section

***************************************************************************/
#include "mess/machine/pc.h"
#include "mess/vidhrdw/pc.h"

#define MDA_HTOTAL  MDA_crtc[HTOTAL]
#define MDA_HDISP   MDA_crtc[HDISP]
#define MDA_HSYNCP  MDA_crtc[HSYNCP]
#define MDA_HSYNCW  MDA_crtc[HSYNCW]

#define MDA_VTOTAL  MDA_crtc[VTOTAL]
#define MDA_VTADJ   MDA_crtc[VTADJ]
#define MDA_VDISP   MDA_crtc[VDISP]
#define MDA_VSYNCW  MDA_crtc[VSYNCW]

#define MDA_INTLACE MDA_crtc[INTLACE]
#define MDA_SCNLINE MDA_crtc[SCNLINE]

#define MDA_CURTOP  MDA_crtc[CURTOP]
#define MDA_CURBOT  MDA_crtc[CURBOT]

#define MDA_VIDH    MDA_crtc[VIDH]
#define MDA_VIDL    MDA_crtc[VIDL]

#define MDA_CURH    MDA_crtc[CURH]
#define MDA_CURL    MDA_crtc[CURL]

#define MDA_LPENH   MDA_crtc[LPENH]
#define MDA_LPENL   MDA_crtc[LPENL]

static int MDA_reg = 0;
static int MDA_index = 0;
static int MDA_crtc[18+1] = {0, };
static int MDA_size = 0;		/* HDISP * VDISP					*/
static int MDA_base = 0;		/* (VIDH*256+VIDL) & VIDMASK		*/
static int MDA_cursor = 0;		/* ((CURH*256+CURL)*2) & CURMASK	*/
static int MDA_maxscan = 16;	/* (SCNLINE&0x1f) + 1				*/
static int MDA_curmode = 0; 	/* CURTOP & 0x60					*/
static int MDA_curminy = 0; 	/* CURTOP & 0x1f					*/
static int MDA_curmaxy = 0; 	/* CURBOT & 0x1f					*/

/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

int pc_mda_vh_start(void)
{
    return generic_vh_start();
}

void pc_mda_vh_stop(void)
{
    generic_vh_stop();
}

WRITE_HANDLER ( pc_mda_videoram_w )
{
	if (videoram[offset] == data) return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

/*	-W	MDA CRT index register	   (MDA/mono EGA/mono VGA)
 *		selects which register (0-11h) is to be accessed through 03B5h
 *		Note: this port is read/write on some VGAs
 *		bit7-6: (VGA) reserved (0)
 *		bit5  : (VGA) reserved for testing (0)
 *		bit4-0: selects which register is to be accessed through 03B5h
 */
void pc_mda_index_w(int data)
{
	MDA_LOG(3,"MDA_index_w",(errorlog,"$%02x\n",data));
	MDA_index = data;
	MDA_reg = (data > 17) ? 18 : data;
}

int pc_mda_index_r(void)
{
	int data = MDA_index;
	MDA_LOG(3,"MDA_index_r",(errorlog,"$%02x\n",data));
	return data;
}

/*	RW	MDA CRT data register  (MDA/mono EGA/mono VGA) (see #P137)
 *		selected by port 3B4. registers 0C-0F may be read
 *		Color adapters are at 3D4/3D5, but are mentioned here for
 *		  better overview.
 *		There are differences in names and some bits functionality
 *		  on EGA, VGA in their native modes, but clones in their
 *		  emulation modes emulate the original 6845 at bit level. The
 *		  default values are for MDA, HGC, CGA only, if not otherwise
 *		  mentioned.
 */
void pc_mda_port_w(int data)
{
	switch (MDA_reg)
	{
		case HTOTAL:
			MDA_LOG(1,"MDA_horz_total_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_HTOTAL = data;
            break;
		case HDISP:
			MDA_LOG(1,"MDA_horz_displayed_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_HDISP = data;
			MDA_size = (int)MDA_HDISP * (int)MDA_VDISP;
            break;
		case HSYNCP:
			MDA_LOG(1,"MDA_horz_sync_pos_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_HSYNCP = data;
            break;
		case HSYNCW:
			MDA_LOG(1,"MDA_horz_sync_width_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_HSYNCW = data;
            break;

		case VTOTAL:
			MDA_LOG(1,"MDA_vert_total_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VTOTAL = data;
            break;
		case VTADJ:
			MDA_LOG(1,"MDA_vert_total_adj_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VTADJ = data;
            break;
		case VDISP:
			MDA_LOG(1,"MDA_vert_displayed_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VDISP = data;
			MDA_size = (int)MDA_HDISP * (int)MDA_VDISP;
            break;
		case VSYNCW:
			MDA_LOG(1,"MDA_vert_sync_width_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VSYNCW = data;
            break;

		case INTLACE:
			MDA_LOG(1,"MDA_interlace_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_INTLACE = data;
            break;
		case SCNLINE:
			MDA_LOG(1,"MDA_scanline_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_SCNLINE = data;
			MDA_maxscan = (MDA_SCNLINE & 0x1f) + 1;
            break;

		case CURTOP:
			MDA_LOG(1,"MDA_cursor_top_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_CURTOP = data;
			MDA_curmode = MDA_CURTOP & 0x60;
			MDA_curminy = MDA_CURTOP & 0x1f;
			dirtybuffer[MDA_cursor] = 1;
            break;
		case CURBOT:
			MDA_LOG(1,"MDA_cursor_bottom_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_CURBOT = data;
			MDA_curmaxy = MDA_CURBOT & 0x1f;
			dirtybuffer[MDA_cursor] = 1;
            break;

		case VIDH:
			MDA_LOG(1,"MDA_base_high_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VIDH = data;
			MDA_base = (MDA_VIDH*256+MDA_VIDL) % videoram_size;
            break;
		case VIDL:
			MDA_LOG(1,"MDA_base_low_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_VIDL = data;
			MDA_base = (MDA_VIDH*256+MDA_VIDL) % videoram_size;
            break;

		case CURH:
			MDA_LOG(2,"MDA_cursor_high_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_CURH = data;
			MDA_cursor = ((MDA_CURH*256+MDA_CURL)*2) % videoram_size;
			dirtybuffer[MDA_cursor] = 1;
            break;
		case CURL:
			MDA_LOG(2,"MDA_cursor_low_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            MDA_CURL = data;
			MDA_cursor = ((MDA_CURH*256+MDA_CURL)*2) % videoram_size;
			dirtybuffer[MDA_cursor] = 1;
            break;

		case LPENH: /* this is read only */
			MDA_LOG(1,"MDA_light_pen_high_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            break;
		case LPENL: /* this is read only */
			MDA_LOG(1,"MDA_light_pen_low_w",(errorlog,"$%02x\n",data));
			if (MDA_crtc[MDA_reg] == data) break;
            break;
    }
}

int pc_mda_port_r(void)
{
	int data = 0xff;
	switch (MDA_reg)
	{
		case HTOTAL:
			MDA_LOG(1,"MDA_horz_total_r",(errorlog,"$%02x\n",data));
            break;
		case HDISP:
			MDA_LOG(1,"MDA_horz_displayed_r",(errorlog,"$%02x\n",data));
            break;
		case HSYNCP:
			MDA_LOG(1,"MDA_horz_sync_pos_r",(errorlog,"$%02x\n",data));
            break;
		case HSYNCW:
			MDA_LOG(1,"MDA_horz_sync_width_r",(errorlog,"$%02x\n",data));
            break;

		case VTOTAL:
			MDA_LOG(1,"MDA_vert_total_r",(errorlog,"$%02x\n",data));
            break;
		case VTADJ:
			MDA_LOG(1,"MDA_vert_total_adj_r",(errorlog,"$%02x\n",data));
            break;
		case VDISP:
			MDA_LOG(1,"MDA_vert_displayed_r",(errorlog,"$%02x\n",data));
            break;
		case VSYNCW:
			MDA_LOG(1,"MDA_vert_sync_width_r",(errorlog,"$%02x\n",data));
            break;

		case INTLACE:
			MDA_LOG(1,"MDA_interlace_r",(errorlog,"$%02x\n",data));
            break;
		case SCNLINE:
			MDA_LOG(1,"MDA_scanline_r",(errorlog,"$%02x\n",data));
            break;

		case CURTOP:
			MDA_LOG(1,"MDA_cursor_top_r",(errorlog,"$%02x\n",data));
            break;
		case CURBOT:
			MDA_LOG(1,"MDA_cursor_bottom_r",(errorlog,"$%02x\n",data));
            break;


		case VIDH:
            data = MDA_VIDH;
			MDA_LOG(1,"MDA_base_high_r",(errorlog,"$%02x\n",data));
            break;
		case VIDL:
            data = MDA_VIDL;
			MDA_LOG(1,"MDA_base_low_r",(errorlog,"$%02x\n",data));
            break;

		case CURH:
            data = MDA_CURH;
			MDA_LOG(2,"MDA_cursor_high_r",(errorlog,"$%02x\n",data));
            break;
		case CURL:
            data = MDA_CURL;
			MDA_LOG(2,"MDA_cursor_low_r",(errorlog,"$%02x\n",data));
            break;

		case LPENH:
            data = MDA_LPENH;
			MDA_LOG(1,"MDA_light_pen_high_r",(errorlog,"$%02x\n",data));
            break;
		case LPENL:
            data = MDA_LPENL;
			MDA_LOG(1,"MDA_light_pen_low_r",(errorlog,"$%02x\n",data));
            break;
    }
    return data;
}

/*
 *	rW	MDA mode control register (see #P138)
 */
void pc_mda_mode_control_w(int data)
{
	MDA_LOG(1,"MDA_mode_control_w",(errorlog, "$%02x: colums %d, gfx %d, enable %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>3)&1, (data>>5)&1));
	if ((pc_port[0x3b8] ^ data) & 0xaa)
		memset(dirtybuffer, 1, videoram_size);
	pc_port[0x3b8] = data;
}

int pc_mda_mode_control_r(void)
{
    int data = pc_port[0x3b8];
	MDA_LOG(1,"MDA_mode_control_r",(errorlog, "$%02x: colums %d, gfx %d, enable %d, blink %d\n",
        data, (data&1)?80:40, (data>>1)&1, (data>>3)&1, (data>>5)&1));
    return data;
}

void pc_mda_color_select_w(int data)
{
	MDA_LOG(1,"MDA_color_select_w",(errorlog, "$%02x\n", data));
	pc_port[0x3b9] = data;
}

int pc_mda_color_select_r(void)
{
	int data = pc_port[0x3b9];
    MDA_LOG(1,"MDA_color_select_w",(errorlog, "$%02x\n", data));
	return data;
}

/*
 * -W  (mono EGA/mono VGA) feature control register
 *		(see PORT 03DAh-W for details; VGA, see PORT 03CAh-R)
 */
void pc_mda_feature_control_w(int data)
{
	MDA_LOG(1,"MDA_feature_control_w",(errorlog, "$%02x\n", data));
	pc_port[0x3da] = data;
}

/*
 * -W  light pen strobe reset (on any value)
 */
void pc_mda_lightpen_strobe_w(int data)
{
	MDA_LOG(1,"MDA_lightpen_strobe_w",(errorlog, "$%02x\n", data));
}

/*	R-	CRT status register (see #P139)
 *		(EGA/VGA) input status 1 register
 *		7	 HGC vertical sync in progress
 *		6-4  adapter 000  hercules
 *					 001  hercules+
 *					 101  hercules InColor
 *					 else unknown
 *		3	 pixel stream (0 black, 1 white)
 *		2-1  reserved
 *		0	 horizontal drive enable
 */
int pc_mda_status_r(void)
{
	static UINT8 mda_hsync = 0;
    int data = (input_port_0_r(0) & 0x80) | 0x08 | mda_hsync;
	mda_hsync ^= 0x01;
	return data;
}

void pc_hgc_config_w(int data)
{
	MDA_LOG(1,"HGC_config_w",(errorlog, "$%02x\n", data));
    pc_port[0x03bf] = data;
}

int pc_hgc_config_r(void)
{
	int data = pc_port[0x03bf];
	MDA_LOG(1,"HGC_config_r",(errorlog, "$%02x\n", data));
    return data;
}

INLINE int DOCLIP(struct rectangle *r1)
{
    const struct rectangle *r2 = &Machine->drv->visible_area;
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
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
void pc_mda_blink_textcolors(int on)
{
	int i, offs, size;

	if (pc_blink == on) return;

    pc_blink = on;
	offs = (MDA_base * 2) % videoram_size;
	size = MDA_size;

	for (i = 0; i < size; i++)
	{
		if( videoram[offs+1] & 0x80 )
			dirtybuffer[offs+1] = 1;
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) and intense background.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/
static void MDA_text_80_inten(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs = (MDA_base * 2) % videoram_size, size = MDA_size;

    /* for every second character in the Video RAM, check if it has
       been modified since last time and update it accordingly. */
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
	{
		if( dirtybuffer[offs] || dirtybuffer[offs+1] )
		{
            struct rectangle r;
			int code = videoram[offs], attr = videoram[offs+1];

            dirtybuffer[offs] = 0;
            dirtybuffer[offs+1] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 9 - 1;
			r.max_y = sy + MDA_maxscan - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
				if( offs == MDA_cursor && MDA_curmode != 0x20 )
				{
					if( MDA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + MDA_curminy < r.max_y )
                            r.min_y = sy + MDA_curminy;
						else
                            r.min_y = r.max_y;
                        if( sy + MDA_curmaxy < r.max_y )
							r.max_y = sy + MDA_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,(attr&8)|2,0,0,sx,sy,&r,TRANSPARENCY_NONE, 0);
                    }
                    dirtybuffer[offs] = 1;
                }
            }
        }
		if( (sx += 9) == (MDA_HDISP * 9) )
		{
			sx = 0;
			sy += MDA_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking characters.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/
static void MDA_text_80_blink(struct osd_bitmap *bitmap)
{
	int i, sx, sy, offs = (MDA_base * 2) % videoram_size, size = MDA_size;

    /* for every second character in the Video RAM, check if it has
       been modified since last time and update it accordingly. */
	for( i = 0, sx = 0, sy = 0; i < size; i++ )
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
			r.max_x = sx + 9 - 1;
			r.max_y = sy + MDA_maxscan - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[0], code, attr, 0, 0,r.min_x,r.min_y, &r, TRANSPARENCY_NONE, 0);
				if( offs == MDA_cursor && MDA_curmode != 0x20 )
                {
					if( MDA_curmode == 0x60 || (pc_framecnt & 32) )
                    {
						if( sy + MDA_curminy < r.max_y )
                            r.min_y = sy + MDA_curminy;
						else
							r.min_y = r.max_y;
                        if( sy + MDA_curmaxy < r.max_y )
							r.max_y = sy + MDA_curmaxy;
						drawgfx(bitmap,Machine->gfx[0],219,(attr&8)|2,0,0,sx,sy,&r,TRANSPARENCY_NONE,0);
                    }
                    dirtybuffer[offs] = 1;
                }
            }
        }
		if( (sx += 9) == (MDA_HDISP * 9) )
		{
			sx = 0;
			sy += MDA_maxscan;
		}
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

/***************************************************************************
  Draw graphics with 720x384 pixels (default); so called Hercules gfx.
  The memory layout is divided into 4 banks where of size 0x2000.
  Every bank holds data for every n'th scanline, 8 pixels per byte,
  bit 7 being the leftmost.
***************************************************************************/
static void HGC_gfx_720(struct osd_bitmap *bitmap)
{
	int i, sx, sy, page, offs, size = MDA_size * 2;

	page = 256 * (pc_port[0x3b8] & 0x80);

    /* for every code in the Video RAM, check if it has been
	   modified since last time and update it accordingly. */
	offs = (MDA_base + page) % videoram_size;
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
			r.max_y = sy + MDA_maxscan / 4 - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, 0,
					0,0, sx,sy, &r, TRANSPARENCY_NONE, 0);
            }
        }
		if( (sx += 8) == (2 * MDA_HDISP * 8) )
		{
			sx = 0;
			sy += MDA_maxscan;
		}
		if(++offs == videoram_size)
			offs = 0;
    }

	offs = ((MDA_base + page) % videoram_size) | 0x2000;
	for( i = 0, sx = 0, sy = MDA_maxscan / 4; i < size; i++ )
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + MDA_maxscan / 4 - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, 0,
					0,0, sx,sy, &r, TRANSPARENCY_NONE, 0);
            }
        }
		if( (sx += 8) == (2 * MDA_HDISP * 8) )
		{
			sx = 0;
			sy += MDA_maxscan;
		}
		if(++offs == videoram_size)
			offs = 0;
    }

	offs = ((MDA_base + page) % videoram_size) | 0x4000;
	for (i = 0, sx = 0, sy = 2 * MDA_maxscan / 4; i < size; i++)
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + MDA_maxscan / 4 - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, 0,
					0,0, sx,sy, &r, TRANSPARENCY_NONE, 0);
            }
        }
		if( (sx += 8) == (2 * MDA_HDISP * 8) )
		{
			sx = 0;
			sy += MDA_maxscan;
		}
		if( ++offs == videoram_size )
			offs = 0;
    }

	offs = ((MDA_base + page) % videoram_size) | 0x6000;
	for (i = 0, sx = 0, sy = 3 * MDA_maxscan / 4; i < size; i++)
	{
		if( dirtybuffer[offs] )
		{
            struct rectangle r;
			int code = videoram[offs];

            dirtybuffer[offs] = 0;

			r.min_x = sx;
			r.min_y = sy;
			r.max_x = sx + 8 - 1;
			r.max_y = sy + MDA_maxscan / 4 - 1;
			if( DOCLIP(&r) )
			{
                /* draw the character */
				drawgfx(bitmap, Machine->gfx[1], code, 0,
					0,0, sx,sy, &r, TRANSPARENCY_NONE, 0);
            }
        }
		if( (sx += 8) == (2 * MDA_HDISP * 8) )
		{
			sx = 0;
			sy += MDA_maxscan;
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
void pc_mda_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	static int video_active = 0;

	if( palette_recalc() )
		full_refresh = 1;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	if( full_refresh )
	{
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->drv->visible_area);
		video_active = 0;
    }

	switch (pc_port[0x03b8] & 0x2a) /* text and gfx modes */
	{
		case 0x08: video_active = 10; MDA_text_80_inten(bitmap); break;
		case 0x28: video_active = 10; MDA_text_80_blink(bitmap); break;
		case 0x0a: video_active = 10; HGC_gfx_720(bitmap);		 break;
		case 0x2a: video_active = 10; HGC_gfx_720(bitmap);		 break;

        default:
			if (video_active && --video_active == 0)
				fillbitmap(bitmap, Machine->pens[0], &Machine->drv->visible_area);
    }
}

