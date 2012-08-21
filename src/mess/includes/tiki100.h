#pragma once

#ifndef __TIKI100__
#define __TIKI100__


#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/z80pio.h"
#include "machine/wd17xx.h"
#include "sound/ay8910.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80DART_TAG		"z80dart"
#define Z80PIO_TAG		"z80pio"
#define Z80CTC_TAG		"z80ctc"
#define FD1797_TAG		"fd1797"
#define AY8912_TAG		"ay8912"

#define TIKI100_VIDEORAM_SIZE	0x8000
#define TIKI100_VIDEORAM_MASK	0x7fff

#define BANK_ROM		0
#define BANK_RAM		1
#define BANK_VIDEO_RAM	2

class tiki100_state : public driver_device
{
public:
	tiki100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_fdc(*this, FD1797_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<z80ctc_device> m_ctc;
	required_device<device_t> m_fdc;
	required_device<ram_device> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	READ8_MEMBER( gfxram_r );
	WRITE8_MEMBER( gfxram_w );
	READ8_MEMBER( keyboard_r );
	WRITE8_MEMBER( keyboard_w );
	WRITE8_MEMBER( video_mode_w );
	WRITE8_MEMBER( palette_w );
	WRITE8_MEMBER( system_w );
	WRITE_LINE_MEMBER( ctc_z1_w );
	WRITE8_MEMBER( video_scroll_w );

	void bankswitch();

	/* memory state */
	int m_rome;
	int m_vire;

	/* video state */
	UINT8 *m_video_ram;
	UINT8 m_scroll;
	UINT8 m_mode;
	UINT8 m_palette;

	/* keyboard state */
	int m_keylatch;
};

#endif
