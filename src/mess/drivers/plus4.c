/*

    TODO:

    - cassette
    - c1551 won't load anything
    - clean up keyboard handling
    - clean up TED
    - dump PLA
    - T6721 speech chip
    - floating bus read (should return the previous byte read by TED)
    - SID card (http://solder.dyndns.info/cgi-bin/showdir.pl?dir=files/commodore/plus4/hardware/SID-Card)

*/

#include "includes/plus4.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define BA15 BIT(offset, 15)
#define BA14 BIT(offset, 14)
#define BA13 BIT(offset, 13)
#define BA12 BIT(offset, 12)
#define BA11 BIT(offset, 11)
#define BA10 BIT(offset, 10)
#define BA9 BIT(offset, 9)
#define BA8 BIT(offset, 8)
#define BA7 BIT(offset, 7)
#define BA6 BIT(offset, 6)
#define BA5 BIT(offset, 5)
#define BA4 BIT(offset, 4)


enum
{
	CS0_BASIC = 0,
	CS0_FUNCTION_LO,
	CS0_C1_LOW,
	CS0_C2_LOW
};


enum
{
	CS1_KERNAL = 0,
	CS1_FUNCTION_HI,
	CS1_C1_HIGH,
	CS1_C2_HIGH
};



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  check_interrupts -
//-------------------------------------------------

void plus4_state::check_interrupts()
{
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, m_ted_irq || m_acia_irq || m_exp_irq);
}



//**************************************************************************
//  MEMORY MANAGEMENT
//**************************************************************************

void plus4_state::bankswitch(offs_t offset, int phi0, int mux, int ras, int *scs, int *phi2, int *user, int *_6551, int *addr_clk, int *keyport, int *kernal, int *cs0, int *cs1)
{
	UINT16 i = ras << 15 | BA10 << 14 | BA11 << 13 | BA13 << 12 | BA9 << 11 | BA8 << 10 | BA14 << 9 | mux << 8 | BA12 << 7 | BA7 << 6 | BA6 << 5 | BA5 << 4 | BA4 << 3 | BA15 << 2 | phi0 << 1 | 1;
/*  UINT8 data = m_pla->read(i);

    *scs = BIT(data, 0);
    *phi2 = BIT(data, 1);
    *user = BIT(data, 2);
    *_6551 = BIT(data, 3);
    *addr_clk = BIT(data, 4);
    *keyport = BIT(data, 5);
    *kernal = BIT(data, 6);
    *f7 = BIT(data, 7);*/

	// the following code is on loan from http://www.zimmers.net/anonftp/pub/cbm/firmware/computers/plus4/pla.c until we get the PLA dumped

	#define I(b) (!!((i) & (1 << b)))

	#define	I0_F7	I(0)
	#define PHI0	I(1)
	#define A15	I(2)
	#define A4	I(3)
	#define A5	I(4)
	#define A6	I(5)
	#define A7	I(6)
	#define A12	I(7)
	#define MUX	I(8)
	#define A14	I(9)
	#define A8	I(10)
	#define A9	I(11)
	#define A13	I(12)
	#define A11	I(13)
	#define A10	I(14)
	#define RAS_	I(15)

	/* unused_  0 when 0111 011x 1001 011x */
	#define F0	RAS_ || !A10 || !A11 || !A13 || A9 || !A8 || !A14 ||	\
			!A12 || A7 || A6 || !A5 || A4 || !A15 || !PHI0
	/* PHI2     1 when 0xxx xxxx xxxx xx11 */
	#define F1	!RAS_ && PHI0 && I0_F7
	/* USER_    0 when 0111 011x 1000 1111 */
	#define F2	RAS_ || !A10 || !A11 || !A13 || A9 || !A8 || !A14 ||	 \
			!A12 || A7 || A6 || A5 || !A4 || !A15 || !PHI0 || !I0_F7
	/* 6551_    0 when x111 011x 1000 011x */
	#define F3	!A10 || !A11 || !A13 || A9 || !A8 || !A14 ||	\
			!A12 || A7 || A6 || A5 || A4 || !A15 || !PHI0
	/* ADDR_CLK 0 when 1111 011x 1110 1111 */
	#define F4	RAS_ || !A10 || !A11 || !A13 || A9 || !A8 || !A14 ||	\
			!A12 || !A7 || !A6 || A5 || !A4 || !A15 || !PHI0 || !I0_F7
	/* KEYPORT_ 0 when 0111 011x 1001 1111 */
	#define F5	RAS_ || !A10 || !A11 || !A13 || A9 || !A8 || !A14 ||	\
			!A12 || A7 || A6 || !A5 || !A4 || !A15 || !PHI0 || !I0_F7
	/* KERNAL_  1 when x111 001x 1xxx x1xx */
	#define F6	A10 && A11 && A13 && !A9 && !A8 && A14 &&	\
			A12 && A15
	/* I0_F7    1 when xxxx xxx1 xxxx xxxx or
              when 0xxx xxxx xxxx xx11 */
	#define F7	MUX || (F1)

	*scs = F0;
	*phi2 = F1;
	*user = F2;
	*_6551 = F3;
	*addr_clk = F4;
	*keyport = F5;
	*kernal = F6;

	// the following logic is actually inside the TED

	*cs0 = 1;
	*cs1 = 1;

	if (m_rom_en)
	{
		if (offset >= 0x8000 && offset < 0xc000)
		{
			*cs0 = 0;
			*cs1 = 1;
		}
		else if ((offset >= 0xc000 && offset < 0xfd00) || (offset >= 0xff20))
		{
			*cs0 = 1;
			*cs1 = 0;
		}
	}
}


