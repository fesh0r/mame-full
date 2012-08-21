#ifndef __XEROX820__
#define __XEROX820__

#include "machine/ram.h"
#include "machine/z80ctc.h"

#define SCREEN_TAG		"screen"

#define Z80_TAG			"u46"
#define Z80KBPIO_TAG	"u105"
#define Z80GPPIO_TAG	"u101"
#define Z80SIO_TAG		"u96"
#define Z80CTC_TAG		"u99"
#define FD1797_TAG		"u109"
#define COM8116_TAG		"u76"
#define I8086_TAG		"i8086"

#define XEROX820_VIDEORAM_SIZE	0x1000
#define XEROX820_VIDEORAM_MASK	0x0fff

class xerox820_state : public driver_device
{
public:
	xerox820_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, Z80_TAG),
	m_kbpio(*this, Z80KBPIO_TAG),
	m_ctc(*this, Z80CTC_TAG),
	m_fdc(*this, FD1797_TAG),
	m_speaker(*this, SPEAKER_TAG),
	m_beeper(*this, BEEPER_TAG),
	m_ram(*this, RAM_TAG),
	m_floppy0(*this, FLOPPY_0),
	m_floppy1(*this, FLOPPY_1),
	m_video_ram(*this, "video_ram"){ }

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	required_device<cpu_device> m_maincpu;
	required_device<z80pio_device> m_kbpio;
	required_device<z80ctc_device> m_ctc;
	required_device<device_t> m_fdc;
	optional_device<device_t> m_speaker;
	optional_device<device_t> m_beeper;
	required_device<ram_device> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	DECLARE_WRITE8_MEMBER( scroll_w );
	//DECLARE_WRITE8_MEMBER( x120_system_w );
	DECLARE_READ8_MEMBER( kbpio_pa_r );
	DECLARE_WRITE8_MEMBER( kbpio_pa_w );
	DECLARE_READ8_MEMBER( kbpio_pb_r );
	DECLARE_WRITE_LINE_MEMBER( intrq_w );
	DECLARE_WRITE_LINE_MEMBER( drq_w );

	void scan_keyboard();
	void bankswitch(int bank);
	void set_floppy_parameters(size_t length);
	void common_kbpio_pa_w(UINT8 data);

	/* keyboard state */
	int m_keydata;						/* keyboard data */
	bool m_bit5;

	/* video state */
	required_shared_ptr<UINT8> m_video_ram; 					/* video RAM */
	UINT8 *m_char_rom;					/* character ROM */
	UINT8 m_scroll;						/* vertical scroll */
	UINT8 m_framecnt;
	int m_ncset2;						/* national character set */
	int m_vatt;							/* X120 video attribute */
	int m_lowlite;						/* low light attribute */
	int m_chrom;						/* character ROM index */

	/* floppy state */
	int m_fdc_irq;						/* interrupt request */
	int m_fdc_drq;						/* data request */
	int m_8n5;							/* 5.25" / 8" drive select */
	int m_dsdd;							/* double sided disk detect */
};

class xerox820ii_state : public xerox820_state
{
public:
	xerox820ii_state(const machine_config &mconfig, device_type type, const char *tag)
		: xerox820_state(mconfig, type, tag)
	{ }

	virtual void machine_reset();

	DECLARE_WRITE8_MEMBER( bell_w );
	DECLARE_WRITE8_MEMBER( slden_w );
	DECLARE_WRITE8_MEMBER( chrom_w );
	DECLARE_WRITE8_MEMBER( lowlite_w );
	DECLARE_WRITE8_MEMBER( sync_w );
	DECLARE_WRITE8_MEMBER( kbpio_pa_w );

	void bankswitch(int bank);
};

#endif
