/**********************************************************************
UK101 Memory map

	CPU: 6502 @ 1.0Mhz

Interrupts:	None.

Video:		Memory mapped

Sound:		None

Hardware:	ACIA

		0000-9FFF	RAM
		A000-BFFF	ROM (basic)
		C000-CFFF	NOP
		D000-D3FF	RAM (video)
		D400-DEFF	NOP
		DF00-DF00	H/W (Keyboard)
		DF01-EFFF	NOP
		F000-F001	H/W (acia)
		F002-F7FF	NOP
		F800-FFFF	ROM (monitor)
**********************************************************************/
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"

/* prototypes */

extern	void	uk101_init_machine (void);
extern	void	uk101_stop_machine (void);

extern	int		uk101_rom_load (void);
extern	int		uk101_rom_id (const char *name, const char *gamename);

extern	int		uk101_vh_start (void);
extern	void	uk101_vh_stop (void);
extern	void	uk101_vh_screenrefresh (struct osd_bitmap *bitmap,
							int full_refresh);
extern	void	superbrd_vh_screenrefresh (struct osd_bitmap *bitmap,
							int full_refresh);

/* functions */

/* port i/o functions */

static	INT8	uk101_keyb;

int	uk101_keyb_r (int offset)
{
	int	rpl = 0xff;
	if (!(uk101_keyb & 0x80))
		rpl &= readinputport (0);
	if (!(uk101_keyb & 0x40))
		rpl &= readinputport (1);
	if (!(uk101_keyb & 0x20))
		rpl &= readinputport (2);
	if (!(uk101_keyb & 0x10))
		rpl &= readinputport (3);
	if (!(uk101_keyb & 0x08))
		rpl &= readinputport (4);
	if (!(uk101_keyb & 0x04))
		rpl &= readinputport (5);
	if (!(uk101_keyb & 0x02))
		rpl &= readinputport (6);
	if (!(uk101_keyb & 0x01))
		rpl &= readinputport (7);
	return (rpl);
}

void uk101_keyb_w (int offset, int data)
{
	uk101_keyb = data;
}

/* memory w/r functions */

static struct MemoryReadAddress uk101_readmem[] =
{
	{0x0000, 0x9fff, MRA_RAM},
	{0xa000, 0xbfff, MRA_ROM},
	{0xc000, 0xcfff, MRA_NOP},
	{0xd000, 0xd3ff, videoram_r},
	{0xd400, 0xdeff, MRA_NOP},
	{0xdf00, 0xdf00, uk101_keyb_r},
	{0xdf01, 0xefff, MRA_NOP},
	{0xf000, 0xf001, MRA_NOP},
	{0xf002, 0xf7ff, MRA_NOP},
	{0xf800, 0xffff, MRA_ROM},
	{-1}
};

static struct MemoryWriteAddress uk101_writemem[] =
{
	{0x0000, 0x9fff, MWA_RAM},
	{0xa000, 0xbfff, MWA_ROM},
	{0xc000, 0xcfff, MWA_NOP},
	{0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size},
	{0xd400, 0xdeff, MWA_NOP},
	{0xdf00, 0xdf00, uk101_keyb_w},
	{0xdf01, 0xefff, MWA_NOP},
	{0xf000, 0xf001, MWA_NOP},
	{0xf002, 0xf7ff, MWA_NOP},
	{0xf800, 0xffff, MWA_ROM},
	{-1}
};

/* graphics output */

struct GfxLayout uk101_charlayout =
{
	8, 16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8},
	8 * 8
};

static struct	GfxDecodeInfo uk101_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &uk101_charlayout, 0, 2},
	{ -1 }
};

static unsigned char uk101_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff
};

static unsigned short uk101_colortable[2 * 2] =
{
	1,0, 0,1
};

static void uk101_init_palette (unsigned char *sys_palette,
					unsigned short *sys_colortable,
					const unsigned char *color_prom)
{
	memcpy (sys_palette, uk101_palette, sizeof (uk101_palette));
	memcpy (sys_colortable, uk101_colortable, sizeof (uk101_colortable));
}

