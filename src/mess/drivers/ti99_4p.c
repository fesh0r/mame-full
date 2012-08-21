/****************************************************************************

    SNUG SGCPU (a.k.a. 99/4p) system

    This system is a reimplementation of the old ti99/4a console.  It is known
    both as the 99/4p ("peripheral box", since the system is a card to be
    inserted in the peripheral box, instead of a self contained console), and
    as the SGCPU ("Second Generation CPU", which was originally the name used
    in TI documentation to refer to either (or both) TI99/5 and TI99/8
    projects).

    The SGCPU was designed and built by the SNUG (System 99 Users Group),
    namely by Michael Becker for the hardware part and Harald Glaab for the
    software part.  It has no relationship with TI.

    The card is architectured around a 16-bit bus (vs. an 8-bit bus in every
    other TI99 system).  It includes 64kb of ROM, including a GPL interpreter,
    an internal DSR ROM which contains system-specific code, part of the TI
    extended Basic interpreter, and up to 1Mbyte of RAM.  It still includes a
    16-bit to 8-bit multiplexer in order to support extension cards designed
    for TI99/4a, but it can support 16-bit cards, too.  It does not include
    GROMs, video or sound: instead, it relies on the HSGPL and EVPC cards to
    do the job.

    IMPORTANT: The SGCPU card relies on a properly set up HSGPL flash memory
    card; without, it will immediately lock up. It is impossible to set it up
    from here (a bootstrap problem; you cannot start without the HSGPL).
    The best chance is to start a ti99_4ev with a plugged-in HSGPL
    and go through the setup process there. Copy the nvram files of the hsgpl into this
    driver's nvram subdirectory. The contents will be directly usable for the SGCPU.

    Michael Zapf

    February 2012: Rewritten as class

*****************************************************************************/

#include "emu.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/wave.h"
#include "sound/dac.h"

#include "machine/tms9901.h"
#include "machine/ti99/peribox.h"

#include "imagedev/cassette.h"
#include "machine/ti99/videowrp.h"
#include "machine/ti99/peribox.h"
#include "machine/ti99/joyport.h"

#define TMS9901_TAG "tms9901"
#define SGCPU_TAG "sgcpu"

#define SAMSMEM_TAG "samsmem"
#define PADMEM_TAG "padmem"

#define VERBOSE 1
#define LOG logerror

