/**********************************************************************
Apple I Memory map

	CPU: 6502 @ .960Mhz

		0000-1FFF	RAM
		2000-D00F	NOP
		D010-D013	PIA6820
		D014-FEFF	NOP
		FF00-FFFF	ROM

Interrupts:	None.

Video:		1K x 7 shift registers

Sound:		None

Hardware:	PIA6820 DSP for keyboard and screen interface

		d010	KEYBOARD DDR	Returns 7 bit ascii key
		d011	KEYBOARD CTR	Bit 7 high signals available key
		d012	DISPLAY DDR	Output to screen, set bit 7 of d013
		d013	DISPLAY CTR	Bit 7 low signals display ready
**********************************************************************/
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"

/* prototypes */

extern	void	apple1_init_machine (void);
extern	void	apple1_stop_machine (void);

extern	int		apple1_rom_load (void);
extern	int		apple1_rom_id (const char *name, const char *gamename);

extern	int		apple1_vh_start (void);
extern	void	apple1_vh_stop (void);
extern	void	apple1_vh_screenrefresh (struct osd_bitmap *bitmap,
							int full_refresh);
extern	int		apple1_interrupt (void);
extern	void	init_apple1 (void);

/* functions */

/* port i/o functions */

/* memory w/r functions */

static struct MemoryReadAddress apple1_readmem[] =
{
	{0x0000, 0x1fff, MRA_RAM},
	{0x2000, 0xd00f, MRA_NOP},
	{0xd010, 0xd013, pia_0_r},
	{0xd014, 0xfeff, MRA_NOP},
	{0xff00, 0xffff, MRA_ROM},
	{-1}
};

static struct MemoryWriteAddress apple1_writemem[] =
{
	{0x0000, 0x1fff, MWA_RAM},
	{0x2000, 0xd00f, MWA_NOP},
	{0xd010, 0xd013, pia_0_w},
	{0xd014, 0xfeff, MWA_NOP},
	{0xff00, 0xffff, MWA_ROM},
	{-1}
};

/* graphics output */

struct GfxLayout apple1_charlayout =
{
	6, 8,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo apple1_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &apple1_charlayout, 0, 2},
	{ -1 }
};

static unsigned char apple1_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0x00, 0xff, 0x00
};

static unsigned short apple1_colortable[2 * 2] =
{
	1, 0,
	0, 1
};

static void apple1_init_palette (unsigned char *sys_palette,
					unsigned short *sys_colortable,
					const unsigned char *color_prom)
{
	memcpy (sys_palette, apple1_palette, sizeof (apple1_palette));
	memcpy (sys_colortable, apple1_colortable, sizeof (apple1_colortable));
}

/* keyboard input */

INPUT_PORTS_START( apple1 )
	PORT_START	/* first sixteen keys */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE )
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_START	/* second sixteen keys */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD, "#", KEYCODE_TILDE, IP_JOY_NONE )
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_START	/* third sixteen keys */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0x0080, 0x0000, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX( 0x0100, 0x0000, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0x0200, 0x0000, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0x0400, 0x0000, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX( 0x0800, 0x0000, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX( 0x1000, 0x0000, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX( 0x2000, 0x0000, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0x4000, 0x0000, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0x8000, 0x0000, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_START	/* fourth sixteen keys */
	PORT_BITX( 0x0001, 0x0000, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x0002, 0x0000, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x0004, 0x0000, IPT_KEYBOARD, "Escape", KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX( 0x0008, 0x0000, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0010, 0x0000, IPT_KEYBOARD, "Shift", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0020, 0x0000, IPT_KEYBOARD, "Reset", KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX( 0x0040, 0x0000, IPT_KEYBOARD, "Clear", KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

/* sound output */

/* machine definition */

static struct MachineDriver machine_driver_apple1 =
{
	{
		{
			CPU_M6502,
			960000,
			apple1_readmem, apple1_writemem,
			0, 0,
			apple1_interrupt, 1,
		},
	},
	50, 2500,
	1,
	apple1_init_machine,
	apple1_stop_machine,
	40 * 6,
	24 * 8,
	{ 0, 40 * 6 - 1, 0, 24 * 8 - 1 },
	apple1_gfxdecodeinfo,
	2, 4,
	apple1_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	apple1_vh_start,
	apple1_vh_stop,
	apple1_vh_screenrefresh,
	0, 0, 0, 0,
};

ROM_START(apple1)
	ROM_REGIONX(0x10000, REGION_CPU1)
	ROM_LOAD("apple1.rom", 0xff00, 0x0100, 0xa30b6af5)
	ROM_REGIONX(0x0400, REGION_GFX1)
	ROM_LOAD("apple1.chr", 0x0000, 0x0400, 0xbe70bb85)
ROM_END

static	const	struct	IODevice io_apple1[] = {
	{ IO_END }
};

/*    YEAR	NAME	PARENT	MACHINE	INPUT	INIT	COMPANY				FULLNAME */
COMP( 1976,	apple1,	0,		apple1,	apple1,	apple1,	"Apple Computers",	"Apple 1 8k" )
