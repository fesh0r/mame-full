/***************************************************************************

  m6845.c

  Functions to emulate the video controller 6845.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6845.h"

/* this one is in mame.c and set if the entire screen needs a redraw */
extern  int scrbitmap_dirty;

int     m6845_font_offset[4] = {0, 0, 0, 0};
static  CRTC6845 crt;
static  int graphics = 0;
static  byte * cleanbuffer;
static  byte * colorbuffer;
static  int update_all = 0;
static  int off_x = 0;
static  int off_y = 0;

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int     m6845_vh_start(void)
{
        videoram_size = 0x4000;

        cleanbuffer = malloc(64 * 32 * 8);
        if (!cleanbuffer)
                return 1;
        memset(cleanbuffer, 0, 64 * 32 * 8);


        colorbuffer = malloc(64 * 32 * 8);
        if (!colorbuffer)
                return 1;
        memset(colorbuffer, 0, 64 * 32 * 8);

        if (generic_vh_start() != 0)
		return 1;

        return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void    m6845_vh_stop(void)
{
        generic_vh_stop();

        if (cleanbuffer)
                free(cleanbuffer);
        cleanbuffer = 0;

        if (colorbuffer)
                free(colorbuffer);
        colorbuffer = 0;
}

/***************************************************************************

  Calculate the horizontal and vertical offset for the
  current register settings of the 6845 CRTC

***************************************************************************/
static  void    m6845_offset_xy(void)
{
        if (crt.horizontal_sync_pos)
                off_x = crt.horizontal_total - crt.horizontal_sync_pos - 14;
        else
                off_x = -15;

        off_y = (crt.vertical_total - crt.vertical_sync_pos) *
                (crt.scan_lines + 1) + crt.vertical_adjust
                - 32;

        if (off_y < 0)
                off_y = 0;

        if (off_y > 128)
                off_y = 128;

        scrbitmap_dirty = 1;

//      if (errorlog) fprintf(errorlog, "m6845 offset x:%d  y:%d\n", off_x, off_y);
}


/***************************************************************************

  Write to an indexed register of the 6845 CRTC

***************************************************************************/
void    m6845_register_w(int offset, int data)
{
        switch (crt.idx)
        {
                case  0:
                        if (crt.horizontal_total != data)
                        {
                                crt.horizontal_total = data;
                                m6845_offset_xy();
                        }
                        break;
                case  1:
                        if (crt.horizontal_displayed != data)
                        {
                                scrbitmap_dirty = 1;
                                crt.horizontal_displayed = data;
                        }
                        break;
                case  2:
                        if (crt.horizontal_sync_pos != data)
                        {
                                crt.horizontal_sync_pos = data;
                                m6845_offset_xy();
                        }
                        break;
                case  3:
                        crt.horizontal_length = data;
                        break;
                case  4:
                        if (crt.vertical_total != data)
                        {
                                crt.vertical_total = data;
                                m6845_offset_xy();
                        }
                        break;
                case  5:
                        if (crt.vertical_adjust != data)
                        {
                                crt.vertical_adjust = data;
                                m6845_offset_xy();
                        }
                        break;
                case  6:
                        if (crt.vertical_displayed != data)
                        {
                                scrbitmap_dirty = 1;
                                crt.vertical_displayed = data;
                        }
                        break;
                case  7:
                        if (crt.vertical_sync_pos != data)
                        {
                                crt.vertical_sync_pos = data;
                                m6845_offset_xy();
                        }
                        break;
                case  8:
                        crt.crt_mode = data;
                        break;
                case  9:
                        data &= 15;
                        if (crt.scan_lines != data)
                        {
                                crt.scan_lines = data;
                                m6845_offset_xy();
                        }
                        break;
                case 10:
                        crt.cursor_top = data;
                        break;
                case 11:
                        crt.cursor_bottom = data;
                        break;
                case 12:
                        data &= 63;
                        if (data != crt.screen_address_hi)
                        {
                                update_all = 1;
                                crt.screen_address_hi = data;
                        }
                        break;
                case 13:
                        if (data != crt.screen_address_lo)
                        {
                                update_all = 1;
                                crt.screen_address_lo = data;
                        }
                        break;
                case 14:
                        data &= 63;
                        if (data != crt.cursor_address_hi)
                        {
                                crt.cursor_address_hi = data;
                                dirtybuffer[256 * crt.cursor_address_hi + crt.cursor_address_lo] = 1;
                        }
                        break;
                case 15:
                        if (data != crt.cursor_address_lo)
                        {
                                crt.cursor_address_lo = data;
                                dirtybuffer[256 * crt.cursor_address_hi + crt.cursor_address_lo] = 1;
                        }
                        break;
        }
}

/***************************************************************************
  Write to the index register of the 6845 CRTC
***************************************************************************/
void    m6845_index_w(int offset, int data)
{
        crt.idx = data & 15;
}

/***************************************************************************
  Read from an indexed register of the 6845 CRTC
***************************************************************************/
int     m6845_register_r(int offset)
{
        return m6845_get_register(crt.idx);
}

/***************************************************************************
  Read from a register of the 6845 CRTC
***************************************************************************/
int     m6845_get_register(int index)
{
        switch (index)
        {
                case  0:
                        return crt.horizontal_total;
                case  1:
                        return crt.horizontal_displayed;
                case  2:
                        return crt.horizontal_sync_pos;
                case  3:
                        return crt.horizontal_length;
                case  4:
                        return crt.vertical_total;
                case  5:
                        return crt.vertical_adjust;
                case  6:
                        return crt.vertical_displayed;
                case  7:
                        return crt.vertical_sync_pos;
                case  8:
                        return crt.crt_mode;
                case  9:
                        return crt.scan_lines;
                case 10:
                        return crt.cursor_top;
                case 11:
                        return crt.cursor_bottom;
                case 12:
                        return crt.screen_address_hi;
                case 13:
                        return crt.screen_address_lo;
                case 14:
                        return crt.cursor_address_hi;
                case 15:
                        return crt.cursor_address_lo;
        }
        return 0;
}

/***************************************************************************
  Read the index register of the 6845 CRTC
***************************************************************************/
int     m6845_index_r(int offset)
{
        return crt.idx;
}

/***************************************************************************
  Switch mode between character generator and graphics
***************************************************************************/
void    m6845_mode_select(int mode)
{
        graphics = (mode) ? 1 : 0;
}

/***************************************************************************
  Invalidate a range of characters with codes from l to h
***************************************************************************/
void    m6845_invalidate_range(int l, int h)
{
int     offs = 256 * crt.screen_address_hi + crt.screen_address_lo;
int     size = crt.horizontal_displayed * crt.vertical_displayed;
int     addr;
int     i;

        for (addr = 0; addr < size; addr++)
        {
                i = (offs + addr) & 0x3fff;
                if (videoram[i] >= l && videoram[i] <= h)
                        dirtybuffer[i] = 1;
        }
}

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void    m6845_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int i, address, offset, cursor, size, code, x, y;
        struct rectangle r;

        if (crt.vertical_displayed == 0 || crt.horizontal_displayed == 0)
        {
                fillbitmap(bitmap,0,&Machine->drv->visible_area);
                return;
        }

        offset = 256 * crt.screen_address_hi + crt.screen_address_lo;
        size = crt.horizontal_displayed * crt.vertical_displayed;
        cursor = 256 * crt.cursor_address_hi + crt.cursor_address_lo;


        if (scrbitmap_dirty)
        {
                scrbitmap_dirty = 0;
                fillbitmap(bitmap,0,&Machine->drv->visible_area);
                memset(dirtybuffer + offset, 1, size);
        }

        /* for every character in the Video RAM, check if it has
         * been modified since last time and update it accordingly. */
        for (address = 0; address < size; address++)
	{
                i = (offset + address) & 0x3fff;
                x = address % crt.horizontal_displayed + off_x;
                y = address / crt.horizontal_displayed;
                if (dirtybuffer[i] ||
                   (update_all &&
                     (cleanbuffer[y*64+x] != videoram[i] ||
                      colorbuffer[y*64+x] != colorram[i&0x3ff])))
		{
                        r.min_x = x * 8;
                        r.max_x = r.min_x +  7;
                        r.min_y = y * (crt.scan_lines + 1) + off_y;
                        r.max_y = r.min_y + crt.scan_lines;

                        colorbuffer[y*64+x] = colorram[i&0x3ff];
                        cleanbuffer[y*64+x] = videoram[i];
                        dirtybuffer[i] = 0;

                        if (graphics)
                        {
                                drawgfx(bitmap,
                                        Machine->gfx[1],
                                        videoram[i],
                                        0,
                                        0,0,
                                        r.min_x,r.min_y,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                        }
                        else
                        {
                                /* get character code */
                                code = videoram[i];
                                /* translate defined character sets */
                                code += m6845_font_offset[(code >> 6) & 3];
                                drawgfx(bitmap,
                                        Machine->gfx[0],
                                        code,
                                        colorram[i & 0x3ff],
                                        0,0,
                                        r.min_x,r.min_y,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                        }
                        osd_mark_dirty(r.min_x,r.min_y,r.max_x,r.max_y,1);

                        if (i == cursor)
                        {
                        struct rectangle rc;
                                /* check if cursor turned off */
                                if ((crt.cursor_top & 0x60) == 0x20)
                                        continue;
                                dirtybuffer[i] = 1;
                                if ((crt.cursor_top & 0x60) == 0x60)
                                {
                                        crt.cursor_visible = 1;
                                }
                                else
                                {
                                        crt.cursor_phase++;
                                        crt.cursor_visible = (crt.cursor_phase >> 3) & 1;
                                }
                                if (!crt.cursor_visible)
                                        continue;
                                rc.min_x = r.min_x;
                                rc.max_x = r.max_x;
                                rc.min_y = r.min_y + (crt.cursor_top & 15);
                                rc.max_y = r.min_y + (crt.cursor_bottom & 15);
                                drawgfx(bitmap,
                                        Machine->gfx[0],
                                        0x7f,
                                        colorram[i & 0x3ff],
                                        0,0,
                                        rc.min_x,rc.min_y,
                                        &rc,
                                        TRANSPARENCY_NONE,0);
                                osd_mark_dirty(rc.min_x,rc.min_y,rc.max_x,rc.max_y,1);
                        }
                }
        }
        update_all = 0;
}

