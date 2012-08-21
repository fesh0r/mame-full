/**********************************************************************

    Luxor ABC-800 keyboard emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __ABC800_KEYBOARD__
#define __ABC800_KEYBOARD__


#include "emu.h"
#include "cpu/mcs48/mcs48.h"
#include "sound/discrete.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define ABC800_KEYBOARD_TAG	"abc800kb"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ABC800_KEYBOARD_ADD(_config) \
    MCFG_DEVICE_ADD(ABC800_KEYBOARD_TAG, ABC800_KEYBOARD, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define ABC800_KEYBOARD_INTERFACE(_name) \
	const abc800_keyboard_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> abc800_keyboard_interface

struct abc800_keyboard_interface
{
	devcb_write_line	m_out_clock_cb;
	devcb_write_line	m_out_keydown_cb;
};


// ======================> abc800_keyboard_device

class abc800_keyboard_device :  public device_t,
								public abc800_keyboard_interface
{
public:
    // construction/destruction
    abc800_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

	DECLARE_WRITE_LINE_MEMBER( rxd_w );
	DECLARE_READ_LINE_MEMBER( txd_r );

	// not really public
	DECLARE_READ8_MEMBER( kb_p1_r );
	DECLARE_WRITE8_MEMBER( kb_p1_w );
	DECLARE_WRITE8_MEMBER( kb_p2_w );
	DECLARE_READ8_MEMBER( kb_t1_r );

protected:
    // device-level overrides
	virtual void device_config_complete();
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	devcb_resolved_write_line	m_out_clock_func;
	devcb_resolved_write_line	m_out_keydown_func;

	inline void serial_output(int state);
	inline void serial_clock();
	inline void key_down(int state);

	required_device<cpu_device> m_maincpu;

	int m_row;
	int m_txd;
	int m_clk;
	int m_stb;
	int m_keydown;

	// timers
	emu_timer *m_serial_timer;
};


// device type definition
extern const device_type ABC800_KEYBOARD;



#endif
