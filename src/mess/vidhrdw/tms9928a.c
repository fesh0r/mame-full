/*
** File: tms9928a.c -- software implementation of the Texas Instruments
**                     TMS9918A and TMS9928A, used by the Coleco, MSX and
**                     TI99/4A.
**
** All undocumented features as described in the following file
** should be emulated.
**
** http://www.msxnet.org/tech/tms9918a.txt
**
** Not emulated:
**  - The screen image is rendered in `one go'. Modifications during
**    screen build up are not shown.
**  - 4kB VRAM emulation not checked.
**  - Unknown if colours are correct (?).
**
** By Sean Young 1999 (sean@msxnet.org).
** Based on code by Mike Balfour. Features added:
** - read-ahead
** - single read/write address
** - AND mask for mode 2
** - multicolor mode
** - undocumented screen modes
** - illegal sprites (max 4 on one line)
** - vertical coordinate corrected -- was one to high (255 => 0, 0 => 1)
** - errors in interrupt emulation
** - back drop correctly emulated.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tms9928a.h"

unsigned char TMS9928A_palette[] =
{
        0x00, 0x00, 0x00, /* Transparent */
        0x00, 0x00, 0x00, /* Black */
        0x20, 0xC0, 0x20, /* Medium Green */
        0x60, 0xE0, 0x60, /* Light Green */
        0x20, 0x20, 0xE0, /* Dark Blue */
        0x40, 0x60, 0xE0, /* Light Blue */
        0xA0, 0x20, 0x20, /* Dark Red */
        0x40, 0xC0, 0xE0, /* Cyan */
        0xE0, 0x20, 0x20, /* Medium Red */
        0xE0, 0x60, 0x60, /* Light Red */
        0xC0, 0xC0, 0x20, /* Dark Yellow */
        0xC0, 0xC0, 0x80, /* Light Yellow */
        0x20, 0x80, 0x20, /* Dark Green */
        0xC0, 0x40, 0xA0, /* Magenta */
        0xA0, 0xA0, 0xA0, /* Gray */
        0xE0, 0xE0, 0xE0, /* White */
};

unsigned short TMS9928A_colortable[] =
{
        0,1,
        0,2,
        0,3,
        0,4,
        0,5,
        0,6,
        0,7,
        0,8,
        0,9,
        0,10,
        0,11,
        0,12,
        0,13,
        0,14,
        0,15,
};

/*
** Defines for `dirty' optimization
*/

#define MAX_DIRTY_COLOUR        (256*3)
#define MAX_DIRTY_PATTERN       (256*3)
#define MAX_DIRTY_NAME          (40*24)

/*
** Forward declarations of internal functions.
*/
static void _TMS9928A_mode0 (struct osd_bitmap*);
static void _TMS9928A_mode1 (struct osd_bitmap*);
static void _TMS9928A_mode2 (struct osd_bitmap*);
static void _TMS9928A_mode12 (struct osd_bitmap*);
static void _TMS9928A_mode3 (struct osd_bitmap*);
static void _TMS9928A_mode23 (struct osd_bitmap*);
static void _TMS9928A_modebogus (struct osd_bitmap*);
static void _TMS9928A_sprites (struct osd_bitmap*);
static void _TMS9928A_change_register (int, UINT8);
static void _TMS9928A_set_dirty (char);

static void (*ModeHandlers[])(struct osd_bitmap*) = {
        _TMS9928A_mode0, _TMS9928A_mode1, _TMS9928A_mode2,  _TMS9928A_mode12,
        _TMS9928A_mode3, _TMS9928A_modebogus, _TMS9928A_mode23,
        _TMS9928A_modebogus };

#define IMAGE_SIZE (256*192)        /* size of rendered image        */

#define TMS_SPRITES_ENABLED ((tms.Regs[1] & 0x50) == 0x40)
#define TMS_MODE ( (tms.Regs[0] & 2) | ((tms.Regs[1] & 0x10)>>4) | \
                 ((tms.Regs[1] & 8)>>1))

static TMS9928A tms;

