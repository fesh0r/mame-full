/*****************************************************************************
 *
 * video/abc802.c
 *
 ****************************************************************************/

#include "includes/abc80x.h"



// these are needed because the MC6845 emulation does
// not position the active display area correctly
#define HORIZONTAL_PORCH_HACK	121
#define VERTICAL_PORCH_HACK		29



//-------------------------------------------------
//  MC6845_UPDATE_ROW( abc802_update_row )
//-------------------------------------------------

static MC6845_UPDATE_ROW( abc802_update_row )
{
	/*

        PAL16R4 equation:

        IF (VCC)    *OS   = FC + RF / RC
                    *RG:  = HS / *RG + *ATE / *RG + ATD / *RG + LL /
                            *RG + AT1 / *RG + AT0 / ATE + *ATD + *LL +
                            *AT1 + *AT0
                    *RI:  = *RI + *INV / *RI + LL / *INV + *LL
                    *RF:  = HS / *RF + *ATE / *RF + ATD / *RF + LL /
                            *RF + AT1 / *RF + AT0 / ATE + *ATD + *LL +
                            *AT1 + AT0
                    *RC:  = HS / *RC + *ATE / *RC + *ATD / *RC + LL /
                            *RC + *ATI / *RC + AT0 / ATE + *LL + *AT1 +
                            *AT0
        IF (VCC)    *O0   = *CUR + *AT0 / *CUR + ATE
                    *O1   = *CUR + *AT1 / *CUR + ATE


        + = AND
        / = OR
        * = Inverted

        ATD     Attribute data
        ATE     Attribute enable
        AT0,AT1 Attribute address
        CUR     Cursor
        FC      FLSH clock
        HS      Horizontal sync
        INV     Inverted signal input
        LL      Load when Low
        OEL     Output Enable when Low
        RC      Row clear
        RF      Row flash
        RG      Row graphic
        RI      Row inverted

    */

	abc802_state *state =  device->machine().driver_data<abc802_state>();
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());

	int rf = 0, rc = 0, rg = 0;

	// prevent wraparound
	if (y >= 240) return;

	y += VERTICAL_PORCH_HACK;

	for (int column = 0; column < x_count; column++)
	{
		UINT8 code = state->m_char_ram[(ma + column) & 0x7ff];
		UINT16 address = code << 4;
		UINT8 ra_latch = ra;
		UINT8 data;

		int ri = (code & ABC802_INV) ? 1 : 0;

		if (column == cursor_x)
		{
			ra_latch = 0x0f;
		}

		if ((state->m_flshclk && rf) || rc)
		{
			ra_latch = 0x0e;
		}

		if (rg)
		{
			address |= 0x800;
		}

		data = state->m_char_rom[(address + ra_latch) & 0xfff];

		if (data & ABC802_ATE)
		{
			int attr = data & 0x03;
			int value = (data & ABC802_ATD) ? 1 : 0;

			switch (attr)
			{
			case 0x00:
				// Row Graphic
				rg = value;
				break;

			case 0x01:
				// Row Flash
				rf = value;
				break;

			case 0x02:
				// Row Clear
				rc = value;
				break;

			case 0x03:
				// undefined
				break;
			}
		}
		else
		{
			data <<= 2;

			if (state->m_80_40_mux)
			{
				for (int bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
				{
					int x = HORIZONTAL_PORCH_HACK + ((column + 3) * ABC800_CHAR_WIDTH) + bit;
					int color = BIT(data, 7) ^ ri;

					bitmap.pix32(y, x) = palette[color];

					data <<= 1;
				}
			}
			else
			{
				for (int bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
				{
					int x = HORIZONTAL_PORCH_HACK + ((column + 3) * ABC800_CHAR_WIDTH) + (bit << 1);
					int color = BIT(data, 7) ^ ri;

					bitmap.pix32(y, x) = palette[color];
					bitmap.pix32(y, x + 1) = palette[color];

					data <<= 1;
				}

				column++;
			}
		}
	}
}


//-------------------------------------------------
//  vs_w - vertical sync write
//-------------------------------------------------

WRITE_LINE_MEMBER( abc802_state::vs_w )
{
	if (!state)
	{
		// flash clock
		if (m_flshclk_ctr & 0x20)
		{
			m_flshclk = !m_flshclk;
			m_flshclk_ctr = 0;
		}
		else
		{
			m_flshclk_ctr++;
		}
	}

	// signal _DEW to DART
	m_dart->ri_w(1, !state);
}


//-------------------------------------------------
//  mc6845_interface crtc_intf
//-------------------------------------------------

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	ABC800_CHAR_WIDTH,
	NULL,
	abc802_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(abc802_state, vs_w),
	NULL
};


//-------------------------------------------------
//  VIDEO_START( abc802 )
//-------------------------------------------------

void abc802_state::video_start()
{
	// find memory regions
	m_char_rom = memregion(MC6845_TAG)->base();

	// register for state saving
	save_item(NAME(m_flshclk_ctr));
	save_item(NAME(m_flshclk));
	save_item(NAME(m_80_40_mux));
}


//-------------------------------------------------
//  SCREEN_UPDATE( abc802 )
//-------------------------------------------------

UINT32 abc802_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	// HACK expand visible area to workaround MC6845
	screen.set_visible_area(0, 767, 0, 311);

	// draw text
	m_crtc->screen_update(screen, bitmap, cliprect);

	return 0;
}


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( abc802_video )
//-------------------------------------------------

MACHINE_CONFIG_FRAGMENT( abc802_video )
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, ABC800_CCLK, crtc_intf)

	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(abc802_state, screen_update)

	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(768, 312)
	MCFG_SCREEN_VISIBLE_AREA(0,768-1, 0, 312-1)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_amber)
MACHINE_CONFIG_END
