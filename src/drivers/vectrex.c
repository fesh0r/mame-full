/***************************************************************************
From Keith Wilkins internals.txt

System Memory Map
~~~~~~~~~~~~~~~~~
 
To save chips/money/space the vectrex memory is a little convoluted but
only uses 2 chips, one conatining four 2 input NAND gates and the other
containing four 2 input OR gates. Its a most admirable job considering
the number of gates used.
 
 
    0000-7fff   Cartridge ROM Space, the CART line is also gated with
                ~E to produce a signal that can be fed directly to the
                output enable of a ROM. This address range is not gated
                with the read-write line but this line is fed to the
                cartridge connector. (r/w)
 
    8000-C7FF   Unmapped space.
 
    C800-CFFF   Vectrex RAM Space 1Kx8, shadowed twice. (r/w)
 
    D000-D7FF   6522PIA shadowed 128 times (r/w)

    D800-DFFF   Dont use this area. Both the 6522 and RAM are selected
                in any reads/writes to this area.
 
    E000-FFFF   System ROM Space 8Kx8 (r/w)
 
6522A
~~~~~
 
This device is used to control all of the vectrex peripheral devices, such
as the Keypads, Vector generator, DAC, Sound chip, etc.
 
The A port is used as a BUS and goes directly to the DAC input and sound
chip input pins D0-D7. The DAC will output whatever value is on this
port.
 
The B Port is used in the following way:
 
    PB0 - SWITCH    Switch Control, enables/disables the analog multiplexer,
                    see Vector drawing hardware description
 
    PB1 - SEL0      Controls multiplexer channel select, see section on
    PB2 - SEL1      Vector drawing.
 
    PB3 - BC1       Chip Select Signal for the AY-3-8192 Sound Chip
    PB4 - BDIR      Read/Write Signal for the AY-3-8192 Sound Chip

    PB5 - COMPARE   Feedback from the OP-AMP that does the comparison
                    for calculation of analog joystick positions.
 
    PB6 - ???       This line is fed to the cartridge connector. It is
                    likely that this line was to have been used as a ROM
                    bank select for cartridges greater than 32K.
 
    PB7 - ~RAMP     This line controls part of the vector drawing
                    hardware, see later. It is an active LOW signal.
 
The 6522PIA Has a number of control lines that are sometimes used as
handshake lines as the can generate interrupts, but on the vectrex they
are used to control the vector drawing hardware
 
    CA1 - IO7       This line is connected to the IO7 line of the AY-3-8192
                    sound chip. My guess it is used to generate an
                    interrupt to the CPU. See the AY-3-8192 description
                    for more details on IO7.
 
    CA2 - ~ZERO     Connected to the integrators that form part of the
                    vector drawing hardware. This line will cause them
                    to be zero'd (both X and Y) and has the effect of
                    bringing the beam back to the centre of the CRT. It
                    is an active LOW signal. See Vector hardware section
                    for more info.
 
    CB1 - Not Connected
 
    CB2 - ~BLANK    This Active LOW signal is the BEAM ON/OFF signal
                    to the Vector drawing hardware, and is used to hide
                    the beam when it is being positioned for re-draw.
                    See Vector hardware section for more info.
 
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/vectrex.h"
#include "machine/vect_via.h"

/* From machine/vectrex.c */ 
int vectrex_load_rom (void);
int vectrex_id_rom (const char *name, const char *gamename);

static struct MemoryReadAddress vectrex_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd7ff, ReadVia },    /* VIA 6522 */
	{ 0xe000, 0xefff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress vectrex_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc800, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, WriteVia },    /* VIA 6522 */
	{ 0xe000, 0xefff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH,  IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,  IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,  IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,  IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,  IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,  IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END

static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0,      &fakelayout,     0, 256 },
	{ -1 } /* end of array */
};


static unsigned char vectrex_color_prom[]    = { VEC_PAL_BW };

static struct DACinterface vectrex_dac_interface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz */
	{ 0x20ff },
	{ input_port_0_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver vectrex_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,	/* 1.5 Mhz */
			0,
			vectrex_readmem, vectrex_writemem,0,0,
			0, 0, /* no vblank interrupt */
			0, 0 /* no interrupts */
		}
	},
	30, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,
	0,

	/* video hardware */
	525, 610, { 0, 525, 0, 600 },
	gfxdecodeinfo,
	256, 256,
	vectrex_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	start_vectrex,
	stop_vectrex,
	vectrex_vh_update,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
 			&vectrex_dac_interface
		}
	}

};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(vectrex_rom)
	ROM_REGION(0x10000)
	ROM_LOAD("system.img", 0xe000, 0x2000, 0x118cb796)
ROM_END


struct GameDriver vectrex_driver =
{
	__FILE__,
	0,
	"vectrex",
	"GCE Vectrex",
	"1982",
	"GCE",
	"Mathis Rosenhauer (Mess driver)\n"
	"James Fidell (original VIA code)\n"
	"Christopher Salomon (technical advice)\n"
	VECTOR_TEAM,
	0,
	&vectrex_machine_driver,

	vectrex_rom,
	vectrex_load_rom,
	vectrex_id_rom,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	vectrex_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0,
	0, 0
};








