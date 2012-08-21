/***************************************************************************

    P8000

    09/2009 Skeleton driver based on Matt Knoth's emulator.
    P8000emu (http://knothusa.net/Home.php) has been a great source of info.
    Other info from
      * http://www.pofo.de/P8000/notes/books/
      * http://www.pofo.de/P8000/

    P8000 memory layout
      * divided into 3 banks of 64k
      * bank A is for roms, only 0000-1FFF is populated
      * bank B is for static ram, only 2000-2FFF exists
      * bank C is for dynamic ram, all 64k is available.
      * selection is done with OUT(c), code
      * code = 0 for do nothing; 1 = bank A; 2 = bank B; 4 = bank C.
      * Reg C = 0; Reg B = start address of memory that is being switched,
        for example B = 20 indicates "bank2" in memory map, and also the
        corresponding address in bank A/B/C.

    P8000 monitor commands
      * B : ?
      * D : display and modify memory
      * F : fill memory
      * G : go to
      * M : move (copy) memory
      * N : dump registers
      * O : boot from floppy
      * P : ?
      * Q : ?
      * R : dump registers
      * S : boot from floppy
      * T : jump to ROM at CEF0
      * X : jump to ROM at DB00
      * return : boot from floppy disk

    P8000_16 : All input must be in uppercase.

    TODO:
      * properly implement Z80 daisy chain in 16 bit board
      * Find out how to enter hardware check on 16 bit board
      * hook the sio back up when it becomes usable


****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z8000/z8000.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "machine/upd765.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/z80dma.h"
#include "sound/beep.h"
#include "machine/terminal.h"


class p8k_state : public driver_device
{
public:
	p8k_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG) { }

	DECLARE_READ8_MEMBER(p8k_port0_r);
	DECLARE_WRITE8_MEMBER(p8k_port0_w);
	DECLARE_READ8_MEMBER(p8k_port24_r);
	DECLARE_WRITE8_MEMBER(p8k_port24_w);
	DECLARE_READ16_MEMBER(portff82_r);
	DECLARE_WRITE16_MEMBER(portff82_w);
	DECLARE_WRITE8_MEMBER(kbd_put);
	DECLARE_WRITE8_MEMBER(kbd_put_16);
	UINT8 m_term_data;
	required_device<cpu_device> m_maincpu;
	required_device<generic_terminal_device> m_terminal;
	DECLARE_DRIVER_INIT(p8k);
};

/***************************************************************************

    P8000 8bit

****************************************************************************/

static ADDRESS_MAP_START(p8k_memmap, AS_PROGRAM, 8, p8k_state)
	AM_RANGE(0x0000, 0x0FFF) AM_RAMBANK("bank0")
	AM_RANGE(0x1000, 0x1FFF) AM_RAMBANK("bank1")
	AM_RANGE(0x2000, 0x2FFF) AM_RAMBANK("bank2")
	AM_RANGE(0x3000, 0x3FFF) AM_RAMBANK("bank3")
	AM_RANGE(0x4000, 0x4FFF) AM_RAMBANK("bank4")
	AM_RANGE(0x5000, 0x5FFF) AM_RAMBANK("bank5")
	AM_RANGE(0x6000, 0x6FFF) AM_RAMBANK("bank6")
	AM_RANGE(0x7000, 0x7FFF) AM_RAMBANK("bank7")
	AM_RANGE(0x8000, 0x8FFF) AM_RAMBANK("bank8")
	AM_RANGE(0x9000, 0x9FFF) AM_RAMBANK("bank9")
	AM_RANGE(0xA000, 0xAFFF) AM_RAMBANK("bank10")
	AM_RANGE(0xB000, 0xBFFF) AM_RAMBANK("bank11")
	AM_RANGE(0xC000, 0xCFFF) AM_RAMBANK("bank12")
	AM_RANGE(0xD000, 0xDFFF) AM_RAMBANK("bank13")
	AM_RANGE(0xE000, 0xEFFF) AM_RAMBANK("bank14")
	AM_RANGE(0xF000, 0xFFFF) AM_RAMBANK("bank15")
ADDRESS_MAP_END

