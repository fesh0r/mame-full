#pragma once

#ifndef __KC_D004_H__
#define __KC_D004_H__

#include "emu.h"
#include "machine/kcexp.h"
#include "machine/z80ctc.h"
#include "cpu/z80/z80.h"
#include "machine/upd765.h"
#include "machine/idectrl.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "imagedev/harddriv.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> kc_d004_device

class kc_d004_device :
		public device_t,
		public device_kcexp_interface
{
public:
	// construction/destruction
	kc_d004_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	kc_d004_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual void device_config_complete() { m_shortname = "kc_d004"; }

	// kcexp_interface overrides
	virtual UINT8 module_id_r() { return 0xa7; }
	virtual void control_w(UINT8 data);
	virtual void read(offs_t offset, UINT8 &data);
	virtual void io_read(offs_t offset, UINT8 &data);
	virtual void io_write(offs_t offset, UINT8 data);

public:
	device_t* get_active_fdd() { return m_floppy; }
	DECLARE_READ8_MEMBER(hw_input_gate_r);
	DECLARE_WRITE8_MEMBER(fdd_select_w);
	DECLARE_WRITE8_MEMBER(hw_terminal_count_w);
	DECLARE_WRITE_LINE_MEMBER(fdc_interrupt);
	DECLARE_WRITE_LINE_MEMBER(fdc_dma_request);

private:
	static const device_timer_id TIMER_RESET = 0;
	static const device_timer_id TIMER_TC_CLEAR = 1;

	required_device<cpu_device> m_cpu;
	required_device<device_t>	m_fdc;
	required_shared_ptr<UINT8>	m_koppel_ram;

	// internal state
	emu_timer *			m_reset_timer;
	emu_timer *			m_tc_clear_timer;

	UINT8 *				m_rom;
	UINT8				m_hw_input_gate;
	UINT16				m_rom_base;
	UINT8				m_enabled;
	UINT8				m_connected;
	UINT8				m_active_fdd;
	device_t *			m_floppy;
};


// ======================> kc_d004_gide_device

class kc_d004_gide_device :
		public kc_d004_device
{
public:
	// construction/destruction
	kc_d004_gide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;

protected:
	// device-level overrides
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "kc_d004gide"; }

public:
	DECLARE_READ8_MEMBER(gide_r);
	DECLARE_WRITE8_MEMBER(gide_w);

private:
	required_device<device_t> m_ide;

	UINT16				m_ide_data;
	int 				m_lh;
};

// device type definition
extern const device_type KC_D004;
extern const device_type KC_D004_GIDE;

#endif  /* __KC_D004_H__ */