/*
** The init, reset and shutdown functions
*/
void TMS9928A_reset () {
    int  i;

    for (i=0;i<8;i++) tms.Regs[i] = 0;
    tms.StatusReg = 0;
    tms.nametbl = tms.pattern = tms.colour = 0;
    tms.spritepattern = tms.spriteattribute = 0;
    tms.colourmask = tms.patternmask = 0;
    tms.Addr = tms.ReadAhead = tms.INT = 0;
    tms.mode = tms.BackColour = 0;
    tms.Change = 1;
    tms.FirstByte = -1;
    _TMS9928A_set_dirty (1);
}

int TMS9928A_start (unsigned int vram) {
    /* 4 or 16 kB vram please */
    if (! ((vram == 0x1000) || (vram == 0x4000)) )
        return 1;

    /* Video RAM */
    tms.vramsize = vram;
    tms.vMem = (UINT8*) malloc (vram);
    if (!tms.vMem) return (1);
    memset (tms.vMem, 0, vram);

    /* Sprite back buffer */
    tms.dBackMem = (UINT8*)malloc (IMAGE_SIZE);
    if (!tms.dBackMem) {
        free (tms.vMem);
        return 1;
    }

    /* dirty buffers */
    tms.DirtyName = (char*)malloc (MAX_DIRTY_NAME);
    if (!tms.DirtyName) {
        free (tms.vMem);
        free (tms.dBackMem);
        return 1;
    }

    tms.DirtyPattern = (char*)malloc (MAX_DIRTY_PATTERN);
    if (!tms.DirtyPattern) {
        free (tms.vMem);
        free (tms.DirtyName);
        free (tms.dBackMem);
        return 1;
    }

    tms.DirtyColour = (char*)malloc (MAX_DIRTY_COLOUR);
    if (!tms.DirtyColour) {
        free (tms.vMem);
        free (tms.DirtyName);
        free (tms.DirtyPattern);
        free (tms.dBackMem);
        return 1;
    }

    /* back bitmap */
    tms.tmpbmp = osd_create_bitmap (256, 192);
    if (!tms.tmpbmp) {
        free (tms.vMem);
        free (tms.dBackMem);
        free (tms.DirtyName);
        free (tms.DirtyPattern);
        free (tms.DirtyColour);
        return 1;
    }

    TMS9928A_reset ();
    tms.LimitSprites = 1;

    return 0;
}

void TMS9928A_stop () {
    free (tms.vMem); tms.vMem = NULL;
    free (tms.dBackMem); tms.dBackMem = NULL;
    free (tms.DirtyColour); tms.DirtyColour = NULL;
    free (tms.DirtyName); tms.DirtyName = NULL;
    free (tms.DirtyPattern); tms.DirtyPattern = NULL;
    osd_free_bitmap (tms.tmpbmp); tms.tmpbmp = NULL;
}

/*
** Set all dirty / clean
*/
static void _TMS9928A_set_dirty (char dirty) {
    tms.anyDirtyColour = tms.anyDirtyName = tms.anyDirtyPattern = dirty;
    memset (tms.DirtyName, dirty, MAX_DIRTY_NAME);
    memset (tms.DirtyColour, dirty, MAX_DIRTY_COLOUR);
    memset (tms.DirtyPattern, dirty, MAX_DIRTY_PATTERN);
}

/*
** The I/O functions.
*/
UINT8 TMS9928A_vram_r () {
    UINT8 b;
    b = tms.ReadAhead;
    tms.ReadAhead = tms.vMem[tms.Addr];
    tms.Addr = (tms.Addr + 1) & (tms.vramsize - 1);
    tms.FirstByte = -1;
    return b;
}

