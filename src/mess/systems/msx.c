/***************************************************************************

  msx.c

  Driver file to handle emulation of the MSX1.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ay8910.h"
#include "machine/8255ppi.h"
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
    { 0x90, 0x91, msx_printer_r },
    { 0xa0, 0xa7, msx_psg_r },
    { 0xa8, 0xab, ppi8255_0_r },
    { 0x98, 0x99, msx_vdp_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
    { 0x90, 0x91, msx_printer_w },
    { 0xa0, 0xa7, msx_psg_w },
    { 0xa8, 0xab, ppi8255_0_w },
    { 0x98, 0x99, msx_vdp_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( msx )
 PORT_START /* 0 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)

 PORT_START /* 1 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)

 PORT_START /* 2 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "`", KEYCODE_TILDE, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Dead Key", KEYCODE_NONE, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)

 PORT_START /* 3 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)

 PORT_START /* 4 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)

 PORT_START /* 5 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)

 PORT_START /* 6 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRAPH", KEYCODE_PGUP, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS", KEYCODE_CAPSLOCK, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "CODE/KANA", KEYCODE_DOWN, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3", KEYCODE_F3, IP_JOY_NONE)

 PORT_START /* 7 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5", KEYCODE_F5, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC", KEYCODE_ESC, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "STOP", KEYCODE_RCONTROL, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SELECT", KEYCODE_END, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)

 PORT_START /* 8 */
  PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
  PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOME", KEYCODE_HOME, IP_JOY_NONE)
  PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "INSERT", KEYCODE_INSERT, IP_JOY_NONE)
  PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE)
  PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE)
  PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE)
  PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE)
  PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE)

 PORT_START /* 9 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON2)
  PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_KEYBOARD)

 PORT_START /* 10 */
  PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
  PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
  PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
  PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)
  PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
  PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
  PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_KEYBOARD)

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

static struct DACinterface dac_interface =
{
    1,
    { 20 }
};

static int msx_interrupt(void)
{
    TMS9928A_interrupt();
    return ignore_interrupt();
}

static struct MachineDriver machine_driver_msx =
{
    /* basic machine hardware */
    {
	{
	    CPU_Z80,
	    3579545,	/* 3.579545 Mhz */
	    readmem,writemem,readport,writeport,
	    msx_interrupt,1
	}
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
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
            SOUND_DAC,
            &dac_interface
        },
	{
	    SOUND_CUSTOM,
	    &scc_custom_interface
	}
    }
};

#ifdef MSX_J
static struct MachineDriver machine_driver_msxj =
{
    /* basic machine hardware */
    {
	{
	    CPU_Z80,
	    3579545,	/* 3.579545 Mhz */
	    readmem,writemem,readport,writeport,
	    msx_interrupt,1
	}
    },
    60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
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
	    SOUND_DAC,
	    &dac_interface
	},
	{
	    SOUND_CUSTOM,
	    &scc_custom_interface
	}
    }
};
#endif

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (msx)
    ROM_REGION (0x10000, REGION_CPU1)
    ROM_LOAD ("msx.rom", 0x0000, 0x8000, 0x94ee12f3)
ROM_END

//ROM_START (msxj)
//    ROM_REGION (0x10000, REGION_CPU1)
//    ROM_LOAD ("msxj.rom", 0x0000, 0x8000, 0xee229390)
//ROM_END

static const struct IODevice io_msx[] = {
{
    IO_CARTSLOT,		/* type */
    MSX_MAX_CARTS,		/* count */
    "rom\0",			/* file extensions */
    NULL,               	/* private */
    msx_id_rom, 		/* id */
    msx_load_rom,		/* init */
    msx_exit_rom,		/* exit */
    NULL,                	/* info */
    NULL,               	/* open */
    NULL,               	/* close */
    NULL,               	/* status */
    NULL,               	/* seek */
    NULL,               	/* input */
    NULL,               	/* output */
    NULL,               	/* input_chunk */
    NULL       		        /* output_chunk */
},
    { IO_END }
};

#define io_msxj io_msx
#define input_ports_msxj input_ports_msx

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1983, msx, 0, msx, msx, msx, "ASCII & Microsoft", "MSX1" )
//COMP( 1983, msxj, msx, msxj, msxj, 0, "ASCII & Microsoft", "MSX1 (Japan)")
