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

extern int z88_display_memory_start;
extern char *z88_memory;

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this fuz88tion,
  it will be called by the main emulation engine.
***************************************************************************/
void z88_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        int y;
        int b;
        int x;
        int pen0, pen1;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

        for (y=0; y<Z88_SCREEN_HEIGHT; y++)
        {
                int by;
                /* 64 bytes per line */
#if 0
                char *line_ptr = z88_memory + z88_display_memory_start + (y<<6);
#else
				char *line_ptr = NULL;	/* DOH! :-) */
#endif

                x = 0;
                for (by=0; by<Z88_SCREEN_WIDTH>>3; by++)
                {
                        unsigned char byte;
        
                        byte = line_ptr[0];
        
                        for (b=0; b<8; b++)
                        {
                                if (byte & 0x080)
                                {
                                        plot_pixel(bitmap,x+b, y, pen1);
                                }
                                else
                                {
                                        plot_pixel(bitmap,x+b, y, pen0);
        
                                }
                                byte = byte<<1;
                        }
        
                        x = x + 8;
                                        
                        line_ptr = line_ptr+1;
                }
         }
}
