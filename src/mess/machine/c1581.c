/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c1581.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_TAG		"u1"
#define M8520_TAG		"u5"
#define WD1770_TAG		"u4"


enum
{
	LED_POWER = 0,
	LED_ACT
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C1563 = &device_creator<c1563_device>;
const device_type C1581 = &device_creator<c1581_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void base_c1581_device::device_config_complete()
{
	switch (m_variant)
	{
	default:
	case TYPE_1581:
		m_shortname = "c1581";
		break;

	case TYPE_1563:
		m_shortname = "c1563";
		break;
	}
}


//-------------------------------------------------
//  ROM( c1581 )
//-------------------------------------------------

ROM_START( c1581 )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
    ROM_DEFAULT_BIOS("r2")
    ROM_SYSTEM_BIOS( 0, "beta", "Beta" )
	ROMX_LOAD( "beta.u2",	       0x0000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594), ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "r1", "Revision 1" )
	ROMX_LOAD( "318045-01.u2",	   0x0000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164), ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "r2", "Revision 2" )
	ROMX_LOAD( "318045-02.u2",	   0x0000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "jiffydos", "JiffyDOS v6.01" )
	ROMX_LOAD( "jiffydos 1581.u2", 0x0000, 0x8000, CRC(98873d0f) SHA1(65bbf2be7bcd5bdcbff609d6c66471ffb9d04bfe), ROM_BIOS(4) )
ROM_END


//-------------------------------------------------
//  ROM( c1563 )
//-------------------------------------------------

ROM_START( c1563 )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
	ROM_LOAD( "1563-rom.bin",     0x0000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *base_c1581_device::device_rom_region() const
{
	switch (m_variant)
	{
	default:
	case TYPE_1581:
		return ROM_NAME( c1581 );

	case TYPE_1563:
		return ROM_NAME( c1563 );
	}
}


//-------------------------------------------------
//  ADDRESS_MAP( c1581_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c1581_mem, AS_PROGRAM, 8, base_c1581_device )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE_LEGACY(M8520_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE_LEGACY(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION(M6502_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  MOS8520_INTERFACE( cia_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( base_c1581_device::cnt_w )
{
	// fast serial clock out
	m_cnt_out = state;

	set_iec_srq();
}

WRITE_LINE_MEMBER( base_c1581_device::sp_w )
{
	// fast serial data out
	m_sp_out = state;

	set_iec_data();
}

READ8_MEMBER( base_c1581_device::cia_pa_r )
{
	/*

        bit     description

        PA0
        PA1     /RDY
        PA2
        PA3     DEV# SEL (SW1)
        PA4     DEV# SEL (SW1)
        PA5
        PA6
        PA7     /DISK CHNG

    */

	UINT8 data = 0;

	// ready
	data |= !(floppy_drive_get_flag_state(m_image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 1;

	// device number
	data |= (m_address - 8) << 3;

	// disk change
	data |= floppy_dskchg_r(m_image) << 7;

	return data;
}

WRITE8_MEMBER( base_c1581_device::cia_pa_w )
{
	/*

        bit     description

        PA0     SIDE0
        PA1
        PA2     /MOTOR
        PA3
        PA4
        PA5     POWER LED
        PA6     ACT LED
        PA7

    */

	// side 0
	wd17xx_set_side(m_fdc, !BIT(data, 0));

	// motor
	int motor = BIT(data, 2);
	floppy_mon_w(m_image, motor);
	floppy_drive_set_ready_state(m_image, !motor, 1);

	// power led
	output_set_led_value(LED_POWER, BIT(data, 5));

	// activity led
	output_set_led_value(LED_ACT, BIT(data, 6));
}

READ8_MEMBER( base_c1581_device::cia_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1
        PB2     CLK IN
        PB3
        PB4
        PB5
        PB6     /WPRT
        PB7     ATN IN

    */

	UINT8 data = 0;

	// data in
	data = !m_bus->data_r();

	// clock in
	data |= !m_bus->clk_r() << 2;

	// write protect
	data |= !floppy_wpt_r(m_image) << 6;

	// attention in
	data |= !m_bus->atn_r() << 7;

	return data;
}

WRITE8_MEMBER( base_c1581_device::cia_pb_w )
{
	/*

        bit     description

        PB0
        PB1     DATA OUT
        PB2
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6
        PB7

    */

	// data out
	m_data_out = BIT(data, 1);

	// clock out
	m_bus->clk_w(this, !BIT(data, 3));

	// attention acknowledge
	m_atn_ack = BIT(data, 4);

	// fast serial direction
	m_fast_ser_dir = BIT(data, 5);

	set_iec_data();
	set_iec_srq();
}

static MOS8520_INTERFACE( cia_intf )
{
	XTAL_16MHz/8,
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, cnt_w),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, sp_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, cia_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, cia_pa_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, cia_pb_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, base_c1581_device, cia_pb_w)
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const wd17xx_interface fdc_intf =
{
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};


//-------------------------------------------------
//  LEGACY_FLOPPY_OPTIONS( c1581 )
//-------------------------------------------------

static LEGACY_FLOPPY_OPTIONS_START( c1581 )
	LEGACY_FLOPPY_OPTION( c1581, "d81", "Commodore 1581 Disk Image", d81_dsk_identify, d81_dsk_construct, NULL, NULL )
LEGACY_FLOPPY_OPTIONS_END


//-------------------------------------------------
//  floppy_interface c1581_floppy_interface
//-------------------------------------------------

static const floppy_interface c1581_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(c1581),
	"floppy_3_5",
	NULL
};


//-------------------------------------------------
//  MACHINE_DRIVER( c1581 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c1581 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1581_mem)

	MCFG_MOS8520_ADD(M8520_TAG, XTAL_16MHz/8, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ fdc_intf)

	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_interface)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor base_c1581_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c1581 );
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  base_c1581_device - constructor
//-------------------------------------------------

