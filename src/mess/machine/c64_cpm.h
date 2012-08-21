/**********************************************************************

    Commodore 64 CP/M cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __CPM__
#define __CPM__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_cpm_cartridge_device

class c64_cpm_cartridge_device : public device_t,
								 public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_cpm_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;

	// not really public
	DECLARE_READ8_MEMBER( dma_r );
	DECLARE_WRITE8_MEMBER( dma_w );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "c64_cpm"; }

	// device_c64_expansion_card_interface overrides
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int roml, int romh, int io1, int io2);
	virtual int c64_game_r(offs_t offset, int ba, int rw, int hiram);

private:
	inline void update_signals();

	required_device<cpu_device> m_maincpu;

	int m_enabled;
	int m_ba;

	int m_reset;
};


// device type definition
extern const device_type C64_CPM;


#endif
