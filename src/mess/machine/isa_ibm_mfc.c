/***************************************************************************

  ISA 8 bit IBM PC Music Feature Card

  TODO:
   - YM-2164
   - MIDI
   - IRQ/base address selection

  Notes:
   - Some software does not function correctly at higher CPU speeds
  (e.g. the Sierra games and Yamaha Compose/PlayRec)

***************************************************************************/


#include "emu.h"
#include "isa_ibm_mfc.h"
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "sound/speaker.h"


//-------------------------------------------------
//  Constants
//-------------------------------------------------

#define TCR_TAC					0x01
#define TCR_TBC					0x02
#define TCR_TAE					0x04
#define TCR_TBE					0x08
#define TCR_EXT8				0x10
#define TCR_TMSK				0x40
#define TCR_IBE					0x80

#define TSR_TAS					0x01
#define TSR_TBS					0x02
#define TSR_TCS					0x80

enum
{
	PC_IRQ_TIMERA,
	PC_IRQ_TIMERB,
	PC_IRQ_RXRDY,
	PC_IRQ_TXRDY,
};

enum
{
	Z80_IRQ_YM,
	Z80_IRQ_RXRDY,
	Z80_IRQ_TXRDY,
	Z80_IRQ_MIDI_RXRDY,
	Z80_IRQ_MIDI_TXRDY,
};


//-------------------------------------------------
//  Globals
//-------------------------------------------------

const device_type ISA8_IBM_MFC = &device_creator<isa8_ibm_mfc_device>;


//-------------------------------------------------
//  Interrupt handling
//-------------------------------------------------

void isa8_ibm_mfc_device::set_pc_interrupt(int src, int state)
{
	if (state)
		m_pc_irq_state |= 1 << src;
	else
		m_pc_irq_state &= ~(1 << src);

	update_pc_interrupts();
}

void isa8_ibm_mfc_device::update_pc_interrupts(void)
{
	// IRQs enabled?
	if (m_tcr & TCR_IBE)
	{
		// IRQs unmasked?
		if (m_tcr & TCR_TMSK)
		{
			m_isa->irq3_w(m_pc_irq_state ? ASSERT_LINE : CLEAR_LINE);
		}
	}
}

void isa8_ibm_mfc_device::set_z80_interrupt(int src, int state)
{
	if (state)
		m_z80_irq_state |= 1 << src;
	else
		m_z80_irq_state &= ~(1 << src);

	update_z80_interrupts();
}

void isa8_ibm_mfc_device::update_z80_interrupts(void)
{
	m_cpu->set_input_line(0, m_z80_irq_state ? ASSERT_LINE : CLEAR_LINE);
}


//-------------------------------------------------
//  Z80 memory map
//-------------------------------------------------

static ADDRESS_MAP_START( prg_map, AS_PROGRAM, 8, isa8_ibm_mfc_device )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8000) AM_RAM // Unknown - tested on startup
	AM_RANGE(0xbfff, 0xbfff) AM_RAM // Unknown - tested on startup
	AM_RANGE(0xc000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, AS_IO, 8, isa8_ibm_mfc_device )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x01) AM_DEVREADWRITE_LEGACY("ym2151", ym2151_r, ym2151_w)
	AM_RANGE(0x10, 0x10) AM_DEVREADWRITE("d71051", i8251_device, data_r, data_w)
	AM_RANGE(0x11, 0x11) AM_DEVREADWRITE("d71051", i8251_device, status_r, control_w)
	AM_RANGE(0x20, 0x23) AM_DEVREADWRITE("d71055c_1", i8255_device, read, write)
ADDRESS_MAP_END


//-------------------------------------------------
//  Jumpers and DIP switches
//-------------------------------------------------

static INPUT_PORTS_START( ibm_mfc )
	PORT_START("J1")
	PORT_DIPNAME( 0x07, 0x03, "IBM MFC J1: IRQ" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )

	PORT_START("SW1")
	PORT_DIPNAME( 0x03, 0x00, "IBM MFC SW1: Base Address" )
	PORT_DIPSETTING(	0x00, "2A00" )
	PORT_DIPSETTING(	0x01, "2A10" )
	PORT_DIPSETTING(	0x02, "2A20" )
	PORT_DIPSETTING(	0x03, "2A30" )
INPUT_PORTS_END

//-------------------------------------------------
//  D71055C PPI (PC)
//-------------------------------------------------

READ8_MEMBER( isa8_ibm_mfc_device::ppi0_i_a )
{
	// Read data from the Z80 PIU
	return m_d71055c_1->pa_r();
}

