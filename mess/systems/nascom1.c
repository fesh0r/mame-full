/************************************************************************
Nascom Memory map

	CPU: z80
		0000-03ff	ROM	(Nascom1 Monitor)
		0400-07ff	ROM	(Nascom2 Monitor extension)
		0800-0bff	RAM (Screen)
		0c00-0c7f	RAM (OS workspace)
		0c80-0cff	RAM (extended OS workspace)
		0d00-0f7f	RAM (Firmware workspace)
		0f80-0fff	RAM (Stack space)
		1000-8fff	RAM (User space)
		9000-97ff	RAM (Programmable graphics RAM/User space)
		9800-afff	RAM (Colour graphics RAM/User space)
		b000-b7ff	ROM (OS extensions)
		b800-bfff	ROM (WP/Naspen software)
		c000-cfff	ROM (Disassembler/colour graphics software)
		d000-dfff	ROM (Assembler/Basic extensions)
		e000-ffff	ROM (Nascom2 Basic)

	Interrupts:

	Ports:
		OUT (00)	0:	Increment keyboard scan
					1:	Reset keyboard scan
					2:
					3:	Read from cassette
					4:
					5:
					6:
					7:
		IN  (00)	Read keyboard
		OUT (01)	Write to cassette/serial
		IN  (01)	Read from cassette/serial
		OUT (02)	Unused
		IN  (02)	?

	Monitors:
		Nasbug1		1K	Original Nascom1
		Nasbug2     	1K
		Nasbug3     Probably non existing
		Nasbug4		2K
		Nassys1		2K	Original Nascom2
		Nassys2     Probably non existing
		Nassys3		2K
		Nassys4		2K
		T4			2K

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/nascom1.h"
#include "devices/cartslot.h"

/* port i/o functions */

PORT_READ_START( nascom1_readport )
	{ 0x00, 0x00, nascom1_port_00_r},
	{ 0x01, 0x01, nascom1_port_01_r},
	{ 0x02, 0x02, nascom1_port_02_r},
PORT_END

PORT_WRITE_START( nascom1_writeport )
	{ 0x00, 0x00, nascom1_port_00_w},
	{ 0x01, 0x01, nascom1_port_01_w},
PORT_END

/* Memory w/r functions */

MEMORY_READ_START( nascom1_readmem )
	{0x0000, 0x07ff, MRA_ROM},
	{0x0800, 0x0bff, videoram_r},
	{0x0c00, 0x0fff, MRA_RAM},
	{0x1000, 0x13ff, MRA_RAM},
	{0x1000, 0x13ff, MRA_RAM},	/* 1Kb */
	{0x1400, 0x4fff, MRA_RAM},	/* 16Kb */
	{0x5000, 0x8fff, MRA_RAM},	/* 32Kb */
	{0x9000, 0xafff, MRA_RAM},	/* 40Kb */
	{0xb000, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START( nascom1_writemem )
	{0x0000, 0x07ff, MWA_ROM},
	{0x0800, 0x0bff, videoram_w, &videoram, &videoram_size},
	{0x0c00, 0x0fff, MWA_RAM},
	{0x1000, 0x13ff, MWA_RAM},
	{0x1400, 0x4fff, MWA_RAM},
	{0x5000, 0x8fff, MWA_RAM},
	{0x9000, 0xafff, MWA_RAM},
	{0xb000, 0xffff, MWA_ROM},
MEMORY_END

/* graphics output */

struct	GfxLayout	nascom1_charlayout =
{
	8, 16,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	8 * 16
};

static	struct	GfxDecodeInfo	nascom1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &nascom1_charlayout, 0, 1},
MEMORY_END

struct	GfxLayout	nascom2_charlayout =
{
	8, 14,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8,  3*8,  4*8,  5*8,  6*8,
	  7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8 },
	8 * 16
};

static	struct	GfxDecodeInfo	nascom2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &nascom2_charlayout, 0, 1},
MEMORY_END

static	unsigned	char	nascom1_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static	unsigned	short	nascom1_colortable[] =
{
	0, 1
};