static ADDRESS_MAP_START(p8k_iomap, AS_IO, 8, p8k_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_READWRITE(p8k_port0_r,p8k_port0_w) // MH7489
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80ctc_0", z80ctc_device, read, write)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("z80pio_0", z80pio_device, read_alt, write_alt)
	AM_RANGE(0x18, 0x1b) AM_DEVREADWRITE("z80pio_1", z80pio_device, read_alt, write_alt)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_2", z80pio_device, read_alt, write_alt)
	AM_RANGE(0x20, 0x20) AM_DEVREADWRITE_LEGACY("i8272", upd765_data_r, upd765_data_w)
	AM_RANGE(0x21, 0x21) AM_DEVREAD_LEGACY("i8272", upd765_status_r)
	//AM_RANGE(0x24, 0x27) AM_DEVREADWRITE("z80sio_0", z80sio_device, read_alt, write_alt)
	AM_RANGE(0x24, 0x27) AM_READWRITE(p8k_port24_r,p8k_port24_w)
	AM_RANGE(0x28, 0x2b) AM_DEVREADWRITE("z80sio_1", z80sio_device, read_alt, write_alt)
	AM_RANGE(0x2c, 0x2f) AM_DEVREADWRITE("z80ctc_1", z80ctc_device, read, write)
	AM_RANGE(0x3c, 0x3c) AM_DEVREADWRITE_LEGACY("z80dma", z80dma_r, z80dma_w)
ADDRESS_MAP_END



READ8_MEMBER( p8k_state::p8k_port0_r )
{
	return 0;
}

// see memory explanation above
WRITE8_MEMBER( p8k_state::p8k_port0_w )
{
	UINT8 breg = cpu_get_reg(m_maincpu, Z80_B) >> 4;
	if ((data==1) || (data==2) || (data==4))
	{
		char banknum[8];
		sprintf(banknum,"bank%d", breg);

		offset = 0;
		if (data==2)
			offset = 16;
		else
		if (data==4)
			offset = 32;

		offset += breg;

		membank(banknum)->set_entry(offset);
	}
	else
	if (data)
		printf("Invalid data %X for bank %d\n",data,breg);
}

READ8_MEMBER( p8k_state::p8k_port24_r )
{
	if (offset == 3)
		return 0xff;
	if (offset == 2)
		return m_term_data;

	return 0;
}

WRITE8_MEMBER( p8k_state::p8k_port24_w )
{
	if (offset == 2)
		m_terminal->write(space, 0, data);
}

WRITE8_MEMBER( p8k_state::kbd_put )
{
	m_term_data = data;
	// This is a dreadful hack..
	// simulate interrupt by saving current pc on
	// the stack and jumping to interrupt handler.
	UINT16 spreg = cpu_get_reg(m_maincpu, Z80_SP);
	UINT16 pcreg = cpu_get_reg(m_maincpu, Z80_PC);
	spreg--;
	space.write_byte(spreg, pcreg >> 8);
	spreg--;
	space.write_byte(spreg, pcreg);
	cpu_set_reg(m_maincpu, Z80_SP, spreg);
	cpu_set_reg(m_maincpu, Z80_PC, 0x078A);
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(p8k_state, kbd_put)
};


/***************************************************************************

    P8000 8bit Peripherals

****************************************************************************/

static void p8k_daisy_interrupt(device_t *device, int state)
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state);
}

/* Z80 DMA */

static WRITE_LINE_DEVICE_HANDLER( p8k_dma_irq_w )
{
	if (state)
	{
		device_t *i8272 = device->machine().device("i8272");
		upd765_tc_w(i8272, state);
	}

	p8k_daisy_interrupt(device, state);
}

static UINT8 memory_read_byte(address_space *space, offs_t address) { return space->read_byte(address); }
static void memory_write_byte(address_space *space, offs_t address, UINT8 data) { space->write_byte(address, data); }

static Z80DMA_INTERFACE( p8k_dma_intf )
{
	DEVCB_LINE(p8k_dma_irq_w),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", IO, memory_write_byte)
};

/* Z80 CTC 0 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 3, FDC, System-Kanal

static Z80CTC_INTERFACE( p8k_ctc_0_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL  		/* ZC/TO2 callback */
};

