/*******************************************************************************

DAI driver by Krzysztof Strzecha and Nathan Woods

What's new:
-----------
05.09.2003      Rundom number generator added. Few video hardware bugs fixed.
		Fixed few i8080 instructions, making much more BASIC games playable.

Notes on emulation status and to do list:
-----------------------------------------
1. A lot to do. Too much to list.

DAI technical information
==========================

CPU:
----
	8080 2MHz


Memory map:
-----------
	0000-bfff RAM
	c000-dfff ROM (non-switchable)
	e000-efff ROM (4 switchable banks)
	f000-f7ff ROM extension (optional)
	f800-f8ff SRAM (stack)
	f900-ffff I/O
		f900-faff spare
		fb00-fbff AMD9511 math chip (optional)
		fc00-fcff 8253 programmable interval timer
		fd00-fdff discrete devices
		fe00-feff 8255 PIO (DCE bus)
		ff00-ffff timer + 5501 interrupt controller 

Interrupts:
-----------


Keyboard:
---------


Video:
-----


Sound:
------


Timings:
--------


*******************************************************************************/

#include "driver.h"
#include "inputx.h"
#include "cpu/i8085/i8085.h"
#include "vidhrdw/generic.h"
#include "includes/dai.h"
#include "includes/pit8253.h"
#include "machine/8255ppi.h"
#include "machine/tms5501.h"
#include "devices/cassette.h"

/* I/O ports */
PORT_READ_START( dai_readport )
PORT_END

PORT_WRITE_START( dai_writeport )
PORT_END

/* memory w/r functions */
MEMORY_READ_START( dai_readmem )
	{ 0x0000, 0xbfff, MRA8_BANK1 },
	{ 0xc000, 0xdfff, MRA8_ROM },
	{ 0xe000, 0xefff, MRA8_BANK2 },
	{ 0xf000, 0xf7ff, MRA8_NOP },
	{ 0xf800, 0xf8ff, MRA8_RAM },
	{ 0xfb00, 0xfbff, amd9511_r },
	{ 0xfc00, 0xfcff, pit8253_0_r },
	{ 0xfd00, 0xfdff, dai_io_discrete_devices_r },
	{ 0xfe00, 0xfeff, ppi8255_0_r },
	{ 0xff00, 0xffff, tms5501_0_r },
MEMORY_END

MEMORY_WRITE_START( dai_writemem )
	{ 0x0000, 0xbfff, MWA8_BANK1},
	{ 0xc000, 0xdfff, MWA8_ROM},
	{ 0xe000, 0xefff, MWA8_BANK2},
	{ 0xf000, 0xf7ff, MWA8_NOP},
	{ 0xf800, 0xf8ff, MWA8_RAM},
	{ 0xfb00, 0xfbff, amd9511_w },
	{ 0xfc00, 0xfcff, pit8253_0_w },
	{ 0xfd00, 0xfdff, dai_io_discrete_devices_w },
	{ 0xfe00, 0xfeff, ppi8255_0_w },
	{ 0xff00, 0xffff, tms5501_0_w },
MEMORY_END