void TMS9928A_vram_w (UINT8 val) {
    int i;

    if (tms.vMem[tms.Addr] != val) {
        tms.vMem[tms.Addr] = val;
        tms.Change = 1;
        /* dirty optimization */
        if ( (tms.Addr >= tms.nametbl) &&
            (tms.Addr < (tms.nametbl + MAX_DIRTY_NAME) ) ) {
            tms.DirtyName[tms.Addr - tms.nametbl] = 1;
            tms.anyDirtyName = 1;
        }

        i = (tms.Addr - tms.colour) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_COLOUR) ) {
            tms.DirtyColour[i] = 1;
            tms.anyDirtyColour = 1;
        }

        i = (tms.Addr - tms.pattern) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_PATTERN) ) {
            tms.DirtyPattern[i] = 1;
            tms.anyDirtyPattern = 1;
        }
    }
    tms.Addr = (tms.Addr + 1) & (tms.vramsize - 1);
    tms.ReadAhead = val;
    tms.FirstByte = -1;
}

UINT8 TMS9928A_register_r () {
    UINT8 b;
    b = tms.StatusReg;
    tms.StatusReg &= 0x5f;
    if (tms.INT) {
        tms.INT = 0;
        if (tms.INTCallback) tms.INTCallback (tms.INT);
    }
    tms.FirstByte = -1;
    return b;
}

void TMS9928A_register_w (UINT8 val) {
    if (tms.FirstByte >= 0) {
        if (val & 0x80) {
            /* register write */
            _TMS9928A_change_register ((int)(val & 7), (UINT8)tms.FirstByte);
        } else {
            /* set read/write address */
            tms.Addr = ((UINT16)val << 8 | tms.FirstByte) & (tms.vramsize - 1);
            if ( !(val & 0x40) ) {
                tms.ReadAhead = tms.vMem[tms.Addr];
                tms.Addr = (tms.Addr + 1) & (tms.vramsize - 1);
            }
        }
        tms.FirstByte = -1;
    } else {
        tms.FirstByte = val;
    }
}