class ti99_4p : public driver_device
{
public:
	ti99_4p(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_WRITE_LINE_MEMBER( console_ready );
	DECLARE_WRITE_LINE_MEMBER( console_ready_dmux );

	DECLARE_WRITE_LINE_MEMBER( extint );
	DECLARE_WRITE_LINE_MEMBER( notconnected );
	DECLARE_READ8_MEMBER( interrupt_level );
	DECLARE_READ16_MEMBER( memread );
	DECLARE_WRITE16_MEMBER( memwrite );

	DECLARE_READ16_MEMBER( samsmem_read );
	DECLARE_WRITE16_MEMBER( samsmem_write );

	DECLARE_WRITE8_MEMBER(external_operation);
	DECLARE_WRITE_LINE_MEMBER( clock_out );

	void	clock_in(int clock);

	// CRU (Communication Register Unit) handling
	DECLARE_READ8_MEMBER( cruread );
	DECLARE_WRITE8_MEMBER( cruwrite );
	DECLARE_READ8_MEMBER( read_by_9901 );
	DECLARE_WRITE_LINE_MEMBER(keyC0);
	DECLARE_WRITE_LINE_MEMBER(keyC1);
	DECLARE_WRITE_LINE_MEMBER(keyC2);
	DECLARE_WRITE_LINE_MEMBER(cs_motor);
	DECLARE_WRITE_LINE_MEMBER(audio_gate);
	DECLARE_WRITE_LINE_MEMBER(cassette_output);
	DECLARE_WRITE8_MEMBER(tms9901_interrupt);
	DECLARE_WRITE_LINE_MEMBER(handset_ack);
	DECLARE_WRITE_LINE_MEMBER(alphaW);

	void set_tms9901_INT2_from_v9938(v99x8_device &vdp, int state);

	tms9900_device*			m_cpu;
	tms9901_device*			m_tms9901;
	ti_sound_system_device*	m_sound;
	ti_exp_video_device*	m_video;
	cassette_image_device*	m_cassette;
	peribox_device*			m_peribox;
	joyport_device*			m_joyport;

	// Pointer to ROM0
	UINT16	*m_rom0;

	// Pointer to DSR ROM
	UINT16	*m_dsr;

	// Pointer to ROM6, first bank
	UINT16	*m_rom6a;

	// Pointer to ROM6, second bank
	UINT16	*m_rom6b;

	// AMS RAM (1 Mib)
	UINT16	*m_ram;

	// Scratch pad ram (1 KiB)
	UINT16	*m_scratchpad;

	// First joystick. 6 for TI-99/4A
	int 	m_firstjoy;

	// READY line
	int		m_ready_line, m_ready_line_dmux;

private:
	DECLARE_READ16_MEMBER( datamux_read );
	DECLARE_WRITE16_MEMBER( datamux_write );
	void	set_keyboard_column(int number, int data);

	int		m_keyboard_column;
	int		m_check_alphalock;

	// True if SGCPU DSR is enabled
	bool m_internal_dsr;

	// True if SGCPU rom6 is enabled
	bool m_internal_rom6;

	// Offset to the ROM6 bank.
	int m_rom6_bank;

	// Wait states
	int m_waitcount;

	// TRUE when mapper is active
	bool m_map_mode;

	// TRUE when mapper registers are accessible
	bool m_access_mapper;

	UINT8	m_lowbyte;
	UINT8	m_highbyte;
	UINT8	m_latch;

	// Mapper registers
	UINT8 m_mapper[16];

	int		m_ready_prev;		// for debugging purposes only

};

static ADDRESS_MAP_START(memmap, AS_PROGRAM, 16, ti99_4p)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( memread, memwrite )
ADDRESS_MAP_END

static ADDRESS_MAP_START(cru_map, AS_IO, 8, ti99_4p)
	AM_RANGE(0x0000, 0x003f) AM_DEVREAD(TMS9901_TAG, tms9901_device, read)
	AM_RANGE(0x0000, 0x01ff) AM_READ( cruread )

	AM_RANGE(0x0000, 0x01ff) AM_DEVWRITE(TMS9901_TAG, tms9901_device, write)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( cruwrite )
ADDRESS_MAP_END

/*
    Input ports, used by machine code for TI keyboard and joystick emulation.

    Since the keyboard microcontroller is not emulated, we use the TI99/4a 48-key keyboard,
    plus two optional joysticks.
*/

static INPUT_PORTS_START(ti99_4p)
	/* 4 ports for keyboard and joystick */
	PORT_START("COL0")	// col 0
		PORT_BIT(0x88, IP_ACTIVE_LOW, IPT_UNUSED)
		/* The original control key is located on the left, but we accept the right control key as well */
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")      PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		/* TI99/4a has a second shift key which maps the same */
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		/* The original function key is located on the right, but we accept the left alt key as well */
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN")      PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_SHIFT_2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT")  PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+') PORT_CHAR(UCHAR_MAMEKEY(F12))

	PORT_START("COL1")	// col 1
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)     PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(UCHAR_MAMEKEY(DOWN))
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)     PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR('~')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)     PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(UCHAR_MAMEKEY(LEFT))
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)     PORT_CHAR('2') PORT_CHAR('@') PORT_CHAR(UCHAR_MAMEKEY(F2))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK")  PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(') PORT_CHAR(UCHAR_MAMEKEY(F9))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)     PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR('\'')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)     PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("COL2")	// col 2
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)     PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('`')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)     PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(UCHAR_MAMEKEY(UP))
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)     PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 # ERASE") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#') PORT_CHAR(UCHAR_MAMEKEY(F3))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO")  PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*') PORT_CHAR(UCHAR_MAMEKEY(F8))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)     PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR('?')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("COL3")	// col 3
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)     PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)     PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('[')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)     PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$') PORT_CHAR(UCHAR_MAMEKEY(F4))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID")   PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&') PORT_CHAR(UCHAR_MAMEKEY(F7))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)     PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR('_')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)     PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)     PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("COL4")	// col 4
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)     PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)     PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(']')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)     PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN")  PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%') PORT_CHAR(UCHAR_MAMEKEY(F5))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^') PORT_CHAR(UCHAR_MAMEKEY(F6))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)     PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)     PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)     PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("COL5")	// col 5
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)     PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR('\\')
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)     PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)     PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR('|')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)     PORT_CHAR('1') PORT_CHAR('!') PORT_CHAR(UCHAR_MAMEKEY(DEL))
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)     PORT_CHAR('0') PORT_CHAR(')') PORT_CHAR(UCHAR_MAMEKEY(F10))
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)     PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('\"')
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('-')

	PORT_START("ALPHA")	/* one more port for Alpha line */
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alpha Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE


