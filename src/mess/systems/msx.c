/***************************************************************************

  msx.c

  Driver file to handle emulation of the MSX1.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ay8910.h"
#include "mess/vidhrdw/tms9928a.h"
#include "mess/machine/msx.h"
#include "mess/sndhrdw/scc.h"

/* vidhrdw/msx.c */
extern int msx_vh_start(void);
extern void msx_vh_stop(void);
extern void msx_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern MSX msx1;

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x1fff, MRA_BANK1 },
    { 0x2000, 0x3fff, MRA_BANK2 },
    { 0x4000, 0x5fff, MRA_BANK3 },
    { 0x6000, 0x7fff, MRA_BANK4 },
    { 0x8000, 0x9fff, MRA_BANK5 },
    { 0xa000, 0xbfff, MRA_BANK6 },
    { 0xc000, 0xdfff, MRA_BANK7 },
    { 0xe000, 0xffff, MRA_BANK8 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x3fff, msx_writemem0 },
    { 0x4000, 0x7fff, msx_writemem1 },
    { 0x8000, 0xbfff, msx_writemem2 },
    { 0xc000, 0xffff, msx_writemem3 },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0xa0, 0xa7, msx_psg_r },
	{ 0xa8, 0xab, msx_ppi_r },
	{ 0x98, 0x99, msx_vdp_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0xa0, 0xa7, msx_psg_w },
	{ 0xa8, 0xab, msx_ppi_w },
	{ 0x98, 0x99, msx_vdp_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( msx )
 PORT_START /* 0 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "0", KEYCODE_0, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "1", KEYCODE_1, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "2", KEYCODE_2, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "3", KEYCODE_3, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "4", KEYCODE_4, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "5", KEYCODE_5, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "6", KEYCODE_6, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "7", KEYCODE_7, IP_JOY_DEFAULT)

 PORT_START /* 1 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "8", KEYCODE_8, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "9", KEYCODE_9, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "-", KEYCODE_MINUS, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "=", KEYCODE_EQUALS, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "\\", KEYCODE_SLASH, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "[", KEYCODE_OPENBRACE, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "]", KEYCODE_CLOSEBRACE, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, ";", KEYCODE_COLON, IP_JOY_DEFAULT)

 PORT_START /* 2 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "'", KEYCODE_QUOTE, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "`", KEYCODE_TILDE, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, ",", KEYCODE_COMMA, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, ".", KEYCODE_STOP, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "/", KEYCODE_SLASH, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "Dead Key", KEYCODE_NONE, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "A", KEYCODE_A, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "B", KEYCODE_B, IP_JOY_DEFAULT)

 PORT_START /* 3 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "C", KEYCODE_C, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "D", KEYCODE_D, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "E", KEYCODE_E, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "F", KEYCODE_F, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "G", KEYCODE_G, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "H", KEYCODE_H, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "I", KEYCODE_I, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "J", KEYCODE_J, IP_JOY_DEFAULT)

 PORT_START /* 4 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "K", KEYCODE_K, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "L", KEYCODE_L, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "M", KEYCODE_M, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "N", KEYCODE_N, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "O", KEYCODE_O, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "P", KEYCODE_P, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "Q", KEYCODE_Q, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "R", KEYCODE_R, IP_JOY_DEFAULT)

 PORT_START /* 5 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "S", KEYCODE_S, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "T", KEYCODE_T, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "U", KEYCODE_U, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "V", KEYCODE_V, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "W", KEYCODE_W, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "X", KEYCODE_X, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "Y", KEYCODE_Y, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "Z", KEYCODE_Z, IP_JOY_DEFAULT)

 PORT_START /* 6 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SHIFT", KEYCODE_LSHIFT, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "CTRL", KEYCODE_LCONTROL, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "GRAPH", KEYCODE_PGUP, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "CAPS", KEYCODE_CAPSLOCK, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "CODE/KANA", KEYCODE_DOWN, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "F1", KEYCODE_F1, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "F2", KEYCODE_F2, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "F3", KEYCODE_F3, IP_JOY_DEFAULT)

 PORT_START /* 7 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "F4", KEYCODE_F4, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "F5", KEYCODE_F5, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "ESC", KEYCODE_ESC, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "TAB", KEYCODE_TAB, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "STOP", KEYCODE_RCONTROL, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "SELECT", KEYCODE_END, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "ENTER", KEYCODE_ENTER, IP_JOY_DEFAULT)

 PORT_START /* 8 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_UNKNOWN, "SPACE", KEYCODE_SPACE, IP_JOY_DEFAULT)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_UNKNOWN, "HOME", KEYCODE_HOME, IP_JOY_DEFAULT)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_UNKNOWN, "INSERT", KEYCODE_INSERT, IP_JOY_DEFAULT)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_UNKNOWN, "DEL", KEYCODE_DEL, IP_JOY_DEFAULT)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_UNKNOWN, "LEFT", KEYCODE_LEFT, IP_JOY_DEFAULT)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_UNKNOWN, "UP", KEYCODE_UP, IP_JOY_DEFAULT)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "DOWN", KEYCODE_DOWN, IP_JOY_DEFAULT)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "RIGHT", KEYCODE_RIGHT, IP_JOY_DEFAULT)

 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON2)
  PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN)

 PORT_START /* 10 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
  PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN)

INPUT_PORTS_END

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};
extern int msx_psg_read_port_a (int offset);

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789773,	/* 1.7897725 MHz */
	{ 10, 10 },
	/*AY8910_DEFAULT_GAIN,*/
	{ msx_psg_port_a_r },
	{ msx_psg_port_b_r },
	{ msx_psg_port_a_w },
	{ msx_psg_port_b_w }
};

static struct CustomSound_interface scc_custom_interface =
{
	SCC_sh_start,
	0, 0
};

static int msx_interrupt(void)
{
	TMS9928A_interrupt();
	return 0;
}

static struct MachineDriver machine_driver_msx =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3579545,	/* 3.579545 Mhz */
			readmem,writemem,readport,writeport,
			TMS9928A_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	msx_ch_reset, /* init_machine */
	msx_ch_stop, /* stop_machine */

	/* video hardware */
	32*8, 24*8, { 0*8, 32*8-1, 0*8, 24*8-1 },
	gfxdecodeinfo,
	TMS9928A_PALETTE_SIZE,TMS9928A_COLORTABLE_SIZE,
	tms9928A_init_palette,

	VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER,
	0,
	msx_vh_start,
	msx_vh_stop,
	msx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_CUSTOM,
			&scc_custom_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (msx)
	ROM_REGIONX (0x10000, REGION_CPU1)
	ROM_LOAD ("msx.rom", 0x0000, 0x8000, 0x94ee12f3)
ROM_END

static const struct IODevice io_msx[] = {
	{
		IO_CARTSLOT,		/* type */
		MSX_MAX_CARTS,		/* count */
		"???\0",			/* file extensions */
        NULL,               /* private */
		msx_id_rom, 		/* id */
		msx_load_rom,		/* init */
		NULL,				/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};


/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1983, msx,	  0, 		msx,	  msx,		msx,	  "ASCII & Microsoft",  "MSX1" )
