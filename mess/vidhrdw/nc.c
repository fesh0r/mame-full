/***************************************************************************

  nc.c

  Functions to emulate the video hardware of the Amstrad PCW.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/nc.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

int nc_vh_start(void)
{

	return 0;
}

void    nc_vh_stop(void)
{
}

/* two colours */
static unsigned short nc_colour_table[NC_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static unsigned char nc_palette[NC_NUM_COLOURS * 3] =
{
	0x000, 0x000, 0x000,
	0x0ff, 0x0ff, 0x0ff
};


/* Initialise the palette */
void nc_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
        memcpy(sys_palette, nc_palette, sizeof (nc_palette));
        memcpy(sys_colortable, nc_colour_table, sizeof (nc_colour_table));
}

extern int nc_display_memory_start;
extern char *nc_memory;

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void nc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        int y;
        int b;
        int x;
        int pen0, pen1;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

        for (y=0; y<NC_SCREEN_HEIGHT; y++)
        {
                int by;
                /* 64 bytes per line */
                char *line_ptr = nc_memory + nc_display_memory_start + (y<<6);

                x = 0;
                for (by=0; by<NC_SCREEN_WIDTH>>3; by++)
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
