#pragma once

#ifndef __PC8001__
#define __PC8001__


#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/cassette.h"
#include "machine/ctronics.h"
#include "machine/8257dma.h"
#include "machine/i8255.h"
#include "machine/i8251.h"
#include "machine/ram.h"
#include "machine/upd1990a.h"
#include "sound/speaker.h"
#include "video/upd3301.h"

#define Z80_TAG			"z80"
#define I8251_TAG		"i8251"
#define I8255A_TAG		"i8255"
#define I8257_TAG		"i8257"
#define UPD1990A_TAG	"upd1990a"
#define UPD3301_TAG		"upd3301"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"

class pc8001_state : public driver_device
{
public:
	pc8001_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_rtc(*this, UPD1990A_TAG),
		  m_dma(*this, I8257_TAG),
		  m_crtc(*this, UPD3301_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_ram(*this, RAM_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<upd1990a_device> m_rtc;
	required_device<device_t> m_dma;
	required_device<upd3301_device> m_crtc;
	required_device<cassette_image_device> m_cassette;
	required_device<centronics_device> m_centronics;
	required_device<device_t> m_speaker;
	required_device<ram_device> m_ram;

	virtual void machine_start();

	virtual void video_start();

	DECLARE_WRITE8_MEMBER( port10_w );
	DECLARE_WRITE8_MEMBER( port30_w );
	DECLARE_READ8_MEMBER( port40_r );
	DECLARE_WRITE8_MEMBER( port40_w );
	DECLARE_WRITE_LINE_MEMBER( crtc_drq_w );
	DECLARE_WRITE_LINE_MEMBER( hrq_w );
	DECLARE_WRITE8_MEMBER( dma_mem_w );
	DECLARE_READ8_MEMBER( dma_io_r );
	DECLARE_WRITE8_MEMBER( dma_io_w );

	/* video state */
	UINT8 *m_char_rom;
	int m_width80;
	int m_color;
};

class pc8001mk2_state : public pc8001_state
{
public:
	pc8001mk2_state(const machine_config &mconfig, device_type type, const char *tag)
		: pc8001_state(mconfig, type, tag)
	{ }

	DECLARE_WRITE8_MEMBER( port31_w );
};

#endif
