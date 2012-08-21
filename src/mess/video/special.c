/***************************************************************************

        Specialist video driver by Miodrag Milanovic

        15/03/2008 Preliminary driver.

****************************************************************************/


#include "includes/special.h"


VIDEO_START( special )
{
}

SCREEN_UPDATE_IND16( special )
{
	special_state *state = screen.machine().driver_data<special_state>();
	UINT8 code;
	int y, x, b;

	for (x = 0; x < 48; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code = state->m_p_videoram[y + x*256];
			for (b = 7; b >= 0; b--)
				bitmap.pix16(y, x*8+(7-b)) =  (code >> b) & 0x01;
		}
	}
	return 0;
}
VIDEO_START( specialp )
{
}

SCREEN_UPDATE_IND16( specialp )
{
	special_state *state = screen.machine().driver_data<special_state>();
	UINT8 code;
	int y, x, b;

	for (x = 0; x < 64; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code = state->m_p_videoram[y + x*256];
			for (b = 7; b >= 0; b--)
				bitmap.pix16(y, x*8+(7-b)) =  (code >> b) & 0x01;
		}
	}
	return 0;
}


const rgb_t specimx_palette[16] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0xaa), // 1
	MAKE_RGB(0x00, 0xaa, 0x00), // 2
	MAKE_RGB(0x00, 0xaa, 0xaa), // 3
	MAKE_RGB(0xaa, 0x00, 0x00), // 4
	MAKE_RGB(0xaa, 0x00, 0xaa), // 5
	MAKE_RGB(0xaa, 0x55, 0x00), // 6
	MAKE_RGB(0xaa, 0xaa, 0xaa), // 7
	MAKE_RGB(0x55, 0x55, 0x55), // 8
	MAKE_RGB(0x55, 0x55, 0xff), // 9
	MAKE_RGB(0x55, 0xff, 0x55), // A
	MAKE_RGB(0x55, 0xff, 0xff), // B
	MAKE_RGB(0xff, 0x55, 0x55), // C
	MAKE_RGB(0xff, 0x55, 0xff), // D
	MAKE_RGB(0xff, 0xff, 0x55), // E
	MAKE_RGB(0xff, 0xff, 0xff)  // F
};

PALETTE_INIT( specimx )
{
	palette_set_colors(machine, 0, specimx_palette, ARRAY_LENGTH(specimx_palette));
}


VIDEO_START( specimx )
{
	special_state *state = machine.driver_data<special_state>();
	state->m_specimx_colorram = auto_alloc_array(machine, UINT8, 0x3000);
	memset(state->m_specimx_colorram,0x70,0x3000);
}

SCREEN_UPDATE_IND16( specimx )
{
	special_state *state = screen.machine().driver_data<special_state>();
	UINT8 code, color;
	int y, x, b;

	for (x = 0; x < 48; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code = state->m_ram->pointer()[0x9000 + y + x*256];
			color = state->m_specimx_colorram[y + x*256];
			for (b = 7; b >= 0; b--)
				bitmap.pix16(y, x*8+(7-b)) =  ((code >> b) & 0x01)==0 ? color & 0x0f : (color >> 4)& 0x0f ;
		}
	}
	return 0;
}

static const rgb_t erik_palette[8] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0xff), // 1
	MAKE_RGB(0xff, 0x00, 0x00), // 2
	MAKE_RGB(0xff, 0x00, 0xff), // 3
	MAKE_RGB(0x00, 0xff, 0x00), // 4
	MAKE_RGB(0x00, 0xff, 0xff), // 5
	MAKE_RGB(0xff, 0xff, 0x00), // 6
	MAKE_RGB(0xff, 0xff, 0xff)  // 7
};

PALETTE_INIT( erik )
{
	palette_set_colors(machine, 0, erik_palette, ARRAY_LENGTH(erik_palette));
}


VIDEO_START( erik )
{
}

SCREEN_UPDATE_IND16( erik )
{
	special_state *state = screen.machine().driver_data<special_state>();
	UINT8 code1, code2, color1, color2;
	int y, x, b;
	UINT8 *erik_video_ram_p1, *erik_video_ram_p2;

	erik_video_ram_p1 = state->m_ram->pointer() + 0x9000;
	erik_video_ram_p2 = state->m_ram->pointer() + 0xd000;

	for (x = 0; x < 48; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code1  = erik_video_ram_p1[y + x*256];
			code2  = erik_video_ram_p2[y + x*256];

			for (b = 7; b >= 0; b--)
			{
				color1 = ((code1 >> b) & 0x01)==0 ? state->m_erik_background : state->m_erik_color_1;
				color2 = ((code2 >> b) & 0x01)==0 ? state->m_erik_background : state->m_erik_color_2;
				bitmap.pix16(y, x*8+(7-b)) =  color1 | color2;
			}
		}
	}
	return 0;
}
