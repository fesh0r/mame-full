/***************************************************************************
	vtech1.c

	Video Technology Models (series 1)
	Laser 110 monochrome
	Laser 210
		Laser 200 (same hardware?)
		aka VZ 200 (Australia)
		aka Salora Fellow (Finland)
		aka Texet8000 (UK)
	Laser 310
        aka VZ 300 (Australia)

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/vtech1.h"

/* from machine/vz.c */
//extern int vtech1_latch;

char vtech1_frame_message[64+1];
int vtech1_frame_time = 0;

void vtech1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

	if( vtech1_frame_time > 0 )
    {
		ui_text(bitmap, vtech1_frame_message, 1, Machine->visible_area.max_y - 9);
        /* if the message timed out, clear it on the next frame */
        if( --vtech1_frame_time == 0 )
			full_refresh = 1;
    }

    if( full_refresh )
	{
		if( vtech1_latch & 0x08 )
		{
			if( vtech1_latch & 0x10 )
				fillbitmap(Machine->scrbitmap, Machine->pens[5], &Machine->visible_area);
			else
				fillbitmap(Machine->scrbitmap, Machine->pens[1], &Machine->visible_area);
		}
        else
		{
			fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
//			fillbitmap(Machine->scrbitmap, Machine->pens[16], &Machine->visible_area); // only 13 entries
		}
        memset(dirtybuffer, 0xff, videoram_size);
    }

	if( vtech1_latch & 0x08 )
    {
        /* graphics mode */
		int color = (vtech1_latch & 0x10) ? 1 : 0;
        for( offs = 0; offs < videoram_size; offs++ )
        {
            if( dirtybuffer[offs] )
            {
                int sx, sy, code;
				sy = 20 + (offs / 32) * 3;
				sx = 16 + (offs % 32) * 8;
                code = videoram[offs];
				drawgfx(bitmap,Machine->gfx[1],code,color,0,0,sx,sy,
                    &Machine->visible_area,TRANSPARENCY_NONE,0);
                dirtybuffer[offs] = 0;
            }
        }
    }
    else
    {
        /* text mode */
        for( offs = 0; offs < 32*16; offs++ )
        {
			if( dirtybuffer[offs] )
            {
                int sx, sy, code, color;
				sy = 20 + (offs / 32) * 12;
				sx = 16 + (offs % 32) * 8;
                code = videoram[offs];
				if( vtech1_latch & 0x10 )
					color = (code & 0x80) ? ((code >> 4) & 7) : 9;
                else
					color = (code & 0x80) ? ((code >> 4) & 7) : 8;
				drawgfx(bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
                    &Machine->visible_area,TRANSPARENCY_NONE,0);
                dirtybuffer[offs] = 0;
            }
        }
    }
}

