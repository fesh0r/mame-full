/***************************************************************************

    Mitsubishi Multi 16

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/pic8259.h"


class multi16_state : public driver_device
{
public:
	multi16_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_pic(*this, "pic8259"),
	m_crtc(*this, "crtc")
	,
		m_p_vram(*this, "p_vram"){ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pic;
	required_device<mc6845_device> m_crtc;
	DECLARE_WRITE8_MEMBER(multi16_6845_address_w);
	DECLARE_WRITE8_MEMBER(multi16_6845_data_w);
	DECLARE_WRITE_LINE_MEMBER(multi16_set_int_line);
	required_shared_ptr<UINT16> m_p_vram;
	UINT8 m_crtc_vreg[0x100],m_crtc_index;
};


static VIDEO_START( multi16 )
{
}

#define mc6845_h_char_total 	(state->m_crtc_vreg[0])
#define mc6845_h_display		(state->m_crtc_vreg[1])
#define mc6845_h_sync_pos		(state->m_crtc_vreg[2])
#define mc6845_sync_width		(state->m_crtc_vreg[3])
#define mc6845_v_char_total		(state->m_crtc_vreg[4])
#define mc6845_v_total_adj		(state->m_crtc_vreg[5])
#define mc6845_v_display		(state->m_crtc_vreg[6])
#define mc6845_v_sync_pos		(state->m_crtc_vreg[7])
#define mc6845_mode_ctrl		(state->m_crtc_vreg[8])
#define mc6845_tile_height		(state->m_crtc_vreg[9]+1)
#define mc6845_cursor_y_start	(state->m_crtc_vreg[0x0a])
#define mc6845_cursor_y_end 	(state->m_crtc_vreg[0x0b])
#define mc6845_start_addr		(((state->m_crtc_vreg[0x0c]<<8) & 0x3f00) | (state->m_crtc_vreg[0x0d] & 0xff))
#define mc6845_cursor_addr  	(((state->m_crtc_vreg[0x0e]<<8) & 0x3f00) | (state->m_crtc_vreg[0x0f] & 0xff))
#define mc6845_light_pen_addr	(((state->m_crtc_vreg[0x10]<<8) & 0x3f00) | (state->m_crtc_vreg[0x11] & 0xff))
#define mc6845_update_addr  	(((state->m_crtc_vreg[0x12]<<8) & 0x3f00) | (state->m_crtc_vreg[0x13] & 0xff))


static SCREEN_UPDATE_IND16( multi16 )
{
	multi16_state *state = screen.machine().driver_data<multi16_state>();
	int x,y;
	int count;
	int xi;

	count = 0;

	for(y=0;y<mc6845_v_display*8;y++)
	{
		for(x=0;x<(mc6845_h_display*8)/16;x++)
		{
			for(xi=0;xi<16;xi++)
			{
				int dot = (BITSWAP16(state->m_p_vram[count],7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8) >> (15-xi)) & 0x1;

				if(screen.visible_area().contains(x*16+xi, y))
					bitmap.pix16(y, x*16+xi) = screen.machine().pens[dot];
			}

			count++;
		}
	}

	return 0;
}

static ADDRESS_MAP_START(multi16_map, AS_PROGRAM, 16, multi16_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0x7ffff) AM_RAM
	AM_RANGE(0xd8000,0xdffff) AM_RAM AM_SHARE("p_vram")
	AM_RANGE(0xe0000,0xeffff) AM_RAM
	AM_RANGE(0xf0000,0xf3fff) AM_MIRROR(0xc000) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

WRITE8_MEMBER( multi16_state::multi16_6845_address_w )
{
	m_crtc_index = data;
	m_crtc->address_w(space, offset, data);
}

WRITE8_MEMBER( multi16_state::multi16_6845_data_w )
{
	m_crtc_vreg[m_crtc_index] = data;
	m_crtc->register_w(space, offset, data);
}

static ADDRESS_MAP_START(multi16_io, AS_IO, 16, multi16_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x02, 0x03) AM_DEVREADWRITE8_LEGACY("pic8259", pic8259_r, pic8259_w, 0xffff) // i8259
	AM_RANGE(0x40, 0x41) AM_WRITE8(multi16_6845_address_w, 0x00ff)
	AM_RANGE(0x40, 0x41) AM_WRITE8(multi16_6845_data_w, 0xff00)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( multi16 )
INPUT_PORTS_END

static IRQ_CALLBACK(multi16_irq_callback)
{
	return pic8259_acknowledge( device->machine().device("pic8259") );
}

WRITE_LINE_MEMBER( multi16_state::multi16_set_int_line )
{
	//printf("%02x\n",interrupt);
	cputag_set_input_line(machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static const struct pic8259_interface multi16_pic8259_config =
{
	DEVCB_DRIVER_LINE_MEMBER(multi16_state, multi16_set_int_line),
	DEVCB_LINE_GND,
	DEVCB_NULL
};

static MACHINE_START(multi16)
{
	device_set_irq_callback(machine.device("maincpu"), multi16_irq_callback);
}


static MACHINE_RESET(multi16)
{
}

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static MACHINE_CONFIG_START( multi16, multi16_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 8000000)
	MCFG_CPU_PROGRAM_MAP(multi16_map)
	MCFG_CPU_IO_MAP(multi16_io)

	MCFG_MACHINE_START(multi16)
	MCFG_MACHINE_RESET(multi16)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MCFG_VIDEO_START(multi16)
	MCFG_SCREEN_UPDATE_STATIC(multi16)
	MCFG_PALETTE_LENGTH(8)

	/* Devices */
	MCFG_MC6845_ADD("crtc", H46505, 16000000/5, mc6845_intf)	/* unknown clock, hand tuned to get ~60 fps */
	MCFG_PIC8259_ADD( "pic8259", multi16_pic8259_config )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( multi16 )
	ROM_REGION( 0x4000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x4000, CRC(5beb5e94) SHA1(d3b9dc9a08995a0f26af9671893417e795370306))
ROM_END

/* Driver */

/*    YEAR  NAME     PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1986, multi16, 0,      0,       multi16,   multi16, driver_device, 0,   "Mitsubishi", "Multi 16", GAME_NOT_WORKING | GAME_NO_SOUND)
