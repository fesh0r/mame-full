/***************************************************************************

  avigo.c

  Functions to emulate the video hardware of the TI Avigo 100 PDA

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/avigo.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/


/* mem size = 0x017c0 */

static unsigned char *avigo_video_memory;

/* current column to read/write */
static UINT8 avigo_screen_column = 0;

READ_HANDLER(avigo_vid_memory_r)
{
        unsigned char *ptr;

        if (offset==0)
        {
           return avigo_screen_column;
        }

        if ((offset<0x0100) || (offset>=0x01f0))
        {
           logerror("vid mem read: %04x\r\n", offset);
           return 0;
        }

        /* 0x0100-0x01f0 contains data for selected column */
        ptr = avigo_video_memory + avigo_screen_column + ((offset-0x0100)*(AVIGO_SCREEN_WIDTH>>3));

        return ptr[0];
}

WRITE_HANDLER(avigo_vid_memory_w)
{
        if (offset==0)
        {
          /* select column to read/write */
          avigo_screen_column = data;

          if (data>=(AVIGO_SCREEN_WIDTH>>3))
          {
            data = 0;

            logerror("vid mem column write: %02x\r\n",data);
          }
          return;
        }

        if ((offset<0x0100) || (offset>=0x01f0))
        {
           logerror("vid mem write: %04x %02x\r\n", offset, data);
           return;
        }

        if ((offset>=0x0100) && (offset<=0x01f0))
        {
                unsigned char *ptr;

                /* 0x0100-0x01f0 contains data for selected column */
                ptr = avigo_video_memory + avigo_screen_column + ((offset-0x0100)*(AVIGO_SCREEN_WIDTH>>3));

                ptr[0] = data;

                return;
        }
}


int avigo_vh_start(void)
{
        /* current selected column to read/write */
        avigo_screen_column = 0;

        /* allocate video memory */
        avigo_video_memory = malloc(((AVIGO_SCREEN_WIDTH>>3)*AVIGO_SCREEN_HEIGHT));

	return 0;
}

void    avigo_vh_stop(void)
{
        if (avigo_video_memory!=NULL)
        {
                free(avigo_video_memory);
                avigo_video_memory = NULL;
        }
}

/* two colours */
static unsigned short avigo_colour_table[AVIGO_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static unsigned char avigo_palette[AVIGO_NUM_COLOURS * 3] =
{
	0x000, 0x000, 0x000,
	0x0ff, 0x0ff, 0x0ff
};


/* Initialise the palette */
void avigo_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
        memcpy(sys_palette, avigo_palette, sizeof (avigo_palette));
        memcpy(sys_colortable, avigo_colour_table, sizeof (avigo_colour_table));
}



/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void avigo_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
        int y;
        int b;
        int x;
        int pen0, pen1;

        pen0 = Machine->pens[0];
        pen1 = Machine->pens[1];

        for (y=0; y<AVIGO_SCREEN_HEIGHT; y++)
        {
                int by;

                unsigned char *line_ptr = avigo_video_memory +  (y*(AVIGO_SCREEN_WIDTH>>3));
				
                x = 0;
                for (by=0; by<AVIGO_SCREEN_WIDTH>>3; by++)
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
