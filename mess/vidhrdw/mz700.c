/***************************************************************************
 *	Sharp MZ700
 *
 *	video hardware
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *	Reference: http://sharpmz.computingmuseum.com
 *
 ***************************************************************************/

#include "mess/includes/mz700.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(N,M,A)	\
	if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define LOG(N,M,A)
#endif

char mz700_frame_message[64+1];
int mz700_frame_time = 0;

void mz700_init_colors (unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

    for (i = 0; i < 8; i++)
	{
		palette[i*3+0] = (i & 2) ? 0xff : 0x00;
		palette[i*3+1] = (i & 4) ? 0xff : 0x00;
		palette[i*3+2] = (i & 1) ? 0xff : 0x00;
	}

	for (i = 0; i < 256; i++)
	{
		colortable[i*2+0] = i & 7;
        colortable[i*2+1] = (i >> 4) & 7;
	}
}

int mz700_vh_start(void)
{
	if (generic_vh_start())
		return 1;
    return 0;
}

void mz700_vh_stop(void)
{
	generic_vh_stop();
}

void mz700_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

	if( mz700_frame_time > 0 )
    {
		ui_text(bitmap, mz700_frame_message, 1, Machine->visible_area.max_y - 9);
        /* if the message timed out, clear it on the next frame */
		if( --mz700_frame_time == 0 )
			full_refresh = 1;
    }

    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }

	for( offs = 0; offs < videoram_size; offs++ )
	{
		if( dirtybuffer[offs] )
		{
			int sx, sy, code, color;

            dirtybuffer[offs] = 0;

            sy = (offs / 40) * 8;
			sx = (offs % 40) * 8;
			code = videoram[offs];
			color = colorram[offs];
			code |= (color & 0x80) << 1;

            drawgfx(bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}
}


