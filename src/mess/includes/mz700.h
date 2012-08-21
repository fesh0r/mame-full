/******************************************************************************
 *  Sharp MZ700
 *
 *  variables and function prototypes
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 ******************************************************************************/

#ifndef MZ700_H_
#define MZ700_H_

#include "machine/i8255.h"
#include "machine/pit8253.h"
#include "machine/z80pio.h"


class mz_state : public driver_device
{
public:
	mz_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	int m_mz700;				/* 1 if running on an mz700 */

	device_t *m_pit;
	i8255_device *m_ppi;

	int m_cursor_timer;
	int m_other_timer;

	int m_intmsk;	/* PPI8255 pin PC2 */

	int m_mz700_ram_lock;		/* 1 if ram lock is active */
	int m_mz700_ram_vram;		/* 1 if vram is banked in */

	/* mz800 specific */
	UINT8 *m_cgram;

	int m_mz700_mode;			/* 1 if in mz700 mode */
	int m_mz800_ram_lock;		/* 1 if lock is active */
	int m_mz800_ram_monitor;	/* 1 if monitor rom banked in */

	int m_hires_mode;			/* 1 if in 640x200 mode */
	int m_screen;			/* screen designation */
	UINT8 *m_colorram;
	UINT8 *m_videoram;
	UINT8 m_speaker_level;
	UINT8 m_prev_state;
	UINT16 m_mz800_ramaddr;
	UINT8 m_mz800_palette[4];
	UINT8 m_mz800_palette_bank;
	DECLARE_READ8_MEMBER(mz700_e008_r);
	DECLARE_WRITE8_MEMBER(mz700_e008_w);
	DECLARE_READ8_MEMBER(mz800_bank_0_r);
	DECLARE_WRITE8_MEMBER(mz700_bank_0_w);
	DECLARE_WRITE8_MEMBER(mz800_bank_0_w);
	DECLARE_READ8_MEMBER(mz800_bank_1_r);
	DECLARE_WRITE8_MEMBER(mz700_bank_1_w);
	DECLARE_WRITE8_MEMBER(mz700_bank_2_w);
	DECLARE_WRITE8_MEMBER(mz700_bank_3_w);
	DECLARE_WRITE8_MEMBER(mz700_bank_4_w);
	DECLARE_WRITE8_MEMBER(mz700_bank_5_w);
	DECLARE_WRITE8_MEMBER(mz700_bank_6_w);
	DECLARE_READ8_MEMBER(mz800_crtc_r);
	DECLARE_WRITE8_MEMBER(mz800_write_format_w);
	DECLARE_WRITE8_MEMBER(mz800_read_format_w);
	DECLARE_WRITE8_MEMBER(mz800_display_mode_w);
	DECLARE_WRITE8_MEMBER(mz800_scroll_border_w);
	DECLARE_READ8_MEMBER(mz800_ramdisk_r);
	DECLARE_WRITE8_MEMBER(mz800_ramdisk_w);
	DECLARE_WRITE8_MEMBER(mz800_ramaddr_w);
	DECLARE_WRITE8_MEMBER(mz800_palette_w);
	DECLARE_WRITE8_MEMBER(mz800_cgram_w);
	DECLARE_DRIVER_INIT(mz800);
	DECLARE_DRIVER_INIT(mz700);
};


/*----------- defined in machine/mz700.c -----------*/

extern const struct pit8253_config mz700_pit8253_config;
extern const struct pit8253_config mz800_pit8253_config;
extern const i8255_interface mz700_ppi8255_interface;
extern const z80pio_interface mz800_z80pio_config;

MACHINE_START( mz700 );

/* bank switching */

/* bankswitching, mz800 only */




/*----------- defined in video/mz700.c -----------*/

PALETTE_INIT( mz700 );
SCREEN_UPDATE_IND16( mz700 );
VIDEO_START( mz800 );
SCREEN_UPDATE_IND16( mz800 );


#endif /* MZ700_H_ */
