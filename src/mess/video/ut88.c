/***************************************************************************

        UT88 video driver by Miodrag Milanovic

        06/03/2008 Preliminary driver.

****************************************************************************/


#include "includes/ut88.h"


const gfx_layout ut88_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

VIDEO_START( ut88 )
{
}

SCREEN_UPDATE_IND16( ut88 )
{
	ut88_state *state = screen.machine().driver_data<ut88_state>();
	int x,y;

	for(y = 0; y < 28; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = state->m_p_videoram[ x + y*64 ] & 0x7f;
			int attr = state->m_p_videoram[ x+1 + y*64 ] & 0x80;
			drawgfx_opaque(bitmap, cliprect, screen.machine().gfx[0], code | attr, 0, 0,0, x*8,y*8);
		}
	}
	return 0;
}

