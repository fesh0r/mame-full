/***************************************************************************

  vidhrdw/odyssey2.c

  Routines to control the Adventurevision video hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"

static UINT8 *odyssey2_display;
int odyssey2_vh_hpos;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int odyssey2_vh_start(void)
{
    odyssey2_vh_hpos = 0;
	odyssey2_display = (UINT8 *)malloc(8 * 8 * 256);
	if( !odyssey2_display )
		return 1;
	memset(odyssey2_display, 0, 8 * 8 * 256);
    return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void odyssey2_vh_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
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

void odyssey2_vh_stop(void)
{
	if( odyssey2_display )
		free(odyssey2_display);
	odyssey2_display = NULL;
}

void odyssey2_vh_write(int data)
{
	return;
}

void odyssey2_vh_update(int x)
{
    return;
}


/***************************************************************************

  Refresh the video screen

***************************************************************************/

void odyssey2_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	return;
}


