/***************************************************************************

  apple1.c

  Functions to emulate the video hardware of the Jupiter Ace.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/includes/apple1.h"

static	int	dsp_pntr;

int	apple1_vh_start (void)
{
	dsp_pntr = 0;
	if (!(videoram = malloc (videoram_size = 40 * 24)))
		return (1);;
	if (generic_vh_start ())
		return (1);

	memset (videoram, 0, videoram_size);
	memset (dirtybuffer, 1, videoram_size);
	return (0);
}

void	apple1_vh_stop (void)
{
	generic_vh_stop ();
}

void	apple1_vh_dsp_w (int data)
{
	int	loop;

	data &= 0x7f;

	switch (data) {
	case 0x0a:
	case 0x0d:
		dirtybuffer[dsp_pntr] = 1;
		dsp_pntr += 40 - (dsp_pntr % 40);
		break;
	case 0x5f:
		dirtybuffer[dsp_pntr] = 1;
		if (dsp_pntr) dsp_pntr--;
		videoram[dsp_pntr] = 0;
		break;
	default:
		dirtybuffer[dsp_pntr] = 1;
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
					dirtybuffer[loop - 40] = 1;
				}
			}
			else if (videoram[loop - 40] != videoram[loop])
			{
				videoram[loop - 40] = videoram[loop];
				dirtybuffer[loop - 40] = 1;
			}
		}
		for (loop = 920; loop < 960; loop++)
		{
			if (!(!videoram[loop] || ((videoram[loop] > 1) &&
						(videoram[loop] <= 32)) || (videoram[loop] > 96)))
			{
				videoram[loop] = 0;
				dirtybuffer[loop] = 1;
			}
		}
		dsp_pntr -= 40;
	}
}

void	apple1_vh_dsp_clr (void)
{
	int	loop;

	dirtybuffer[dsp_pntr] = 1;
	dsp_pntr = 0;
	for (loop = 0; loop < 960; loop++)
	{
		if (!(!videoram[loop] || ((videoram[loop] > 1) &&
						(videoram[loop] <= 32)) || (videoram[loop] > 96)))
		{
			dirtybuffer[loop] = 1;
		}
		videoram[loop] = 0;
	}
}

void	apple1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int offs;
	int code;
	int sx, sy;

	/* do we need a full refresh? */

	if (full_refresh) memset (dirtybuffer, 1, videoram_size);

	for (offs = 0; offs < videoram_size; offs++ )
	{
		if (dirtybuffer[offs] || (offs == dsp_pntr))
		{
			if (offs == dsp_pntr) code = 1;
			else code = videoram[offs];
			sy = (offs / 40) * 8;
			sx = (offs % 40) * 6;

			drawgfx (bitmap, Machine->gfx[0], code, 1,
			  0, 0, sx,sy, &Machine->visible_area, TRANSPARENCY_NONE, 0);

			dirtybuffer[offs] = 0;
		}
	}
}
