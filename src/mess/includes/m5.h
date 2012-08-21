#ifndef __M5__
#define __M5__

#include "machine/z80ctc.h"

#define Z80_TAG			"ic17"
#define Z80CTC_TAG		"ic19"
#define SN76489AN_TAG	"ic15"
#define TMS9918A_TAG	"ic10"
#define TMS9929A_TAG	"ic10"
#define I8255A_TAG		"i8255a"
#define Z80_FD5_TAG		"z80fd5"
#define UPD765_TAG		"upd765"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"

class m5_state : public driver_device
{
public:
	m5_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, Z80_TAG),
		  m_fd5cpu(*this, Z80_FD5_TAG),
		  m_ctc(*this, Z80CTC_TAG),
		  m_ppi(*this, I8255A_TAG),
		  m_fdc(*this, UPD765_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_fd5cpu;
	required_device<z80ctc_device> m_ctc;
	required_device<i8255_device> m_ppi;
	required_device<device_t> m_fdc;
	required_device<cassette_image_device> m_cassette;
	required_device<centronics_device> m_centronics;
	required_device<ram_device> m_ram;
	required_device<device_t> m_floppy0;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_READ8_MEMBER( sts_r );
	DECLARE_WRITE8_MEMBER( com_w );

	DECLARE_READ8_MEMBER( ppi_pa_r );
	DECLARE_WRITE8_MEMBER( ppi_pa_w );
	DECLARE_WRITE8_MEMBER( ppi_pb_w );
	DECLARE_READ8_MEMBER( ppi_pc_r );
	DECLARE_WRITE8_MEMBER( ppi_pc_w );

	DECLARE_READ8_MEMBER( fd5_data_r );
	DECLARE_WRITE8_MEMBER( fd5_data_w );
	DECLARE_READ8_MEMBER( fd5_com_r );
	DECLARE_WRITE8_MEMBER( fd5_com_w );
	DECLARE_WRITE8_MEMBER( fd5_ctrl_w );
	DECLARE_WRITE8_MEMBER( fd5_tc_w );

	// video state
//  const TMS9928a_interface *m_vdp_intf;

	// floppy state
	UINT8 m_fd5_data;
	UINT8 m_fd5_com;
	int m_intra;
	int m_ibfa;
	int m_obfa;
	DECLARE_DRIVER_INIT(pal);
	DECLARE_DRIVER_INIT(ntsc);
};

#endif
