#include "includes/v1050.h"

/*

    TODO:

    - bright in reverse video

*/

#define V1050_ATTR_BRIGHT	0x01
#define V1050_ATTR_BLINKING	0x02
#define V1050_ATTR_ATTEN	0x04
#define V1050_ATTR_REVERSE	0x10
#define V1050_ATTR_BLANK	0x20
#define V1050_ATTR_BOLD		0x40
#define V1050_ATTR_BLINK	0x80

/* Video RAM Access */

READ8_MEMBER( v1050_state::attr_r )
{
	return m_attr;
}

WRITE8_MEMBER( v1050_state::attr_w )
{
	m_attr = data;
}

READ8_MEMBER( v1050_state::videoram_r )
{
	if (offset >= 0x2000)
	{
		m_attr = (m_attr & 0xfc) | (m_attr_ram[offset] & 0x03);
	}

	return m_video_ram[offset];
}

WRITE8_MEMBER( v1050_state::videoram_w )
{
	m_video_ram[offset] = data;

	if (offset >= 0x2000 && BIT(m_attr, 2))
	{
		m_attr_ram[offset] = m_attr & 0x03;
	}
}

/* MC6845 Interface */

static MC6845_UPDATE_ROW( v1050_update_row )
{
	v1050_state *state = device->machine().driver_data<v1050_state>();
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = (((ra & 0x03) + 1) << 13) | ((ma & 0x1fff) + column);
		UINT8 data = state->m_video_ram[address & V1050_VIDEORAM_MASK];
		UINT8 attr = (state->m_attr & 0xfc) | (state->m_attr_ram[address] & 0x03);

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7);

			/* blinking */
			if ((attr & V1050_ATTR_BLINKING) && !(attr & V1050_ATTR_BLINK)) color = 0;

			/* reverse video */
			color ^= BIT(attr, 4);

			/* bright */
			if (color && (!(attr & V1050_ATTR_BOLD) ^ (attr & V1050_ATTR_BRIGHT))) color = 2;

			/* display blank */
			if (attr & V1050_ATTR_BLANK) color = 0;

			bitmap.pix32(y, x) = palette[color];

			data <<= 1;
		}
	}
}

WRITE_LINE_MEMBER( v1050_state::crtc_vs_w )
{
	device_set_input_line(m_subcpu, INPUT_LINE_IRQ0, state ? ASSERT_LINE : CLEAR_LINE);

	set_interrupt(INT_VSYNC, state);
}

static const mc6845_interface crtc_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	v1050_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(v1050_state, crtc_vs_w),
	NULL
};

/* Palette */

static PALETTE_INIT( v1050 )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color_rgb(machine, 1, 0x00, 0xc0, 0x00); /* green */
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00); /* bright green */
}

/* Video Start */

void v1050_state::video_start()
{
	/* allocate memory */
	m_attr_ram = auto_alloc_array(machine(), UINT8, V1050_VIDEORAM_SIZE);

	/* register for state saving */
	save_item(NAME(m_attr));
	save_pointer(NAME(m_attr_ram), V1050_VIDEORAM_SIZE);
}

/* Machine Drivers */

MACHINE_CONFIG_FRAGMENT( v1050_video )
	MCFG_MC6845_ADD(H46505_TAG, H46505, XTAL_15_36MHz/8, crtc_intf)

	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DEVICE(H46505_TAG, h46505_device, screen_update)

	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(640, 400)
	MCFG_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MCFG_PALETTE_LENGTH(3)
    MCFG_PALETTE_INIT(v1050)
MACHINE_CONFIG_END
