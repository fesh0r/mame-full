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

static struct tilemap *microtan_tilemap;

/**************************************************************************/

PALETTE_INIT( microtan )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0xff, 0xff, 0xff);

	colortable[0] = 0;
	colortable[1] = 1;
}

WRITE8_HANDLER( microtan_videoram_w )
{
	if (videoram[offset] != data || microtan_chunky_buffer[offset] != microtan_chunky_graphics)
	{
		videoram[offset] = data;
		microtan_chunky_buffer[offset] = microtan_chunky_graphics;
		tilemap_mark_tile_dirty(microtan_tilemap, offset);
	}
}

static void microtan_gettileinfo(int memory_offset)
{
	SET_TILE_INFO(
		microtan_chunky_buffer[memory_offset],	/* gfx */
		videoram[memory_offset],				/* character */
		0,										/* color */
		0)										/* flags */
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

VIDEO_START( microtan )
{
	microtan_chunky_buffer = auto_malloc(videoram_size);

	microtan_tilemap = tilemap_create(
		microtan_gettileinfo,
		tilemap_scan_rows,
		TILEMAP_OPAQUE,
		8, 16,
		32, 16);

	if (!microtan_chunky_buffer || !microtan_tilemap)
		return 1;

	microtan_chunky_graphics = 0;
	memset(microtan_chunky_buffer, microtan_chunky_graphics, sizeof(microtan_chunky_buffer));

	return 0;
}

VIDEO_UPDATE( microtan )
{
	if (microtan_frame_time > 0)
    {
		ui_text(bitmap, microtan_frame_message, 1, Machine->visible_area.max_y - 9);
        /* if the message timed out, clear it on the next frame */
		if( --microtan_frame_time == 0 )
			tilemap_mark_all_tiles_dirty(microtan_tilemap);
    }
	tilemap_draw(bitmap, NULL, microtan_tilemap, 0, 0);
}


