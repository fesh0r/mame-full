/**********************************************************************

    Commodore VIC-20 Standard 8K/16K ROM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic20std.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC20_STD = &device_creator<vic20_standard_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic20_standard_cartridge_device - constructor
//-------------------------------------------------

vic20_standard_cartridge_device::vic20_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC20_STD, "VIC-20 Standard Cartridge", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic20_standard_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  vic20_cd_r - cartridge data read
//-------------------------------------------------

UINT8 vic20_standard_cartridge_device::vic20_cd_r(address_space &space, offs_t offset, int ram1, int ram2, int ram3, int blk1, int blk2, int blk3, int blk5, int io2, int io3)
{
	UINT8 data = 0;

	if (!blk1 && (m_blk1 != NULL))
	{
		data = m_blk1[offset & 0x1fff];
	}
	else if (!blk2 && (m_blk2 != NULL))
	{
		data = m_blk2[offset & 0x1fff];
	}
	else if (!blk3 && (m_blk3 != NULL))
	{
		data = m_blk3[offset & 0x1fff];
	}
	else if (!blk5 && (m_blk5 != NULL))
	{
		data = m_blk5[offset & 0x1fff];
	}

	return data;
}