/* Z80 CTC 1 */
// to implement: callbacks!
// manual states the callbacks should go to
// Baud Gen 0, Baud Gen 1, Baud Gen 2,

static Z80CTC_INTERFACE( p8k_ctc_1_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL,			/* ZC/TO2 callback */
};

/* Z80 PIO 0 */

static Z80PIO_INTERFACE( p8k_pio_0_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 1 */

static Z80PIO_INTERFACE( p8k_pio_1_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 2 */

static Z80PIO_INTERFACE( p8k_pio_2_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_INPUT_PORT("DSW"),	/* port a read */
	DEVCB_NULL,	/* port a write */
	DEVCB_NULL,	/* ready a */
	DEVCB_NULL,	/* port b read */
	DEVCB_NULL,	/* port b write */
	DEVCB_NULL	/* ready b */
};

/* Z80 SIO 0 */

static WRITE8_DEVICE_HANDLER( pk8_sio_0_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_sio_0_intf =
{
	p8k_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_sio_0_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 SIO 1 */

static WRITE8_DEVICE_HANDLER( pk8_sio_1_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_sio_1_intf =
{
	p8k_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_sio_1_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_config p8k_daisy_chain[] =
{
	{ "z80dma" },	/* FDC related */
	{ "z80pio_2" },
	{ "z80ctc_0" },
	{ "z80sio_0" },
	{ "z80sio_1" },
	{ "z80pio_0" },
	{ "z80pio_1" },
	{ "z80ctc_1" },
	{ NULL }
};

/* Intel 8272 Interface */

static WRITE_LINE_DEVICE_HANDLER( p8k_i8272_irq_w )
{
	z80pio_device *z80pio = device->machine().device<z80pio_device>("z80pio_2");

	z80pio->port_b_write((state) ? 0x10 : 0x00);
}

static const struct upd765_interface p8k_i8272_intf =
{
	DEVCB_LINE(p8k_i8272_irq_w),
	DEVCB_DEVICE_LINE("z80dma", z80dma_rdy_w),
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static const floppy_interface p8k_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	NULL,
	NULL
};

/* Input ports */
static INPUT_PORTS_START( p8k )
	PORT_START("DSW")
	PORT_BIT( 0x7f, 0x7f, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Hardware Test")
	PORT_DIPSETTING(    0x00, DEF_STR(Off))
	PORT_DIPSETTING(    0x80, DEF_STR(On))
INPUT_PORTS_END


static MACHINE_RESET( p8k )
{
	p8k_state *state = machine.driver_data<p8k_state>();
	state->membank("bank0")->set_entry(0);
	state->membank("bank1")->set_entry(0);
	state->membank("bank2")->set_entry(0);
	state->membank("bank3")->set_entry(0);
	state->membank("bank4")->set_entry(0);
	state->membank("bank5")->set_entry(0);
	state->membank("bank6")->set_entry(0);
	state->membank("bank7")->set_entry(0);
	state->membank("bank8")->set_entry(0);
	state->membank("bank9")->set_entry(0);
	state->membank("bank10")->set_entry(0);
	state->membank("bank11")->set_entry(0);
	state->membank("bank12")->set_entry(0);
	state->membank("bank13")->set_entry(0);
	state->membank("bank14")->set_entry(0);
	state->membank("bank15")->set_entry(0);
}

DRIVER_INIT_MEMBER(p8k_state,p8k)
{
	UINT8 *RAM = memregion("maincpu")->base();
	membank("bank0")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank1")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank2")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank3")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank4")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank5")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank6")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank7")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank8")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank9")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank10")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank11")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank12")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank13")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank14")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
	membank("bank15")->configure_entries(0, 48, &RAM[0x0000], 0x1000);
}


/***************************************************************************

    P8000 16bit

****************************************************************************/

WRITE8_MEMBER( p8k_state::kbd_put_16 )
{
	// keyboard int handler is at 0x0700
	m_term_data = data;
	// This is another dire hack..
	UINT8 offs = space.read_byte(0x43a5);
	UINT16 addr = 0x41b0 + (UINT16) offs;
	space.write_byte(addr, data);
	space.write_byte(0x43a0, 1);
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf_16 )
{
	DEVCB_DRIVER_MEMBER(p8k_state, kbd_put_16)
};




