
/*
    Video Display Processor emulation

    The video chip used in the SMS and GG is a derivative of the
    TMS9918, with an additional screen mode (mode 4). Only one licenced
    SMS game is known to use the regular TMS9918 modes (F-15 Strike Eagle),
    so this emulation only covers Mode 4 and doesn't emulate any other
    aspects of the TMS9918.

    The GG only has mode 4, with some of the CRT-specific features
    removed. In addition, the color memory is doubled, with four bits
    per component for a total of 4096 possible colors. You can still
    only have 32 colors on-screen, though.

    The exact behavior of line interrupts are unknown. A number of games
    exhibit incorrectly rendered graphics where raster effects appear
    to be off by a line, due to this. Some games have bad flicker such
    as Outrun and Gunstar Heroes.

    The color RAM cannot be read, it is unknown what data is returned
    when the data port is read in CRAM mode.

    Missing features:

    - Background / sprite priority. The priority level is set by the
      background attributes, not by sprites.

    - Sprite collision flag. Fantasy Zone (GG) uses it.

    - The leftmost 8 columns can be locked so vertical scrolling has no
      effect. Golvelius and Gauntlet use this.

    - Sprite clipping. Ghouls and Ghosts also requires clipping on the
      leftmost edge as it uses delayed sprites.

    - Zoomed sprites. X-Men and X-Men 2 (GG) use it.
*/


#include "driver.h"
#include "mess/vidhrdw/smsvdp.h"
#include "vidhrdw/generic.h"

/* VDP data */

UINT8 vram[0x4000];         /* 16k video RAM */
UINT8 cram[0x40];           /* On-chip color RAM */
UINT8 vdpreg[0x10];         /* VDP registers */
UINT8 pending;              /* 1= expecting 2nd write to ctrl port */
UINT8 latch;                /* Result of 1st write to ctrl port*/
UINT8 status;               /* VDP status flag */
UINT8 buffer;               /* Read-ahead buffer */
UINT8 mode;                 /* Accessing: 0=VRAM 1=CRAM */
UINT8 cram_ptr;             /* Index into color RAM */
UINT16 vram_ptr;            /* Index into video RAM */
UINT16 ntab;                /* Address of name table in VRAM */
UINT16 satb;                /* Address of sprite table in VRAM */
int curline;                /* Current scan line */
int linesleft;              /* # of lines until a raster interrupt */

/* Display stuff */

UINT8 is_vram_dirty;        /* 1= patterns were modified */
UINT8 vram_dirty[512];      /* Table of altered patterns */
UINT8 is_cram_dirty;        /* 1= color was modified */
UINT8 cram_dirty[32];       /* Table of altered colors */
UINT8 cache[64*512*4];      /* Converted tile cache */

struct osd_bitmap *tmpbitmap = NULL;
static int irq_state = 0;

int GameGear;

int sms_vdp_start(void)
{
    return SMSVDP_start(0);
}

int gamegear_vdp_start(void)
{
    return SMSVDP_start(1);
}

int sms_vdp_curline_r (int offset)
{
    return (curline);
}

int SMSVDP_start (int vdp_type)
{
    GameGear = vdp_type;

    memset(vram, 0, 0x4000);
    memset(cram, 0, 0x40);
    memset(vdpreg, 0, 0x10);
    memset(cache, 0, sizeof(cache));

    pending = latch = status = buffer = mode = \
    cram_ptr = vram_ptr = ntab = satb = irq_state = \
    curline = linesleft = 0;

    is_vram_dirty = 1;
    memset(vram_dirty, 1, sizeof(vram_dirty));

    is_cram_dirty = 1;
    memset(cram_dirty, 1, sizeof(cram_dirty));

    tmpbitmap = osd_create_bitmap(256, 224);
    if(!tmpbitmap) return (1);

    return (0);
}

void sms_vdp_stop(void)
{
    if(tmpbitmap) osd_free_bitmap(tmpbitmap);
}