static void _TMS9928A_change_register (int reg, UINT8 val) {
    static const UINT8 Mask[8] =
        { 0x03, 0xfb, 0x0f, 0xff, 0x07, 0x7f, 0x07, 0xff };
    static const char *modes[] = {
        "Mode 0 (GRAPHIC 1)", "Mode 1 (TEXT 1)", "Mode 2 (GRAPHIC 2)",
        "Mode 1+2 (TEXT 1 variation)", "Mode 3 (MULTICOLOR)",
        "Mode 1+3 (BOGUS)", "Mode 2+3 (MULTICOLOR variation)",
        "Mode 1+2+3 (BOGUS)" };
    UINT8 b,oldval;
    int mode;

    val &= Mask[reg];
    oldval = tms.Regs[reg];
    if (oldval == val) return;
    tms.Regs[reg] = val;

    if (errorlog) fprintf (errorlog,
        "TMS9928A: Reg %d = %02xh\n", reg, (int)val);
    tms.Change = 1;
    switch (reg) {
    case 0:
        if ( (val ^ oldval) & 2) {
            /* re-calculate masks and pattern generator & colour */
            if (val & 2) {
                tms.colour = ((tms.Regs[3] & 0x80) * 64) & (tms.vramsize - 1);
                tms.colourmask = (tms.Regs[3] & 0x7f) * 8 | 7;
                tms.pattern = ((tms.Regs[4] & 4) * 2048) & (tms.vramsize - 1);
                tms.patternmask = (tms.Regs[4] & 3) * 256 | 255;
            } else {
                tms.colour = (tms.Regs[3] * 64) & (tms.vramsize - 1);
                tms.pattern = (tms.Regs[4] * 2048) & (tms.vramsize - 1);
            }
            tms.mode = TMS_MODE;
            if (errorlog) fprintf (errorlog, "TMS9928A: %s\n", modes[tms.mode]);
            _TMS9928A_set_dirty (1);
        }
        break;
    case 1:
        /* check for changes in the INT line */
        b = (val & 0x20) && (tms.StatusReg & 0x80) ;
        if (b != tms.INT) {
            tms.INT = b;
            if (tms.INTCallback) tms.INTCallback (tms.INT);
        }
        mode = TMS_MODE;
        if (tms.mode != mode) {
            tms.mode = mode;
            _TMS9928A_set_dirty (1);
            if (errorlog) fprintf (errorlog, "TMS9928A: %s\n", modes[tms.mode]);
        }
        break;
    case 2:
        tms.nametbl = (val * 1024) & (tms.vramsize - 1);
        tms.anyDirtyName = 1;
        memset (tms.DirtyName, 1, MAX_DIRTY_NAME);
        break;
    case 3:
        if (tms.Regs[0] & 2) {
            tms.colour = ((val & 0x80) * 64) & (tms.vramsize - 1);
            tms.colourmask = (val & 0x7f) * 8 | 7;
         } else {
            tms.colour = (val * 64) & (tms.vramsize - 1);
        }
        tms.anyDirtyColour = 1;
        memset (tms.DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    case 4:
        if (tms.Regs[0] & 2) {
            tms.pattern = ((val & 4) * 2048) & (tms.vramsize - 1);
            tms.patternmask = (val & 3) * 256 | 255;
        } else {
            tms.pattern = (val * 2048) & (tms.vramsize - 1);
        }
        tms.anyDirtyPattern = 1;
        memset (tms.DirtyPattern, 1, MAX_DIRTY_PATTERN);
        break;
    case 5:
        tms.spriteattribute = (val * 128) & (tms.vramsize - 1);
        break;
    case 6:
        tms.spritepattern = (val * 2048) & (tms.vramsize - 1);
        break;
    case 7:
        /* The backdrop is updated at TMS9928A_refresh() */
        tms.anyDirtyColour = 1;
        memset (tms.DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    }
}

/*
** Interface functions
*/

void TMS9928A_int_callback (void (*callback)(int)) {
    tms.INTCallback = callback;
}

void TMS9928A_set_spriteslimit (int limit) {
    tms.LimitSprites = limit;
}

/*
** Updates the screen (the dMem memory area).
*/
void TMS9928A_refresh (struct osd_bitmap *bmp, int full_refresh) {
    int c;

    if (tms.Change) {
        c = tms.Regs[7] & 15; if (!c) c=1;
        if (tms.BackColour != c) {
            tms.BackColour = c;
            palette_change_color (0,
                TMS9928A_palette[c * 3], TMS9928A_palette[c * 3 + 1],
                TMS9928A_palette[c * 3 + 2]);
        }
    }

    if (tms.Change || full_refresh) {
        if (! (tms.Regs[1] & 0x40) ) {
            fillbitmap (bmp, Machine->pens[tms.BackColour],
                &Machine->drv->visible_area);
        } else {
            if (tms.Change) ModeHandlers[tms.mode] (tms.tmpbmp);
            copybitmap (bmp, tms.tmpbmp, 0, 0, 0, 0,
                &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
            if (TMS_SPRITES_ENABLED) {
                _TMS9928A_sprites (bmp);
            }
        }
    } else {
	tms.StatusReg = tms.oldStatusReg;
    }

    /* store Status register, so it can be restored at the next frame
       if there are no changes (sprite collision bit is lost) */
    tms.oldStatusReg = tms.StatusReg;
    tms.Change = 0;

    return;
}

int TMS9928A_interrupt () {
    int b;

    /* when skipping frames, calculate sprite collision */
    if (osd_skip_this_frame()) {
        if (tms.Change) {
            if (TMS_SPRITES_ENABLED) {
                _TMS9928A_sprites (NULL);
            }
        } else {
	    tms.StatusReg = tms.oldStatusReg;
	}
    }

    tms.StatusReg |= 0x80;
    b = (tms.Regs[1] & 0x20) != 0;
    if (b != tms.INT) {
        tms.INT = b;
        if (tms.INTCallback) tms.INTCallback (tms.INT);
    }

    return b;
}

static void _TMS9928A_mode1 (struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    if (tms.anyDirtyColour) {
	/* colours at sides must be reset */
	for (y=0;y<192;y++) {
	    for (x=0;x<8;x++) bmp->line[y][x] = bg;
	    for (x=248;x<256;x++) bmp->line[y][x] = bg;
	}
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
		!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
                    bmp->line[y*8+yy][8+x*6+xx] = (pattern & 0x80) ? fg : bg;
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode12 (struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    if (tms.anyDirtyColour) {
	/* colours at sides must be reset */
	for (y=0;y<192;y++) {
	    for (x=0;x<8;x++) bmp->line[y][x] = bg;
	    for (x=248;x<256;x++) bmp->line[y][x] = bg;
	}
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = (tms.vMem[tms.nametbl+name]+(y/8)*256)&tms.patternmask;
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
		!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
                    bmp->line[y*8+yy][8+x*6+xx] = (pattern & 0x80) ? fg : bg;
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode0 (struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode,colour;
    UINT8 fg,bg,*patternptr;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode] ||
                tms.DirtyColour[charcode/64]) )
                continue;
            patternptr = tms.vMem + tms.pattern + charcode*8;
            colour = tms.vMem[tms.colour+charcode/8];
            fg = Machine->pens[colour / 16];
            bg = Machine->pens[colour & 15];
            for (yy=0;yy<8;yy++) {
                pattern=*patternptr++;
                for (xx=0;xx<8;xx++) {
                    bmp->line[y*8+yy][x*8+xx] = (pattern & 0x80) ? fg : bg;
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode2 (struct osd_bitmap *bmp) {
    int colour,name,x,y,yy,pattern,xx,charcode;
    UINT8 fg,bg;
    UINT8 *colourptr,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name]+(y/8)*256;
            colour = (charcode&tms.colourmask);
            pattern = (charcode&tms.patternmask);
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[pattern] ||
                tms.DirtyColour[colour]) )
                continue;
            patternptr = tms.vMem+tms.pattern+colour*8;
            colourptr = tms.vMem+tms.colour+pattern*8;
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                colour = *colourptr++;
                fg = Machine->pens[colour / 16];
                bg = Machine->pens[colour & 15];
                for (xx=0;xx<8;xx++) {
                    bmp->line[y*8+yy][x*8+xx] = (pattern & 0x80) ? fg : bg;
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode3 (struct osd_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
		!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem+tms.pattern+charcode*8+(y&3)*2;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
                    bmp->line[y*8+yy*4+yyy][x*8+0] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+1] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+2] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+3] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+4] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+5] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+6] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+7] = bg;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_mode23 (struct osd_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms.vMem[tms.nametbl+name];
            if ( !(tms.DirtyName[name++] || tms.DirtyPattern[charcode]) &&
		!tms.anyDirtyColour)
                continue;
            patternptr = tms.vMem + tms.pattern +
                ((charcode+(y&3)*2+(y/8)*256)&tms.patternmask)*8;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
                    bmp->line[y*8+yy*4+yyy][x*8+0] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+1] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+2] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+3] = fg;
                    bmp->line[y*8+yy*4+yyy][x*8+4] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+5] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+6] = bg;
                    bmp->line[y*8+yy*4+yyy][x*8+7] = bg;
                }
            }
        }
    }
    _TMS9928A_set_dirty (0);
}

