
/*
    For more information, please see:
    http://www.emucamp.com/cgfm2/smsvdp.txt
*/

#include "driver.h"
#include "vidhrdw/smsvdp.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

UINT8 reg[0x10];
UINT8 cram[0x40];
UINT8 vram[0x4000];
int addr;
int code;
int pending;
int latch;
int buffer;
int status;

int is_vram_dirty;
int is_cram_dirty;
UINT8 vram_dirty[0x200];
UINT8 cram_dirty[0x20];
UINT8 cache[0x20000];

int GameGear;
int vpstart;
int vpend;
int ntab;
int satb;
int curline;
int linesleft;
int irq_state;      /* The status of the IRQ line, as seen by the VDP */

/*--------------------------------------------------------------------------*/

/* Precalculated return values from the V counter */
static UINT8 vcnt[0x200] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
                                  0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

/*--------------------------------------------------------------------------*/

int sms_vdp_start(void)
{
    return SMSVDP_start(0);
}

int gamegear_vdp_start(void)
{
    return SMSVDP_start(1);
}

READ_HANDLER ( sms_vdp_curline_r )
{
    return (vcnt[(curline & 0x1FF)]);
}

int SMSVDP_start (int vdp_type)
{
    GameGear = vdp_type;

    vpstart = GameGear ? 24 : 0;
    vpend   = GameGear ? 168 : 192;

    /* Clear RAM */
    memset(reg, 0, 0x10);
    memset(cram, 0, 0x40);
    memset(vram, 0, 0x4000);

    /* Initialize VDP state variables */
    addr = code = pending = latch = buffer = status = \
    ntab = satb = curline = linesleft = irq_state = 0;

    is_vram_dirty = 1;
    memset(vram_dirty, 1, 0x200);

    is_cram_dirty = 1;
    memset(cram_dirty, 1, 0x20);

    /* Clear pattern cache */
    memset(cache, 0, 0x20000);

    /* Make temp bitmap for rendering */
    tmpbitmap = bitmap_alloc(256, 224);
    if(!tmpbitmap) return (1);

    return (0);
}


void sms_vdp_stop(void)
{
    if(tmpbitmap) osd_free_bitmap(tmpbitmap);
}


int sms_vdp_interrupt(void)
{
    /* Bump scanline counter */
    curline = (curline + 1) % 262;

    if(curline <= 0xC0)
    {
        /* Technically, this happens at cycle 0xF4 of line 0xBF, but
           this is close enough. */
        if(curline == 0xC0)
        {
            status |= STATUS_VINT;
        }

        if(curline == 0x00)
        {
            linesleft = reg[10];
        }

        if(linesleft == 0x00)
        {
            linesleft = reg[10];
            status |= STATUS_HINT;
        }
        else
        {
            linesleft -= 1;
        }

        if((status & STATUS_HINT) && (reg[0] & 0x10))
        {
            irq_state = 1;
            z80_set_irq_line(0, ASSERT_LINE);
        }
    }
    else
    {
        linesleft = reg[10];

        if((curline < 0xE0) && (status & STATUS_VINT) && (reg[1] & 0x20))
        {
            irq_state = 1;
            z80_set_irq_line(0, ASSERT_LINE);
        }
    }

    if( (curline >= vpstart) && (curline < vpend) && (!osd_skip_this_frame()) )
    {
        sms_cache_tiles();
        sms_refresh_line(tmpbitmap, curline);
    }

    return (Z80_IGNORE_INT);
}



READ_HANDLER ( sms_vdp_data_r )
{
    int temp = 0;

    /* Clear pending write flag */
    pending = 0;

    switch(code)
    {
        case 0x00: /* VRAM */
        case 0x01: /* VRAM */
        case 0x02: /* VRAM */

            /* Return read buffer contents */
            temp = buffer;

            /* Load read buffer */
            buffer = vram[(addr & 0xFFFF)];
            break;

        case 0x03: /* CRAM (invalid) */
            /* This should never happen; only known use is a
               dummy read in the GG game 'NBA Action' */
            break;
    }

    /* Bump internal address register */
    addr += 1;
    return (temp);
}


