/*******************************************************************************

DAI driver by Krzysztof Strzecha and Nathan Woods


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
		fe00-feff 8255 PIO
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

/* I/O ports */
PORT_READ_START( dai_readport )
PORT_END

PORT_WRITE_START( dai_writeport )
PORT_END

/* memory w/r functions */
MEMORY_READ_START( dai_readmem )
	{ 0x0000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_BANK2 },
	{ 0xf000, 0xf7ff, MRA_NOP },
	{ 0xf800, 0xf8ff, MRA_RAM },
	{ 0xfb00, 0xfbff, amd9511_r },
	{ 0xfc00, 0xfcff, pit8253_0_r },
	{ 0xfd00, 0xfdff, dai_io_discrete_devices_r },
	{ 0xfe00, 0xfeff, ppi8255_0_r },
	{ 0xff00, 0xffff, tms5501_0_r },
MEMORY_END

MEMORY_WRITE_START( dai_writemem )
	{ 0x0000, 0xbfff, MWA_BANK1},
	{ 0xc000, 0xdfff, MWA_ROM},
	{ 0xe000, 0xefff, MWA_BANK2},
	{ 0xf000, 0xf7ff, MWA_NOP},
	{ 0xf800, 0xf8ff, MWA_RAM},
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
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "H",		KEYCODE_H,		CODE_NONE,	'h')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "P",		KEYCODE_P,		CODE_NONE,	'p')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "X",		KEYCODE_X,		CODE_NONE,	'x')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Up",		KEYCODE_UP,		CODE_NONE,	UCHAR_MAMEKEY(UP))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [1] - port ff07 bit 1 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "1  !",		KEYCODE_1,		CODE_NONE,	'1',	'!')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "9  )",		KEYCODE_9,		CODE_NONE,	'9',	')')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "A",		KEYCODE_A,		CODE_NONE,	'a')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "I",		KEYCODE_I,		CODE_NONE,	'i')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "Q",		KEYCODE_Q,		CODE_NONE,	'q')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Y",		KEYCODE_Y,		CODE_NONE,	'y')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Down",		KEYCODE_DOWN,		CODE_NONE,	UCHAR_MAMEKEY(DOWN))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [2] - port ff07 bit 2 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "2  \"",	KEYCODE_2,		CODE_NONE,	'2',	'\"')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ":  *",		KEYCODE_MINUS,		CODE_NONE,	':',	'*')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "B",		KEYCODE_B,		CODE_NONE,	'b')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "J",		KEYCODE_J,		CODE_NONE,	'j')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "R",		KEYCODE_R,		CODE_NONE,	'r')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Z",		KEYCODE_Z,		CODE_NONE,	'z')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Left",		KEYCODE_LEFT,		CODE_NONE,	UCHAR_MAMEKEY(LEFT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [3] - port ff07 bit 3 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "3  #",		KEYCODE_3,		CODE_NONE,	'3',	'#')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, ";  +",		KEYCODE_COLON,		CODE_NONE,	';',	'+')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "C",		KEYCODE_C,		CODE_NONE,	'c')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "K",		KEYCODE_K,		CODE_NONE,	'k')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "S",		KEYCODE_S,		CODE_NONE,	's')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "[  ]",		KEYCODE_QUOTE,		CODE_NONE,	'[',	']')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Right",	KEYCODE_RIGHT,		CODE_NONE,	UCHAR_MAMEKEY(RIGHT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [4] - port ff07 bit 4 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "4  $",		KEYCODE_4,		CODE_NONE,	'4',	'$')
		PORT_KEY1(0x02, IP_ACTIVE_HIGH, ",",		KEYCODE_COMMA,		CODE_NONE,	',')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "D",		KEYCODE_D,		CODE_NONE,	'd')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "L",		KEYCODE_L,		CODE_NONE,	'l')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "T",		KEYCODE_T,		CODE_NONE,	't')
		PORT_KEY2(0x20, IP_ACTIVE_HIGH, "^  ~",		KEYCODE_OPENBRACE,	CODE_NONE,	'^',	'~')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Tab",		KEYCODE_TAB,		CODE_NONE,	9)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [5] - port ff07 bit 5 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "5  %",		KEYCODE_5,		CODE_NONE,	'5',	'%')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "-  =",		KEYCODE_EQUALS,		CODE_NONE,	'-',	'=')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "E",		KEYCODE_E,		CODE_NONE,	'e')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "M",		KEYCODE_M,		CODE_NONE,	'm')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "U",		KEYCODE_U,		CODE_NONE,	'u')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Space",	KEYCODE_SPACE,		CODE_NONE,	' ')
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Ctrl",		KEYCODE_LCONTROL,	CODE_NONE,	UCHAR_MAMEKEY(LCONTROL))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [6] - port ff07 bit 6 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "6  &",		KEYCODE_6,		CODE_NONE,	'6',	'&')
		PORT_KEY1(0x02, IP_ACTIVE_HIGH, ".",		KEYCODE_STOP,		CODE_NONE,	'.')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "F",		KEYCODE_F,		CODE_NONE,	'f')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "N",		KEYCODE_N,		CODE_NONE,	'n')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "V",		KEYCODE_V,		CODE_NONE,	'v')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Rept",		KEYCODE_INSERT,		CODE_NONE,	UCHAR_MAMEKEY(INSERT))
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Break",	KEYCODE_ESC,		CODE_NONE,	27)
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [7] - port ff07 bit 7 */
		PORT_KEY2(0x01, IP_ACTIVE_HIGH, "7  '",		KEYCODE_7,		CODE_NONE,	'7',	'\'')
		PORT_KEY2(0x02, IP_ACTIVE_HIGH, "/  ?",		KEYCODE_SLASH,		CODE_NONE,	'/',	'?')
		PORT_KEY1(0x04, IP_ACTIVE_HIGH, "G",		KEYCODE_G,		CODE_NONE,	'g')
		PORT_KEY1(0x08, IP_ACTIVE_HIGH, "O",		KEYCODE_O,		CODE_NONE,	'o')
		PORT_KEY1(0x10, IP_ACTIVE_HIGH, "W",		KEYCODE_W,		CODE_NONE,	'w')
		PORT_KEY1(0x20, IP_ACTIVE_HIGH, "Char del",	KEYCODE_DEL,		CODE_NONE,	UCHAR_MAMEKEY(DEL))
		PORT_KEY1(0x40, IP_ACTIVE_HIGH, "Shift",	KEYCODE_LSHIFT,		CODE_NONE,	UCHAR_MAMEKEY(LSHIFT))
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START /* [8] */
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)
		PORT_BIT(0xcb, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END


/* machine definition */
static MACHINE_DRIVER_START( dai )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_MEMORY(dai_readmem, dai_writemem)
	MDRV_CPU_PORTS(dai_readport, dai_writeport)
	MDRV_CPU_VBLANK_INT(dai_vblank_int, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( dai )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(528, 260)
	MDRV_VISIBLE_AREA(0, 528-1, 0, 260-1)
	MDRV_PALETTE_LENGTH(sizeof (dai_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (dai_colortable))
	MDRV_PALETTE_INIT( dai )

	MDRV_VIDEO_START( dai )
	MDRV_VIDEO_UPDATE( dai )
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
SYSTEM_CONFIG_END


/*     YEAR  NAME	PARENT  COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY	FULLNAME */
COMP( 1989, dai,	0,      0,		dai,	dai,	0,		dai,	"",		"DAI")
