/***************************************************************************

  trs80.c

  Functions to emulate the video hardware of the TRS80.

***************************************************************************/

#include "driver.h"
#include "drivers/trs80.h"

#define FW  TRS80_FONT_W
#define FH  TRS80_FONT_H

extern  byte port_ff;
extern  void trs80_shutdown_machine(void);
/* the following defined in gfxlayer.c and set by layer_mark_all_dirty */
/* we don't use gfxlayer.c functions, but need to know when to redraw it all */
extern  int  scrbitmap_dirty;   

/***************************************************************************
  Statics for this module
***************************************************************************/
static  int color = 0;
static  int scanlines = 0;

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
int trs80_vh_start(void)
{
        videoram_size = 16 * 64;

        if (generic_vh_start() != 0)
		return 1;

        memset(dirtybuffer, 0, videoram_size);

        return 0;
}

void    trs80_vh_stop(void)
{
        generic_vh_stop();

        /* kludge to have the trs80 shutdown called */
        trs80_shutdown_machine();
}

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void trs80_vh_screenrefresh(struct osd_bitmap *bitmap)
{
/* some tables to reduce calculating odd multiplications */
static  int mx[64] = {
         0*FW*2, 1*FW*2, 2*FW*2, 3*FW*2, 4*FW*2, 5*FW*2, 6*FW*2, 7*FW*2,
         8*FW*2, 9*FW*2,10*FW*2,11*FW*2,12*FW*2,13*FW*2,14*FW*2,15*FW*2,
        16*FW*2,17*FW*2,18*FW*2,19*FW*2,20*FW*2,21*FW*2,22*FW*2,23*FW*2,
        24*FW*2,25*FW*2,26*FW*2,27*FW*2,28*FW*2,29*FW*2,30*FW*2,31*FW*2,
        32*FW*2,33*FW*2,34*FW*2,35*FW*2,36*FW*2,37*FW*2,38*FW*2,39*FW*2,
        40*FW*2,41*FW*2,42*FW*2,43*FW*2,44*FW*2,45*FW*2,46*FW*2,47*FW*2,
        48*FW*2,49*FW*2,50*FW*2,51*FW*2,52*FW*2,53*FW*2,54*FW*2,55*FW*2,
        56*FW*2,57*FW*2,58*FW*2,59*FW*2,60*FW*2,61*FW*2,62*FW*2,63*FW*2};

static  int my[16] = {
         0*FH*3, 1*FH*3, 2*FH*3, 3*FH*3, 4*FH*3, 5*FH*3, 6*FH*3, 7*FH*3,
         8*FH*3, 9*FH*3,10*FH*3,11*FH*3,12*FH*3,13*FH*3,14*FH*3,15*FH*3};

/* Special translation if video RAM with only 7 bits is present
   I don't know if it's entirely correct, but it's close ;-)    */
static  byte xlt_videoram[2][256] = {
       {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
        0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
        0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff},
       {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf}
};

        int offs = 0;
        int xlt = (input_port_0_r(0) & 0x40) ? 1 : 0;

        /* check for changed color settings */
        if (color != (input_port_0_r(0) & 7))
        {
                color = input_port_0_r(0) & 7;
                fillbitmap(tmpbitmap,Machine->pens[0],&Machine->drv->visible_area);
                scrbitmap_dirty = 1;
        }

        /* check for changed scanlines mode setting */
        if (scanlines != (input_port_0_r(0) & 8))
        {
        int chr, scanline;
                scanlines = input_port_0_r(0) & 8;
                if (scanlines)
                {
                        /* wipe out every third scan line in our 2x3 or 4x3 scaled characters */
                        for (chr = 0; chr < 256; chr++)
                        {
                                for (scanline = 0; scanline < FH * 3 / 2; scanline += 3)
                                {
                                        memset(Machine->gfx[0]->gfxdata->line[chr * Machine->gfx[0]->height + scanline+2], 0, FW * 2);
                                        memset(Machine->gfx[1]->gfxdata->line[chr * Machine->gfx[1]->height + scanline+2], 0, FW * 2);
                                        memset(Machine->gfx[2]->gfxdata->line[chr * Machine->gfx[2]->height + scanline+2], 0, FW * 4);
                                        memset(Machine->gfx[3]->gfxdata->line[chr * Machine->gfx[3]->height + scanline+2], 0, FW * 4);
                                }
                        }
                }
                else
                {
                        /* duplicate every third scan line in our 2x3 or 4x3 scaled characters */
                        for (chr = 0; chr < 256; chr++)
                        {
                                for (scanline = 0; scanline < FH * 3 / 2; scanline += 3)
                                {
                                        memcpy(Machine->gfx[0]->gfxdata->line[chr * Machine->gfx[0]->height + scanline+2],
                                               Machine->gfx[0]->gfxdata->line[chr * Machine->gfx[0]->height + scanline+1], FW * 2);
                                        memcpy(Machine->gfx[1]->gfxdata->line[chr * Machine->gfx[1]->height + scanline+2],
                                               Machine->gfx[1]->gfxdata->line[chr * Machine->gfx[1]->height + scanline+1], FW * 2);
                                        memcpy(Machine->gfx[2]->gfxdata->line[chr * Machine->gfx[2]->height + scanline+2],
                                               Machine->gfx[2]->gfxdata->line[chr * Machine->gfx[2]->height + scanline+1], FW * 4);
                                        memcpy(Machine->gfx[3]->gfxdata->line[chr * Machine->gfx[3]->height + scanline+2],
                                               Machine->gfx[3]->gfxdata->line[chr * Machine->gfx[3]->height + scanline+1], FW * 4);
                                }
                        }
                }
                scrbitmap_dirty = 1;
        }

        /* draw entire scrbitmap because of usrintrf functions
           called osd_clearbitmap or color change / scanline change */
        if (scrbitmap_dirty)
        {
                scrbitmap_dirty = 0;
                memset(dirtybuffer, 1, videoram_size);
        }

        /* do we have double width characters enabled ? */
        if (port_ff & 8)
        {
                /* for every second character in the Video RAM, check if it has
                   been modified since last time and update it accordingly. */
                for (offs = videoram_size - 2; offs >= 0; offs -= 2)
                {
                        if (dirtybuffer[offs])
                        {
                        struct rectangle r;

                                dirtybuffer[offs] = 0;

                                r.min_x = mx[offs % 64];
                                r.min_y = my[offs / 64];
                                r.max_x = r.min_x + FW * 4 - 1;
                                r.max_y = r.min_y + FH * 3 - 1;

                                /* draw the upper half */
                                drawgfx(bitmap,
                                        Machine->gfx[2],
                                        xlt_videoram[xlt][videoram[offs]],
                                        color,0,0,
                                        r.min_x,r.min_y,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                                /* draw the lower half */
                                drawgfx(bitmap,
                                        Machine->gfx[3],
                                        xlt_videoram[xlt][videoram[offs]],
                                        color,0,0,
                                        r.min_x,r.min_y + FH * 3 / 2,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                                osd_mark_dirty(r.min_x,r.min_y,r.max_x,r.max_y,1);
                        }
                }
        }
        else
        /* normal width characters */
        {
                /* for every character in the Video RAM, check if it has
                 * been modified since last time and update it accordingly. */
                for (offs = videoram_size - 1; offs >= 0; offs--)
                {
                        if (dirtybuffer[offs])
                        {
                        struct rectangle r;

                                dirtybuffer[offs] = 0;

                                r.min_x = mx[offs % 64];
                                r.min_y = my[offs / 64];
                                r.max_x = r.min_x + FW * 2 - 1;
                                r.max_y = r.min_y + FH * 3 - 1;

                                /* draw the upper half */
                                drawgfx(bitmap,
                                        Machine->gfx[0],
                                        xlt_videoram[xlt][videoram[offs]],
                                        color,0,0,
                                        r.min_x,r.min_y,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                                /* draw the lower half */
                                drawgfx(bitmap,
                                        Machine->gfx[1],
                                        xlt_videoram[xlt][videoram[offs]],
                                        color,0,0,
                                        r.min_x,r.min_y + FH * 3 / 2,
                                        &r,
                                        TRANSPARENCY_NONE,0);
                                osd_mark_dirty(r.min_x,r.min_y,r.max_x,r.max_y,1);
                        }
                }
        }
}

