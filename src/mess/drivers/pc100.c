/***************************************************************************

    NEC PC-100

    TODO:
    - there's a regression with i8259, check this code:
    F8209: B8 FB 00                  mov     ax,0FBh
    F820C: E6 02                     out     2h,al
    F820E: B9 00 00                  mov     cx,0h
    F8211: FB                        sti
    F8212: 0A E4                     or      ah,ah <- it's supposed to trigger an irq there!
    F8214: E1 FC                     loopz   0F8212h
    F8216: FA                        cli
    F8217: 0A E9                     or      ch,cl
    F8219: 74 15                     je      0F8230h

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "machine/i8255.h"
#include "machine/pic8259.h"

class pc100_state : public driver_device
{
public:
	pc100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_palram(*this, "palram"){ }

	DECLARE_READ16_MEMBER(pc100_vram_r);
	DECLARE_WRITE16_MEMBER(pc100_vram_w);
	DECLARE_READ16_MEMBER(pc100_kanji_r);
	DECLARE_WRITE16_MEMBER(pc100_kanji_w);
	DECLARE_READ8_MEMBER(pc100_key_r);
	DECLARE_WRITE8_MEMBER(pc100_output_w);
	DECLARE_WRITE16_MEMBER(pc100_paletteram_w);
	DECLARE_READ8_MEMBER(pc100_shift_r);
	DECLARE_WRITE8_MEMBER(pc100_shift_w);
	DECLARE_READ8_MEMBER(pc100_vs_vreg_r);
	DECLARE_WRITE8_MEMBER(pc100_vs_vreg_w);
	DECLARE_WRITE8_MEMBER(pc100_crtc_addr_w);
	DECLARE_WRITE8_MEMBER(pc100_crtc_data_w);
	DECLARE_WRITE8_MEMBER(lower_mask_w);
	DECLARE_WRITE8_MEMBER(upper_mask_w);
	DECLARE_WRITE8_MEMBER(crtc_bank_w);
	DECLARE_WRITE_LINE_MEMBER(pc100_set_int_line);
	UINT16 *m_kanji_rom;
	UINT16 *m_vram;
	required_shared_ptr<UINT16> m_palram;
	UINT16 m_kanji_addr;
	UINT8 m_timer_mode;

	UINT8 m_bank_r,m_bank_w;

	struct{
		UINT8 shift;
		UINT16 mask;
		UINT16 vstart;
		UINT8 addr;
		UINT8 reg[8];
	}m_crtc;
};

static VIDEO_START( pc100 )
{
}

static SCREEN_UPDATE_IND16( pc100 )
{
	pc100_state *state = screen.machine().driver_data<pc100_state>();
	int x,y;
	int count;
	int xi;
	int dot;
	int pen[4],pen_i;

	count = ((state->m_crtc.vstart + 0x20) * 0x40);

	for(y=0;y<512;y++)
	{
		count &= 0xffff;

		for(x=0;x<1024/16;x++)
		{
			for(xi=0;xi<16;xi++)
			{
				for(pen_i=0;pen_i<4;pen_i++)
					pen[pen_i] = (state->m_vram[count+pen_i*0x10000] >> xi) & 1;

				dot = 0;
				for(pen_i=0;pen_i<4;pen_i++)
					dot |= pen[pen_i]<<pen_i;

				if(y < 512 && x*16+xi < 768) /* TODO: safety check */
					bitmap.pix16(y, x*16+xi) = screen.machine().pens[dot];
			}

			count++;
		}
	}

	return 0;
}

READ16_MEMBER( pc100_state::pc100_vram_r )
{
	return m_vram[offset+m_bank_r*0x10000];
}

WRITE16_MEMBER( pc100_state::pc100_vram_w )
{
	UINT16 old_vram;
	int i;

	for(i=0;i<4;i++)
	{
		if((m_bank_w >> i) & 1)
		{
			old_vram = m_vram[offset+i*0x10000];
			COMBINE_DATA(&m_vram[offset+i*0x10000]);
			m_vram[offset+i*0x10000] = ((m_vram[offset+i*0x10000]|(m_vram[offset+i*0x10000]<<16)) >> m_crtc.shift);
			if(ACCESSING_BITS_0_15)
				m_vram[offset+i*0x10000] = (m_vram[offset+i*0x10000] & ~m_crtc.mask) | (old_vram & m_crtc.mask);
			else if(ACCESSING_BITS_8_15)
				m_vram[offset+i*0x10000] = (m_vram[offset+i*0x10000] & ((~m_crtc.mask) & 0xff00)) | (old_vram & (m_crtc.mask|0xff));
			else if(ACCESSING_BITS_0_7)
				m_vram[offset+i*0x10000] = (m_vram[offset+i*0x10000] & ((~m_crtc.mask) & 0xff)) | (old_vram & (m_crtc.mask|0xff00));

		}
	}
}

