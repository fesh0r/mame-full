/***************************************************************************

  z88.c

  Functions to emulate the video hardware of the Amstrad PCW.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/z88.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

int z88_vh_start(void)
{

	return 0;
}

void    z88_vh_stop(void)
{
}

/* two colours */
static unsigned short z88_colour_table[Z88_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static unsigned char z88_palette[Z88_NUM_COLOURS * 3] =
{
	0x000, 0x000, 0x000,
	0x0ff, 0x0ff, 0x0ff
};


/* Initialise the palette */
void z88_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
        memcpy(sys_palette, z88_palette, sizeof (z88_palette));
        memcpy(sys_colortable, z88_colour_table, sizeof (z88_colour_table));
}

extern unsigned char *z88_memory;
extern struct blink_hw blink;

/* temp - change to gfxelement structure */

static void z88_render_8x8(struct osd_bitmap *bitmap, int x, int y, unsigned char *pData)
{
        int h,b;
        int pen0, pen1;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

        for (h=0; h<8; h++)
        {
            UINT8 data;

            data = pData[y];
            for (b=0; b<8; b++)
            {
                int pen;

                if (data & 0x080)
                {
                  pen = pen1;
                }
                else
                {
                  pen = pen0;
                }

                plot_pixel(bitmap, x+b, y+h, pen);

                data = data<<1;
            }
        }
}

static void z88_render_6x8(struct osd_bitmap *bitmap, int x, int y, unsigned char *pData)
{
        int h,b;
        int pen0, pen1;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

        for (h=0; h<8; h++)
        {
            UINT8 data;

            data = pData[y];
            for (b=0; b<6; b++)
            {
                int pen;
                if (data & 0x080)
                {
                  pen = pen1;
                }
                else
                {
                  pen = pen0;
                }

                plot_pixel(bitmap, x+1+b, y+h, pen);
                data = data<<1;

            }
        }
}



/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this fuz88tion,
  it will be called by the main emulation engine.
***************************************************************************/
void z88_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        int x,y;
        unsigned char *ptr = z88_memory + blink.sbf;

        for (y=0; y<(Z88_SCREEN_HEIGHT>>3); y++)
        {
                for (x=0; x<(Z88_SCREEN_WIDTH>>3); x++)
                {
                  int char_index;

                  UINT8 byte0, byte1;

                  byte0 = ptr[(x<<1)];
                  byte1 = ptr[(x<<2)];

                  char_index = (byte0 & 0x0ff) | ((byte1 & 0x01)<<8);



                }

                ptr+=256;
        }
}
