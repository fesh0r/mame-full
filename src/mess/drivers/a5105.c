/***************************************************************************

    A5105

    12/05/2009 Skeleton driver.

    http://www.robotrontechnik.de/index.htm?/html/computer/a5105.htm

    - this looks like "somehow" inspired by the MSX1 machine?


ToDo:
- Fix hang when it should scroll
- Cassette Load (bit 7 of port 91)


****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/upd7220.h"
#include "machine/ram.h"
#include "machine/upd765.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "sound/wave.h"
#include "sound/beep.h"
#include "formats/basicdsk.h"


#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()

class a5105_state : public driver_device
{
public:
	a5105_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_hgdc(*this, "upd7220"),
	m_cass(*this, CASSETTE_TAG),
	m_beep(*this, BEEPER_TAG),
	m_video_ram(*this, "video_ram"){ }

	required_device<cpu_device> m_maincpu;
	required_device<upd7220_device> m_hgdc;
	required_device<cassette_image_device> m_cass;
	required_device<device_t> m_beep;
	DECLARE_READ8_MEMBER(a5105_memsel_r);
	DECLARE_READ8_MEMBER(key_r);
	DECLARE_READ8_MEMBER(key_mux_r);
	DECLARE_WRITE8_MEMBER(key_mux_w);
	DECLARE_WRITE8_MEMBER(a5105_ab_w);
	DECLARE_WRITE8_MEMBER(a5105_memsel_w);
	WRITE8_MEMBER( a5105_upd765_w );
	DECLARE_WRITE8_MEMBER(pcg_addr_w);
	DECLARE_WRITE8_MEMBER(pcg_val_w);
	required_shared_ptr<UINT8> m_video_ram;
	UINT8 *m_ram_base;
	UINT8 *m_rom_base;
	UINT8 *m_char_rom;
	UINT16 m_pcg_addr;
	UINT16 m_pcg_internal_addr;
	UINT8 m_key_mux;
	UINT8 m_memsel[4];
	virtual void machine_reset();
	virtual void video_start();
};

/* TODO */
static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
	a5105_state *state = device->machine().driver_data<a5105_state>();

	int xi,gfx;
	UINT8 pen;

	gfx = state->m_video_ram[address & 0x1ffff];

	for(xi=0;xi<8;xi++)
	{
		pen = ((gfx >> xi) & 1) ? 7 : 0;

		bitmap.pix16(y, x + xi) = pen;
	}
}

static UPD7220_DRAW_TEXT_LINE( hgdc_draw_text )
{
	a5105_state *state = device->machine().driver_data<a5105_state>();
	int x;
	int xi,yi;
	int tile,color;
	UINT8 tile_data;

	for( x = 0; x < pitch; x++ )
	{
		tile = (state->m_video_ram[((addr+x)*2) & 0x1ffff] & 0xff);
		color = (state->m_video_ram[((addr+x)*2+1) & 0x1ffff] & 0x0f);

		for( yi = 0; yi < lr; yi++)
		{
			tile_data = state->m_char_rom[(tile*8+yi) & 0x7ff];

			if(cursor_on && cursor_addr == addr+x) //TODO
				tile_data^=0xff;

			for( xi = 0; xi < 8; xi++)
			{
				int res_x,res_y;
				int pen = (tile_data >> xi) & 1 ? color : 0;

				if(yi >= 8) { pen = 0; }

				res_x = x * 8 + xi;
				res_y = y * lr + yi;

				if(res_x > screen_max_x || res_y > screen_max_y)
					continue;

				bitmap.pix16(res_y, res_x) = pen;
			}
		}
	}
}

static ADDRESS_MAP_START(a5105_mem, AS_PROGRAM, 8, a5105_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE_BANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_READWRITE_BANK("bank4")
ADDRESS_MAP_END

WRITE8_MEMBER( a5105_state::pcg_addr_w )
{
	m_pcg_addr = data << 3;
	m_pcg_internal_addr = 0;
}

WRITE8_MEMBER( a5105_state::pcg_val_w )
{
	m_char_rom[m_pcg_addr | m_pcg_internal_addr] = data;

	gfx_element_mark_dirty(machine().gfx[0], m_pcg_addr >> 3);

	m_pcg_internal_addr++;
	m_pcg_internal_addr&=7;
}

READ8_MEMBER( a5105_state::key_r )
{
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3",
	                                        "KEY4", "KEY5", "KEY6", "KEY7",
	                                        "KEY8", "KEY9", "KEYA", "UNUSED",
	                                        "UNUSED", "UNUSED", "UNUSED", "UNUSED" };

	return ioport(keynames[m_key_mux & 0x0f])->read();
}