static ADDRESS_MAP_START(pc100_map, AS_PROGRAM, 16, pc100_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xbffff) AM_RAM // work ram
	AM_RANGE(0xc0000,0xdffff) AM_READWRITE(pc100_vram_r,pc100_vram_w) // vram, blitter based!
	AM_RANGE(0xf8000,0xfffff) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

READ16_MEMBER( pc100_state::pc100_kanji_r )
{
	return m_kanji_rom[m_kanji_addr];
}


WRITE16_MEMBER( pc100_state::pc100_kanji_w )
{
	COMBINE_DATA(&m_kanji_addr);
}

READ8_MEMBER( pc100_state::pc100_key_r )
{
	if(offset)
		return ioport("DSW")->read(); // bit 5: horizontal/vertical monitor dsw

	return 0;
}

WRITE8_MEMBER( pc100_state::pc100_output_w )
{
	if(offset == 0)
		m_timer_mode = (data & 0x18) >> 3;
}

WRITE16_MEMBER( pc100_state::pc100_paletteram_w )
{
	COMBINE_DATA(&m_palram[offset]);

	{
		int r,g,b;

		r = (m_palram[offset] >> 0) & 7;
		g = (m_palram[offset] >> 3) & 7;
		b = (m_palram[offset] >> 6) & 7;

		palette_set_color_rgb(machine(), offset, pal3bit(r),pal3bit(g),pal3bit(b));
	}
}

READ8_MEMBER( pc100_state::pc100_shift_r )
{
	return m_crtc.shift;
}


WRITE8_MEMBER( pc100_state::pc100_shift_w )
{
	m_crtc.shift = data & 0xf;
}

READ8_MEMBER( pc100_state::pc100_vs_vreg_r )
{
	if(offset)
		return m_crtc.vstart >> 8;

	return m_crtc.vstart & 0xff;
}

WRITE8_MEMBER( pc100_state::pc100_vs_vreg_w )
{
	if(offset)
		m_crtc.vstart = (m_crtc.vstart & 0xff) | (data << 8);
	else
		m_crtc.vstart = (m_crtc.vstart & 0xff00) | (data & 0xff);
}

WRITE8_MEMBER( pc100_state::pc100_crtc_addr_w )
{
	m_crtc.addr = data & 7;
}

WRITE8_MEMBER( pc100_state::pc100_crtc_data_w )
{
	m_crtc.reg[m_crtc.addr] = data;
}


/* everything is 8-bit bus wide */
static ADDRESS_MAP_START(pc100_io, AS_IO, 16, pc100_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE8_LEGACY("pic8259", pic8259_r, pic8259_w, 0x00ff) // i8259
//  AM_RANGE(0x04, 0x07) i8237?
//  AM_RANGE(0x08, 0x0b) upd765
//  AM_RANGE(0x10, 0x17) i8255 #1
	AM_RANGE(0x18, 0x1f) AM_DEVREADWRITE8("ppi8255_2", i8255_device, read, write,0x00ff) // i8255 #2
	AM_RANGE(0x20, 0x23) AM_READ8(pc100_key_r,0x00ff) //i/o, keyboard, mouse
	AM_RANGE(0x22, 0x25) AM_WRITE8(pc100_output_w,0x00ff) //i/o, keyboard, mouse
//  AM_RANGE(0x28, 0x2b) i8251
	AM_RANGE(0x30, 0x31) AM_READWRITE8(pc100_shift_r,pc100_shift_w,0x00ff) // crtc shift
	AM_RANGE(0x38, 0x39) AM_WRITE8(pc100_crtc_addr_w,0x00ff) //crtc address reg
	AM_RANGE(0x3a, 0x3b) AM_WRITE8(pc100_crtc_data_w,0x00ff) //crtc data reg
	AM_RANGE(0x3c, 0x3f) AM_READWRITE8(pc100_vs_vreg_r,pc100_vs_vreg_w,0x00ff) //crtc vertical start position
	AM_RANGE(0x40, 0x5f) AM_RAM_WRITE(pc100_paletteram_w) AM_SHARE("palram")
//  AM_RANGE(0x60, 0x61) crtc command (16-bit wide)
	AM_RANGE(0x80, 0x81) AM_READWRITE(pc100_kanji_r,pc100_kanji_w)
	AM_RANGE(0x82, 0x83) AM_WRITENOP //kanji-related?
	AM_RANGE(0x84, 0x87) AM_WRITENOP //kanji "strobe" signal 0/1
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( pc100 )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "DSW" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Monitor" )
	PORT_DIPSETTING(    0x20, "Horizontal" )
	PORT_DIPSETTING(    0x00, "Vertical" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout kanji_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8 },
	{ STEP16(0,16) },
	16*16
};

static GFXDECODE_START( pc100 )
	GFXDECODE_ENTRY( "kanji", 0x0000, kanji_layout, 8, 1 )
GFXDECODE_END

WRITE8_MEMBER( pc100_state::lower_mask_w )
{
	m_crtc.mask = (m_crtc.mask & 0xff00) | data;
}

