/***************************************************************************

  saa5050.c

  Functions to emulate the video hardware of the saa5050.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static	INT8	frame_count;

extern struct GfxLayout saa5050_charlayout;

#define SAA5050_DBLHI	0x0001
#define SAA5050_SEPGR	0x0002
#define SAA5050_FLASH	0x0004
#define SAA5050_BOX		0x0008
#define SAA5050_GRAPH	0x0010
#define SAA5050_CONCEAL	0x0020
#define SAA5050_HOLDGR	0x0040

#define SAA5050_BLACK   0
#define SAA5050_WHITE   7

struct	{
	UINT16	saa5050_flags;
	UINT8	saa5050_forecol;
	UINT8	saa5050_backcol;
	UINT8	saa5050_prvcol;
	UINT8	saa5050_prvchr;
} saa5050_state;


int	saa5050_vh_start (void)
{
	frame_count = 0;
	if( generic_vh_start() )
		return 1;
    return 0;
}

void saa5050_vh_stop (void)
{
	generic_vh_stop();
}

void saa5050_vh_callback (void)
{
	if (frame_count++ > 49) frame_count = 0;
}

/* || */

/* BOX and dirtybuffer not implemented */

void saa5050_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{

int code, colour;
int sx, sy;

if (full_refresh) memset (dirtybuffer, 1, videoram_size);

for (sy = 0; sy < 24; sy++) {

  /* Set start of line state */

  saa5050_state.saa5050_flags = 0;
  saa5050_state.saa5050_prvchr = 32;
  saa5050_state.saa5050_forecol = SAA5050_WHITE;
  saa5050_state.saa5050_prvcol = SAA5050_WHITE;

  for (sx = 0; sx < 40; sx++) {
	code = videoram[sy * 80 + sx];
	if (code < 32) {
	  switch (code) {
	  case 0x01: case 0x02: case 0x03: case 0x04:
	  case 0x05: case 0x06: case 0x07:
		saa5050_state.saa5050_prvcol = saa5050_state.saa5050_forecol = code;
		saa5050_state.saa5050_flags &= ~(SAA5050_GRAPH | SAA5050_CONCEAL);
		break;
	  case 0x11: case 0x12: case 0x13: case 0x14:
	  case 0x15: case 0x16: case 0x17:
		saa5050_state.saa5050_prvcol = (saa5050_state.saa5050_forecol =
															(code & 0x07));
		saa5050_state.saa5050_flags &= ~SAA5050_CONCEAL;
		saa5050_state.saa5050_flags |= SAA5050_GRAPH;
		break;
  	  case 0x08:
		saa5050_state.saa5050_flags |= SAA5050_FLASH;
		break;
  	  case 0x09:
		saa5050_state.saa5050_flags &= ~SAA5050_FLASH;
		break;
  	  case 0x0a:
		saa5050_state.saa5050_flags |= SAA5050_BOX;
		break;
  	  case 0x0b:
		saa5050_state.saa5050_flags &= ~SAA5050_BOX;
		break;
  	  case 0x0c:
		saa5050_state.saa5050_flags &= ~SAA5050_DBLHI;
		break;
  	  case 0x0d:
		saa5050_state.saa5050_flags |= SAA5050_DBLHI;
		break;
	  case 0x18:
		saa5050_state.saa5050_flags |= SAA5050_CONCEAL;
		break;
  	  case 0x19:
		saa5050_state.saa5050_flags |= SAA5050_SEPGR;
		break;
  	  case 0x1a:
		saa5050_state.saa5050_flags &= ~SAA5050_SEPGR;
		break;
	  case 0x1c:
		saa5050_state.saa5050_backcol = SAA5050_BLACK;
		break;
	  case 0x1d:
		saa5050_state.saa5050_backcol = saa5050_state.saa5050_prvcol;
		break;
  	  case 0x1e:
		saa5050_state.saa5050_flags |= SAA5050_HOLDGR;
		break;
  	  case 0x1f:
		saa5050_state.saa5050_flags &= ~SAA5050_HOLDGR;
		break;
	  }
	  if (saa5050_state.saa5050_flags & SAA5050_HOLDGR)
	  									code = saa5050_state.saa5050_prvchr;
	  else code = 32;
	}
	if (saa5050_state.saa5050_flags & SAA5050_CONCEAL) code = 32;
	else if ((saa5050_state.saa5050_flags & SAA5050_FLASH) &&
											(frame_count > 38)) code = 32;
	else {
	  saa5050_state.saa5050_prvchr = code;
	  if ((saa5050_state.saa5050_flags & SAA5050_GRAPH) && (code & 0x20)) {
		code += (code & 0x40) ? 64 : 96;
		if (saa5050_state.saa5050_flags & SAA5050_SEPGR) code += 64;
	  }
	}
	/* if (dirtybuffer[sy * 80 + sx]) { */

	  if (code & 0x80)
	  colour = (saa5050_state.saa5050_forecol << 3) |
	  								saa5050_state.saa5050_backcol;
	  else colour = saa5050_state.saa5050_forecol |
	  								(saa5050_state.saa5050_backcol << 3);
	  if (saa5050_state.saa5050_flags & SAA5050_DBLHI) {
		drawgfx (bitmap, Machine->gfx[1], code, colour, 0, 0,
		  sx * 6, sy * 10, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		drawgfx (bitmap, Machine->gfx[2], code, colour, 0, 0,
		  sx * 6, (sy + 1) * 10, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	  } else {
		drawgfx (bitmap, Machine->gfx[0], code, colour, 0, 0,
		  sx * 6, sy * 10, &Machine->visible_area, TRANSPARENCY_NONE, 0);
	  }

	  dirtybuffer[sy * 80 + sx] = 0;
	/* } */
  }
  if (saa5050_state.saa5050_flags & SAA5050_DBLHI) {
    sy++;
	saa5050_state.saa5050_flags &= ~SAA5050_DBLHI;
  }
}

}
