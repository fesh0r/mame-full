#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/pdp1.h"
/*
 * PRECISION CRT DISPLAY (TYPE 30)
 * is the only display - hardware emulated, this is needed for SPACEWAR!
 *
 * Only keys required in SPACEWAR! are emulated.
 *
 * The loading storing OS... is not emulated (I haven't a clue where to
 * get programs for the machine (ROMS!!!))
 *
 * The only supported program is SPACEWAR!
 * For Web addresses regarding PDP1 and SPACEWAR look at the
 * pdp1/pdp1.c file
 *
 */

/* From machine/pdp1.c */
int pdp1_load_rom (void);
int pdp1_id_rom (const char *name, const char *gamename);
void pdp1_init_machine(void);
int pdp1_read_mem(int offset);
void pdp1_write_mem(int offset, int data);

/* every memory handler is the same for now */

/* note: MEMORY HANDLERS used everywhere, since we don't need bytes, we
 * need 18 bit words, the handler functions return integers, so it should
 * be all right to use them.
 * This gives sometimes IO warnings!
 */
static struct MemoryReadAddress pdp1_readmem[] =
{
	{ 0x0000, 0xffff, pdp1_read_mem },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress pdp1_writemem[] =
{
	{ 0x0000, 0xffff, pdp1_write_mem },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( input_ports )

    PORT_START      /* IN0 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Spin Left Player 1", KEYCODE_A, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Spin Right Player 1", KEYCODE_S, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Thrust Player 1", KEYCODE_D, IP_JOY_DEFAULT )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Fire Player 1", KEYCODE_F, IP_JOY_DEFAULT )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Spin Left Player 2", KEYCODE_LEFT, IP_JOY_DEFAULT )
	PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Spin Right Player 2", KEYCODE_RIGHT, IP_JOY_DEFAULT )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Thrust Player 2", KEYCODE_UP, IP_JOY_DEFAULT )
	PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Fire Player 2", KEYCODE_DOWN, IP_JOY_DEFAULT )

    PORT_START /* IN1 */
	PORT_BITX(	  0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 1", KEYCODE_1, IP_JOY_NONE )
    PORT_DIPSETTING(    0x80, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(	  0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 2", KEYCODE_2, IP_JOY_NONE )
    PORT_DIPSETTING(    0x40, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(	  0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 3", KEYCODE_3, IP_JOY_NONE )
    PORT_DIPSETTING(    0x20, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(	  0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 4", KEYCODE_4, IP_JOY_NONE )
    PORT_DIPSETTING(    0x10, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(	  0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 5", KEYCODE_5, IP_JOY_NONE )
    PORT_DIPSETTING(    0x08, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX(	  0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Sense Switch 6", KEYCODE_6, IP_JOY_NONE )
    PORT_DIPSETTING(    0x04, "On" )
    PORT_DIPSETTING(    0x00, "Off" )
    INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0x55,0x55,0x55, /* DK GREY - for MAME text only */
	0x80,0x80,0x80, /* LT GREY - for MAME text only */
};

static unsigned short colortable[] =
{
	0x00, 0x01,
	0x01, 0x00,
};

/* note I don't know about the speed of the machine, I only know
 * how long each instruction takes in micro seconds
 * below speed should therefore also be read in something like
 * microseconds of instructions
 */
static struct MachineDriver pdp1_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_PDP1,
			2000000,
			0,
			pdp1_readmem, pdp1_writemem,0,0,
			0, 0 /* no vblank interrupt */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,
	pdp1_init_machine,
	0,

	/* video hardware */
	VIDEO_BITMAP_WIDTH, VIDEO_BITMAP_HEIGHT, { 0, VIDEO_BITMAP_WIDTH-1, 0, VIDEO_BITMAP_HEIGHT-1 },

	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),

	0,

	VIDEO_TYPE_RASTER,
	0,
	pdp1_vh_start,
	pdp1_vh_stop,
	pdp1_vh_update,

	/* sound hardware */
	0,0,0,0
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver pdp1_driver =
{
	__FILE__,
	0,
	"pdp1",
	"pdp1 SPACEWAR!",
	"1962",
	"?????",
	"Spacewar! was conceived in 1961 by Martin Graetz,\n"
	"Stephen Russell, and Wayne Wiitanen. It was first\n"
	"realized on the PDP-1 in 1962 by Stephen Russell,\n"
	"Peter Samson, Dan Edwards, and Martin Graetz, together\n"
	"with Alan Kotok, Steve Piner, and Robert A Saunders.\n"
	"Spacewar! is in the public domain, but this credit\n"
	"paragraph must accompany all distributed versions\n"
	"of the program.\n"
	"\n"
	"Brian Silverman (original Java Source)\n"
	"Vadim Gerasimov (original Java Source)\n"
	"Chris Salomon (MESS driver)\n",
	0,
	&pdp1_machine_driver,
	0,

	0,
	pdp1_load_rom,
	pdp1_id_rom,
	0,      /* number of ROM slots */
	0,      /* number of floppy drives supported */
	0,      /* number of hard drives supported */
	0,      /* number of cassette drives supported */
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0,
};