//-------------------------------------------------
//  read_memory -
//-------------------------------------------------

UINT8 plus4_state::read_memory(address_space &space, offs_t offset, int ba, int scs, int phi2, int user, int _6551, int addr_clk, int keyport, int kernal, int cs0, int cs1)
{
	UINT8 data = 0;
	int c1l = 1, c1h = 1, c2l = 1, c2h = 1;

	//logerror("offset %04x user %u 6551 %u addr_clk %u keyport %u kernal %u cs0 %u cs1 %u m_rom_en %u\n", offset,user,_6551,addr_clk,keyport,kernal,cs0,cs1,m_rom_en);

	if (!scs && m_t6721)
	{
		data = t6721_speech_r(m_t6721, offset & 0x03);
	}
	else if (!user)
	{
		if (m_spi_user)
		{
			data = m_spi_user->read(space, 0);
		}
		else
		{
			data |= ((m_cassette->get_state() & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED) << 2;
		}
	}
	else if (!_6551 && m_acia)
	{
		data = m_acia->read(space, offset & 0x03);
	}
	else if (!keyport)
	{
		data = m_spi_kb->read(space, 0);
	}
	else if (!cs0)
	{
		switch (m_addr & 0x03)
		{
		case CS0_BASIC:
			data = m_kernal[offset & 0x7fff];
			break;

		case CS0_FUNCTION_LO:
			if (m_function != NULL)
			{
				data = m_function[offset & 0x7fff];
			}
			break;

		case CS0_C1_LOW:
			c1l = 0;
			break;

		case CS0_C2_LOW:
			c2l = 0;
			break;
		}
	}
	else if (!cs1)
	{
		if (kernal)
		{
			data = m_kernal[offset & 0x7fff];
		}
		else
		{
			switch ((m_addr >> 2) & 0x03)
			{
			case CS1_KERNAL:
				data = m_kernal[offset & 0x7fff];
				break;

			case CS1_FUNCTION_HI:
				if (m_function != NULL)
				{
					data = m_function[offset & 0x7fff];
				}
				break;

			case CS1_C1_HIGH:
				c1h = 0;
				break;

			case CS1_C2_HIGH:
				c2h = 0;
				break;
			}
		}
	}
	else if (offset >= 0xff00 && offset < 0xff20)
	{
		data = m_ted->read(space, offset & 0x1f);
	}
	else if (offset < 0xfd00 || offset >= 0xff20)
	{
		data = m_ram->pointer()[offset & m_ram->mask()];
	}

	data |= m_exp->cd_r(space, offset, ba, cs0, c1l, c1h, cs1, c2l, c2h);

	return data;
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( plus4_state::read )
{
	int phi0 = 1, mux = 0, ras = 0, ba = 1;
	int scs, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1;

	bankswitch(offset, phi0, mux, ras, &scs, &phi2, &user, &_6551, &addr_clk, &keyport, &kernal, &cs0, &cs1);

	return read_memory(space, offset, ba, scs, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1);
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( plus4_state::write )
{
	int scs, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1;
	int phi0 = 1, mux = 0, ras = 0, ba = 1;
	int c1l = 1, c1h = 1, c2l = 1, c2h = 1;

	bankswitch(offset, phi0, mux, ras, &scs, &phi2, &user, &_6551, &addr_clk, &keyport, &kernal, &cs0, &cs1);

	//logerror("write offset %04x data %02x user %u 6551 %u addr_clk %u keyport %u kernal %u cs0 %u cs1 %u m_rom_en %u\n", offset,data,user,_6551,addr_clk,keyport,kernal,cs0,cs1,m_rom_en);

	if (!scs && m_t6721)
	{
		t6721_speech_w(m_t6721, offset & 0x03, data);
	}
	else if (!user && m_spi_user)
	{
		m_spi_user->write(space, 0, data);
	}
	else if (!_6551 && m_acia)
	{
		m_acia->write(space, offset & 0x03, data);
	}
	else if (!addr_clk)
	{
		m_addr = offset & 0x0f;
	}
	else if (!keyport)
	{
		m_spi_kb->write(space, 0, data);
	}
	else if (offset >= 0xff00 && offset < 0xff20)
	{
		m_ted->write(space, offset & 0x1f, data);
	}
	else if (offset == 0xff3e)
	{
		m_rom_en = 1;

		m_ted->rom_switch_w(m_rom_en);
	}
	else if (offset == 0xff3f)
	{
		m_rom_en = 0;

		m_ted->rom_switch_w(m_rom_en);
	}
	else if (offset < 0xfd00 || offset >= 0xff20)
	{
		m_ram->pointer()[offset & m_ram->mask()] = data;
	}

	m_exp->cd_w(space, offset, data, ba, cs0, c1l, c1h, cs1, c2l, c2h);
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( plus4_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( plus4_mem, AS_PROGRAM, 8, plus4_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(read, write)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( plus4 )
//-------------------------------------------------

static INPUT_PORTS_START( plus4 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)				PORT_CHAR('@')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3)									PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2)									PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1)									PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("HELP f7") PORT_CODE(KEYCODE_F4)				PORT_CHAR(UCHAR_MAMEKEY(F8)) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)							PORT_CHAR(0xA3)

	PORT_MODIFY("ROW1")
	/* Both Shift keys were mapped to the same bit */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Shift (Left & Right)") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  \xE2\x86\x91") PORT_CODE(KEYCODE_0)		PORT_CHAR('0') PORT_CHAR(0x2191)

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)								PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)									PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN)									PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)									PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC)									PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT)									PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)								PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT)									PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home Clear") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(UCHAR_MAMEKEY(HOME))

	PORT_INCLUDE( c16_special )				/* SPECIAL */

	PORT_INCLUDE( c16_controls )			/* CTRLSEL, JOY0, JOY1 */
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( c16 )
//-------------------------------------------------