inline void base_c1581_device::set_iec_data()
{
	int atn = m_bus->atn_r();
	int data = !m_data_out && !(m_atn_ack && !atn);

	// fast serial data
	if (m_fast_ser_dir) data &= m_sp_out;

	m_bus->data_w(this, data);
}


//-------------------------------------------------
//  base_c1581_device - constructor
//-------------------------------------------------

inline void base_c1581_device::set_iec_srq()
{
	int srq = 1;

	// fast serial clock
	if (m_fast_ser_dir) srq &= m_cnt_out;

	m_bus->srq_w(this, srq);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  base_c1581_device - constructor
//-------------------------------------------------

base_c1581_device::base_c1581_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, UINT32 variant)
    : device_t(mconfig, type, name, tag, owner, clock),
	  device_cbm_iec_interface(mconfig, *this),
	  m_maincpu(*this, M6502_TAG),
	  m_cia(*this, M8520_TAG),
	  m_fdc(*this, WD1770_TAG),
	  m_image(*this, FLOPPY_0),
	  m_variant(variant)
{
}


//-------------------------------------------------
//  c1563_device - constructor
//-------------------------------------------------

c1563_device::c1563_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: base_c1581_device(mconfig, C1563, "C1563", tag, owner, clock, TYPE_1563) { }


//-------------------------------------------------
//  c1581_device - constructor
//-------------------------------------------------

c1581_device::c1581_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: base_c1581_device(mconfig, C1581, "C1581", tag, owner, clock, TYPE_1581) { }


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void base_c1581_device::device_start()
{
	// state saving
	save_item(NAME(m_data_out));
	save_item(NAME(m_atn_ack));
	save_item(NAME(m_fast_ser_dir));
	save_item(NAME(m_sp_out));
	save_item(NAME(m_cnt_out));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void base_c1581_device::device_reset()
{
    m_maincpu->reset();

    m_cia->reset();

	wd17xx_mr_w(m_fdc, 0);
	wd17xx_mr_w(m_fdc, 1);

	m_sp_out = 1;
	m_cnt_out = 1;
}


//-------------------------------------------------
//  cbm_iec_srq -
//-------------------------------------------------

void base_c1581_device::cbm_iec_srq(int state)
{
	if (!m_fast_ser_dir)
	{
		m_cia->cnt_w(state);
	}
}


//-------------------------------------------------
//  cbm_iec_atn -
//-------------------------------------------------

void base_c1581_device::cbm_iec_atn(int state)
{
	m_cia->flag_w(state);

	set_iec_data();
}


//-------------------------------------------------
//  cbm_iec_data -
//-------------------------------------------------

void base_c1581_device::cbm_iec_data(int state)
{
	if (!m_fast_ser_dir)
	{
		m_cia->sp_w(state);
	}
}


//-------------------------------------------------
//  cbm_iec_reset -
//-------------------------------------------------

void base_c1581_device::cbm_iec_reset(int state)
{
	if (!state)
	{
		device_reset();
	}
}
