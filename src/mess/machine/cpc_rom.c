/*
 * cpc_rom.c
 * Amstrad CPC mountable ROM image device
 *
 */

#include "emu.h"
#include "machine/cpc_rom.h"
#include "includes/amstrad.h"

const device_type CPC_ROM = &device_creator<cpc_rom_device>;


//**************************************************************************
//  DEVICE CONFIG INTERFACE
//**************************************************************************

CPC_EXPANSION_INTERFACE(sub_exp_intf)
{
	DEVCB_LINE(cpc_irq_w),
	DEVCB_LINE(cpc_nmi_w),//LINE_MEMBER(cpc_expansion_slot_device,nmi_w),
	DEVCB_NULL,  // RESET
	DEVCB_LINE(cpc_romdis),  // ROMDIS
	DEVCB_LINE(cpc_romen)  // /ROMEN
};

// device machine config
static MACHINE_CONFIG_FRAGMENT( cpc_rom )
	MCFG_ROMSLOT_ADD("rom1")
	MCFG_ROMSLOT_ADD("rom2")
	MCFG_ROMSLOT_ADD("rom3")
	MCFG_ROMSLOT_ADD("rom4")
	MCFG_ROMSLOT_ADD("rom5")
	MCFG_ROMSLOT_ADD("rom6")

	// pass-through
	MCFG_CPC_EXPANSION_SLOT_ADD("exp",sub_exp_intf,cpc_exp_cards,NULL,NULL)

MACHINE_CONFIG_END


machine_config_constructor cpc_rom_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_rom );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_rom_device::cpc_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_ROM, "ROM Box", tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_rom_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_rom_device::device_reset()
{
}


/*** ROM image device ***/

// device type definition
const device_type ROMSLOT = &device_creator<rom_image_device>;

//-------------------------------------------------
//  rom_image_device - constructor
//-------------------------------------------------

rom_image_device::rom_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, ROMSLOT, "ROM image", tag, owner, clock),
	  device_image_interface(mconfig, *this)
{

}

//-------------------------------------------------
//  rom_image_device - destructor
//-------------------------------------------------

rom_image_device::~rom_image_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rom_image_device::device_start()
{
	m_base = NULL;
}

/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( rom )
-------------------------------------------------*/
bool rom_image_device::call_load()
{
	device_image_interface* image = this;
	UINT64 size = image->length();

	m_base = (UINT8*)malloc(16384);
	if(size <= 16384)
	{
		image->fread(m_base,size);
	}
	else
	{
		image->fseek(size-16384,SEEK_SET);
		image->fread(m_base,16384);
	}

	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
    DEVICE_IMAGE_UNLOAD( rom )
-------------------------------------------------*/
void rom_image_device::call_unload()
{
	free(m_base);
	m_base = NULL;
}
