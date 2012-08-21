/***************************************************************************

        Heathkit H89

        12/05/2009 Skeleton driver.

    Monitor Commands:
    B Boot
    C Convert (number)
    G Go (address)
    I In (address)
    O Out (address,data)
    R Radix (H/O)
    S Substitute (address)
    T Test Memory
    V View

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ins8250.h"
#include "machine/terminal.h"


class h89_state : public driver_device
{
public:
	h89_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_uart(*this, "ins8250"),
		  m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<ins8250_device> m_uart;
	required_device<serial_terminal_device> m_terminal;

	DECLARE_WRITE8_MEMBER( port_f2_w );

	UINT8 m_port_f2;
};


static ADDRESS_MAP_START(h89_mem, AS_PROGRAM, 8, h89_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( h89_io, AS_IO, 8, h89_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//  AM_RANGE(0x78, 0x7b)    expansion 1
//  AM_RANGE(0x7c, 0x7f)    expansion 2
//  AM_RANGE(0xd0, 0xd7)    8250 UART DCE
//  AM_RANGE(0xd8, 0xdf)    8250 UART DTE
//  AM_RANGE(0xe0, 0xe7)    8250 UART LP
	AM_RANGE(0xe8, 0xef)	AM_DEVREADWRITE("ins8250", ins8250_device, ins8250_r, ins8250_w) // 8250 UART console
//  AM_RANGE(0xf0, 0xf1)    front panel
	AM_RANGE(0xf2, 0xf2)	AM_WRITE(port_f2_w) AM_READ_PORT("SW501")
//  AM_RANGE(0xf8, 0xf9)    cassette
//  AM_RANGE(0xfa, 0xff)    serial I/O
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( h89 )
	PORT_START("SW501")
	PORT_DIPNAME( 0x03, 0x00, "Expansion 1" )  PORT_DIPLOCATION("S1:1,S1:2")
	PORT_DIPSETTING( 0x00, "H-88-1" )
	PORT_DIPSETTING( 0x01, "H/Z-47" )
	PORT_DIPSETTING( 0x02, "Z-67" )
	PORT_DIPSETTING( 0x03, "Undefined" )
	PORT_DIPNAME( 0x0c, 0x00, "Expansion 2" )  PORT_DIPLOCATION("S1:3,S1:4")
	PORT_DIPSETTING( 0x00, "H-89-37" )
	PORT_DIPSETTING( 0x04, "H/Z-47" )
	PORT_DIPSETTING( 0x08, "Z-67" )
	PORT_DIPSETTING( 0x0c, "Undefined" )
	PORT_DIPNAME( 0x10, 0x00, "Boot from" )  PORT_DIPLOCATION("S1:5")
	PORT_DIPSETTING( 0x00, "Expansion 1" )
	PORT_DIPSETTING( 0x10, "Expansion 2" )
	PORT_DIPNAME( 0x20, 0x20, "Perform memory test at start" )  PORT_DIPLOCATION("S1:6")
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Console Baud rate" )  PORT_DIPLOCATION("S1:7")
	PORT_DIPSETTING( 0x00, "9600" )
	PORT_DIPSETTING( 0x40, "19200" )
	PORT_DIPNAME( 0x80, 0x00, "Boot mode" )  PORT_DIPLOCATION("S1:8")
	PORT_DIPSETTING( 0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING( 0x80, "Auto" )
INPUT_PORTS_END


static MACHINE_RESET(h89)
{
}

static TIMER_DEVICE_CALLBACK( h89_irq_timer )
{
	h89_state *state = timer.machine().driver_data<h89_state>();

	if (state->m_port_f2 & 0x02)
		device_set_input_line_and_vector(state->m_maincpu, 0, HOLD_LINE, 0xcf);
}

WRITE8_MEMBER( h89_state::port_f2_w )
{
	m_port_f2 = data;
}

static const serial_terminal_interface terminal_intf =
{
	DEVCB_DEVICE_LINE_MEMBER("ins8250", ins8250_uart_device, rx_w)
};

static const ins8250_interface h89_ins8250_interface =
{
	DEVCB_DEVICE_LINE_MEMBER(TERMINAL_TAG, serial_terminal_device, rx_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_CONFIG_START( h89, h89_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_12_288MHz / 6)
	MCFG_CPU_PROGRAM_MAP(h89_mem)
	MCFG_CPU_IO_MAP(h89_io)

	MCFG_MACHINE_RESET(h89)

#if 0
	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE_STATIC(h89)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
#endif

	MCFG_INS8250_ADD( "ins8250", h89_ins8250_interface, XTAL_1_8432MHz )

	MCFG_SERIAL_TERMINAL_ADD(TERMINAL_TAG, terminal_intf, 9600)

	MCFG_TIMER_ADD_PERIODIC("irq_timer", h89_irq_timer, attotime::from_hz(100))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( h89 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "2732_444-142_mtr90.rom", 0x0000, 0x1000, CRC(c4ff47c5) SHA1(d6f3d71ff270a663003ec18a3ed1fa49f627123a))
	ROM_LOAD( "2716_444-19_h17.rom", 0x1800, 0x0800, CRC(26e80ae3) SHA1(0c0ee95d7cb1a760f924769e10c0db1678f2435c))

	ROM_REGION( 0x10000, "otherroms", ROMREGION_ERASEFF )
	ROM_LOAD( "2732_444-84_mtr84.rom", 0x0000, 0x1000, CRC(c98e5f4c) SHA1(03347206dca145ff69ca08435db822b70ce106af))
	ROM_LOAD( "2732_mms84a_magnoliamms.bin", 0x0000, 0x1000, CRC(5563f42a) SHA1(1b74cafca8213d5c083f16d8a848933ab56eb43b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1979, h89,    0,      0,       h89,       h89, driver_device,     0,    "Heath Inc", "Heathkit H89", GAME_NOT_WORKING | GAME_NO_SOUND)