static void _TMS9928A_modebogus (struct osd_bitmap *bmp) {
    UINT8 fg,bg;
    int x,y,n,xx;

    if ( !(tms.anyDirtyColour || tms.anyDirtyName || tms.anyDirtyPattern) )
         return;

    fg = Machine->pens[tms.Regs[7] / 16];
    bg = Machine->pens[tms.Regs[7] & 15];

    for (y=0;y<192;y++) {
        xx=0;
        n=8; while (n--) bmp->line[y][xx++] = bg;
        for (x=0;x<40;x++) {
            n=4; while (n--) bmp->line[y][xx++] = fg;
            n=2; while (n--) bmp->line[y][xx++] = bg;
        }
        n=8; while (n--) bmp->line[y][xx++] = bg;
    }

    _TMS9928A_set_dirty (0);
}

/*
** This function renders the sprites. Sprite collision is calculated in
** in a back buffer (tms.dBackMem), because sprite collision detection
** is rather complicated (transparent sprites also cause the sprite
** collision bit to be set, and ``illegal'' sprites do not count
** (they're not displayed)).
**
** This code should be optimized. One day.
*/
static void _TMS9928A_sprites (struct osd_bitmap *bmp) {
    UINT8 *attributeptr,*patternptr,c;
    int p,x,y,size,i,j,large,yy,xx,limit[192],
        illegalsprite,illegalspriteline;
    UINT16 line,line2;

    attributeptr = tms.vMem + tms.spriteattribute;
    size = (tms.Regs[1] & 2) ? 16 : 8;
    large = (int)(tms.Regs[1] & 1);

    for (x=0;x<192;x++) limit[x] = 4;
    tms.StatusReg = 0x80;
    illegalspriteline = 255;
    illegalsprite = 0;

    memset (tms.dBackMem, 0, IMAGE_SIZE);
    for (p=0;p<32;p++) {
        y = *attributeptr++;
        if (y == 208) break;
        if (y > 208) {
            y=-(~y&255);
        } else {
            y++;
        }
        x = *attributeptr++;
        patternptr = tms.vMem + tms.spritepattern +
            ((size == 16) ? *attributeptr & 0xfc : *attributeptr) * 8;
        attributeptr++;
        c = (*attributeptr & 0x0f);
        if (*attributeptr & 0x80) x -= 32;
        attributeptr++;

        if (!large) {
            /* draw sprite (not enlarged) */
            for (yy=y;yy<(y+size);yy++) {
                if ( (yy < 0) || (yy > 191) ) continue;
                if (limit[yy] == 0) {
                    /* illegal sprite line */
                    if (yy < illegalspriteline) {
                        illegalspriteline = yy;
                        illegalsprite = p;
                    } else if (illegalspriteline == yy) {
                        if (illegalsprite > p) {
                            illegalsprite = p;
                        }
                    }
                    if (tms.LimitSprites) continue;
                } else limit[yy]--;
                line = 256*patternptr[yy-y] + patternptr[yy-y+16];
                for (xx=x;xx<(x+size);xx++) {
                    if (line & 0x8000) {
                        if ((xx >= 0) && (xx < 256)) {
                            if (tms.dBackMem[yy*256+xx]) {
                                tms.StatusReg |= 0x20;
                            } else {
                                tms.dBackMem[yy*256+xx] = 0xff;
                                if (c && bmp) bmp->line[yy][xx] =
				    Machine->pens[c];
                            }
                        }
                    }
                    line *= 2;
                }
            }
        } else {
            /* draw enlarged sprite */
            for (i=0;i<size;i++) {
                yy=y+i*2;
                line2 = 256*patternptr[i] + patternptr[i+16];
                for (j=0;j<2;j++) {
                    if ( (yy >= 0) && (yy <= 191) ) {
                        if (limit[yy] == 0) {
                            /* illegal sprite line */
                            if (yy < illegalspriteline) {
                                illegalspriteline = yy;
                                 illegalsprite = p;
                            } else if (illegalspriteline == yy) {
                                if (illegalsprite > p) {
                                    illegalsprite = p;
                                }
                            }
                            if (tms.LimitSprites) continue;
                        } else limit[yy]--;
                        line = line2;
                        for (xx=x;xx<(x+size*2);xx+=2) {
                            if (line & 0x8000) {
                                if ((xx >=0) && (xx < 256)) {
                                    if (tms.dBackMem[yy*256+xx]) {
                                        tms.StatusReg |= 0x20;
                                    } else {
                                        tms.dBackMem[yy*256+xx] = 0xff;
                                        if (c && bmp) bmp->line[yy][xx] =
                                            Machine->pens[c];
                                    }
                                }
                                if (((xx+1) >=0) && ((xx+1) < 256)) {
                                    if (tms.dBackMem[yy*256+xx+1]) {
                                        tms.StatusReg |= 0x20;
                                    } else {
                                        tms.dBackMem[yy*256+xx+1] = 0xff;
                                        if (c && bmp) bmp->line[yy][xx+1] =
                                            Machine->pens[c];
                                    }
                                }
                            }
                            line *= 2;
                        }
                    }
                    yy++;
                }
            }
        }
    }
    if (illegalspriteline == 255) {
        tms.StatusReg |= (p > 31) ? 31 : p;
    } else {
        tms.StatusReg |= 0x40 + illegalsprite;
    }
}
