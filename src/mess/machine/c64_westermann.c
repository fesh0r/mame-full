/**********************************************************************

    Westermann Learning cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_westermann.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_WESTERMANN = &device_creator<c64_westermann_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_westermann_cartridge_device - constructor
//-------------------------------------------------

c64_westermann_cartridge_device::c64_westermann_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_WESTERMANN, "C64 Westermann cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_westermann_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_westermann_cartridge_device::device_reset()
{
	m_game = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_westermann_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int ba, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!roml)
	{
		data = m_roml[offset & 0x1fff];
	}
	else if (!romh)
	{
		if (m_romh_mask)
		{
			data = m_romh[offset & 0x1fff];
		}
		else
		{
			data = m_roml[offset & 0x3fff];
		}
	}
	else if (!io2)
	{
		m_game = 1;
	}

	return data;
}