static MACHINE_RESET( p8k_16 )
{
}

// TODO: all of this needs upgrading to current standards

static READ16_DEVICE_HANDLER( p8k_16_sio_r )
{
	switch (offset & 0x06)
	{
	case 0x00:
		return (UINT16)dynamic_cast<z80sio_device*>(device)->data_read(0);
	case 0x02:
		return (UINT16)dynamic_cast<z80sio_device*>(device)->data_read(1);
	case 0x04:
		return (UINT16)dynamic_cast<z80sio_device*>(device)->control_read(0);
	case 0x06:
		return (UINT16)dynamic_cast<z80sio_device*>(device)->control_read(1);
	}

	return 0;
}

static WRITE16_DEVICE_HANDLER( p8k_16_sio_w )
{
	data &= 0xff;

	switch (offset & 0x06)
	{
	case 0x00:
		dynamic_cast<z80sio_device*>(device)->data_write(0, (UINT8)data);
		break;
	case 0x02:
		dynamic_cast<z80sio_device*>(device)->data_write(1, (UINT8)data);
		break;
	case 0x04:
		dynamic_cast<z80sio_device*>(device)->control_write(0, (UINT8)data);
		break;
	case 0x06:
		dynamic_cast<z80sio_device*>(device)->control_write(1, (UINT8)data);
		break;
	}
}

static READ16_DEVICE_HANDLER( p8k_16_pio_r )
{
	return 0; //(UINT16)z80pio_r(device, (offset & 0x06) >> 1);
}

static WRITE16_DEVICE_HANDLER( p8k_16_pio_w )
{
	//z80pio_w(device, (offset & 0x06) >> 1, (UINT8)(data & 0xff));
}

static READ16_DEVICE_HANDLER( p8k_16_ctc_r )
{
	return (UINT16)downcast<z80ctc_device *>(device)->read(*device->machine().memory().first_space(),(offset & 0x06) >> 1);
}

static WRITE16_DEVICE_HANDLER( p8k_16_ctc_w )
{
	downcast<z80ctc_device *>(device)->write(*device->machine().memory().first_space(), (offset & 0x06) >> 1, (UINT8)(data & 0xff));
}

READ16_MEMBER( p8k_state::portff82_r )
{
	if (offset == 3) // FF87
		return 0xff;
	else
	if (offset == 1) // FF83
		return m_term_data;
	return 0;
}

WRITE16_MEMBER( p8k_state::portff82_w )
{
	if (offset == 1) // FF83
		m_terminal->write(space, 0, data);
}

static ADDRESS_MAP_START(p8k_16_memmap, AS_PROGRAM, 16, p8k_state)
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x07fff) AM_RAM
	AM_RANGE(0x08000, 0xfffff) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START(p8k_16_iomap, AS_IO, 16, p8k_state)
//  AM_RANGE(0x0fef0, 0x0feff) // clock
	//AM_RANGE(0x0ff80, 0x0ff87) AM_DEVREADWRITE_LEGACY("z80sio_0", p8k_16_sio_r, p8k_16_sio_w)
	AM_RANGE(0x0ff80, 0x0ff87) AM_READWRITE(portff82_r,portff82_w)
	AM_RANGE(0x0ff88, 0x0ff8f) AM_DEVREADWRITE_LEGACY("z80sio_1", p8k_16_sio_r, p8k_16_sio_w)
	AM_RANGE(0x0ff90, 0x0ff97) AM_DEVREADWRITE_LEGACY("z80pio_0", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ff98, 0x0ff9f) AM_DEVREADWRITE_LEGACY("z80pio_1", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ffa0, 0x0ffa7) AM_DEVREADWRITE_LEGACY("z80pio_2", p8k_16_pio_r, p8k_16_pio_w)
	AM_RANGE(0x0ffa8, 0x0ffaf) AM_DEVREADWRITE_LEGACY("z80ctc_0", p8k_16_ctc_r, p8k_16_ctc_w)
	AM_RANGE(0x0ffb0, 0x0ffb7) AM_DEVREADWRITE_LEGACY("z80ctc_1", p8k_16_ctc_r, p8k_16_ctc_w)