int sms_vdp_interrupt(void)
{
    static int need_vint = 0;

    curline = (curline + 1) % 262;

    if ((curline < 192) && (!osd_skip_this_frame()))
    {
        sms_cache_tiles();
        sms_refresh_line(tmpbitmap, curline);
    }

    if(curline >= 193)
    {
        if((curline < 224) && (status & STATUS_VIRQ) && (vdpreg[1] & 0x20))
        {
            irq_state = 1;
        }
    }
    else
    {
        if(curline == 192) need_vint = 1;

        if(!curline) linesleft = vdpreg[10];
        if(linesleft) linesleft--;
        else
        {
            linesleft = vdpreg[10];

            status |= STATUS_HIRQ;
        }

        if((status & STATUS_HIRQ) && (vdpreg[0] & 0x10))
        {
            irq_state = 1;
        }
    }

    if(need_vint == 1)
    {
        need_vint = 0;
        status |= STATUS_VIRQ;
    }

    return (irq_state ? 0x38 : ignore_interrupt());
}



int sms_vdp_data_r(int offset)
{
    int temp;

    if (mode == MODE_CRAM)
	{
        temp = cram[cram_ptr];
        cram_ptr = (cram_ptr + 1) & (GameGear ? 0x3F : 0x1F);
	}
    else
	{
        temp = buffer;
        buffer = vram[vram_ptr];
        vram_ptr = (vram_ptr + 1) & 0x3FFF;
	}
    return (temp);
}

int sms_vdp_ctrl_r(int offset)
{
    int temp = status;
    pending = 0;
    irq_state = 0;
    status &= ~(STATUS_VIRQ | STATUS_HIRQ | STATUS_SPRCOL);
    return (temp);
}


void sms_vdp_data_w(int offset, int data)
{
    if (mode == MODE_CRAM)
	{
        if(data != cram[cram_ptr])
        {
            cram[cram_ptr] = data;
            cram_dirty[GameGear ? cram_ptr >> 1 : cram_ptr] = is_cram_dirty = 1;
        }
        cram_ptr = (cram_ptr + 1) & (GameGear ? 0x3F : 0x1F);
	}
    else
    {
        if(data != vram[vram_ptr])
        {
            vram[vram_ptr] = data;
            vram_dirty[vram_ptr >> 5] = is_vram_dirty = 1;
        }

        buffer = data;
        vram_ptr = (vram_ptr + 1) & 0x3FFF;
    }
}




void sms_vdp_ctrl_w(int offset, int data)
{
    if (pending == 0)
	{
        pending = 1;
        latch = data;
	}
	else
	{
        pending = 0;

        if((data & 0xF0) == 0x80)
        {
            vdpreg[data & 0x0F] = latch;
            ntab = (vdpreg[2] << 10) & 0x3800;
            satb = (vdpreg[5] <<  7) & 0x3F00;
        }
        else
        if((data & 0xC0) == 0xC0)
        {
            cram_ptr = (latch & (GameGear ? 0x3F : 0x1F));
            mode = MODE_CRAM;
        }
        else
        {
            vram_ptr = (int)((data << 8) | latch) & 0x3FFF;
            mode = MODE_VRAM;

            if(!(data & 0x40))
            {
                buffer = vram[vram_ptr];
                vram_ptr = (vram_ptr + 1) & 0x3FFF;
            }
        }
    }
}