WRITE8_MEMBER( isa8_ibm_mfc_device::ppi0_o_b )
{
	// Write data to the Z80 PIU - no action required
}

WRITE8_MEMBER( isa8_ibm_mfc_device::ppi0_o_c )
{
	// PC Port B /OBF (C1) -> Z80 Port B /STB (C2)
	m_d71055c_1->pc2_w(BIT(data, 1));

	// PC Port A IBF (C5) -> Z80 Port A /ACK (C6)
#if 0 // TODO
	m_d71055c_1->pc6_w(!BIT(data, 5));
#else
	if (!BIT(data, 5) && BIT(m_pc_ppi_c, 5))
		m_d71055c_1->pc6_w(0);
#endif

	// Bit 0 (INTRB) is TxRDY
	set_pc_interrupt(PC_IRQ_TXRDY, BIT(data, 0));

	// Bit 3 (INTRA) is RxRDY
	set_pc_interrupt(PC_IRQ_RXRDY, BIT(data, 3));

	m_pc_ppi_c = data;
}

READ8_MEMBER( isa8_ibm_mfc_device::ppi0_i_c )
{
	// Receive data bit 8
	return BIT(m_z80_ppi_c, 5) << 7;
}

static I8255_INTERFACE( d71055c_0_intf )
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi0_i_a),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi0_o_b),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi0_i_c),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi0_o_c),
};


//-------------------------------------------------
//  D71055C PPI (Z80)
//-------------------------------------------------

WRITE8_MEMBER( isa8_ibm_mfc_device::ppi1_o_a )
{
	// Write data to the PC PIU - no action required
}

READ8_MEMBER( isa8_ibm_mfc_device::ppi1_i_b )
{
	// Read data from the PC PIU
	return m_d71055c_0->pb_r();
}

WRITE8_MEMBER( isa8_ibm_mfc_device::ppi1_o_c )
{
	// PortA /OBF (C7) -> PortA /STB (C2)
	m_d71055c_0->pc4_w(BIT(data, 7));

	// PortB IBF (C1) -> PortB /ACK (C2)
#if 0 // TODO
	m_d71055c_0->pc2_w(!BIT(data, 1));
#else
	if (!BIT(data, 1) && BIT(m_z80_ppi_c, 1))
		m_d71055c_0->pc2_w(0);
#endif

	set_z80_interrupt(Z80_IRQ_TXRDY, BIT(data, 3));
	set_z80_interrupt(Z80_IRQ_RXRDY, BIT(data, 0));

	m_z80_ppi_c = data;
}

static I8255_INTERFACE( d71055c_1_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi1_o_a),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi1_i_b),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ppi1_o_c),
};


//-------------------------------------------------
//  D8253 PIT
//-------------------------------------------------

WRITE_LINE_MEMBER( isa8_ibm_mfc_device::d8253_clk0_out )
{
	if (m_tcr & TCR_TAE)
		set_pc_interrupt(PC_IRQ_TIMERA, 1);
}

WRITE_LINE_MEMBER( isa8_ibm_mfc_device::d8253_clk1_out )
{
	if (m_tcr & TCR_TBE)
		set_pc_interrupt(PC_IRQ_TIMERB, 1);
}

WRITE_LINE_MEMBER( isa8_ibm_mfc_device::d8253_clk2_out )
{
	pit8253_clk1_w(m_d8253, state);
}

static const struct pit8253_config d8253_intf =
{
	{
		{
			XTAL_4MHz / 8,
			DEVCB_NULL,
			DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, d8253_clk0_out)
		},
		{
			0,
			DEVCB_NULL,
			DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, d8253_clk1_out)
		},
		{
			XTAL_4MHz / 2,
			DEVCB_NULL,
			DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, d8253_clk2_out)
		}
	}
};


//-------------------------------------------------
//  uPD71051 USART
//-------------------------------------------------

READ_LINE_MEMBER( isa8_ibm_mfc_device::d70151_dsr_r )
{
	return (m_tcr & TCR_EXT8) ? 1 : 0;
}

void isa8_ibm_mfc_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	m_d71051->transmit_clock();
	m_d71051->receive_clock();
}

static const i8251_interface d71051_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, d70151_dsr_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  YM-2164
//-------------------------------------------------


WRITE_LINE_MEMBER(isa8_ibm_mfc_device::ibm_mfc_ym_irq)
{
	set_z80_interrupt(Z80_IRQ_YM, state);
}

