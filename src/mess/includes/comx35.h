#pragma once

#ifndef __COMX35__
#define __COMX35__


#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "formats/comx35_comx.h"
#include "imagedev/cassette.h"
#include "imagedev/printer.h"
#include "imagedev/snapquik.h"
#include "machine/cdp1871.h"
#include "machine/comxexp.h"
#include "machine/comxpl80.h"
#include "machine/comx_clm.h"
#include "machine/comx_eb.h"
#include "machine/comx_epr.h"
#include "machine/comx_fd.h"
#include "machine/comx_joy.h"
#include "machine/comx_prn.h"
#include "machine/comx_ram.h"
#include "machine/comx_thm.h"
#include "machine/ram.h"
#include "machine/rescap.h"

#define SCREEN_TAG			"screen"

#define CDP1870_TAG			"u1"
#define CDP1869_TAG			"u2"
#define CDP1802_TAG			"u3"
#define CDP1871_TAG			"u4"
#define EXPANSION_TAG		"slot"

#define COMX35_CHARRAM_SIZE 0x800
#define COMX35_CHARRAM_MASK 0x7ff

class comx35_state : public driver_device
{
public:
	comx35_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vis(*this, CDP1869_TAG),
		  m_kbe(*this, CDP1871_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_ram(*this, RAM_TAG),
		  m_expansion(*this, EXPANSION_TAG),
		  m_ef4(0)
	{ }

	required_device<cosmac_device> m_maincpu;
	required_device<cdp1869_device> m_vis;
	required_device<cdp1871_device> m_kbe;
	required_device<cassette_image_device> m_cassette;
	required_device<ram_device> m_ram;
	required_device<comx_expansion_slot_device> m_expansion;

	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	enum
	{
		TIMER_ID_RESET
	};

	void check_interrupt();

	DECLARE_READ8_MEMBER( mem_r );
	DECLARE_WRITE8_MEMBER( mem_w );
	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );
	DECLARE_WRITE8_MEMBER( cdp1869_w );
	DECLARE_READ_LINE_MEMBER( clear_r );
	DECLARE_READ_LINE_MEMBER( ef2_r );
	DECLARE_READ_LINE_MEMBER( ef4_r );
	DECLARE_WRITE_LINE_MEMBER( q_w );
	DECLARE_READ_LINE_MEMBER( shift_r );
	DECLARE_READ_LINE_MEMBER( control_r );
	DECLARE_WRITE_LINE_MEMBER( ef4_w );
	DECLARE_WRITE_LINE_MEMBER( int_w );
	DECLARE_WRITE_LINE_MEMBER( prd_w );
	DECLARE_INPUT_CHANGED_MEMBER( trigger_reset );

	// processor state
	int m_clear;				// CPU mode
	int m_q;					// Q flag
	int m_ef4;					// EF4 flag
	int m_iden;					// interrupt/DMA enable
	int m_dma;					// memory refresh DMA
	int m_int;					// interrupt request
	int m_prd;					// predisplay
	int m_cr1;					// interrupt enable

	// video state
	UINT8 *m_charram;			// character memory
};

// ---------- defined in video/comx35.c ----------

MACHINE_CONFIG_EXTERN( comx35_pal_video );
MACHINE_CONFIG_EXTERN( comx35_ntsc_video );

#endif