INPUT_PORTS_END

/*
    Memory access
*/
READ16_MEMBER( ti99_4p::memread )
{
	int addroff = offset << 1;
	if (m_rom0 == NULL) return 0;	// premature access

	UINT16 zone = addroff & 0xe000;
	UINT16 value = 0;

	if (zone==0x0000)
	{
		// ROM0
		value = m_rom0[(addroff & 0x1fff)>>1];
		return value;
	}
	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		value = samsmem_read(space, offset, mem_mask);
		return value;
	}

	if (zone==0x4000)
	{
		if (m_internal_dsr)
		{
			value = m_dsr[(addroff & 0x1fff)>>1];
			return value;
		}
		else
		{
			if (m_access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				value = m_mapper[offset & 0x000f]<<8;
				return value;
			}
		}
	}

	if (zone==0x6000 && m_internal_rom6)
	{
		if (m_rom6_bank==0)
			value = m_rom6a[(addroff & 0x1fff)>>1];
		else
			value = m_rom6b[(addroff & 0x1fff)>>1];

		return value;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)	// cannot read from sound
		{
			value = 0;
			return value;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			value = m_scratchpad[(addroff & 0x03ff)>>1];
			return value;
		}
		// Video: 8800, 8802
		if ((addroff & 0xfffd)==0x8800)
		{
			value = m_video->read16(space, offset, mem_mask);
			return value;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	value = datamux_read(space, offset, mem_mask);
	return value;
}

WRITE16_MEMBER( ti99_4p::memwrite )
{
//  device_adjust_icount(m_cpu, -4);

	int addroff = offset << 1;
	UINT16 zone = addroff & 0xe000;

	if (zone==0x0000)
	{
		// ROM0
		if (VERBOSE>4) LOG("sgcpu: ignoring ROM write access at %04x\n", addroff);
		return;
	}

	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		samsmem_write(space, offset, data, mem_mask);
		return;
	}

	if (zone==0x4000)
	{
		if (m_internal_dsr)
		{
			if (VERBOSE>4) LOG("sgcpu: ignoring DSR write access at %04x\n", addroff);
			return;
		}
		else
		{
			if (m_access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				m_mapper[offset & 0x000f] = data;
				return;
			}
		}
	}

	if (zone==0x6000 && m_internal_rom6)
	{
		m_rom6_bank = offset & 0x0001;
		return;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)		//sound write
		{
			m_sound->write(space, 0, (data >> 8) & 0xff);
			return;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			m_scratchpad[(addroff & 0x03ff)>>1] = data;
			return;
		}
		// Video: 8C00, 8C02
		if ((addroff & 0xfffd)==0x8c00)
		{
			m_video->write16(space, offset, data, mem_mask);
			return;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	datamux_write(space, offset, data, mem_mask);
}

/***************************************************************************
    Internal datamux; similar to TI-99/4A. However, here we have just
    one device, the peripheral box, so it is much simpler.
***************************************************************************/

/*
    The datamux is connected to the clock line in order to operate
    the wait state counter.
*/
void ti99_4p::clock_in(int clock)
{
	if (clock==ASSERT_LINE && m_waitcount!=0)
	{
		m_waitcount--;
		if (m_waitcount==0) console_ready_dmux(ASSERT_LINE);
	}
}