READ_HANDLER ( sms_vdp_ctrl_r )
{
    int temp = status;

    pending = 0;

    status &= ~(STATUS_VINT | STATUS_HINT | STATUS_SPRCOL);

    if(irq_state == 1)
    {
        irq_state = 0;
        z80_set_irq_line(0, CLEAR_LINE);
    }

    return (temp);
}


WRITE_HANDLER ( sms_vdp_data_w )
{
    pending = 0;

    switch(code)
    {
        case 0x00:
        case 0x01:
        case 0x02:
            {
                int address = (addr & 0x3FFF);
                int _index   = (addr & 0x3FFF) >> 5;

                if(data != vram[address])
                {
                    vram[address] = data;
                    vram_dirty[_index] = is_vram_dirty = 1;
                }
            }
            break;

        case 0x03:
            {
                int address = GameGear ? (addr & 0x3F) : (addr & 0x1F);
                int _index   = GameGear ? ((addr & 0x3E) >> 1) : (addr & 0x1F);

                if(data != cram[address])
                {
                    cram[address] = data;
                    cram_dirty[_index] = is_cram_dirty = 1;
                }
            }
            break;
    }

    addr += 1;
}




WRITE_HANDLER ( sms_vdp_ctrl_w )
{
    if(pending == 0)
    {
        pending = 1;
        latch = data;
    }
    else
    {
        pending = 0;

        if((data & 0xF0) == 0x80)
        {
            reg[(data & 0x0F)] = latch;
            ntab = (reg[2] << 10) & 0x3800;
            satb = (reg[5] <<  7) & 0x3F00;
        }
        else
        {
            code = (data >> 6) & 3;
            addr = (data << 8 | latch);

            if(code == 0x00)
            {
                buffer = vram[(addr & 0x3FFF)];
                addr += 1;
            }
        }
    }
}


void sms_refresh_line(struct osd_bitmap *bitmap, int line)
{
    int i, x, color;
    int charindex, palselect;
    int sy, sx, sn, sl, width = 8, height = (reg[1] & 0x02 ? 16 : 8);
    int v_line = (line + reg[9]) % 224, v_row = v_line & 7;
    UINT16 *nametable = (UINT16 *)&(vram[ntab+((v_line >> 3) << 6)]);
    UINT8 *objtable = (UINT8 *)&(vram[satb]);
    int xscroll = (((reg[0] & 0x40)&&(line < 16)) ? 0 : reg[8]);

    /* Check if display is disabled */
    if(!(reg[1] & 0x40))
    {
        memset(&bitmap->line[line][0], Machine->pens[0x10 + reg[7]], 0x100);
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
            bitmap->line[line][(xscroll+(i<<3)+x) & 0xFF] = Machine->pens[color | palselect];
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
            if(reg[0]&0x08) sx -= 8;   /* sprite shift */
            sn = objtable[0x81 + (i << 1)];
            if(reg[6]&0x04) sn += 256; /* pattern table select */
            if(reg[1]&0x02) sn &= 0x01FE; /* force even index */

            sl = (line - sy);

            for(x=0;x<width;x++)
            {
                color = cache[(sn << 6)+(sl << 3) + (x)];
                if(color) bitmap->line[line][(sx + x) & 0xFF] = Machine->pens[0x10 | color];
            }
        }
    }

    if(reg[0] & 0x20)
    {
        memset(&bitmap->line[line][0], Machine->pens[0x10 + reg[7]], 8);
    }
}


void sms_update_palette(void)
{
    int i, r, g, b;

    if(is_cram_dirty == 0) return;
    is_cram_dirty = 0;

    for(i = 0; i < 0x20; i += 1)
    {
        if(cram_dirty[i] == 1)
        {
            cram_dirty[i] = 0;

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
}


void sms_cache_tiles(void)
{
    int i, x, y, c;
    int b0, b1, b2, b3;
    int i0, i1, i2, i3;

    if(is_vram_dirty == 0) return;
    is_vram_dirty = 0;

    for(i = 0; i < 0x200; i += 1)
    {
        if(vram_dirty[i] == 1)
        {
            vram_dirty[i] = 0;

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
}


void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh)
{
    sms_update_palette();
	palette_recalc();
    copybitmap (bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}