//  AM_RANGE(0x0ffc0, 0x0ffc1) // SCR
//  AM_RANGE(0x0ffc8, 0x0ffc9) // SBR
//  AM_RANGE(0x0ffd0, 0x0ffd1) // NBR
//  AM_RANGE(0x0ffd8, 0x0ffd9) // SNVR
//  AM_RANGE(0x0ffe0, 0x0ffe1) // RETI
//  AM_RANGE(0x0fff0, 0x0fff1) // TRPL
//  AM_RANGE(0x0fff8, 0x0fff9) // IF1L
ADDRESS_MAP_END


/***************************************************************************

    P8000 16bit Peripherals

****************************************************************************/

static void p8k_16_daisy_interrupt(device_t *device, int state)
{
	// this must be studied a little bit more :-)
}

/* Z80 CTC 0 */

static Z80CTC_INTERFACE( p8k_16_ctc_0_intf )
{
	DEVCB_LINE(p8k_16_daisy_interrupt),	/* interrupt handler */
	DEVCB_NULL,				/* ZC/TO0 callback */
	DEVCB_NULL,				/* ZC/TO1 callback */
	DEVCB_NULL  			/* ZC/TO2 callback */
};

/* Z80 CTC 1 */

static Z80CTC_INTERFACE( p8k_16_ctc_1_intf )
{
	DEVCB_LINE(p8k_16_daisy_interrupt),	/* interrupt handler */
	DEVCB_NULL,				/* ZC/TO0 callback */
	DEVCB_NULL,				/* ZC/TO1 callback */
	DEVCB_NULL				/* ZC/TO2 callback */
};

/* Z80 PIO 0 */

static const z80pio_interface p8k_16_pio_0_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 1 */

static const z80pio_interface p8k_16_pio_1_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 PIO 2 */

