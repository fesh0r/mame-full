/***************************************************************************

  vidhrdw/advision.c

  Routines to control the Adventurevision video hardware
  
  Video hardware is composed of a vertical array of 40 LEDs which is
  reflected off a spinning mirror. 

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *adv_bitmap;
extern int advision_framestart;
extern int advision_videoenable;
extern int advision_videobank;

int advision_vh_hpos;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int advision_vh_start(void)
{
    advision_vh_hpos = 0;
    if ((adv_bitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
    return 0;
}
            
/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
static unsigned char advision_palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x00,0x00, /* RED */
	0x55,0x55,0x55, /* DK GREY - for MAME text only */
	0x80,0x80,0x80, /* LT GREY - for MAME text only */
};

static unsigned short advision_colortable[] =
{
	0x00, 0x01,
	0x01, 0x00,
    0x01, 0x00,
    0x00, 0x01
};

void advision_vh_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
    memcpy(game_palette,advision_palette,3*4);
    memcpy(game_colortable,advision_colortable,8 * sizeof(unsigned short));
}

void advision_vh_stop(void)
{
    osd_free_bitmap(adv_bitmap);
}

void advision_vh_write(int data) {
    int ypos;
    int i;
    
    ypos = (5 - advision_videobank) * 16;
    for(i=0; i<8; i++) {
        if (data & 0x80) {
            adv_bitmap->line[ypos + (i*2) + 30][advision_vh_hpos + 85] = Machine->pens[0];
        }
        else {
            adv_bitmap->line[ypos + (i*2) + 30][advision_vh_hpos + 85] = Machine->pens[1];
        }       
        data = data << 1;
    }
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

void advision_vh_screenrefresh(struct osd_bitmap *bitmap)
{
    advision_framestart = 1;
    advision_vh_hpos=0;
    copybitmap(bitmap,adv_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}


