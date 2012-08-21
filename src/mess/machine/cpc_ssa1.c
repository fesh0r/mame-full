/*
 * cpc_ssa1.c  --  Amstrad SSA-1 Speech Synthesiser, dk'Tronics Speech Synthesiser
 *
 *  Created on: 16/07/2011
 *
 */


#include "emu.h"
#include "cpc_ssa1.h"
#include "includes/amstrad.h"

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type CPC_SSA1 = &device_creator<cpc_ssa1_device>;
const device_type CPC_DKSPEECH = &device_creator<cpc_dkspeech_device>;

CPC_EXPANSION_INTERFACE(sub_exp_intf)
{
	DEVCB_LINE(cpc_irq_w),
	DEVCB_LINE(cpc_nmi_w),
	DEVCB_NULL,  // RESET
	DEVCB_LINE(cpc_romdis),  // ROMDIS
	DEVCB_LINE(cpc_romen)  // /ROMEN
};

//-------------------------------------------------
//  device I/O handlers
//-------------------------------------------------

READ8_MEMBER(cpc_ssa1_device::ssa1_r)
{
	UINT8 ret = 0xff;

	if(get_sby() == 0)
		ret &= ~0x80;

	if(get_lrq() != 0)
		ret &= ~0x40;

	return ret;
}

WRITE8_MEMBER(cpc_ssa1_device::ssa1_w)
{
	sp0256_ALD_w(m_sp0256_device,0,data);
}

READ8_MEMBER(cpc_dkspeech_device::dkspeech_r)
{
	UINT8 ret = 0xff;

	// SBY is not connected

	if(get_lrq() != 0)
		ret &= ~0x80;

	return ret;
}

WRITE8_MEMBER(cpc_dkspeech_device::dkspeech_w)
{
	sp0256_ALD_w(m_sp0256_device,0,data & 0x3f);
}

WRITE_LINE_MEMBER(cpc_ssa1_device::lrq_cb)
{
	set_lrq(state);
}

WRITE_LINE_MEMBER(cpc_ssa1_device::sby_cb)
{
	set_sby(state);
}

WRITE_LINE_MEMBER(cpc_dkspeech_device::lrq_cb)
{
	set_lrq(state);
}

WRITE_LINE_MEMBER(cpc_dkspeech_device::sby_cb)
{
	set_sby(state);
}

static sp0256_interface sp0256_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER,cpc_ssa1_device,lrq_cb),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER,cpc_ssa1_device,sby_cb)
};

static sp0256_interface sp0256_dk_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER,cpc_dkspeech_device,lrq_cb),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER,cpc_dkspeech_device,sby_cb)
};

//-------------------------------------------------
//  Device ROM definition
//-------------------------------------------------

// Has no actual ROM, just that internal to the SP0256
ROM_START( cpc_ssa1 )
	ROM_REGION( 0x10000, "sp0256", 0 )
	ROM_LOAD( "sp0256-al2.bin",   0x1000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) )
ROM_END

// Available in ROM and cassette versions.  For now, we'll let the user choose to load the software via ROM (using a ROM box slot device) or cassette.
ROM_START( cpc_dkspeech )
	ROM_REGION( 0x10000, "sp0256", 0 )
	ROM_LOAD( "sp0256-al2.bin",   0x1000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) )
ROM_END

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *cpc_ssa1_device::device_rom_region() const
{
	return ROM_NAME( cpc_ssa1 );
}

const rom_entry *cpc_dkspeech_device::device_rom_region() const
{
	return ROM_NAME( cpc_dkspeech );
}

// device machine config
static MACHINE_CONFIG_FRAGMENT( cpc_ssa1 )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("sp0256",SP0256,XTAL_3_12MHz)
	MCFG_SOUND_CONFIG(sp0256_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	// pass-through
	MCFG_CPC_EXPANSION_SLOT_ADD("exp",sub_exp_intf,cpc_exp_cards,NULL,NULL)

MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( cpc_dkspeech )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("sp0256",SP0256,XTAL_4MHz)  // uses the CPC's clock from pin 50 of the expansion port
	MCFG_SOUND_CONFIG(sp0256_dk_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	// pass-through
	MCFG_CPC_EXPANSION_SLOT_ADD("exp",sub_exp_intf,cpc_exp_cards,NULL,NULL)

MACHINE_CONFIG_END

machine_config_constructor cpc_ssa1_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_ssa1 );
}

machine_config_constructor cpc_dkspeech_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_dkspeech );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_ssa1_device::cpc_ssa1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_SSA1, "SSA-1", tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this),
	m_lrq(1),
	m_sp0256_device(*this,"sp0256")
{
}

cpc_dkspeech_device::cpc_dkspeech_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_DKSPEECH, "DK'Tronics Speech Synthesiser", tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this),
	m_lrq(1),
	m_sp0256_device(*this,"sp0256")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_ssa1_device::device_start()
{
	device_t* cpu = machine().device("maincpu");
	address_space* space = cpu->memory().space(AS_IO);
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());

	m_rom = memregion("sp0256")->base();

//  m_sp0256_device = subdevice("sp0256");

	space->install_readwrite_handler(0xfaee,0xfaee,0,0,read8_delegate(FUNC(cpc_ssa1_device::ssa1_r),this),write8_delegate(FUNC(cpc_ssa1_device::ssa1_w),this));
	space->install_readwrite_handler(0xfbee,0xfbee,0,0,read8_delegate(FUNC(cpc_ssa1_device::ssa1_r),this),write8_delegate(FUNC(cpc_ssa1_device::ssa1_w),this));
}

void cpc_dkspeech_device::device_start()
{
	device_t* cpu = machine().device("maincpu");
	address_space* space = cpu->memory().space(AS_IO);
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());

	m_rom = memregion("sp0256")->base();

//  m_sp0256_device = subdevice("sp0256");

	space->install_readwrite_handler(0xfbfe,0xfbfe,0,0,read8_delegate(FUNC(cpc_dkspeech_device::dkspeech_r),this),write8_delegate(FUNC(cpc_dkspeech_device::dkspeech_w),this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_ssa1_device::device_reset()
{
	m_sp0256_device->reset();
}

void cpc_dkspeech_device::device_reset()
{
	m_sp0256_device->reset();
}

