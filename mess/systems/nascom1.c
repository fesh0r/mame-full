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
		Nasbug2
		Nasbug3
		Nasbug4		2K
		Nassys1		2K	Original Nascom2
		Nassys2
		Nassys3		2K
		Nassys4		2K
		T4			2K

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "mess/includes/nascom1.h"

/* port i/o functions */

static	struct	IOReadPort	nascom1_readport[] =
{
	{ 0x00, 0x00, nascom1_port_00_r},
	{ 0x01, 0x01, nascom1_port_01_r},
	{ 0x02, 0x02, nascom1_port_02_r},
	{-1}
};

static	struct	IOWritePort	nascom1_writeport[] =
{
	{ 0x00, 0x00, nascom1_port_00_w},
	{ 0x01, 0x01, nascom1_port_01_w},
	{-1}
};

/* Memory w/r functions */

static	struct	MemoryReadAddress	nascom1_readmem[] =
{
	{0x0000, 0x07ff, MRA_ROM},
	{0x0800, 0x0bff, videoram_r},
	{0x0c00, 0x0fff, MRA_RAM},
	{0x1000, 0x13ff, MRA_RAM},
	{0x1000, 0x13ff, MRA_RAM},	/* 1Kb */
	{0x1400, 0x4fff, MRA_RAM},	/* 16Kb */
	{0x5000, 0x8fff, MRA_RAM},	/* 32Kb */
	{0x9000, 0xafff, MRA_RAM},	/* 40Kb */
	{0xb000, 0xffff, MRA_ROM},
	{-1}
};

static	struct	MemoryWriteAddress	nascom1_writemem[] =
{
	{0x0000, 0x07ff, MWA_ROM},
	{0x0800, 0x0bff, videoram_w, &videoram, &videoram_size},
	{0x0c00, 0x0fff, MWA_RAM},
	{0x1000, 0x13ff, MWA_RAM},
	{0x1400, 0x4fff, MWA_RAM},
	{0x5000, 0x8fff, MWA_RAM},
	{0x9000, 0xafff, MWA_RAM},
	{0xb000, 0xffff, MWA_ROM},
	{-1}
};

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
	{-1}
};

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
	{-1}
};

static	unsigned	char	nascom1_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static	unsigned	short	nascom1_colortable[] =
{
	0, 1
};

static	void	nascom1_init_palette (unsigned char *sys_palette,
			unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, nascom1_palette, sizeof (nascom1_palette));
	memcpy (sys_colortable, nascom1_colortable, sizeof (nascom1_colortable));
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

/* Machine definition */

static	struct	MachineDriver	machine_driver_nascom1 =
{
	{
		{
			CPU_Z80,
			1000000,
			nascom1_readmem, nascom1_writemem,
			nascom1_readport, nascom1_writeport,
			interrupt, 1,
		},
	},
	50, 2500,
	1,
	nascom1_init_machine,
	nascom1_stop_machine,
	48 * 8,
	16 * 16,
	{ 0, 48 * 8 - 1, 0, 16 * 16 - 1},
	nascom1_gfxdecodeinfo,
	sizeof (nascom1_palette) / 3,
	sizeof (nascom1_colortable),
	nascom1_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	nascom1_vh_start,
	nascom1_vh_stop,
	nascom1_vh_screenrefresh,
	0, 0, 0, 0,
};

static	struct	MachineDriver	machine_driver_nascom2 =
{
	{
		{
			CPU_Z80,
			2000000,
			nascom1_readmem, nascom1_writemem,
			nascom1_readport, nascom1_writeport,
			interrupt, 1,
		},
	},
	50, 2500,
	1,
	nascom1_init_machine,
	nascom1_stop_machine,
	48 * 8,
	16 * 14,
	{ 0, 48 * 8 - 1, 0, 16 * 14 - 1},
	nascom2_gfxdecodeinfo,
	sizeof (nascom1_palette) / 3,
	sizeof (nascom1_colortable),
	nascom1_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	nascom1_vh_start,
	nascom1_vh_stop,
	nascom2_vh_screenrefresh,
	0, 0, 0, 0,
};

ROM_START(nascom1)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("nasbugt2.rom", 0x0000, 0x0800, 0x00000001)
	ROM_REGION(0x0800, REGION_GFX1)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
ROM_END

ROM_START(nascom2)
	ROM_REGION(0x10000, REGION_CPU1)
	ROM_LOAD("nassys1.rom", 0x0000, 0x0800, 0xb6300716)
	ROM_LOAD("basic.rom", 0xe000, 0x2000, 0x5cb5197b)
	ROM_REGION(0x1000, REGION_GFX1)
	ROM_LOAD("nascom1.chr", 0x0000, 0x0800, 0x33e92a04)
	ROM_LOAD("nasgra.chr", 0x0800, 0x0800, 0x2bc09d32)
ROM_END

static	const	struct	IODevice	io_nascom1[] =
{
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"nas\0bin\0",			/* file extn */
		NULL,					/* private */
		NULL,					/* id */
		nascom1_init_cassette,	/* init */
		nascom1_exit_cassette,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{
		IO_CARTSLOT,			/* type */
		1,						/* count */
		"nas\0bin\0",			/* file extn */
		NULL,					/* private */
		NULL,					/* id */
		nascom1_init_cartridge,	/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

static	const	struct	IODevice	io_nascom2[] =
{
	{
		IO_CASSETTE,			/* type */
		1,						/* count */
		"cas\0nas\0bin\0",		/* file extn */
		NULL,					/* private */
		NULL,					/* id */
		nascom1_init_cassette,	/* init */
		nascom1_exit_cassette,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{
		IO_CARTSLOT,			/* type */
		1,						/* count */
		"nas\0bin\0",			/* file extn */
		NULL,					/* private */
		NULL,					/* id */
		nascom1_init_cartridge,	/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

/*		YEAR	NAME		PARENT		MACHINE		INPUT		INIT	COMPANY		FULLNAME */
COMP(	1978,	nascom1,	0,			nascom1,	nascom1,	0,		"Nascom",	"Nascom 1" )
COMP(	1979,	nascom2,	nascom1,	nascom2,	nascom1,	0,		"Nascom",	"Nascom 2" )
