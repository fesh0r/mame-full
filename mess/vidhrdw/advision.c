/***************************************************************************

  vidhrdw/advision.c

  Routines to control the Adventurevision video hardware

  Video hardware is composed of a vertical array of 40 LEDs which is
  reflected off a spinning mirror.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/advision.h"

static UINT8 advision_led_latch[8];
static UINT8 *advision_display;

int advision_vh_hpos;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int advision_vh_start(void)
{
    advision_vh_hpos = 0;
	advision_display = (UINT8 *)malloc(8 * 8 * 256);
	if( !advision_display )
		return 1;
	memset(advision_display, 0, 8 * 8 * 256);
    return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void advision_vh_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	int i;
	for( i = 0; i < 8; i++ )
	{
		game_palette[i*3+0] = i * 0x22; /* 8 shades of RED */
		game_palette[i*3+1] = 0x00;
		game_palette[i*3+2] = 0x00;
		game_colortable[i*2+0] = 0;
		game_colortable[i*2+0] = i;
	}
	game_palette[8*3+0] = 0x55; /* DK GREY - for MAME text only */
	game_palette[8*3+1] = 0x55;
	game_palette[8*3+2] = 0x55;
	game_palette[9*3+0] = 0xf0; /* LT GREY - for MAME text only */
	game_palette[9*3+1] = 0xf0;
	game_palette[9*3+2] = 0xf0;
}

void advision_vh_stop(void)
{
	if( advision_display )
		free(advision_display);
	advision_display = NULL;
}

void advision_vh_write(int data)
{
	advision_led_latch[advision_videobank] = data;
}

void advision_vh_update(int x)
{
    UINT8 *dst = &advision_display[x];
	int y;

	for( y = 0; y < 8; y++ )
	{
		UINT8 data = advision_led_latch[7-y];
		if( (data & 0x80) == 0 ) dst[0 * 256] = 8;
		if( (data & 0x40) == 0 ) dst[1 * 256] = 8;
		if( (data & 0x20) == 0 ) dst[2 * 256] = 8;
		if( (data & 0x10) == 0 ) dst[3 * 256] = 8;
		if( (data & 0x08) == 0 ) dst[4 * 256] = 8;
		if( (data & 0x04) == 0 ) dst[5 * 256] = 8;
		if( (data & 0x02) == 0 ) dst[6 * 256] = 8;
		if( (data & 0x01) == 0 ) dst[7 * 256] = 8;
		advision_led_latch[7-y] = 0xff;
		dst += 8 * 256;
    }
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

void advision_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int x, y, bit;

    static int framecount = 0;

    if( (framecount++ % 8) == 0 )
	{
		advision_framestart = 1;
		advision_vh_hpos = 0;
    }

	for( x = (framecount%2)*128; x < (framecount%2)*128+128; x++ )
	{
		UINT8 *led = &advision_display[x];
		for( y = 0; y < 8; y++ )
		{
			for( bit = 0; bit < 8; bit++ )
			{
				if( *led > 0 )
					plot_pixel(bitmap, 85 + x, 30 + 2 *( y * 8 + bit), Machine->pens[--(*led)]);
				led += 256;
			}
		}
	}
}


