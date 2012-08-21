/*********************************************************************

    a2swyft.c

    Implementation of the IAI SwyftCard

*********************************************************************/

#include "emu.h"
#include "includes/apple2.h"
#include "machine/a2swyft.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_SWYFT = &device_creator<a2bus_swyft_device>;

#define SWYFT_ROM_REGION  "swyft_rom"

ROM_START( swyft )
	ROM_REGION(0x4000, SWYFT_ROM_REGION, 0)
    ROM_LOAD( "swyftcard rom.bin", 0x000000, 0x004000, CRC(5d6673e9) SHA1(1554bd03c536789f0ff7d1ef6c992265e311935d) )
ROM_END

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *a2bus_swyft_device::device_rom_region() const
{
	return ROM_NAME( swyft );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_swyft_device::a2bus_swyft_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, A2BUS_SWYFT, "IAI SwyftCard", tag, owner, clock),
		device_a2bus_card_interface(mconfig, *this)
{
	m_shortname = "a2swyft";
}

a2bus_swyft_device::a2bus_swyft_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_a2bus_card_interface(mconfig, *this)
{
	m_shortname = "a2swyft";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_swyft_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	astring tempstring;
	m_rom = device().machine().root_device().memregion(this->subtag(tempstring, SWYFT_ROM_REGION))->base();

	save_item(NAME(m_rombank));
}

void a2bus_swyft_device::device_reset()
{
    m_rombank = 0;

    // take over the machine
	apple2_state *state = machine().driver_data<apple2_state>();
    raise_slot_inh();
    state->m_maincpu->reset();
}

UINT8 a2bus_swyft_device::read_c0nx(address_space &space, UINT8 offset)
{
    switch (offset)
    {
        case 0:
            m_rombank = 0;
            raise_slot_inh();
            break;

        case 1:
            m_rombank = 0;
            lower_slot_inh();
            break;

        case 2:
            m_rombank = 0x1000;
            raise_slot_inh();
            break;
    }

    return 0xff;
}

void a2bus_swyft_device::write_c0nx(address_space &space, UINT8 offset, UINT8 data)
{
    switch (offset)
    {
        case 0:
            m_rombank = 0;
            raise_slot_inh();
            break;

        case 1:
            m_rombank = 0;
            lower_slot_inh();
            break;

        case 2:
            m_rombank = 0x1000;
            raise_slot_inh();
            break;
    }
}

UINT8 a2bus_swyft_device::read_inh_rom(address_space &space, UINT16 offset)
{
    if (offset < 0x1000)    // banked area d000-dfff
    {
        return m_rom[offset + m_rombank];
    }
    else                    // fixed area e000-ffff
    {
        return m_rom[offset - 0x1000 + 0x2000];
    }
}