void sms_refresh_line(struct osd_bitmap *bitmap, int line)
{
    int i, x, color;
    int charindex, palselect;
    int sy, sx, sn, sl, width = 8, height = (vdpreg[1] & 0x02 ? 16 : 8);
    int v_line = (line + vdpreg[9]) % 224, v_row = v_line & 7;
    UINT16 *nametable = (UINT16 *)&(vram[ntab+((v_line >> 3) << 6)]);
    UINT8 *objtable = (UINT8 *)&(vram[satb]);
    int xscroll = (((vdpreg[0] & 0x40)&&(line < 16)) ? 0 : vdpreg[8]);

    /* Check if display is disabled */
    if(!(vdpreg[1] & 0x40))
    {
        memset(&bitmap->line[line][0], Machine->pens[0x10 + vdpreg[7]], 0x100);
        return;
    }


    for(i=0;i<32;i++)
    {
    	UINT16 tile = nametable[i];

    	#ifndef LSB_FIRST
    	tile = ((nametable[i] & 0xff) << 8) | (nametable[i] >> 8);
    	#endif

        charindex = (tile & 0x07FF);
        palselect = (tile >> 7) & 0x10;

        for(x=0;x<8;x++)
        {
            color = cache[ (charindex << 6) + (v_row << 3) + (x)];
            bitmap->line[line][(xscroll+(i<<3)+x)%256] = Machine->pens[color | palselect];
        }
    }

    for(i=0;(i<64)&&(objtable[i]!=208);i++);
    for(i--;i>=0;i--)
    {
        sy = objtable[i]+1; /* sprite y position starts at line 1 */
        if(sy>240) sy-=256; /* wrap from top if y position is > 240 */
        if((line>=sy)&&(line<(sy+height)))
        {
            sx = objtable[0x80 + (i << 1)];
            if(vdpreg[0]&0x08) sx -= 8;   /* sprite shift */
            sn = objtable[0x81 + (i << 1)];
            if(vdpreg[6]&0x04) sn += 256; /* pattern table select */
            if(vdpreg[1]&0x02) sn &= 0x01FE; /* force even indexes */

            sl = (line - sy);

            for(x=0;x<width;x++)
            {
                color = cache[(sn << 6)+(sl << 3) + (x)];
                if(color) bitmap->line[line][(sx + x) % 256] = Machine->pens[0x10 | color];
            }
        }
    }

    if(vdpreg[0] & 0x20)
    {
        memset(&bitmap->line[line][0], Machine->pens[0x10 + vdpreg[7]], 8);
    }
}


void sms_update_palette(void)
{
    int i, r, g, b;

    if(is_cram_dirty == 0) return;

    for(i = 0; i < 0x20; i += 1)
    {
        if(cram_dirty[i] == 1)
        {
            if(GameGear == 1)
            {
                r = ((cram[i * 2 + 0] >> 0) & 0x0F) << 4;
                g = ((cram[i * 2 + 0] >> 4) & 0x0F) << 4;
                b = ((cram[i * 2 + 1] >> 0) & 0x0F) << 4;
            }
            else
            {
                r = ((cram[i] >> 0) & 0x03) << 6;
                g = ((cram[i] >> 2) & 0x03) << 6;
                b = ((cram[i] >> 4) & 0x03) << 6;
            }

            palette_change_color(i, r, g, b);
        }
    }

    is_cram_dirty = 0;
    memset(cram_dirty, 0, sizeof(cram_dirty));
}


void sms_cache_tiles(void)
{
    int i, x, y, c;
    int b0, b1, b2, b3;
    int i0, i1, i2, i3;

    if(is_vram_dirty == 0) return;

    for(i = 0; i < 0x200; i += 1)
    {
        if(vram_dirty[i] == 1)
        {
            for(y=0;y<8;y++)
            {
                b0 = vram[(i << 5) + (y << 2) + 0];
                b1 = vram[(i << 5) + (y << 2) + 1];
                b2 = vram[(i << 5) + (y << 2) + 2];
                b3 = vram[(i << 5) + (y << 2) + 3];

                for(x=0;x<8;x++)
                {
                    i0 = (b0 >> (7-x)) & 1;
                    i1 = (b1 >> (7-x)) & 1;
                    i2 = (b2 >> (7-x)) & 1;
                    i3 = (b3 >> (7-x)) & 1;

                    c = (i3 << 3 | i2 << 2 | i1 << 1 | i0);

                    cache[ (0 << 15) + (i << 6) + ((  y) << 3) + (  x) ] = c;
                    cache[ (1 << 15) + (i << 6) + ((  y) << 3) + (7-x) ] = c;
                    cache[ (2 << 15) + (i << 6) + ((7-y) << 3) + (  x) ] = c;
                    cache[ (3 << 15) + (i << 6) + ((7-y) << 3) + (7-x) ] = c;
                }
            }
        }
    }
    is_vram_dirty = 0;
    memset(vram_dirty, 0, sizeof(vram_dirty));
}


void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
    sms_update_palette();
	palette_recalc();
    copybitmap (bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
}