/* keyboard input */

INPUT_PORTS_START( uk101 )
	PORT_START	/* DF00 & 0x80 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
	PORT_START /* DF00 & 0x40 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_START /* DF00 & 0x20 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_START /* DF00 & 0x10 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
	PORT_START /* DF00 & 0x08 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
	PORT_START /* DF00 & 0x04 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
	PORT_START /* DF00 & 0x02 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )
	PORT_START /* DF00 & 0x01 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right Shift", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left Shift", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Escape", KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "~", KEYCODE_TILDE, IP_JOY_NONE )
INPUT_PORTS_END

/* sound output */

/* machine definition */

static struct MachineDriver machine_driver_uk101 =
{
	{
		{
			CPU_M6502,
			1000000,
			uk101_readmem, uk101_writemem,
			0, 0,
			interrupt, 1,
		},
	},
	50, 2500,
	1,
	uk101_init_machine,
	uk101_stop_machine,
	64 * 8,
	16 * 16,
	{ 0, 64 * 8 - 1, 0, 16 * 16 - 1 },
	uk101_gfxdecodeinfo,
	2, 4,
	uk101_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	uk101_vh_start,
	uk101_vh_stop,
	uk101_vh_screenrefresh,
	0, 0, 0, 0,
};

static struct MachineDriver machine_driver_superbrd =
{
	{
		{
			CPU_M6502,
			1000000,
			uk101_readmem, uk101_writemem,
			0, 0,
			interrupt, 1,
		},
	},
	50, 2500,
	1,
	uk101_init_machine,
	uk101_stop_machine,
	32 * 8,
	16 * 16,
	{ 0, 32 * 8 - 1, 0, 16 * 16 - 1 },
	uk101_gfxdecodeinfo,
	2, 4,
	uk101_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	uk101_vh_start,
	uk101_vh_stop,
	superbrd_vh_screenrefresh,
	0, 0, 0, 0,
};

ROM_START(uk101)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("basuk01.rom", 0xa000, 0x0800, 0x9d3caa92)
	ROM_LOAD("basuk02.rom", 0xa800, 0x0800, 0x0039ef6a)
	ROM_LOAD("basuk03.rom", 0xb000, 0x0800, 0x0d011242)
	ROM_LOAD("basuk04.rom", 0xb800, 0x0800, 0x667223e8)
	ROM_LOAD("monuk02.rom", 0xf800, 0x0800, 0xe5b7028d)
	ROM_REGION(0x800, REGION_GFX1)
	ROM_LOAD("chguk101.rom", 0x0000, 0x0800, 0xfce2c84a)
ROM_END

ROM_START(superbrd)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("basus01.rom", 0xa000, 0x0800, 0xf4f5dec0)
	ROM_LOAD("basus02.rom", 0xa800, 0x0800, 0x0039ef6a)
	ROM_LOAD("basus03.rom", 0xb000, 0x0800, 0xca25f8c1)
	ROM_LOAD("basus04.rom", 0xb800, 0x0800, 0x8ee6030e)
	ROM_LOAD("monus02.rom", 0xf800, 0x0800, 0x04ac5822)
	ROM_REGION(0x800, REGION_GFX1)
	ROM_LOAD("chguk101.rom", 0x0000, 0x0800, 0xfce2c84a)
ROM_END

static	const	struct	IODevice io_uk101[] = {
	{ IO_END }
};

static	const	struct	IODevice io_superbrd[] = {
	{ IO_END }
};

/*    YEAR	NAME		PARENT	MACHINE		INPUT	INIT	COMPANY				FULLNAME */
COMP( 1979,	uk101,		0,		uk101,		uk101,	0,		"Compukit",			"Compukit UK101" )
COMP( 1979, superbrd,	uk101,	superbrd,	uk101,	0,		"Ohio Scientific",	"Superboard" )

