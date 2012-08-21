#include "emu.h"
#include "includes/cgc7900.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define OVERLAY_CUR					BIT(cell, 31)	/* places a cursor in the cell if SET */
#define OVERLAY_BLK					BIT(cell, 30)	/* blinks the foreground character in the cell if SET */
#define OVERLAY_VF					BIT(cell, 28)	/* makes the foreground visible if SET (else transparent) */
#define OVERLAY_VB					BIT(cell, 27)	/* makes the background visible if SET (else transparent) */
#define OVERLAY_PL					BIT(cell, 24)	/* uses bits 0-7 as PLOT DOT descriptor if SET (else ASCII) */
#define OVERLAY_BR					BIT(cell, 18)	/* turns on Red in background if SET */
#define OVERLAY_BG					BIT(cell, 17)	/* turns on Green in background if SET */
#define OVERLAY_BB					BIT(cell, 16)	/* turns on Blue in background if SET */
#define OVERLAY_FR					BIT(cell, 10)	/* turns on Red in foreground if SET */
#define OVERLAY_FG					BIT(cell, 9)	/* turns on Green in foreground if SET */
#define OVERLAY_FB					BIT(cell, 8)	/* turns on Blue in background if SET */
#define OVERLAY_DATA				(cell & 0xff)	/* ASCII or Plot Dot character */

#define IMAGE_SELECT				BIT(m_roll_overlay[0], 13)
#define OVERLAY_CURSOR_BLINK		BIT(m_roll_overlay[0], 12)
#define OVERLAY_CHARACTER_BLINK		BIT(m_roll_overlay[0], 11)

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
    cgc7900_z_mode_r - Z mode read
-------------------------------------------------*/

READ16_MEMBER( cgc7900_state::z_mode_r )
{
	return 0;
}

/*-------------------------------------------------
    cgc7900_z_mode_w - Z mode write
-------------------------------------------------*/

WRITE16_MEMBER( cgc7900_state::z_mode_w )
{
}

/*-------------------------------------------------
    cgc7900_color_status_w - color status write
-------------------------------------------------*/

WRITE16_MEMBER( cgc7900_state::color_status_w )
{
}

/*-------------------------------------------------
    cgc7900_sync_r - sync information read
-------------------------------------------------*/

READ16_MEMBER( cgc7900_state::sync_r )
{
	/*

        bit     signal      description

         0      _VERT       vertical retrace (0=vblank)
         1                  interlace (1=first field, 0=second field)
         2      _HG         horizontal retrace (0=hblank)
         3      1
         4      1
         5      1
         6      1
         7      1
         8      1
         9      1
        10      1
        11      1
        12      1
        13      1
        14      1
        15      1

    */

	return 0xffff;
}

/***************************************************************************
    VIDEO
***************************************************************************/

/*-------------------------------------------------
    update_clut - update color lookup table
-------------------------------------------------*/

void cgc7900_state::update_clut()
{
	for (int i = 0; i < 256; i++)
	{
		UINT16 addr = i * 2;
		UINT32 data = (m_clut_ram[addr + 1] << 16) | m_clut_ram[addr];
		UINT8 b = data & 0xff;
		UINT8 g = (data >> 8) & 0xff;
		UINT8 r = (data >> 16) & 0xff;

		palette_set_color_rgb(machine(), i + 8, r, g, b);
	}
}

/*-------------------------------------------------
    draw_bitmap - draw bitmap image
-------------------------------------------------*/

void cgc7900_state::draw_bitmap(screen_device *screen, bitmap_ind16 &bitmap)
{
}

/*-------------------------------------------------
    draw_overlay - draw text overlay
-------------------------------------------------*/

void cgc7900_state::draw_overlay(screen_device *screen, bitmap_ind16 &bitmap)
{
	for (int y = 0; y < 768; y++)
	{
		int sy = y / 8;
		int line = y % 8;

		for (int sx = 0; sx < 85; sx++)
		{
			UINT16 addr = (sy * 170) + (sx * 2);
			UINT32 cell = (m_overlay_ram[addr] << 16) | m_overlay_ram[addr + 1];
			UINT8 data = m_char_rom[(OVERLAY_DATA << 3) | line];
			int fg = (cell >> 8) & 0x07;
			int bg = (cell >> 16) & 0x07;

			for (int x = 0; x < 8; x++)
			{
				if (OVERLAY_CUR)
				{
					if (!OVERLAY_CURSOR_BLINK || m_blink)
					{
						bitmap.pix16(y, (sx * 8) + x) = 7;
					}
				}
				else
				{
					if (BIT(data, x) && (!OVERLAY_CHARACTER_BLINK || m_blink))
					{
						if (OVERLAY_VF) bitmap.pix16(y, (sx * 8) + x) = fg;
					}
					else
					{
						if (OVERLAY_VB) bitmap.pix16(y, (sx * 8) + x) = bg;
					}
				}
			}
		}
	}
}

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( blink_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( blink_tick )
{
	cgc7900_state *state = timer.machine().driver_data<cgc7900_state>();

	state->m_blink = !state->m_blink;
}

/*-------------------------------------------------
    PALETTE_INIT( cgc7900 )
-------------------------------------------------*/

static PALETTE_INIT( cgc7900 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00 );
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff );
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00 );
	palette_set_color_rgb(machine, 3, 0x00, 0xff, 0xff );
	palette_set_color_rgb(machine, 4, 0xff, 0x00, 0x00 );
	palette_set_color_rgb(machine, 5, 0xff, 0x00, 0xff );
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00 );
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff );
}

/*-------------------------------------------------
    VIDEO_START( cgc7900 )
-------------------------------------------------*/

void cgc7900_state::video_start()
{
	/* find memory regions */
	m_char_rom = memregion("gfx1")->base();
}

/*-------------------------------------------------
    SCREEN_UPDATE_IND16( cgc7900 )
-------------------------------------------------*/

UINT32 cgc7900_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	update_clut();
	draw_bitmap(&screen, bitmap);
	draw_overlay(&screen, bitmap);

    return 0;
}

/*-------------------------------------------------
    gfx_layout cgc7900_charlayout
-------------------------------------------------*/

static const gfx_layout cgc7900_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ RGN_FRAC(0,1) },
	{ STEP8(7,-1) },
	{ STEP8(0,8) },
	8*8
};

/*-------------------------------------------------
    GFXDECODE( cgc7900 )
-------------------------------------------------*/

static GFXDECODE_START( cgc7900 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, cgc7900_charlayout, 0, 1 )
GFXDECODE_END

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

/*-------------------------------------------------
    MACHINE_DRIVER( cgc7900_video )
-------------------------------------------------*/

MACHINE_CONFIG_FRAGMENT( cgc7900_video )
    MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_UPDATE_DRIVER(cgc7900_state, screen_update)
    MCFG_SCREEN_SIZE(1024, 768)
    MCFG_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)

	MCFG_GFXDECODE(cgc7900)

	MCFG_PALETTE_LENGTH(8+256) /* 8 overlay colors + 256 bitmap colors */
	MCFG_PALETTE_INIT(cgc7900)

	MCFG_TIMER_ADD_PERIODIC("blink", blink_tick, attotime::from_hz(XTAL_28_48MHz/7500000))
MACHINE_CONFIG_END
