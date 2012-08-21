/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C1551__
#define __C1551__


#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/64h156.h"
#include "machine/6525tpi.h"
#include "machine/pls100.h"
#include "machine/plus4exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1551_device

class c1551_device :  public device_t,
					  public device_plus4_expansion_card_interface
{
public:
    // construction/destruction
    c1551_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_READ8_MEMBER( port_r );
	DECLARE_WRITE8_MEMBER( port_w );
	DECLARE_READ8_MEMBER( tcbm_data_r );
	DECLARE_WRITE8_MEMBER( tcbm_data_w );
	DECLARE_READ8_MEMBER( yb_r );
	DECLARE_WRITE8_MEMBER( yb_w );
	DECLARE_READ8_MEMBER( tpi0_pc_r );
	DECLARE_WRITE8_MEMBER( tpi0_pc_w );
	DECLARE_READ8_MEMBER( tpi1_pa_r );
	DECLARE_WRITE8_MEMBER( tpi1_pa_w );
	DECLARE_READ8_MEMBER( tpi1_pb_r );
	DECLARE_READ8_MEMBER( tpi1_pc_r );
	DECLARE_WRITE8_MEMBER( tpi1_pc_w );

protected:
    // device-level overrides
    virtual void device_config_complete() { m_shortname = "c1551"; }
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

    // device_plus4_expansion_card_interface overrides
	virtual UINT8 plus4_cd_r(address_space &space, offs_t offset, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h);
	virtual void plus4_cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h);
	virtual void plus4_breset_w(int state);

private:
	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_tpi0;
	required_device<device_t> m_tpi1;
	required_device<c64h156_device> m_ga;
	required_device<legacy_floppy_image_device> m_image;

	// TCBM bus
	UINT8 m_tcbm_data;						// data
	int m_status;							// status
	int m_dav;								// data valid
	int m_ack;								// acknowledge
	int m_dev;								// device number

	// timers
	emu_timer *m_irq_timer;
};



// device type definition
extern const device_type C1551;



#endif
