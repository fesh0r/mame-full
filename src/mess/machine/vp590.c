/**********************************************************************

    RCA VIP Color Board VP590 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vp590.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CDP1862_TAG		"u2"
#define SCREEN_TAG		":screen"

#define COLOR_RAM_SIZE	0x100



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VP590 = &device_creator<vp590_device>;


//-------------------------------------------------
//  CDP1862_INTERFACE( cgc_intf )
//-------------------------------------------------

READ_LINE_MEMBER( vp590_device::rd_r )
{
	return BIT(m_color, 1);
}

READ_LINE_MEMBER( vp590_device::bd_r )
{
	return BIT(m_color, 2);
}

READ_LINE_MEMBER( vp590_device::gd_r )
{
	return BIT(m_color, 3);
}

static CDP1862_INTERFACE( cgc_intf )
{
	SCREEN_TAG,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vp590_device, rd_r),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vp590_device, bd_r),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vp590_device, gd_r),
	RES_R(510),		// R3
	RES_R(360),		// R4
	RES_K(1),		// R5
	RES_K(1.5),		// R6
	RES_K(3.9),		// R7
	RES_K(10),		// R8
	RES_K(2),		// R9
	RES_K(3.3)		// R10
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( vp590 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vp590 )
	MCFG_CDP1862_ADD(CDP1862_TAG, CPD1862_CLOCK, cgc_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vp590_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vp590 );
}


//-------------------------------------------------
//  INPUT_PORTS( vp590 )
//-------------------------------------------------

static INPUT_PORTS_START( vp590 )
	PORT_START("J1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 0")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 1")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 2")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 3")
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 4")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 5")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 6")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 7")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 8")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad 9")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad A")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad B")
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad C")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad D")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad E")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 1 Keypad F")

	PORT_START("J2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 0")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 1")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 2")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 3")
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 4")
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 5")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 6")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 7")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 8")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad 9")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad A")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad B")
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad C")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad D")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad E")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("Player 2 Keypad F")
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor vp590_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( vp590 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vp590_device - constructor
//-------------------------------------------------

vp590_device::vp590_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, VP590, "VP590", tag, owner, clock),
	device_vip_expansion_card_interface(mconfig, *this),
	m_cgc(*this, CDP1862_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp590_device::device_start()
{
	// allocate memory
	m_color_ram = auto_alloc_array(machine(), UINT8, COLOR_RAM_SIZE);

	// state saving
	save_pointer(NAME(m_color_ram), COLOR_RAM_SIZE);
	save_item(NAME(m_color));
	save_item(NAME(m_keylatch));
}


//-------------------------------------------------
//  vip_program_w - program write
//-------------------------------------------------

void vp590_device::vip_program_w(address_space &space, offs_t offset, UINT8 data, int cdef, int *minh)
{
	if (offset >= 0xc000 && offset < 0xe000)
	{
		UINT8 mask = 0xff;

		m_a12 = (offset & 0x1000) ? 1 : 0;

		if (!m_a12)
		{
			// mask out A4 and A3
			mask = 0xe7;
		}

		// write to CDP1822
		m_color_ram[offset & mask] = data << 1;

		m_cgc->con_w(0);
	}
}


//-------------------------------------------------
//  vip_io_w - I/O write
//-------------------------------------------------

void vp590_device::vip_io_w(address_space &space, offs_t offset, UINT8 data)
{
	switch (offset)
	{
	case 0x02:
		m_keylatch = data & 0x0f;
		break;

	case 0x05:
		m_cgc->bkg_w(1);
		m_cgc->bkg_w(0);
		break;
	}
}


//-------------------------------------------------
//  vip_dma_w - DMA write
//-------------------------------------------------

void vp590_device::vip_dma_w(address_space &space, offs_t offset, UINT8 data)
{
	UINT8 mask = 0xff;

	if (!m_a12)
	{
		// mask out A4 and A3
		mask = 0xe7;
	}

	m_color = m_color_ram[offset & mask];

	m_cgc->dma_w(space, offset, data);
}


//-------------------------------------------------
//  vip_screen_update - screen update
//-------------------------------------------------

UINT32 vp590_device::vip_screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_cgc->screen_update(screen, bitmap, cliprect);

	return 0;
}


//-------------------------------------------------
//  vip_ef3_r - EF3 read
//-------------------------------------------------

int vp590_device::vip_ef3_r()
{
	return BIT(ioport("J1")->read(), m_keylatch) ? CLEAR_LINE : ASSERT_LINE;
}


//-------------------------------------------------
//  vip_ef4_r - EF4 read
//-------------------------------------------------

int vp590_device::vip_ef4_r()
{
	return BIT(ioport("J2")->read(), m_keylatch) ? CLEAR_LINE : ASSERT_LINE;
}