static const z80pio_interface p8k_16_pio_2_intf =
{
	DEVCB_LINE(p8k_16_daisy_interrupt),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Z80 SIO 0 */

static WRITE8_DEVICE_HANDLER( pk8_16_sio_0_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_16_sio_0_intf =
{
	p8k_16_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_16_sio_0_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 SIO 1 */

static WRITE8_DEVICE_HANDLER( pk8_16_sio_1_serial_transmit )
{
// send character to terminal
}

static const z80sio_interface p8k_16_sio_1_intf =
{
	p8k_16_daisy_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	pk8_16_sio_1_serial_transmit,	/* transmit handler */
	NULL					/* receive handler */
};

/* Z80 Daisy Chain */

static const z80_daisy_config p8k_16_daisy_chain[] =
{
	{ "z80ctc_0" },
	{ "z80ctc_1" },
	{ "z80sio_0" },
	{ "z80sio_1" },
	{ "z80pio_0" },
	{ "z80pio_1" },
	{ "z80pio_2" },
	{ NULL }
};



/* F4 Character Displayer */
static const gfx_layout p8k_charlayout =
{
	8, 12,					/* 8 x 12 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( p8k )
	GFXDECODE_ENTRY( "chargen", 0x0000, p8k_charlayout, 0, 1 )
GFXDECODE_END


/***************************************************************************

    Machine Drivers

****************************************************************************/

static MACHINE_CONFIG_START( p8k, p8k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz )
	MCFG_CPU_CONFIG(p8k_daisy_chain)
	MCFG_CPU_PROGRAM_MAP(p8k_memmap)
	MCFG_CPU_IO_MAP(p8k_iomap)
	MCFG_MACHINE_RESET(p8k)

	/* peripheral hardware */
	MCFG_Z80DMA_ADD("z80dma", XTAL_4MHz, p8k_dma_intf)
	MCFG_Z80CTC_ADD("z80ctc_0", 1229000, p8k_ctc_0_intf)	/* 1.22MHz clock */
	MCFG_Z80CTC_ADD("z80ctc_1", 1229000, p8k_ctc_1_intf)	/* 1.22MHz clock */
	MCFG_Z80SIO_ADD("z80sio_0", 9600, p8k_sio_0_intf)	/* 9.6kBaud default */
	MCFG_Z80SIO_ADD("z80sio_1", 9600, p8k_sio_1_intf)	/* 9.6kBaud default */
	MCFG_Z80PIO_ADD("z80pio_0", 1229000, p8k_pio_0_intf)
	MCFG_Z80PIO_ADD("z80pio_1", 1229000, p8k_pio_1_intf)
	MCFG_Z80PIO_ADD("z80pio_2", 1229000, p8k_pio_2_intf)
	MCFG_UPD765A_ADD("i8272", p8k_i8272_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(p8k_floppy_interface)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( p8k_16, p8k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z8001, XTAL_4MHz )
	MCFG_CPU_CONFIG(p8k_16_daisy_chain)
	MCFG_CPU_PROGRAM_MAP(p8k_16_memmap)
	MCFG_CPU_IO_MAP(p8k_16_iomap)
	MCFG_MACHINE_RESET(p8k_16)

	/* peripheral hardware */
	MCFG_Z80CTC_ADD("z80ctc_0", XTAL_4MHz, p8k_16_ctc_0_intf)
	MCFG_Z80CTC_ADD("z80ctc_1", XTAL_4MHz, p8k_16_ctc_1_intf)
	MCFG_Z80SIO_ADD("z80sio_0", 9600, p8k_16_sio_0_intf)
	MCFG_Z80SIO_ADD("z80sio_1", 9600, p8k_16_sio_1_intf)
	MCFG_Z80PIO_ADD("z80pio_0", XTAL_4MHz, p8k_16_pio_0_intf )
	MCFG_Z80PIO_ADD("z80pio_1", XTAL_4MHz, p8k_16_pio_1_intf )
	MCFG_Z80PIO_ADD("z80pio_2", XTAL_4MHz, p8k_16_pio_2_intf )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	/* video hardware */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf_16)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( p8000 )
	ROM_REGION( 0x30000, "maincpu", 0 )
	ROM_LOAD("mon8_1_3.1",	0x0000, 0x1000, CRC(ad1bb118) SHA1(2332963acd74d5d1a009d9bce8a2b108de01d2a5))
	ROM_LOAD("mon8_2_3.1",	0x1000, 0x1000, CRC(daced7c2) SHA1(f1f778e72568961b448020fc543ed6e81bbe81b1))

	// this is for the p8000's terminal, not the p8000 itself
	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
	ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

ROM_START( p8000_16 )
	ROM_REGION16_BE( 0x4000, "maincpu", 0 )
	ROM_LOAD16_BYTE("mon16_1h_3.1_udos",   0x0000, 0x1000, CRC(0c3c28da) SHA1(0cd35444c615b404ebb9cf80da788593e573ddb5))
	ROM_LOAD16_BYTE("mon16_1l_3.1_udos",   0x0001, 0x1000, CRC(e8857bdc) SHA1(f89c65cbc479101130c71806fd3ddc28e6383f12))
	ROM_LOAD16_BYTE("mon16_2h_3.1_udos",   0x2000, 0x1000, CRC(cddf58d5) SHA1(588bad8df75b99580459c7a8e898a3396907e3a4))
	ROM_LOAD16_BYTE("mon16_2l_3.1_udos",   0x2001, 0x1000, CRC(395ee7aa) SHA1(d72fadb1608cd0915cd5ce6440897303ac5a12a6))

	// this is for the p8000's terminal, not the p8000 itself
	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD("p8t_zs",    0x0000, 0x0800, CRC(f9321251) SHA1(a6a796b58d50ec4a416f2accc34bd76bc83f18ea))
	ROM_LOAD("p8tdzs.2",  0x0800, 0x0800, CRC(32736503) SHA1(6a1d7c55dddc64a7d601dfdbf917ce1afaefbb0a))
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                   FULLNAME       FLAGS */
COMP( 1989, p8000,      0,      0,       p8k,       p8k, p8k_state,     p8k,    "EAW electronic Treptow", "P8000 (8bit Board)",  GAME_NOT_WORKING)
COMP( 1989, p8000_16,   p8000,  0,       p8k_16,    p8k, driver_device,     0,      "EAW electronic Treptow", "P8000 (16bit Board)",  GAME_NOT_WORKING)