READ16_MEMBER( ti99_4p::datamux_read )
{
	UINT8 hbyte = 0;
	UINT16 addroff = (offset << 1);

	m_peribox->readz(space, addroff+1, &m_latch, mem_mask);
	m_lowbyte = m_latch;

	m_peribox->readz(space, addroff, &hbyte, mem_mask);
	m_highbyte = hbyte;

	// use the latch and the currently read byte and put it on the 16bit bus
//  printf("read  address = %04x, value = %04x, memmask = %4x\n", addroff,  (hbyte<<8) | sgcpu->latch, mem_mask);

	// Insert four wait states and let CPU enter wait state
	m_waitcount = 6;
	console_ready_dmux(CLEAR_LINE);

	return (hbyte<<8) | m_latch ;
}

/*
    Write access.
    TODO: use the 16-bit expansion in the box for suitable cards
*/
WRITE16_MEMBER( ti99_4p::datamux_write )
{
	UINT16 addroff = (offset << 1);
//  printf("write address = %04x, value = %04x, memmask = %4x\n", addroff, data, mem_mask);

	// read more about the datamux in datamux.c

	// Write to the PEB
	m_peribox->write(space, addroff+1, data & 0xff);

	// Write to the PEB
	m_peribox->write(space, addroff, (data>>8) & 0xff);

	// Insert four wait states and let CPU enter wait state
	m_waitcount = 6;
	console_ready_dmux(CLEAR_LINE);
}

/***************************************************************************
   CRU interface
***************************************************************************/

#define MAP_CRU_BASE 0x0f00
#define SAMS_CRU_BASE 0x1e00

/*
    CRU write
*/
WRITE8_MEMBER( ti99_4p::cruwrite )
{
	int addroff = offset<<1;

	if ((addroff & 0xff00)==MAP_CRU_BASE)
	{
		if ((addroff & 0x000e)==0)	m_internal_dsr = data;
		if ((addroff & 0x000e)==2)	m_internal_rom6 = data;
		if ((addroff & 0x000e)==4)	m_peribox->senila((data!=0)? ASSERT_LINE : CLEAR_LINE);
		if ((addroff & 0x000e)==6)	m_peribox->senilb((data!=0)? ASSERT_LINE : CLEAR_LINE);
		// TODO: more CRU bits? 8=Fast timing / a=KBENA
		return;
	}
	if ((addroff & 0xff00)==SAMS_CRU_BASE)
	{
		if ((addroff & 0x000e)==0) m_access_mapper = data;
		if ((addroff & 0x000e)==2) m_map_mode = data;
		return;
	}

	// No match - pass to peribox
	m_peribox->cruwrite(addroff, data);
}

READ8_MEMBER( ti99_4p::cruread )
{
	UINT8 value = 0;
	m_peribox->crureadz(offset<<4, &value);
	return value;
}

/***************************************************************************
   AMS Memory implementation
***************************************************************************/