static const ym2151_interface ibm_mfc_ym2151_interface =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, isa8_ibm_mfc_device, ibm_mfc_ym_irq),
	DEVCB_NULL
};


//-------------------------------------------------
//  Machine config
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( ibm_mfc )
	MCFG_CPU_ADD("ibm_mfc", Z80, XTAL_11_8MHz / 2)
	MCFG_CPU_PROGRAM_MAP(prg_map)
	MCFG_CPU_IO_MAP(io_map)

	MCFG_I8255_ADD("d71055c_0", d71055c_0_intf)
	MCFG_I8255_ADD("d71055c_1", d71055c_1_intf)
	MCFG_I8251_ADD("d71051", d71051_intf)
	MCFG_PIT8253_ADD("d8253", d8253_intf)

	MCFG_SPEAKER_STANDARD_STEREO("ymleft", "ymright")
	MCFG_SOUND_ADD("ym2151", YM2151, XTAL_4MHz)
	MCFG_SOUND_CONFIG(ibm_mfc_ym2151_interface)
	MCFG_SOUND_ROUTE(0, "ymleft", 1.00)
	MCFG_SOUND_ROUTE(1, "ymright", 1.00)
MACHINE_CONFIG_END


//-------------------------------------------------
//  ISA interface
//-------------------------------------------------

READ8_MEMBER( isa8_ibm_mfc_device::ibm_mfc_r )
{
	UINT8 val = 0xff;

	switch (offset)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		{
			val = m_d71055c_0->read(space, offset);
			break;
		}

		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
		{
			val = (m_pc_irq_state ? 0x80 : 0) | (m_pc_irq_state & 3);
			break;
		}

		default:
		{
			fatalerror("Unhandled IBM MFC read from %d\n", offset);
		}
	}

	return val;
}

WRITE8_MEMBER( isa8_ibm_mfc_device::ibm_mfc_w )
{
	switch (offset)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		{
			machine().scheduler().boost_interleave(attotime::zero, attotime::from_usec(1000));
			m_d71055c_0->write(space, offset, data);
			break;
		}

		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		{
			pit8253_w(m_d8253, offset & 3, data);
			break;
		}

		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
		{
			m_tcr = data;

			if (~m_tcr & TCR_TAC)
				set_pc_interrupt(PC_IRQ_TIMERA, 0);

			if (~m_tcr & TCR_TBC)
				set_pc_interrupt(PC_IRQ_TIMERB, 0);

			break;
		}

		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
		{
			// TSR is read-only but Yamaha software attempts to write to it
			break;
		}
	}
}


//-------------------------------------------------
//  ROM definition
//-------------------------------------------------

ROM_START( ibm_mfc )
	ROM_REGION( 0x8000, "ibm_mfc", 0 )
	ROM_LOAD( "xc215 c 0.bin", 0x0000, 0x8000, CRC(28c58a4f) SHA1(e7edf28d20e6c146e3144526c89cd6beea64663b) )
ROM_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_ibm_mfc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ibm_mfc );
}


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor isa8_ibm_mfc_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( ibm_mfc );
}


//-------------------------------------------------
//  rom_region - return a pointer to the device's
//  internal ROM region
//-------------------------------------------------

const rom_entry *isa8_ibm_mfc_device::device_rom_region() const
{
	return ROM_NAME( ibm_mfc );
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_ibm_mfc_device - constructor
//-------------------------------------------------

isa8_ibm_mfc_device::isa8_ibm_mfc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8_IBM_MFC, "IBM PC Music Feature Card", tag, owner, clock),
		device_isa8_card_interface(mconfig, *this),
		m_cpu(*this, "ibm_mfc"),
		m_ym2151(*this, "ym2151"),
		m_d8253(*this, "d8253"),
		m_d71051(*this, "d71051"),
		m_d71055c_0(*this, "d71055c_0"),
		m_d71055c_1(*this, "d71055c_1")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_ibm_mfc_device::device_start()
{
	set_isa_device();
	m_isa->install_device(0x2a20, 0x2a20 + 15, 0, 0, read8_delegate(FUNC(isa8_ibm_mfc_device::ibm_mfc_r), this), write8_delegate(FUNC(isa8_ibm_mfc_device::ibm_mfc_w), this));

	// Create a 500KHz timer for the USART
	m_serial_timer = timer_alloc();
	m_serial_timer->adjust(attotime::zero, 0, attotime::from_hz(XTAL_4MHz / 8));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_ibm_mfc_device::device_reset()
{
	m_tcr = 0;
	m_pc_irq_state = 0;
	m_z80_irq_state = 0;
}