static INPUT_PORTS_START( c16 )
	PORT_INCLUDE( plus4 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)								PORT_CHAR(0xA3)

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)					PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)							PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PGUP)									PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)							PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_PGDN)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)								PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)								PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)									PORT_CHAR(UCHAR_MAMEKEY(LEFT))
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  m6502_interface cpu_intf
//-------------------------------------------------

READ8_MEMBER( plus4_state::cpu_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4       CST RD
        5
        6       IEC CLK IN
        7       IEC DATA IN, CST SENSE (Plus/4)

    */

	UINT8 data = 0xff;
	UINT8 c16_port7501 = m6510_get_port(m_maincpu);

	if (BIT(c16_port7501, 0) || !m_iec->data_r() || ((m_cassette->get_state() & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED))
		data &= ~0x80;

	if (BIT(c16_port7501, 1) || !m_iec->clk_r())
		data &= ~0x40;

	if (m_cassette->input() > +0.0)
		data |=  0x10;
	else
		data &= ~0x10;

	return data;
}

READ8_MEMBER( plus4_state::c16_cpu_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4       CST RD
        5
        6       IEC CLK IN
        7       IEC DATA IN

    */

	UINT8 data = 0xff;
	UINT8 c16_port7501 = m6510_get_port(m_maincpu);

	if (BIT(c16_port7501, 0) || !m_iec->data_r())
		data &= ~0x80;

	if (BIT(c16_port7501, 1) || !m_iec->clk_r())
		data &= ~0x40;

	if (m_cassette->input() > +0.0)
		data |=  0x10;
	else
		data &= ~0x10;

	return data;
}

WRITE8_MEMBER( plus4_state::cpu_w )
{
	/*

        bit     description

        0       IEC DATA
        1       IEC CLK, CST WR
        2       IEC ATN
        3       CST MTR
        4
        5
        6
        7

    */

	// serial bus
	m_iec->data_w(!BIT(data, 0));
	m_iec->clk_w(!BIT(data, 1));
	m_iec->atn_w(!BIT(data, 2));

	// cassette write
	m_cassette->output(!BIT(data, 1) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	// cassette motor
	m_cassette->change_state(BIT(data, 3) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
}

static const m6502_interface cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(plus4_state, cpu_r),
	DEVCB_DRIVER_MEMBER(plus4_state, cpu_w)
};

static const m6502_interface c16_cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(plus4_state, c16_cpu_r),
	DEVCB_DRIVER_MEMBER(plus4_state, cpu_w)
};

//-------------------------------------------------
//  ted7360_interface ted_intf
//-------------------------------------------------

static INTERRUPT_GEN( c16_raster_interrupt )
{
	plus4_state *state = device->machine().driver_data<plus4_state>();

	state->m_ted->raster_interrupt_gen();
}

static INTERRUPT_GEN( c16_frame_interrupt )
{
	plus4_state *state = device->machine().driver_data<plus4_state>();

	int value, i;
	static const char *const c16ports[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	/* Lines 0-7 : common keyboard */
	for (i = 0; i < 8; i++)
	{
		value = 0xff;
		value &= ~device->machine().root_device().ioport(c16ports[i])->read();

		/* Shift Lock is mapped on Left/Right Shift */
		if ((i == 1) && (device->machine().root_device().ioport("SPECIAL")->read() & 0x80))
			value &= ~0x80;

		state->m_keyline[i] = value;
	}

	if (device->machine().root_device().ioport("CTRLSEL")->read() & 0x01)
	{
		value = 0xff;
		if (device->machine().root_device().ioport("JOY0")->read() & 0x10)			/* Joypad1_Button */
			{
				if (device->machine().root_device().ioport("SPECIAL")->read() & 0x40)
					value &= ~0x80;
				else
					value &= ~0x40;
			}

		value &= ~(device->machine().root_device().ioport("JOY0")->read() & 0x0f);	/* Other Inputs Joypad1 */

		if (device->machine().root_device().ioport("SPECIAL")->read() & 0x40)
			state->m_keyline[9] = value;
		else
			state->m_keyline[8] = value;
	}

	if (device->machine().root_device().ioport("CTRLSEL")->read() & 0x10)
	{
		value = 0xff;
		if (device->machine().root_device().ioport("JOY1")->read() & 0x10)			/* Joypad2_Button */
			{
				if (device->machine().root_device().ioport("SPECIAL")->read() & 0x40)
					value &= ~0x40;
				else
					value &= ~0x80;
			}

		value &= ~(device->machine().root_device().ioport("JOY1")->read() & 0x0f);	/* Other Inputs Joypad2 */

		if (device->machine().root_device().ioport("SPECIAL")->read() & 0x40)
			state->m_keyline[8] = value;
		else
			state->m_keyline[9] = value;
	}

	state->m_ted->frame_interrupt_gen();
}

WRITE_LINE_MEMBER( plus4_state::ted_irq_w )
{
	m_ted_irq = state;

	check_interrupts();
}

READ8_MEMBER( plus4_state::ted_ram_r )
{
	int phi0 = 1, mux = 0, ras = 1, ba = 0;
	int speech, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1;

	bankswitch(offset, phi0, mux, ras, &speech, &phi2, &user, &_6551, &addr_clk, &keyport, &kernal, &cs0, &cs1);

	return read_memory(space, offset, ba, speech, phi2, user, _6551, addr_clk, keyport, kernal, 1, 1);
}

READ8_MEMBER( plus4_state::ted_rom_r )
{
	int phi0 = 1, mux = 0, ras = 1, ba = 0;
	int speech, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1;

	bankswitch(offset, phi0, mux, ras, &speech, &phi2, &user, &_6551, &addr_clk, &keyport, &kernal, &cs0, &cs1);

	return read_memory(space, offset, ba, speech, phi2, user, _6551, addr_clk, keyport, kernal, cs0, cs1);
}

READ8_MEMBER( plus4_state::ted_k_r )
{
	UINT8 value = 0xff;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(m_port6529, i))
			value &= m_keyline[i];
	}

	/* looks like joy 0 needs dataline2 low
     * and joy 1 needs dataline1 low
     * write to 0xff08 (value on databus) reloads latches */
	if (!BIT(offset, 2))
		value &= m_keyline[8];

	if (!BIT(offset, 1))
		value &= m_keyline[9];

	return value;
}

