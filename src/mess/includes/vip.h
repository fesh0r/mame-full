#pragma once

#ifndef __VIP__
#define __VIP__

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "imagedev/cassette.h"
#include "imagedev/snapquik.h"
#include "machine/ram.h"
#include "machine/vip_byteio.h"
#include "machine/vip_exp.h"
#include "sound/discrete.h"
#include "video/cdp1861.h"

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"u1"
#define CDP1861_TAG		"u2"
#define DISCRETE_TAG	"discrete"


class vip_state : public driver_device
{
public:
	vip_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vdc(*this, CDP1861_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_beeper(*this, DISCRETE_TAG),
		  m_byteio(*this, VIP_BYTEIO_PORT_TAG),
		  m_exp(*this, VIP_EXPANSION_SLOT_TAG),
		  m_ram(*this, RAM_TAG),
		  m_8000(1),
		  m_vdc_int(CLEAR_LINE),
		  m_vdc_dma_out(CLEAR_LINE),
		  m_vdc_ef1(CLEAR_LINE),
		  m_exp_int(CLEAR_LINE),
		  m_exp_dma_out(CLEAR_LINE),
		  m_exp_dma_in(CLEAR_LINE),
		  m_byteio_ef3(CLEAR_LINE),
		  m_byteio_ef4(CLEAR_LINE),
		  m_exp_ef1(CLEAR_LINE),
		  m_exp_ef3(CLEAR_LINE),
		  m_exp_ef4(CLEAR_LINE)
	{ }

	required_device<cosmac_device> m_maincpu;
	required_device<cdp1861_device> m_vdc;
	required_device<cassette_image_device> m_cassette;
	required_device<discrete_sound_device> m_beeper;
	required_device<vip_byteio_port_device> m_byteio;
	required_device<vip_expansion_slot_device> m_exp;
	required_device<ram_device> m_ram;

	virtual void machine_start();
	virtual void machine_reset();

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void update_interrupts();

	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );
	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );

	DECLARE_READ_LINE_MEMBER( clear_r );
	DECLARE_READ_LINE_MEMBER( ef1_r );
	DECLARE_READ_LINE_MEMBER( ef2_r );
	DECLARE_READ_LINE_MEMBER( ef3_r );
	DECLARE_READ_LINE_MEMBER( ef4_r );
	DECLARE_WRITE_LINE_MEMBER( q_w );
	DECLARE_READ8_MEMBER( dma_r );
	DECLARE_WRITE8_MEMBER( dma_w );

	DECLARE_WRITE_LINE_MEMBER( vdc_int_w );
	DECLARE_WRITE_LINE_MEMBER( vdc_dma_out_w );
	DECLARE_WRITE_LINE_MEMBER( vdc_ef1_w );

	DECLARE_WRITE_LINE_MEMBER( byteio_inst_w );

	DECLARE_WRITE_LINE_MEMBER( exp_int_w );
	DECLARE_WRITE_LINE_MEMBER( exp_dma_out_w );
	DECLARE_WRITE_LINE_MEMBER( exp_dma_in_w );

	DECLARE_INPUT_CHANGED_MEMBER( reset_w );
	DECLARE_INPUT_CHANGED_MEMBER( beeper_w );

	// memory state
	int m_8000;

	// interrupt state
	int m_vdc_int;
	int m_vdc_dma_out;
	int m_vdc_ef1;
	int m_exp_int;
	int m_exp_dma_out;
	int m_exp_dma_in;
	int m_byteio_ef3;
	int m_byteio_ef4;
	int m_exp_ef1;
	int m_exp_ef3;
	int m_exp_ef4;

	// keyboard state
	int m_keylatch;

	// expansion state
	UINT8 m_byteio_data;
};

#endif