READ8_MEMBER( a5105_state::key_mux_r )
{
	return m_key_mux;
}

WRITE8_MEMBER( a5105_state::key_mux_w )
{
	/*
        xxxx ---- unknown
        ---- xxxx keyboard mux
    */

	m_key_mux = data;
}

WRITE8_MEMBER( a5105_state::a5105_ab_w )
{
/*port $ab
        ---- 100x tape motor, active low
        ---- 101x tape data
        ---- 110x led (color green)
        ---- 111x key click, active high
*/
	switch (data & 6)
	{
	case 0:
		if (BIT(data, 0))
			m_cass->change_state(CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		else
			m_cass->change_state(CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		break;

	case 2:
		m_cass->output( BIT(data, 0) ? -1.0 : +1.0);
		break;

	case 4:
		//led
		break;

	case 6:
		beep_set_state(m_beep, BIT(data, 0));
		break;
	}
}

READ8_MEMBER( a5105_state::a5105_memsel_r )
{
	UINT8 res;

	res = (m_memsel[0] & 3) << 0;
	res|= (m_memsel[1] & 3) << 2;
	res|= (m_memsel[2] & 3) << 4;
	res|= (m_memsel[3] & 3) << 6;

	return res;
}

WRITE8_MEMBER( a5105_state::a5105_memsel_w )
{
	address_space *prog = m_maincpu->memory().space( AS_PROGRAM );

	if (m_memsel[0] != ((data & 0x03) >> 0))
	{
		m_memsel[0] = (data & 0x03) >> 0;

		switch (m_memsel[0])
		{
		case 0:
			membank("bank1")->set_base(m_rom_base);
			prog->install_read_bank(0x0000, 0x3fff, "bank1");
			prog->unmap_write(0x0000, 0x3fff);
			break;
		case 2:
			membank("bank1")->set_base(m_ram_base);
			prog->install_readwrite_bank(0x0000, 0x3fff, "bank1");
			break;
		default:
			prog->unmap_readwrite(0x0000, 0x3fff);
			break;
		}
	}

	if (m_memsel[1] != ((data & 0x0c) >> 2))
	{
		m_memsel[1] = (data & 0x0c) >> 2;

		switch (m_memsel[1])
		{
		case 0:
			membank("bank2")->set_base(m_rom_base + 0x4000);
			prog->install_read_bank(0x4000, 0x7fff, "bank2");
			prog->unmap_write(0x4000, 0x4000);
			break;
		case 1:
			membank("bank2")->set_base(machine().root_device().memregion("k5651")->base());
			prog->install_read_bank(0x4000, 0x7fff, "bank2");
			prog->unmap_write(0x4000, 0x4000);
			break;
		case 2:
			membank("bank2")->set_base(m_ram_base + 0x4000);
			prog->install_readwrite_bank(0x4000, 0x7fff, "bank2");
			break;
		default:
			prog->unmap_readwrite(0x4000, 0x7fff);
			break;
		}
	}

	if (m_memsel[2] != ((data & 0x30) >> 4))
	{
		m_memsel[2] = (data & 0x30) >> 4;

		switch (m_memsel[2])
		{
		case 0:
			membank("bank3")->set_base(m_rom_base + 0x8000);
			prog->install_read_bank(0x8000, 0xbfff, "bank3");
			prog->unmap_write(0x8000, 0xbfff);
			break;
		case 2:
			membank("bank3")->set_base(m_ram_base + 0x8000);
			prog->install_readwrite_bank(0x8000, 0xbfff, "bank3");
			break;
		default:
			prog->unmap_readwrite(0x8000, 0xbfff);
			break;
		}
	}

	if (m_memsel[3] != ((data & 0xc0) >> 6))
	{
		m_memsel[3] = (data & 0xc0) >> 6;

		switch (m_memsel[3])
		{
		case 2:
			membank("bank4")->set_base(m_ram_base + 0xc000);
			prog->install_readwrite_bank(0xc000, 0xffff, "bank4");
			break;
		default:
			prog->unmap_readwrite(0xc000, 0xffff);
			break;
		}
	}

	//printf("Memsel change to %02x %02x %02x %02x\n",m_memsel[0],m_memsel[1],m_memsel[2],m_memsel[3]);
}

WRITE8_MEMBER( a5105_state::a5105_upd765_w )
{
	upd765_tc_w(machine().device("upd765"), BIT(data, 4));
}

static ADDRESS_MAP_START(a5105_io, AS_IO, 8, a5105_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x40, 0x40) AM_DEVREAD_LEGACY("upd765", upd765_status_r)
	AM_RANGE(0x41, 0x41) AM_DEVREADWRITE_LEGACY("upd765", upd765_data_r, upd765_data_w)
	AM_RANGE(0x48, 0x4f) AM_WRITE(a5105_upd765_w)

	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE("z80ctc", z80ctc_device, read, write)
	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE("z80pio", z80pio_device, read, write)
	AM_RANGE(0x98, 0x99) AM_DEVREADWRITE("upd7220", upd7220_device, read, write)

	AM_RANGE(0x9c, 0x9c) AM_WRITE(pcg_val_w)
//  AM_RANGE(0x9d, 0x9d) crtc area (ff-based), palette routes here
	AM_RANGE(0x9e, 0x9e) AM_WRITE(pcg_addr_w)

//  AM_RANGE(0xa0, 0xa1) ay8910?
	AM_RANGE(0xa8, 0xa8) AM_READWRITE(a5105_memsel_r,a5105_memsel_w)
	AM_RANGE(0xa9, 0xa9) AM_READ(key_r)
	AM_RANGE(0xaa, 0xaa) AM_READWRITE(key_mux_r,key_mux_w)
	AM_RANGE(0xab, 0xab) AM_WRITE(a5105_ab_w) //misc output, see above
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a5105 )
	PORT_START("KEY0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 =") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 \\") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('\\')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 /") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')

	PORT_START("KEY1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("< >") PORT_CODE(KEYCODE_COLON) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ *") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xc3\xb6 \xc3\x96") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00f6) PORT_CHAR(0x00d6)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xc3\xa4 \xc3\x84") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00e4) PORT_CHAR(0x00c4)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xc3\xbc \xc3\x9c") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00fc) PORT_CHAR(0x00dc)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("# ^") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('#') PORT_CHAR('^')

	PORT_START("KEY2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("' `") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('\'') PORT_CHAR('`')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("? beta") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('?')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", ;") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". :") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED) // appears to do nothing
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("KEY3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')

	PORT_START("KEY4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("KEY5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("KEY6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) /* gives keyclick but does nothing */PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CODE") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("KEY7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)		PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)		PORT_CHAR(9)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STOP") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SELECT") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)

	PORT_START("KEY8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_INSERT)		PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("KEY9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)		PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)		PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)	PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))

	PORT_START("KEYA")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)		PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	PORT_START("UNUSED")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


