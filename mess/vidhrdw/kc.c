/***************************************************************************

  kc.c

  Functions to emulate the video hardware of the kc85/4.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

extern unsigned char *kc85_4_display_video_ram;

int kc85_4_vh_start(void)
{

	return 0;
}

void    kc85_4_vh_stop(void) 
{

}


/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void kc85_4_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        unsigned char *pixel_ram = kc85_4_display_video_ram;
        unsigned char *colour_ram = pixel_ram + 0x04000;

        int i;

        for (i=0; i<0x04000; i++)
        {
                int a;
                int gfx_byte;
                int x,y;
                int foreground_pen, background_pen;
                int colour;

                x = ((i>>8) & 0x03f)*8;
                y = (i & 0x0ff);

                //x = (i>>5) & 0x01f8;
                //y = i & 0x0ff;

                if (x>=320)
                        x = 319;

                if (y>=(32*8))
                        y = (32*8)-1;


                colour = colour_ram[i];

                background_pen = (colour&7) | 0x010;
                foreground_pen = (colour>>3) & 0x015;

                background_pen = Machine->pens[background_pen];
                foreground_pen = Machine->pens[foreground_pen];

                gfx_byte = pixel_ram[i];

                for (a=0; a<8; a++)
                {
                        int pen;

                        if (gfx_byte & 0x080)
                        {
                            pen = foreground_pen;
                        }
                        else
                        {
                            pen = background_pen;
                        }

                        plot_pixel(bitmap, x+a, y, pen);

                        gfx_byte = gfx_byte<<1;
                }


        }

}