static MOS7360_INTERFACE( ted_intf )
{
	SCREEN_TAG,
	MOS7501_TAG,
	DEVCB_DRIVER_LINE_MEMBER(plus4_state, ted_irq_w),
	DEVCB_DRIVER_MEMBER(plus4_state, ted_ram_r),
	DEVCB_DRIVER_MEMBER(plus4_state, ted_rom_r),
	DEVCB_DRIVER_MEMBER(plus4_state, ted_k_r)
};


//-------------------------------------------------
//  MOS6529_INTERFACE( spi_user_intf )
//-------------------------------------------------

static MOS6529_INTERFACE( spi_user_intf )
{
	DEVCB_DEVICE_MEMBER(PLUS4_USER_PORT_TAG, plus4_user_port_device, p_r),
	DEVCB_DEVICE_MEMBER(PLUS4_USER_PORT_TAG, plus4_user_port_device, p_w)
};


//-------------------------------------------------
//  MOS6529_INTERFACE( spi_kb_intf )
//-------------------------------------------------

UINT8 plus4_state::read_keyboard(UINT8 databus)
{
	UINT8 value = 0xff;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(m_port6529, i))
			value &= m_keyline[i];
	}

	/* looks like joy 0 needs dataline2 low
     * and joy 1 needs dataline1 low
     * write to 0xff08 (value on databus) reloads latches */
	if (!BIT(databus, 2))
		value &= m_keyline[8];

	if (!BIT(databus, 1))
		value &= m_keyline[9];

	return value;
}

