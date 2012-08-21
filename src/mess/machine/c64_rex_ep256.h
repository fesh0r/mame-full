/**********************************************************************

    Rex Datentechnik 256KB EPROM cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __REX_EP256__
#define __REX_EP256__


#include "emu.h"
#include "imagedev/cartslot.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_rex_ep256_cartridge_device

class c64_rex_ep256_cartridge_device : public device_t,
									   public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_rex_ep256_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device-level overrides
	virtual void device_config_complete() { m_shortname = "rexep256"; }
	virtual void device_start();
	virtual void device_reset();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(address_space &space, offs_t offset, int ba, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int roml, int romh, int io1, int io2);

private:
	UINT8 *m_rom;

	UINT8 m_bank;
	int m_reset;
};


// device type definition
extern const device_type C64_REX_EP256;



#endif
