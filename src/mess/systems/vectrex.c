#include "driver.h"
#include "vidhrdw/vector.h"
#include "mess/machine/6522via.h"

/* From machine/vectrex.c */
extern unsigned char *vectrex_ram;
extern int vectrex_ram_r (int offset);
extern void vectrex_ram_w (int offset, int data);
extern int vectrex_load_rom (void);
extern int vectrex_id_rom (const char *name, const char *gamename);

/* From vidhrdw/vectrex.c */
extern int vectrex_start(void);
extern void vectrex_stop (void);
extern void vectrex_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void vectrex_vh_update (struct osd_bitmap *bitmap, int full_refresh);

extern int spectrum1_start(void);
extern void spectrum1_led_w (int offset, int data);
extern void spectrum1_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void spectrum1_vh_update (struct osd_bitmap *bitmap, int full_refresh);

/*****************************************************************

GCE Vectrex

*****************************************************************/

static struct MemoryReadAddress vectrex_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc800, 0xcbff, vectrex_ram_r},
	{ 0xcc00, 0xcfff, vectrex_ram_r},
	{ 0xd000, 0xd7ff, via_0_r },    /* VIA 6522 */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress vectrex_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc800, 0xcbff, vectrex_ram_w, &vectrex_ram },
	{ 0xcc00, 0xcfff, vectrex_ram_w },
	{ 0xd000, 0xd7ff, via_0_w },    /* VIA 6522 */
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }
};

INPUT_PORTS_START( vectrex_input_ports )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "3D Imager", IP_KEY_NONE )
	PORT_DIPSETTING(0x01, DEF_STR ( On ))
	PORT_DIPSETTING(0x00, DEF_STR ( Off ))
	PORT_DIPNAME( 0x02, 0x00, "Separate images", IP_KEY_NONE )
	PORT_DIPSETTING(0x02, DEF_STR ( Yes ))
	PORT_DIPSETTING(0x00, DEF_STR ( No ))
	PORT_DIPNAME( 0x1c, 0x10, "Left eye", IP_KEY_NONE )
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x04, "Red")
	PORT_DIPSETTING(0x08, "Green")
	PORT_DIPSETTING(0x0c, "Blue")
	PORT_DIPSETTING(0x10, "Color")
	PORT_DIPNAME( 0xe0, 0x80, "Right eye", IP_KEY_NONE )
	PORT_DIPSETTING(0x00, "Black")
	PORT_DIPSETTING(0x20, "Red")
	PORT_DIPSETTING(0x40, "Green")
	PORT_DIPSETTING(0x60, "Blue")
	PORT_DIPSETTING(0x80, "Color")

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Timer 2 refresh", IP_KEY_NONE )
	PORT_DIPSETTING(0x01, DEF_STR ( Yes ))
	PORT_DIPSETTING(0x00, DEF_STR ( No ))
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
	{ 0, 0, &fakelayout, 0, 256 },
	{ -1 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz */
	{ 50 },
    AY8910_DEFAULT_GAIN,
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
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,
	0,

	/* video hardware */
	380, 480, { 0, 500, 0, 600 },
	gfxdecodeinfo,
	254, 0,
	vectrex_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	vectrex_start,
	vectrex_stop,
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
 			&dac_interface
		}
	}

};

ROM_START(vectrex_rom)
	ROM_REGION(0x10000)
	ROM_LOAD("system.img", 0xe000, 0x2000, 0xba13fb57)
ROM_END


struct GameDriver vectrex_driver =
{
	__FILE__,
	0,
	"vectrex",
	"GCE Vectrex",
	"1982",
	"GCE",
	"Mathis Rosenhauer\n"
	"Christopher Salomon (technical advice)\n"
	VECTOR_TEAM,
	0,
	&vectrex_machine_driver,
	0,
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

	vectrex_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0,
};

/*****************************************************************

  RA+A Spectrum I+

  The Spectrum I+ was a modified Vectrex. It had a 32K ROM cart
  and 2K additional battery backed RAM (0x8000 - 0x87ff). PB6
  was used to signal inserted coins to the VIA. The unit was
  controlled by 8 buttons (2x4 buttons of controller 1 and 2).
  Each button had a LED which were mapped to 0xa000.
  The srvice mode can be accessed by pressing button
  8 during startup. As soon as all LEDs light up,
  press 2 and 3 without releasing 8. Then release 8 and
  after that 2 and 3. You can leave the screen where you enter
  ads by pressing 8 several times.

*****************************************************************/

static struct MemoryReadAddress spectrum1_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM }, /* Battery backed RAM for the Spectrum I+ */
	{ 0xc800, 0xcbff, vectrex_ram_r },
	{ 0xcc00, 0xcfff, vectrex_ram_r },
	{ 0xd000, 0xd7ff, via_0_r },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress spectrum1_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, spectrum1_led_w },
	{ 0xc800, 0xcbff, vectrex_ram_w, &vectrex_ram },
	{ 0xcc00, 0xcfff, vectrex_ram_w },
	{ 0xd000, 0xd7ff, via_0_w },
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }
};

INPUT_PORTS_START( spectrum1_input_ports )
	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON8 )

	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SELECT1 )

INPUT_PORTS_END

static struct MachineDriver raaspec_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,	/* 1.5 Mhz */
			0,
			spectrum1_readmem, spectrum1_writemem,0,0,
			0, 0, /* no vblank interrupt */
			0, 0 /* no interrupts */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,
	0,

	/* video hardware */
	380, 480, { 0, 500, 0, 600 },
	gfxdecodeinfo,
	254, 0,
	spectrum1_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	spectrum1_start,
	vectrex_stop,
	spectrum1_vh_update,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
 			&dac_interface
		}
	}

};

ROM_START(spectrum1_rom)
	ROM_REGION(0x10000)
	ROM_LOAD("spectrum.bin", 0x0000, 0x8000, 0x20af7f3f)
	ROM_LOAD("system.img", 0xe000, 0x2000, 0xba13fb57)
ROM_END


struct GameDriver raaspec_driver =
{
	__FILE__,
	0,
	"raaspec",
	"RA+A Spectrum I+",
	"1984",
	"Roy Abel & Associates",
	"Mathis Rosenhauer\n"
	"Bruce Tomlin (hardware info)\n"
	VECTOR_TEAM,
	0,
	&raaspec_machine_driver,
	0,

	spectrum1_rom,
	0,
	0,
	1,	/* number of ROM slots */
	0,	/* number of floppy drives supported */
	0,	/* number of hard drives supported */
	0,	/* number of cassette drives supported */
	0, 0,
	0,
	0,	/* sound_prom */

	spectrum1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0,
};