MACHINE_RESET_MEMBER(a5105_state)
{
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	a5105_ab_w(*space, 0, 9); // turn motor off
	beep_set_frequency(m_beep, 500);

	m_ram_base = (UINT8*)machine().device<ram_device>(RAM_TAG)->pointer();
	m_rom_base = (UINT8*)machine().root_device().memregion("maincpu")->base();

	membank("bank1")->set_base(m_rom_base);
	membank("bank2")->set_base(m_rom_base + 0x4000);
	membank("bank3")->set_base(m_ram_base);
	membank("bank4")->set_base(m_ram_base + 0x4000);
}


static const gfx_layout a5105_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( a5105 )
	GFXDECODE_ENTRY( "pcg", 0x0000, a5105_chars_8x8, 0, 8 )
GFXDECODE_END


static PALETTE_INIT( gdc )
{
	int i;
	int r,g,b;

	for ( i = 0; i < 16; i++ )
	{
		r = i & 4 ? ((i & 8) ? 0xaa : 0xff) : 0x00;
		g = i & 2 ? ((i & 8) ? 0xaa : 0xff) : 0x00;
		b = i & 1 ? ((i & 8) ? 0xaa : 0xff) : 0x00;

		palette_set_color(machine, i, MAKE_RGB(r,g,b));
	}
}