/*
    Memory read. The SAMS card has two address areas: The memory is at locations
    0x2000-0x3fff and 0xa000-0xffff, and the mapper area is at 0x4000-0x401e
    (only even addresses).
*/
READ16_MEMBER( ti99_4p::samsmem_read )
{
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (m_map_mode)
		address = (m_mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	return m_ram[address>>1];
}

/*
    Memory write
*/
WRITE16_MEMBER( ti99_4p::samsmem_write )
{
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (m_map_mode)
		address = (m_mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	m_ram[address>>1] = data;
}

/***************************************************************************
    Keyboard/tape control
****************************************************************************/
static const char *const column[] = { "COL0", "COL1", "COL2", "COL3", "COL4", "COL5" };

READ8_MEMBER( ti99_4p::read_by_9901 )
{
	int answer=0;

	switch (offset & 0x03)
	{
	case TMS9901_CB_INT7:
		// Read pins INT3*-INT7* of TI99's 9901.
		// bit 1: INT1 status (interrupt; not set at this place)
		// bit 2: INT2 status (interrupt; not set at this place)
		// bit 3-7: keyboard status bits 0 to 4
		//
		// |K|K|K|K|K|I2|I1|C|
		//
		if (m_keyboard_column >= m_firstjoy) // joy 1 and 2
		{
			answer = m_joyport->read_port();
		}
		else
		{
			answer = ioport(column[m_keyboard_column])->read();
		}
		if (m_check_alphalock)
		{
			answer &= ~(ioport("ALPHA")->read());
		}
		answer = (answer << 3) & 0xf8;
		break;

	case TMS9901_INT8_INT15:
		// Read pins INT8*-INT15* of TI99's 9901.
		// bit 0-2: keyboard status bits 5 to 7
		// bit 3: tape input mirror
		// bit 5-7: weird, not emulated

		// |1|1|1|1|0|K|K|K|
		if (m_keyboard_column >= m_firstjoy) answer = 0x07;
		else answer = ((ioport(column[m_keyboard_column])->read())>>5) & 0x07;
		answer |= 0xf0;
		break;

	case TMS9901_P0_P7:
		break;

	case TMS9901_P8_P15:
		// Read pins P8-P15 of TI99's 9901.
		// bit 26: high
		// bit 27: tape input
		answer = 4;
		if (m_cassette->input() > 0) answer |= 8;
		break;
	}
	return answer;
}

/*
    WRITE key column select (P2-P4)
*/
void ti99_4p::set_keyboard_column(int number, int data)
{
	if (data!=0)	m_keyboard_column |= 1 << number;
	else			m_keyboard_column &= ~(1 << number);

	if (m_keyboard_column >= m_firstjoy)
	{
		m_joyport->write_port(m_keyboard_column - m_firstjoy + 1);
	}
}

WRITE_LINE_MEMBER( ti99_4p::keyC0 )
{
	set_keyboard_column(0, state);
}

WRITE_LINE_MEMBER( ti99_4p::keyC1 )
{
	set_keyboard_column(1, state);
}

WRITE_LINE_MEMBER( ti99_4p::keyC2 )
{
	set_keyboard_column(2, state);
}

/*
    WRITE alpha lock line (P5)
*/
WRITE_LINE_MEMBER( ti99_4p::alphaW )
{
	m_check_alphalock = (state==0);
}

/*
    command CS1 (only) tape unit motor (P6)
*/
WRITE_LINE_MEMBER( ti99_4p::cs_motor )
{
	m_cassette->change_state((state!=0)? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
}

/*
    audio gate (P8)

    Set to 1 before using tape: this enables the mixing of tape input sound
    with computer sound.

    We do not really need to emulate this as the tape recorder generates sound
    on its own.
*/
WRITE_LINE_MEMBER( ti99_4p::audio_gate )
{
}

/*
    tape output (P9)
*/
WRITE_LINE_MEMBER( ti99_4p::cassette_output )
{
	m_cassette->output((state!=0)? +1 : -1);
}

/* TMS9901 setup. The callback functions pass a reference to the TMS9901 as device. */
const tms9901_interface tms9901_wiring_sgcpu =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	// read handler
	DEVCB_DRIVER_MEMBER(ti99_4p, read_by_9901),

	{	// write handlers
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC0),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC1),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC2),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, alphaW),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, cs_motor),
		DEVCB_NULL,
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, audio_gate),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, cassette_output),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	},

	/* interrupt handler */
	DEVCB_DRIVER_MEMBER(ti99_4p, tms9901_interrupt)
};

/***************************************************************************
    Control lines
****************************************************************************/

/*
    We may have lots of devices pulling down this line; so we should use a AND
    gate to do it right. On the other hand, when READY is down, there is just
    no chance to make another device pull down the same line; the CPU just
    won't access any other device in this time.
*/
WRITE_LINE_MEMBER( ti99_4p::console_ready )
{
	m_ready_line = state;
	int combined = (m_ready_line == ASSERT_LINE && m_ready_line_dmux == ASSERT_LINE)? ASSERT_LINE : CLEAR_LINE;

	if (VERBOSE>6)
	{
		if (m_ready_prev != combined) LOG("ti99_4p: READY level = %d\n", combined);
	}
	m_ready_prev = combined;
	m_cpu->set_ready(combined);
}