/* keyboard input */
INPUT_PORTS_START (dai)
	PORT_START /* [0] - port ff07 bit 0 */
		PORT_KEY1(0x01, IP_ACTIVE_HIGH, "0",		KEYCODE_0,		CODE_NONE,	'0')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "8  (",		KEYCODE_8,		CODE_NONE,	'8',	'(')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "Return",	KEYCODE_ENTER,		CODE_NONE,	13)
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "H  h",		KEYCODE_H,		CODE_NONE,	'h',	'H')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "P  p",		KEYCODE_P,		CODE_NONE,	'p',	'P')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "X  x",		KEYCODE_X,		CODE_NONE,	'x',	'X')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Up",		KEYCODE_UP,		CODE_NONE,	UCHAR_MAMEKEY(UP))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [1] - port ff07 bit 1 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "1  !",		KEYCODE_1,		CODE_NONE,	'1',	'!')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "9  )",		KEYCODE_9,		CODE_NONE,	'9',	')')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "A  a",		KEYCODE_A,		CODE_NONE,	'a',	'A')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "I  i",		KEYCODE_I,		CODE_NONE,	'i',	'I')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "Q  q",		KEYCODE_Q,		CODE_NONE,	'q',	'Q')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "Y  y",		KEYCODE_Y,		CODE_NONE,	'y',	'Y')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Down",		KEYCODE_DOWN,		CODE_NONE,	UCHAR_MAMEKEY(DOWN))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [2] - port ff07 bit 2 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "2  \"",	KEYCODE_2,		CODE_NONE,	'2',	'\"')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ":  *",		KEYCODE_MINUS,		CODE_NONE,	':',	'*')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "B  b",		KEYCODE_B,		CODE_NONE,	'b',	'B')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "J  j",		KEYCODE_J,		CODE_NONE,	'j',	'J')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "R  r",		KEYCODE_R,		CODE_NONE,	'r',	'R')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "Z  z",		KEYCODE_Z,		CODE_NONE,	'z',	'Z')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Left",		KEYCODE_LEFT,		CODE_NONE,	UCHAR_MAMEKEY(LEFT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [3] - port ff07 bit 3 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "3  #",		KEYCODE_3,		CODE_NONE,	'3',	'#')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ";  +",		KEYCODE_COLON,		CODE_NONE,	';',	'+')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "C  c",		KEYCODE_C,		CODE_NONE,	'c',	'C')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "K  k",		KEYCODE_K,		CODE_NONE,	'k',	'K')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "S  s",		KEYCODE_S,		CODE_NONE,	's',	'S')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "[  ]",		KEYCODE_QUOTE,		CODE_NONE,	'[',	']')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Right",	KEYCODE_RIGHT,		CODE_NONE,	UCHAR_MAMEKEY(RIGHT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [4] - port ff07 bit 4 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "4  $",		KEYCODE_4,		CODE_NONE,	'4',	'$')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ",  <",		KEYCODE_COMMA,		CODE_NONE,	',',	'<')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "D  d",		KEYCODE_D,		CODE_NONE,	'd',	'D')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "L  l",		KEYCODE_L,		CODE_NONE,	'l',	'L')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "T  t",		KEYCODE_T,		CODE_NONE,	't',	'T')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "^  ~",		KEYCODE_OPENBRACE,	CODE_NONE,	'^',	'~')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Tab",		KEYCODE_TAB,		CODE_NONE,	9)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [5] - port ff07 bit 5 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "5  %",		KEYCODE_5,		CODE_NONE,	'5',	'%')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "-  =",		KEYCODE_EQUALS,		CODE_NONE,	'-',	'=')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "E  e",		KEYCODE_E,		CODE_NONE,	'e',	'E')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "M  m",		KEYCODE_M,		CODE_NONE,	'm',	'M')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "U  u",		KEYCODE_U,		CODE_NONE,	'u',	'U')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Space",	KEYCODE_SPACE,		CODE_NONE,	' ')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Ctrl",		KEYCODE_LCONTROL,	CODE_NONE,	UCHAR_MAMEKEY(LCONTROL))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [6] - port ff07 bit 6 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "6  &",		KEYCODE_6,		CODE_NONE,	'6',	'&')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ".  >",		KEYCODE_STOP,		CODE_NONE,	'.',	'>')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "F  f",		KEYCODE_F,		CODE_NONE,	'f',	'F')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "N  n",		KEYCODE_N,		CODE_NONE,	'n',	'N')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "V  v",		KEYCODE_V,		CODE_NONE,	'v',	'V')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Rept",		KEYCODE_INSERT,		CODE_NONE,	UCHAR_MAMEKEY(INSERT))
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Break",	KEYCODE_ESC,		CODE_NONE,	27)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [7] - port ff07 bit 7 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "7  '",		KEYCODE_7,		CODE_NONE,	'7',	'\'')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "/  ?",		KEYCODE_SLASH,		CODE_NONE,	'/',	'?')
		PORT_KEY2(0x04, IP_ACTIVE_HIGH, "G  g",		KEYCODE_G,		CODE_NONE,	'g',	'G')
		PORT_KEY2(0x08, IP_ACTIVE_HIGH, "O  o",		KEYCODE_O,		CODE_NONE,	'o',	'O')
		PORT_KEY2(0x10, IP_ACTIVE_HIGH, "W  w",		KEYCODE_W,		CODE_NONE,	'w',	'W')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Char del",	KEYCODE_DEL,		CODE_NONE,	UCHAR_MAMEKEY(DEL))
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Shift",	KEYCODE_LSHIFT,		CODE_NONE,	UCHAR_SHIFT_1)
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Shift",	KEYCODE_RSHIFT,		CODE_NONE,	UCHAR_SHIFT_2)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [8] */
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)
		PORT_BIT(0xcb, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static struct Wave_interface dai_wave_interface = {
	1,		/* 1 cassette recorder */
	{ 50 }		/* mixing levels in percent */
};

/* machine definition */
static MACHINE_DRIVER_START( dai )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_MEMORY(dai_readmem, dai_writemem)
	MDRV_CPU_PORTS(dai_readport, dai_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( dai )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1056, 542)
	MDRV_VISIBLE_AREA(0, 1056-1, 0, 302-1)
	MDRV_PALETTE_LENGTH(sizeof (dai_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (dai_colortable))
	MDRV_PALETTE_INIT( dai )

	MDRV_VIDEO_START( dai )
	MDRV_VIDEO_UPDATE( dai )

	/* sound hardware */
	MDRV_SOUND_ADD(WAVE, dai_wave_interface)
	MDRV_SOUND_ADD(CUSTOM, dai_sound_interface)
MACHINE_DRIVER_END

#define io_dai		io_NULL

ROM_START(dai)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("dai.bin", 0xc000, 0x2000, CRC(ca71a7d5))
	ROM_LOAD("dai00.bin", 0x10000, 0x1000, CRC(fa7d39ac))
	ROM_LOAD("dai01.bin", 0x11000, 0x1000, CRC(cb5809f2))
	ROM_LOAD("dai02.bin", 0x12000, 0x1000, CRC(03f72d4a))
	ROM_LOAD("dai03.bin", 0x13000, 0x1000, CRC(c475c96f))
	ROM_REGION(0x2000, REGION_GFX1,0)
	ROM_LOAD ("nch.bin", 0x0000, 0x1000, CRC(a9f5b30b))
ROM_END


SYSTEM_CONFIG_START(dai)
	CONFIG_RAM_DEFAULT(48 * 1024)
	CONFIG_DEVICE_CASSETTE(1, NULL)
SYSTEM_CONFIG_END


/*     YEAR  NAME	PARENT  COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY	FULLNAME */
COMP( 1978, dai,	0,      0,		dai,	dai,	0,		dai,	"Data Applications International",	"DAI Personal Computer")
