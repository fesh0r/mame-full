/***************************************************************************
	vz.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	Guy Thomason, Jason Oakley, Bushy Maunder and anybody else
	on the vzemu list :)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from machine/vz.c */
extern int vz_latch;

char vz_frame_message[64+1];
int vz_frame_time = 0;

void vz_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    int offs;

    if( full_refresh )
	{
		if( vz_latch & 0x08 )
            fillbitmap(Machine->scrbitmap, Machine->pens[1], &Machine->drv->visible_area);
        else
            fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->drv->visible_area);
        memset(dirtybuffer, 0xff, videoram_size);
    }

    if( vz_latch & 0x08 )
    {
		int color = (vz_latch & 0x10) ? 1 : 0;
        for( offs = 0; offs < videoram_size; offs++ )
        {
            if( dirtybuffer[offs] )
            {
                int sx, sy, code;
				sy = 20 + (offs / 32) * 3;
				sx = 16 + (offs % 32) * 8;
                code = videoram[offs];
                drawgfx(
					bitmap,Machine->gfx[1],code,color,0,0,sx,sy,
                    &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				osd_mark_dirty(sx,sy,sx+7,sy+2,0);
                dirtybuffer[offs] = 0;
            }
        }
    }
    else
    {
        for( offs = 0; offs < 32*16; offs++ )
        {
			if( dirtybuffer[offs] )
            {
                int sx, sy, code, color;
				sy = 20 + (offs / 32) * 12;
				sx = 16 + (offs % 32) * 8;
                code = videoram[offs];
				if( vz_latch & 0x10 )
					color = (code & 0x80) ? ((code >> 4) & 7) + 4 : 9;
                else
					color = (code & 0x80) ? ((code >> 4) & 7) : 0;
                drawgfx(
					bitmap,Machine->gfx[0],code,color,0,0,sx,sy,
                    &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				osd_mark_dirty(sx,sy,sx+7,sy+11,0);
                dirtybuffer[offs] = 0;
            }
        }
    }
	if( vz_frame_time > 0 )
	{
		ui_text(vz_frame_message, 2, Machine->drv->visible_area.max_y - 9);
		/* if the message timed out, clear it on the next frame */
		if( --vz_frame_time == 0 )
			memset(dirtybuffer, 1, videoram_size);
	}
}


