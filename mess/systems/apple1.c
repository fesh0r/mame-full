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


A memory map of an Apple-1 system would be as follows:
$0000-$00FF:	Zero page: location of single or double byte values used by programs
$0024-$002B:	Zero page locations used by the Monitor
$0100-$01FF:	Stack: used by the 6502 processor as a temporary holding place for addresses or data
$0200-$027F:	Keyboard input buffer storage used by the Monitor
$0280-$0FFF:	RAM space available for a program in a 4K system
$1000-$1FFF:	RAM space available for a program in an 8K system not using cassette BASIC
$C028:			Port for output to cassette
$C100-$C1FF:	ROM program used to operate the cassette interface
$D010:  		Port where a byte of keyboard input appears
$D011:  		Port to indicate that "return" key on keyboard was pressed
$D012:  		Port to output a byte character to the display monitor
$D013:  		Port to cause display to skip down to the next line
$E000-$EFFF:	RAM space available for a program in an 8K system modified to use cassette BASIC
$F000-$FFFF:	PROM (programmable read-only memory) used by the Apple Monitor program

**********************************************************************/
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"
#include "includes/apple1.h"
#include "devices/snapquik.h"

/* port i/o functions */

/* memory w/r functions */

static ADDRESS_MAP_START( apple1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xcfff) AM_NOP
	AM_RANGE(0xd000, 0xd00f) AM_NOP
	AM_RANGE(0xd010, 0xd013) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xd014, 0xfeff) AM_NOP
	AM_RANGE(0xff00, 0xffff) AM_ROM
ADDRESS_MAP_END

/* graphics output */

struct GfxLayout apple1_charlayout =
{
	7, 8,
	128,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo apple1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &apple1_charlayout, 0, 1},
	{ -1 }
};


static unsigned char apple1_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0x00, 0xff, 0x00	/* Green */
};

static unsigned short apple1_colortable[] =
{
	0, 1
};

static PALETTE_INIT( apple1 )
{
	palette_set_colors(0, apple1_palette, sizeof(apple1_palette) / 3);
	memcpy(colortable, apple1_colortable, sizeof (apple1_colortable));
}

/* keyboard input */

INPUT_PORTS_START( apple1 )
	PORT_START	/* 0: first sixteen keys */
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_START	/* 1: second sixteen keys */
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "#", KEYCODE_TILDE, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_START	/* 2: third sixteen keys */
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
	PORT_BITX( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_START	/* 3: fourth sixteen keys */
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Escape", KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Shift", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Clear", KEYCODE_F2, IP_JOY_NONE )
	PORT_START	/* 4: Machine config */
	PORT_DIPNAME( 0x01, 0, "RAM Size")
	PORT_DIPSETTING(0, "8Kb")
	PORT_DIPSETTING(1, "52Kb")
INPUT_PORTS_END

/* sound output */

/* machine definition */
static MACHINE_DRIVER_START( apple1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 960000)        /* 1.023 Mhz */
	MDRV_CPU_PROGRAM_MAP(apple1_map, 0)
	MDRV_CPU_VBLANK_INT(apple1_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40 * 7, 24 * 8)
	MDRV_VISIBLE_AREA(0, 40 * 7 - 1, 0, 24 * 8 - 1)
	MDRV_GFXDECODE(apple1_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof (apple1_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(apple1_colortable)/sizeof(unsigned short))
	MDRV_PALETTE_INIT(apple1)

	MDRV_VIDEO_START(apple1)
	MDRV_VIDEO_UPDATE(apple1)
MACHINE_DRIVER_END


ROM_START(apple1)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("apple1.rom", 0xff00, 0x0100, CRC(a30b6af5))
	ROM_REGION(0x0400, REGION_GFX1,0)
	ROM_LOAD("apple1.vid", 0x0000, 0x0400, CRC(a3f2d66f))
ROM_END

SYSTEM_CONFIG_START(apple1)
	CONFIG_DEVICE_SNAPSHOT("snp\0", apple1)
	CONFIG_RAM_DEFAULT	(0x2000)
	CONFIG_RAM			(0xD000)
SYSTEM_CONFIG_END

/*    YEAR	NAME	PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG	COMPANY				FULLNAME */
COMP( 1976,	apple1,	0,		0,		apple1,		apple1,		apple1,	apple1,	"Apple Computer",	"Apple I" )