READ8_MEMBER( plus4_state::spi_kb_r )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6
        7

    */

	return m_port6529 & (read_keyboard (0xff /*databus */ ) | (m_port6529 ^ 0xff));
}

WRITE8_MEMBER( plus4_state::spi_kb_w )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6
        7

    */

	m_port6529 = data;
}

static MOS6529_INTERFACE( spi_kb_intf )
{
	DEVCB_DRIVER_MEMBER(plus4_state, spi_kb_r),
	DEVCB_DRIVER_MEMBER(plus4_state, spi_kb_w)
};


//-------------------------------------------------
//  CBM_IEC_INTERFACE( iec_intf )
//-------------------------------------------------

static CBM_IEC_INTERFACE( iec_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(PLUS4_USER_PORT_TAG, plus4_user_port_device, atn_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  CBM_IEC_INTERFACE( c16_iec_intf )
//-------------------------------------------------

static CBM_IEC_INTERFACE( c16_iec_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  PLUS4_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

READ8_MEMBER( plus4_state::exp_dma_r )
{
	return m_maincpu->memory().space(AS_PROGRAM)->read_byte(offset);
}

WRITE8_MEMBER( plus4_state::exp_dma_w )
{
	m_maincpu->memory().space(AS_PROGRAM)->write_byte(offset, data);
}

WRITE_LINE_MEMBER( plus4_state::exp_irq_w )
{
	m_exp_irq = state;

	check_interrupts();
}

static PLUS4_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_DRIVER_MEMBER(plus4_state, exp_dma_r),
	DEVCB_DRIVER_MEMBER(plus4_state, exp_dma_w),
	DEVCB_DRIVER_LINE_MEMBER(plus4_state, exp_irq_w),
	DEVCB_CPU_INPUT_LINE(MOS7501_TAG, INPUT_LINE_HALT)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( plus4 )
//-------------------------------------------------

void plus4_state::machine_start()
{
	cbm_common_init();

	// find memory regions
	m_kernal = memregion("kernal")->base();

	if (memregion("function") != NULL)
	{
		m_function = memregion("function")->base();
	}

	// state saving
	save_item(NAME(m_addr));
	save_item(NAME(m_rom_en));
	save_item(NAME(m_ted_irq));
	save_item(NAME(m_acia_irq));
	save_item(NAME(m_exp_irq));
}


//-------------------------------------------------
//  MACHINE_RESET( plus4 )
//-------------------------------------------------

void plus4_state::machine_reset()
{
	m_maincpu->reset();

	m_iec->reset();

	if (m_acia)
	{
		m_acia->reset();
	}

	m_exp->reset();

	if (m_user)
	{
		m_user->reset();
	}

	m_addr = 0;
	m_rom_en = 1;

	for (int i = 0; i < 10; i++)
	{
		m_keyline[i] = 0xff;
	}
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( ntsc )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc, plus4_state )
	// basic machine hardware
	MCFG_CPU_ADD(MOS7501_TAG, M7501, XTAL_14_31818MHz/16)
	MCFG_CPU_PROGRAM_MAP(plus4_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c16_frame_interrupt)
	MCFG_CPU_PERIODIC_INT(c16_raster_interrupt, TED7360_HRETRACERATE)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video and sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_MOS7360_ADD(MOS7360_TAG, SCREEN_TAG, XTAL_14_31818MHz/4, ted_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_PLS100_ADD(PLA_TAG)
	MCFG_ACIA6551_ADD(MOS6551_TAG)
	MCFG_MOS6529_ADD(MOS6529_USER_TAG, spi_user_intf)
	MCFG_MOS6529_ADD(MOS6529_KB_TAG, spi_kb_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c16, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD( CASSETTE_TAG, cbm_cassette_interface )
	MCFG_CBM_IEC_ADD(iec_intf, NULL)
	MCFG_PLUS4_EXPANSION_SLOT_ADD(PLUS4_EXPANSION_SLOT_TAG, XTAL_14_31818MHz/16, expansion_intf, plus4_expansion_cards, NULL, NULL)
	MCFG_PLUS4_USER_PORT_ADD(PLUS4_USER_PORT_TAG, plus4_user_port_cards, NULL, NULL)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list", "plus4_cart")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "plus4_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pal )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal, plus4_state )
	// basic machine hardware
	MCFG_CPU_ADD(MOS7501_TAG, M7501, XTAL_17_73447MHz/20)
	MCFG_CPU_PROGRAM_MAP(plus4_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c16_frame_interrupt)
	MCFG_CPU_PERIODIC_INT(c16_raster_interrupt, TED7360_HRETRACERATE)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video and sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_MOS7360_ADD(MOS7360_TAG, SCREEN_TAG, XTAL_17_73447MHz/5, ted_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_PLS100_ADD(PLA_TAG)
	MCFG_ACIA6551_ADD(MOS6551_TAG)
	MCFG_MOS6529_ADD(MOS6529_USER_TAG, spi_user_intf)
	MCFG_MOS6529_ADD(MOS6529_KB_TAG, spi_kb_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c16, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD( CASSETTE_TAG, cbm_cassette_interface )
	MCFG_CBM_IEC_ADD(iec_intf, NULL)
	MCFG_PLUS4_EXPANSION_SLOT_ADD(PLUS4_EXPANSION_SLOT_TAG, XTAL_17_73447MHz/20, expansion_intf, plus4_expansion_cards, NULL, NULL)
	MCFG_PLUS4_USER_PORT_ADD(PLUS4_USER_PORT_TAG, plus4_user_port_cards, NULL, NULL)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list", "plus4_cart")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "plus4_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( c16 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( c16, pal )
	MCFG_CPU_MODIFY(MOS7501_TAG)
	MCFG_CPU_CONFIG(c16_cpu_intf)

	MCFG_DEVICE_REMOVE(MOS6551_TAG)
	MCFG_DEVICE_REMOVE(MOS6529_USER_TAG)
	MCFG_DEVICE_REMOVE(PLUS4_USER_PORT_TAG)

	MCFG_DEVICE_MODIFY(CBM_IEC_TAG)
	MCFG_DEVICE_CONFIG(c16_iec_intf)

	MCFG_DEVICE_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("16K")
	MCFG_RAM_EXTRA_OPTIONS("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( c232 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( c232, c16 )
	MCFG_DEVICE_MODIFY(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( v364 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( v364, ntsc )
	MCFG_T6721_ADD(T6721_TAG)
	//MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( c264 )
//-------------------------------------------------

ROM_START( c264 )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "basic-264.bin", 0x0000, 0x4000, CRC(6a2fc8e3) SHA1(473fce23afa07000cdca899fbcffd6961b36a8a0) )
	ROM_LOAD( "kernal-264.bin", 0x4000, 0x4000, CRC(8f32abe7) SHA1(d481faf5fcbb331878dc7851c642d04f26a32873) )

	ROM_REGION( 0x8000, "function", ROMREGION_ERASE00 )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( plus4n )
//-------------------------------------------------

ROM_START( plus4n )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_DEFAULT_BIOS("r5")
	ROM_SYSTEM_BIOS( 0, "r4", "Revision 4" )
	ROMX_LOAD( "318005-04.u24", 0x4000, 0x4000, CRC(799a633d) SHA1(5df52c693387c0e2b5d682613a3b5a65477311cf), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "r5", "Revision 5" )
	ROMX_LOAD( "318005-05.u24", 0x4000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "jiffydos", "JiffyDOS v6.01" )
	ROMX_LOAD( "jiffydos plus4.u24", 0x0000, 0x8000, CRC(818d3f45) SHA1(9bc1b1c3da9ca642deae717905f990d8e36e6c3b), ROM_BIOS(3) ) // first half contains R5 kernal

	ROM_LOAD( "318006-01.u23", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )

	ROM_REGION( 0x8000, "function", 0 )
	ROM_LOAD( "317053-01.u25", 0x0000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.u26", 0x4000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u19", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( plus4p )
//-------------------------------------------------

ROM_START( plus4p )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01.u23", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )

	ROM_DEFAULT_BIOS("r5")
	ROM_SYSTEM_BIOS( 0, "r3", "Revision 3" )
	ROMX_LOAD( "318004-03.u4", 0x4000, 0x4000, CRC(77bab934) SHA1(97814dab9d757fe5a3a61d357a9a81da588a9783), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "r4", "Revision 4" )
	ROMX_LOAD( "318004-04.u4", 0x4000, 0x4000, CRC(be54ed79) SHA1(514ad3c29d01a2c0a3b143d9c1d4143b1912b793), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "r5", "Revision 5" )
	ROMX_LOAD( "318004-05.u4", 0x4000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb), ROM_BIOS(3) )

	ROM_REGION( 0x8000, "function", 0 )
	ROM_LOAD( "317053-01.u25", 0x0000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01.u26", 0x4000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u19", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c16 )
//-------------------------------------------------

ROM_START( c16 )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01.u3", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )

	ROM_DEFAULT_BIOS("r5")
	ROM_SYSTEM_BIOS( 0, "r3", "Revision 3" )
	ROMX_LOAD( "318004-03.u4", 0x4000, 0x4000, CRC(77bab934) SHA1(97814dab9d757fe5a3a61d357a9a81da588a9783), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "r4", "Revision 4" )
	ROMX_LOAD( "318004-04.u4", 0x4000, 0x4000, CRC(be54ed79) SHA1(514ad3c29d01a2c0a3b143d9c1d4143b1912b793), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "r5", "Revision 5" )
	ROMX_LOAD( "318004-05.u4", 0x4000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb), ROM_BIOS(3) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u16", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c116 )
//-------------------------------------------------

ROM_START( c116 )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01.u3", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )

	ROM_DEFAULT_BIOS("r5")
	ROM_SYSTEM_BIOS( 0, "r3", "Revision 3" )
	ROMX_LOAD( "318004-03.u4", 0x4000, 0x4000, CRC(77bab934) SHA1(97814dab9d757fe5a3a61d357a9a81da588a9783), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "r4", "Revision 4" )
	ROMX_LOAD( "318004-04.u4", 0x4000, 0x4000, CRC(be54ed79) SHA1(514ad3c29d01a2c0a3b143d9c1d4143b1912b793), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "r5", "Revision 5" )
	ROMX_LOAD( "318004-05.u4", 0x4000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb), ROM_BIOS(3) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u101", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c16h )
//-------------------------------------------------

ROM_START( c16h )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01.u3", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )

	ROM_DEFAULT_BIOS("r2")
	ROM_SYSTEM_BIOS( 0, "r1", "Revision 1" )
	ROMX_LOAD( "318030-01.u4", 0x4000, 0x4000, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "r2", "Revision 2" )
	ROMX_LOAD( "318030-02.u4", 0x4000, 0x4000, CRC(775f60c5) SHA1(20cf3c4bf6c54ef09799af41887218933f2e27ee), ROM_BIOS(2) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u16", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c232 )
//-------------------------------------------------

ROM_START( c232 )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01.u4", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "318004-01.u5", 0x4000, 0x4000, CRC(dbdc3319) SHA1(3c77caf72914c1c0a0875b3a7f6935cd30c54201) )

	ROM_REGION( 0x8000, "function", ROMREGION_ERASE00 )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02.u7", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( v364 )
//-------------------------------------------------

ROM_START( v364 )
	ROM_REGION( 0x8000, "kernal", 0 )
	ROM_LOAD( "318006-01", 0x0000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd) )
	ROM_LOAD( "kern364p", 0x4000, 0x4000, CRC(84fd4f7a) SHA1(b9a5b5dacd57ca117ef0b3af29e91998bf4d7e5f) )

	ROM_REGION( 0x10000, "function", 0 )
	ROM_LOAD( "317053-01", 0x0000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f) )
	ROM_LOAD( "317054-01", 0x4000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693) )
	ROM_LOAD( "spk3cc4.bin", 0x8000, 0x4000, CRC(5227c2ee) SHA1(59af401cbb2194f689898271c6e8aafa28a7af11) )

	ROM_REGION( 0xf5, PLA_TAG, 0 )
	ROM_LOAD( "251641-02", 0x00, 0xf5, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT                    COMPANY                         FULLNAME                        FLAGS
COMP( 1983, c264,	0,		0,		ntsc,	plus4,	driver_device,	0,		"Commodore Business Machines",	"Commodore 264 (Prototype)",	GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
COMP( 1984, plus4n,	c264,	0,		ntsc,	plus4,	driver_device,	0,		"Commodore Business Machines",	"Plus/4 (NTSC)",				GAME_SUPPORTS_SAVE )
COMP( 1984, plus4p,	c264,	0,		pal,	plus4,	driver_device,	0,		"Commodore Business Machines",	"Plus/4 (PAL)",					GAME_SUPPORTS_SAVE )
COMP( 1984, c16,	c264,	0,		c16,	c16,	driver_device,	0,		"Commodore Business Machines",	"Commodore 16",					GAME_SUPPORTS_SAVE )
COMP( 1984, c16h,	c264,	0,		c16,	c16,	driver_device,	0,		"Commodore Business Machines",	"Commodore 16 (Hungary)",		GAME_SUPPORTS_SAVE )
COMP( 1984, c116,	c264,	0,		c16,	c16,	driver_device,	0,		"Commodore Business Machines",	"Commodore 116",				GAME_SUPPORTS_SAVE )
COMP( 1984, c232,	c264,	0,		c232,	plus4,	driver_device,	0,		"Commodore Business Machines",	"Commodore 232 (Prototype)",	GAME_SUPPORTS_SAVE )
COMP( 1985, v364,	c264,	0,		v364,	plus4,	driver_device,	0,		"Commodore Business Machines",	"Commodore V364 (Prototype)",	GAME_SUPPORTS_SAVE )