WRITE8_MEMBER( pc100_state::upper_mask_w )
{
	m_crtc.mask = (m_crtc.mask & 0xff) | (data << 8);
}

WRITE8_MEMBER( pc100_state::crtc_bank_w )
{
	m_bank_w = data & 0xf;
	m_bank_r = (data & 0x30) >> 4;
}

static I8255A_INTERFACE( pc100_ppi8255_interface_2 )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pc100_state, lower_mask_w),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pc100_state, upper_mask_w),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pc100_state, crtc_bank_w)
};

static IRQ_CALLBACK(pc100_irq_callback)
{
	return pic8259_acknowledge( device->machine().device( "pic8259" ) );
}

WRITE_LINE_MEMBER( pc100_state::pc100_set_int_line )
{
	//printf("%02x\n",interrupt);
	cputag_set_input_line(machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

static const struct pic8259_interface pc100_pic8259_config =
{
	DEVCB_DRIVER_LINE_MEMBER(pc100_state, pc100_set_int_line),
	DEVCB_LINE_GND,
	DEVCB_NULL
};

static MACHINE_START(pc100)
{
	pc100_state *state = machine.driver_data<pc100_state>();

	device_set_irq_callback(machine.device("maincpu"), pc100_irq_callback);
	state->m_kanji_rom = (UINT16 *)(*machine.root_device().memregion("kanji"));
	state->m_vram = (UINT16 *)(*state->memregion("vram"));
}

static MACHINE_RESET(pc100)
{
}

static INTERRUPT_GEN(pc100_vblank_irq)
{
	pic8259_ir4_w(device->machine().device("pic8259"), 1);
}

static TIMER_DEVICE_CALLBACK( pc100_600hz_irq )
{
	pc100_state *state = timer.machine().driver_data<pc100_state>();

	if(state->m_timer_mode == 0)
		pic8259_ir2_w(timer.machine().device("pic8259"), 1);
}

static TIMER_DEVICE_CALLBACK( pc100_100hz_irq )
{
	pc100_state *state = timer.machine().driver_data<pc100_state>();

	if(state->m_timer_mode == 1)
		pic8259_ir2_w(timer.machine().device("pic8259"), 1);
}

static TIMER_DEVICE_CALLBACK( pc100_50hz_irq )
{
	pc100_state *state = timer.machine().driver_data<pc100_state>();

	if(state->m_timer_mode == 2)
		pic8259_ir2_w(timer.machine().device("pic8259"), 1);
}

static TIMER_DEVICE_CALLBACK( pc100_10hz_irq )
{
	pc100_state *state = timer.machine().driver_data<pc100_state>();

	if(state->m_timer_mode == 3)
		pic8259_ir2_w(timer.machine().device("pic8259"), 1);
}

#define MASTER_CLOCK 6988800

static MACHINE_CONFIG_START( pc100, pc100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, MASTER_CLOCK)
	MCFG_CPU_PROGRAM_MAP(pc100_map)
	MCFG_CPU_IO_MAP(pc100_io)
	MCFG_CPU_VBLANK_INT("screen",pc100_vblank_irq)

	MCFG_MACHINE_START(pc100)
	MCFG_MACHINE_RESET(pc100)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(1024, 1024)
	MCFG_SCREEN_VISIBLE_AREA(0, 768-1, 0, 512-1)
	MCFG_VIDEO_START(pc100)
	MCFG_SCREEN_UPDATE_STATIC(pc100)
	MCFG_GFXDECODE(pc100)
	MCFG_PALETTE_LENGTH(16)
//  MCFG_PALETTE_INIT(black_and_white)

	MCFG_TIMER_ADD_PERIODIC("600hz", pc100_600hz_irq, attotime::from_hz(MASTER_CLOCK/600))
	MCFG_TIMER_ADD_PERIODIC("100hz", pc100_100hz_irq, attotime::from_hz(MASTER_CLOCK/100))
	MCFG_TIMER_ADD_PERIODIC("50hz", pc100_50hz_irq, attotime::from_hz(MASTER_CLOCK/50))
	MCFG_TIMER_ADD_PERIODIC("10hz", pc100_10hz_irq, attotime::from_hz(MASTER_CLOCK/10))
	MCFG_I8255_ADD( "ppi8255_2", pc100_ppi8255_interface_2 )
	MCFG_PIC8259_ADD( "pic8259", pc100_pic8259_config )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pc100 )
	ROM_REGION( 0x8000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x8000, CRC(fd54a80e) SHA1(605a1b598e623ba2908a14a82454b9d32ea3c331))

	ROM_REGION( 0x20000, "kanji", ROMREGION_ERASEFF )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, BAD_DUMP CRC(29298591) SHA1(d10174553ceea556fc53fc4e685d939524a4f64b))

	ROM_REGION( 0x20000*4, "vram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY  FULLNAME       FLAGS */
COMP( 198?, pc100,  0,      0,       pc100,     pc100, driver_device,   0,      "NEC",   "PC-100", GAME_NOT_WORKING | GAME_NO_SOUND)
