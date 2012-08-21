/**********************************************************************

    Brown Boxes Double Quick Brown Box emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - 64/128 mode switch
    - dump of the initial NVRAM contents

*/

#include "c64_dqbb.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_DQBB = &device_creator<c64_dqbb_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_dqbb_cartridge_device - constructor
//-------------------------------------------------

c64_dqbb_cartridge_device::c64_dqbb_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_DQBB, "C64 Double Quick Brown Box cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	device_nvram_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_dqbb_cartridge_device::device_start()
{
	// allocate memory
	c64_nvram_pointer(machine(), 0x4000);

	// state saving
	save_item(NAME(m_cs));
	save_item(NAME(m_we));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_dqbb_cartridge_device::device_reset()
{
	m_exrom = 0; // TODO 1 in 128 mode
	m_game = 1;
	m_cs = 0;
	m_we = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_dqbb_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int ba, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!m_cs && (!roml || !romh))
	{
		data = m_nvram[offset & 0x3fff];
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_dqbb_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int roml, int romh, int io1, int io2)
{
	if (!m_cs && m_we && (offset >= 0x8000 && offset < 0xc000))
	{
		m_nvram[offset & 0x3fff] = data;
	}
	else if (!io1)
	{
		/*

            bit     description

            0
            1
            2       GAME
            3
            4       WE
            5
            6       EXROM
            7       _CS

        */

		m_exrom = !BIT(data, 6);
		m_game = !BIT(data, 2);
		m_we = BIT(data, 4);
		m_cs = BIT(data, 7);
	}
}