static PALETTE_INIT( nascom1 )
{
	palette_set_colors(0, nascom1_palette, sizeof(nascom1_palette) / 3);
	memcpy(colortable, nascom1_colortable, sizeof (nascom1_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (nascom1)
	PORT_START	/* 0: count = 0 */
	PORT_BIT(0x6f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START	/* 1: count = 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "up", KEYCODE_UP, IP_JOY_NONE)

	PORT_START	/* 2: count = 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "left", KEYCODE_LEFT, IP_JOY_NONE)

	PORT_START	/* 3: count = 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "down", KEYCODE_DOWN, IP_JOY_NONE)

	PORT_START	/* 4: count = 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "right", KEYCODE_RIGHT, IP_JOY_NONE)

	PORT_START	/* 5: count = 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 6: count = 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 7: count = 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 8: count = 8 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BIT(0x58, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_START	/* 9: Machine config */
	PORT_DIPNAME(0x03, 3, "RAM Size")
	PORT_DIPSETTING(0, "1Kb")
	PORT_DIPSETTING(1, "16Kb")
	PORT_DIPSETTING(2, "32Kb")
	PORT_DIPSETTING(3, "40Kb")
INPUT_PORTS_END

/* Sound output */

static INTERRUPT_GEN( nascom_interrupt )
{
	cpu_set_irq_line(0, 0, PULSE_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( nascom1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 1000000)
	MDRV_CPU_MEMORY(nascom1_readmem, nascom1_writemem)
	MDRV_CPU_PORTS(nascom1_readport, nascom1_writeport)
	MDRV_CPU_VBLANK_INT(nascom_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( nascom1 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(48 * 8, 16 * 16)
	MDRV_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 16 - 1)
	MDRV_GFXDECODE( nascom1_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH( sizeof (nascom1_palette) / 3 )
	MDRV_COLORTABLE_LENGTH( sizeof (nascom1_colortable) )
	MDRV_PALETTE_INIT( nascom1 )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( nascom1 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nascom2 )
	MDRV_IMPORT_FROM( nascom1 )
	MDRV_CPU_REPLACE( "main", Z80, 2000000 )
	MDRV_SCREEN_SIZE(48 * 8, 16 * 14)
	MDRV_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 14 - 1)
	MDRV_GFXDECODE( nascom2_gfxdecodeinfo )
	MDRV_VIDEO_UPDATE( nascom2 )
MACHINE_DRIVER_END

ROM_START(nascom1)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt1.rom", 0x0000, 0x0400, 0x0)
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
ROM_END

ROM_START(nascom1a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt2.rom", 0x0000, 0x0400, 0xe371b58a)
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
ROM_END

ROM_START(nascom1b)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nasbugt4.rom", 0x0000, 0x0800, 0xf391df68)
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
ROM_END

ROM_START(nascom2)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nassys1.rom", 0x0000, 0x0800, 0xb6300716)
	ROM_LOAD("basic.rom", 0xe000, 0x2000, 0x5cb5197b)
	ROM_REGION(0x1000, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
	ROM_LOAD("nasgra.chr", 0x0800, 0x0800, 0x2bc09d32)
ROM_END

ROM_START(nascom2a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("nassys3.rom", 0x0000, 0x0800, 0x3da17373)
	ROM_LOAD("basic.rom", 0xe000, 0x2000, 0x5cb5197b)
	ROM_REGION(0x1000, REGION_GFX1,0)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
	ROM_LOAD("nasgra.chr", 0x0800, 0x0800, 0x2bc09d32)
ROM_END

SYSTEM_CONFIG_START(nascom)
#ifdef CART
	CONFIG_DEVICE_CARTSLOT_REQ(1, "nas\0bin\0", nascom1_init_cartridge, NULL, NULL)
#endif
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(nascom1)
	CONFIG_IMPORT_FROM(nascom)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "nas\0bin\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_nascom1_cassette, device_unload_nascom1_cassette, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(nascom2)
	CONFIG_IMPORT_FROM(nascom)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "cas\0nas\0bin\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_nascom1_cassette, device_unload_nascom1_cassette, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT		MACHINE		INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP( 1978,	nascom1,	0,			nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T1)" )
COMP( 1978,	nascom1a,	nascom1,	nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T2)" )
COMP( 1978,	nascom1b,	nascom1,	nascom1,	nascom1,	0,		nascom1,	"Nascom Microcomputers",	"Nascom 1 (NasBug T4)" )
COMP( 1979,	nascom2,	nascom1,	nascom2,	nascom1,	0,		nascom2,	"Nascom Microcomputers",	"Nascom 2 (NasSys 1)" )
COMP( 1979,	nascom2a,	nascom1,	nascom2,	nascom1,	0,		nascom2,	"Nascom Microcomputers",	"Nascom 2 (NasSys 3)" )