/*
    The exception of the above rule. Memory access over the datamux also operates
    the READY line, and the datamux raises READY depending on the clock pulse.
    So we must make sure this does not interfere.
*/
WRITE_LINE_MEMBER( ti99_4p::console_ready_dmux )
{
	m_ready_line_dmux = state;
	int combined = (m_ready_line == ASSERT_LINE && m_ready_line_dmux == ASSERT_LINE)? ASSERT_LINE : CLEAR_LINE;

	if (VERBOSE>7)
	{
		if (m_ready_prev != combined) LOG("ti99_4p: READY dmux level = %d\n", state);
	}
	m_ready_prev = combined;
	m_cpu->set_ready(combined);
}


WRITE_LINE_MEMBER( ti99_4p::extint )
{
	if (VERBOSE>6) LOG("ti99_4p: EXTINT level = %02x\n", state);
	if (m_tms9901 != NULL)
		m_tms9901->set_single_int(1, state);
}

WRITE_LINE_MEMBER( ti99_4p::notconnected )
{
	if (VERBOSE>6) LOG("ti99_4p: Setting a not connected line ... ignored\n");
}

/*
    Clock line from the CPU. Used to control wait state generation.
*/
WRITE_LINE_MEMBER( ti99_4p::clock_out )
{
	clock_in(state);
}

WRITE8_MEMBER( ti99_4p::tms9901_interrupt )
{
	// offset contains the interrupt level (0-15)
	// However, the TI board just ignores that level and hardwires it to 1
	// See below (interrupt_level)
	m_cpu->set_input_line(INPUT_LINE_99XX_INTREQ, data);
}

READ8_MEMBER( ti99_4p::interrupt_level )
{
	// On the TI-99 systems these IC lines are not used; the input lines
	// at the CPU are hardwired to level 1.
	return 1;
}

WRITE8_MEMBER( ti99_4p::external_operation )
{
	static const char* extop[8] = { "inv1", "inv2", "IDLE", "RSET", "inv3", "CKON", "CKOF", "LREX" };
	if (VERBOSE>1) LOG("External operation %s not implemented on the SGCPU board\n", extop[offset]);
}

/*****************************************************************************/

static PERIBOX_CONFIG( peribox_conf )
{
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, extint),			// INTA
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, notconnected),	// INTB
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, console_ready),	// READY
	0x70000												// Address bus prefix (AMA/AMB/AMC)
};

static TMS99xx_CONFIG( sgcpu_cpuconf )
{
	DEVCB_DRIVER_MEMBER(ti99_4p, external_operation),
	DEVCB_DRIVER_MEMBER(ti99_4p, interrupt_level),
	DEVCB_NULL,		// Instruction acquisition
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, clock_out),
	DEVCB_NULL,		// wait
	DEVCB_NULL		// Hold acknowledge
};

MACHINE_START( ti99_4p )
{
	ti99_4p *driver = machine.driver_data<ti99_4p>();

	driver->m_cpu = static_cast<tms9900_device*>(machine.device("maincpu"));
	driver->m_peribox = static_cast<peribox_device*>(machine.device(PERIBOX_TAG));
	driver->m_sound = static_cast<ti_sound_system_device*>(machine.device(TISOUND_TAG));
	driver->m_video = static_cast<ti_exp_video_device*>(machine.device(VIDEO_SYSTEM_TAG));
	driver->m_cassette = static_cast<cassette_image_device*>(machine.device(CASSETTE_TAG));
	driver->m_tms9901 = static_cast<tms9901_device*>(machine.device(TMS9901_TAG));
	driver->m_joyport = static_cast<joyport_device*>(machine.device(JOYPORT_TAG));

	driver->m_ram = (UINT16*)(*machine.root_device().memregion(SAMSMEM_TAG));
	driver->m_scratchpad = (UINT16*)(*machine.root_device().memregion(PADMEM_TAG));

	driver->m_peribox->senila(CLEAR_LINE);
	driver->m_peribox->senilb(CLEAR_LINE);

	driver->m_firstjoy = 6;

	driver->m_ready_line = driver->m_ready_line_dmux = ASSERT_LINE;

	UINT16 *rom = (UINT16*)(*machine.root_device().memregion("maincpu"));
	driver->m_rom0  = rom + 0x2000;
	driver->m_dsr   = rom + 0x6000;
	driver->m_rom6a = rom + 0x3000;
	driver->m_rom6b = rom + 0x7000;
}

