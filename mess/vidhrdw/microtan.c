/***************************************************************************
 *	Microtan 65
 *
 *	video hardware
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *	Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *	and to Fabrice Frances <frances@ensica.fr>
 *	for his site http://www.ifrance.com/oric/microtan.html
 *
 ***************************************************************************/

#include "includes/microtan.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

UINT8 microtan_chunky_graphics = 0;
UINT8 *microtan_chunky_buffer = NULL;

char microtan_frame_message[64+1];
int microtan_frame_time = 0;

void microtan_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	palette[0*3+0] = 0x00;
	palette[0*3+1] = 0x00;
	palette[0*3+2] = 0x00;

    palette[1*3+0] = 0xff;
	palette[1*3+1] = 0xff;
	palette[1*3+2] = 0xff;

	colortable[0] = 0;
	colortable[1] = 1;
}

WRITE_HANDLER( microtan_videoram_w )
{
	if (videoram[offset] != data || microtan_chunky_buffer[offset] != microtan_chunky_graphics)
	{
		videoram[offset] = data;
		microtan_chunky_buffer[offset] = microtan_chunky_graphics;
		dirtybuffer[offset] = 1;
	}
}

int microtan_vh_start(void)
{
	if (video_start_generic())
		return 1;
	microtan_chunky_buffer = auto_malloc(videoram_size);
    microtan_chunky_graphics = 0;
	memset(microtan_chunky_buffer, microtan_chunky_graphics, sizeof(microtan_chunky_buffer));

    return 0;
}

void microtan_vh_stop(void)
{
	microtan_chunky_buffer = NULL;
}

void microtan_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
    int offs;

	if( microtan_frame_time > 0 )
    {
		ui_text(bitmap, microtan_frame_message, 1, Machine->visible_area.max_y - 9);
        /* if the message timed out, clear it on the next frame */
		if( --microtan_frame_time == 0 )
			full_refresh = 1;
    }

    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < 32*16; offs++ )
	{
		if( dirtybuffer[offs] )
		{
			int sx, sy, code;
			sy = (offs / 32) * 16;
			sx = (offs % 32) * 8;
			code = videoram[offs];
			drawgfx(bitmap,Machine->gfx[microtan_chunky_buffer[offs]],code,0,0,0,sx,sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
            dirtybuffer[offs] = 0;
		}
	}
}


