/***************************************************************************

  apple1.c

  Functions to emulate the video hardware of the Apple 1.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple1.h"

static struct tilemap *apple1_tilemap;
static int dsp_pntr;

/**************************************************************************/

static void apple1_gettileinfo(int memory_offset)
{
	int code;	
	if (memory_offset == dsp_pntr)
		code = 1;
	else
		code = videoram[memory_offset];

	SET_TILE_INFO(
		0,		/* gfx */
		code,	/* character */
		0,		/* color */
		0)		/* flags */
}

/**************************************************************************/

VIDEO_START( apple1 )
{
	dsp_pntr = 0;
	videoram_size = 40 * 24;

	videoram = auto_malloc(videoram_size);

	apple1_tilemap = tilemap_create(
		apple1_gettileinfo,
		tilemap_scan_rows,
		TILEMAP_OPAQUE,
		7, 8,
		40, 24);
	
	if (!videoram || !apple1_tilemap)
		return 1;

	memset(videoram, 0, videoram_size);
	return 0;
}

void apple1_vh_dsp_w (int data)
{
	int	loop;

	data &= 0x7f;

	switch (data) {
	case 0x0a:
	case 0x0d:
		tilemap_mark_tile_dirty(apple1_tilemap, dsp_pntr);
		dsp_pntr += 40 - (dsp_pntr % 40);
		break;
	case 0x5f:
		tilemap_mark_tile_dirty(apple1_tilemap, dsp_pntr);
		if (dsp_pntr)
			dsp_pntr--;
		videoram[dsp_pntr] = 0;
		break;
	default:
		tilemap_mark_tile_dirty(apple1_tilemap, dsp_pntr);
		videoram[dsp_pntr] = data;
		dsp_pntr++;
		break;
	}

/* || */

	if (dsp_pntr >= 960)
	{
		for (loop = 40; loop < 960; loop++)
		{
			if (!videoram[loop - 40] || ((videoram[loop - 40] > 1) &&
										(videoram[loop - 40] <= 32)) ||
										(videoram[loop - 40] > 96))
			{
				if (!(!videoram[loop] || ((videoram[loop] > 1) &&
										(videoram[loop] <= 32)) ||
										(videoram[loop] > 96)))
				{
					videoram[loop - 40] = videoram[loop];
					tilemap_mark_tile_dirty(apple1_tilemap, loop - 40);
				}
			}
			else if (videoram[loop - 40] != videoram[loop])
			{
				videoram[loop - 40] = videoram[loop];
				tilemap_mark_tile_dirty(apple1_tilemap, loop - 40);
			}
		}
		for (loop = 920; loop < 960; loop++)
		{
			if (!(!videoram[loop] || ((videoram[loop] > 1) &&
						(videoram[loop] <= 32)) || (videoram[loop] > 96)))
			{
				videoram[loop] = 0;
				tilemap_mark_tile_dirty(apple1_tilemap, loop);
			}
		}
		dsp_pntr -= 40;
	}
}

void apple1_vh_dsp_clr (void)
{
	int	loop;

	tilemap_mark_tile_dirty(apple1_tilemap, dsp_pntr);
	dsp_pntr = 0;

	for (loop = 0; loop < 960; loop++)
	{
		if (!(!videoram[loop] || ((videoram[loop] > 1) &&
						(videoram[loop] <= 32)) || (videoram[loop] > 96)))
		{
			tilemap_mark_tile_dirty(apple1_tilemap, loop);
		}
		videoram[loop] = 0;
	}
}

VIDEO_UPDATE( apple1 )
{
	tilemap_mark_tile_dirty(apple1_tilemap, dsp_pntr);
	tilemap_draw(bitmap, NULL, apple1_tilemap, 0, 0);
}