/*
    set the state of int2 (called by the v9938)
*/
void ti99_4p::set_tms9901_INT2_from_v9938(v99x8_device &vdp, int state)
{
	m_tms9901->set_single_int(2, state);
}

static TI_SOUND_CONFIG( sound_conf )
{
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, console_ready)	// READY
};

static JOYPORT_CONFIG( joyport4a_60 )
{
	DEVCB_NULL,
	60
};

/*
    Reset the machine.
*/
MACHINE_RESET( ti99_4p )
{
	ti99_4p *driver = machine.driver_data<ti99_4p>();
	driver->m_tms9901->set_single_int(12, 0);

	driver->m_cpu->set_ready(ASSERT_LINE);
	driver->m_cpu->set_hold(CLEAR_LINE);
}

TIMER_DEVICE_CALLBACK( sgcpu_hblank_interrupt )
{
	timer.machine().device<v9938_device>(VDP_TAG)->interrupt();
}

/*
    Machine description.
*/
static MACHINE_CONFIG_START( ti99_4p_60hz, ti99_4p )
	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MCFG_TMS99xx_ADD("maincpu", TMS9900, 3000000, memmap, cru_map, sgcpu_cpuconf)
	MCFG_MACHINE_START( ti99_4p )

	/* video hardware */
	// Although we should have a 60 Hz screen rate, we have to set it to 30 here.
	// The reason is that that the number of screen lines is counted twice for the
	// interlace mode, but in non-interlace modes only half of the lines are
	// painted. Accordingly, the full set of lines is refreshed at 30 Hz,
	// not 60 Hz. This should be fixed in the v9938 emulation.
	MCFG_TI_V9938_ADD(VIDEO_SYSTEM_TAG, 30, SCREEN_TAG, 2500, 512+32, (212+28)*2, DEVICE_SELF, ti99_4p, set_tms9901_INT2_from_v9938)
	MCFG_TIMER_ADD_SCANLINE("scantimer", sgcpu_hblank_interrupt, SCREEN_TAG, 0, 1)

	// tms9901
	MCFG_TMS9901_ADD(TMS9901_TAG, tms9901_wiring_sgcpu, 3000000)

	// Peripheral expansion box (SGCPU composition)
	MCFG_PERIBOX_SG_ADD( PERIBOX_TAG, peribox_conf )

	// sound hardware
	MCFG_TI_SOUND_94624_ADD( TISOUND_TAG, sound_conf )

	// Cassette drives
	MCFG_SPEAKER_STANDARD_MONO("cass_out")
	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )

	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "cass_out", 0.25)

	// Joystick port
	MCFG_TI_JOYPORT4A_ADD( JOYPORT_TAG, joyport4a_60 )

MACHINE_CONFIG_END


ROM_START(ti99_4p)
	/*CPU memory space*/
	ROM_REGION16_BE(0x10000, "maincpu", 0)
	ROM_LOAD16_BYTE("sgcpu_hb.bin", 0x0000, 0x8000, CRC(aa100730) SHA1(35e585b2dcd3f2a0005bebb15ede6c5b8c787366) ) /* system ROMs */
	ROM_LOAD16_BYTE("sgcpu_lb.bin", 0x0001, 0x8000, CRC(2a5dc818) SHA1(dec141fe2eea0b930859cbe1ebd715ac29fa8ecb) ) /* system ROMs */
	ROM_REGION16_BE(0x080000, SAMSMEM_TAG, 0)
	ROM_FILL(0x000000, 0x080000, 0x0000)
	ROM_REGION16_BE(0x0400, PADMEM_TAG, 0)
	ROM_FILL(0x000000, 0x0400, 0x0000)
ROM_END

/*    YEAR  NAME      PARENT   COMPAT   MACHINE      INPUT    INIT      COMPANY     FULLNAME */
COMP( 1996, ti99_4p,  0,	   0,		ti99_4p_60hz, ti99_4p, driver_device, 0, "System 99 Users Group",		"SGCPU (a.k.a. 99/4P)" , 0 )