VIDEO_START_MEMBER( a5105_state )
{
	// find memory regions
	m_char_rom = memregion("pcg")->base();
}

static UPD7220_INTERFACE( hgdc_intf )
{
	"screen",
	hgdc_display_pixels,
	hgdc_draw_text,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static ADDRESS_MAP_START( upd7220_map, AS_0, 8, a5105_state)
	ADDRESS_MAP_GLOBAL_MASK(0x1ffff)
	AM_RANGE(0x00000, 0x1ffff) AM_RAM AM_SHARE("video_ram")
ADDRESS_MAP_END

static LEGACY_FLOPPY_OPTIONS_START(a5105)
	LEGACY_FLOPPY_OPTION(a5105, "img", "A5105 disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([5])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface a5105_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(a5105),
	"floppy_5_25",
	NULL
};


static const upd765_interface a5105_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static Z80CTC_INTERFACE( a5105_ctc_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0),				/* interrupt callback */
	DEVCB_DEVICE_LINE_MEMBER("z80ctc", z80ctc_device, trg2),		/* ZC/TO0 callback */
	DEVCB_NULL,										/* ZC/TO1 callback */
	DEVCB_DEVICE_LINE_MEMBER("z80ctc", z80ctc_device, trg3)		/* ZC/TO2 callback */
};

static Z80PIO_INTERFACE( a5105_pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0),				/* callback when change interrupt status */
	DEVCB_NULL,										/* port A read callback */
	DEVCB_NULL,										/* port A write callback */
	DEVCB_NULL,										/* portA ready active callback */
	DEVCB_NULL,										/* port B read callback */
	DEVCB_NULL,										/* port B write callback */
	DEVCB_NULL										/* portB ready active callback */
};

static const z80_daisy_config a5105_daisy_chain[] =
{
	{ "z80ctc" },
	{ "z80pio" },
	{ NULL }
};

static MACHINE_CONFIG_START( a5105, a5105_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_15MHz / 4)
	MCFG_CPU_PROGRAM_MAP(a5105_mem)
	MCFG_CPU_IO_MAP(a5105_io)
	MCFG_CPU_CONFIG(a5105_daisy_chain)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DEVICE("upd7220", upd7220_device, screen_update)
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 25*8-1)
	MCFG_GFXDECODE(a5105)
	MCFG_PALETTE_LENGTH(16)
	MCFG_PALETTE_INIT(gdc)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* Devices */
	MCFG_UPD7220_ADD("upd7220", XTAL_15MHz, hgdc_intf, upd7220_map)
	MCFG_Z80CTC_ADD( "z80ctc", XTAL_15MHz / 4, a5105_ctc_intf )
	MCFG_Z80PIO_ADD( "z80pio", XTAL_15MHz / 4, a5105_pio_intf )

	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )

	MCFG_UPD765A_ADD("upd765", a5105_interface)
	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(a5105_floppy_interface)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( a5105 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k1505_00.rom", 0x0000, 0x8000, CRC(0ed5f556) SHA1(5c33139db9f59e50da5c76729752f8e653ae34ae))
	ROM_LOAD( "k1505_80.rom", 0x8000, 0x2000, CRC(350e4958) SHA1(7e5b587c59676e8549561117ea0b70234f439a94))

	ROM_REGION( 0x800, "pcg", ROMREGION_ERASE00 )

	ROM_REGION( 0x4000, "k5651", ROMREGION_ERASEFF )
	ROM_LOAD( "k5651_40.rom", 0x0000, 0x2000, CRC(f4ad4739) SHA1(9a7bbe6f0d180dd513c7854f441cee986c8d9765))
	ROM_LOAD( "k5651_60.rom", 0x2000, 0x2000, CRC(c77dde3f) SHA1(7c16226be6c4c71013e8008fba9d2e9c5640b6a7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME           FLAGS */
COMP( 1989, a5105,  0,      0,       a5105,     a5105, driver_device,   0,      "VEB Robotron",   "BIC A5105", GAME_NOT_WORKING | GAME_NO_SOUND)
