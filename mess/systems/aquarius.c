/************************************************************************
Aquarius Memory map

	CPU: z80

	Memory map
		0000 1fff	BASIC
		2000 2fff	expansion?
		3000 33ff	screen ram
		3400 37ff	colour ram
		3800 3fff	RAM (standard)
		4000 7fff	RAM (expansion)
		8000 ffff	RAM (emulator only)

	Ports: Out
		fc			Buzzer, bit 0.
		fe			Printer.

	Ports: In
		fc			Tape in, bit 1.
		fe			Printer.
		ff			Keyboard, Bit set in .B selects keyboard matrix
					line. Return bit 0 - 5 low for pressed key.

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/aquarius.h"

/* structures */

/* port i/o functions */

PORT_READ_START( aquarius_readport )
	{0xfe, 0xfe, aquarius_port_fe_r},
	{0xff, 0xff, aquarius_port_ff_r},
PORT_END

PORT_WRITE_START( aquarius_writeport )
	{0xfc, 0xfc, aquarius_port_fc_w},
	{0xfe, 0xfe, aquarius_port_fe_w},
	{0xff, 0xff, aquarius_port_ff_w},
PORT_END

/* Memory w/r functions */

MEMORY_READ_START( aquarius_readmem )
	{0x0000, 0x1fff, MRA8_ROM},
	{0x2000, 0x2fff, MRA8_NOP},
	{0x3000, 0x37ff, videoram_r},
	{0x3800, 0x3fff, MRA8_RAM},
	{0x4000, 0x7fff, MRA8_NOP},
	{0x8000, 0xffff, MRA8_NOP},

MEMORY_END

MEMORY_WRITE_START( aquarius_writemem )
	{0x0000, 0x1fff, MWA8_ROM},
	{0x2000, 0x2fff, MWA8_NOP},
	{0x3000, 0x37ff, aquarius_videoram_w, &videoram, &videoram_size},
	{0x3800, 0x3fff, MWA8_RAM},
	{0x4000, 0x7fff, MWA8_NOP},
	{0x8000, 0xffff, MWA8_NOP},
MEMORY_END

/* graphics output */

struct	GfxLayout	aquarius_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8,
	  4*8, 5*8, 6*8, 7*8, },
	8 * 8
};

static struct GfxDecodeInfo aquarius_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &aquarius_charlayout, 0, 256},
	{ -1 } /* end of array */
};

static unsigned char aquarius_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0x00, 0x00,	/* Red */
	0x00, 0xff, 0x00,	/* Green */
	0xff, 0xff, 0x00,	/* Yellow */
	0x00, 0x00, 0xff,	/* Blue */
	0x7f, 0x00, 0x7f,	/* Violet */
	0x7f, 0xff, 0xff,	/* Light Blue-Green */
	0xff, 0xff, 0xff,	/* White */
	0xc0, 0xc0, 0xc0,	/* Light Gray */
	0x00, 0xff, 0xff,	/* Blue-Green */
	0xff, 0x00, 0xff,	/* Magenta */
	0x00, 0x00, 0x7f,	/* Dark Blue */
	0xff, 0xff, 0x7f,	/* Light Yellow */
	0x7f, 0xff, 0x7f,	/* Light Green */
	0xff, 0x7f, 0x00,	/* Orange */
	0x7f, 0x7f, 0x7f,	/* Dark Gray */
};

static	unsigned	short	aquarius_colortable[] =
{
    0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15, 0,
    0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15, 1,
    0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15, 2,
    0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15, 3,
    0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15, 4,
    0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15, 5,
    0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15, 6,
    0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15, 7,
    0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15, 8,
    0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15, 9,
    0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,10,
    0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,11,
    0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,12,
    0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,13,
    0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,14,
    0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,15,
};

static PALETTE_INIT( aquarius )
{
	palette_set_colors(0, aquarius_palette, sizeof(aquarius_palette) / 3);
	memcpy(colortable, aquarius_colortable, sizeof (aquarius_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (aquarius)
	PORT_START	/* 0: count = 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "=     + NEXT", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Del   \\", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":     * PEEK", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Rtn", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ";     @ POKE", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ".     > VAL", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 1: count = 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "-     _ FOR", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "/     ^", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "0     ?", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "p     P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l     L POINT", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",     <  STR$", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 2: count = 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "9     ) COPY", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "o     O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "k     K PRESET", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "m     M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "n     N RIGHT$", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "j     J PSET", KEYCODE_J, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 3: count = 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8     ( RETURN", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "i     I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7     ' GOSUB", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "u     U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "h     H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "b     B MID$", KEYCODE_B, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 4: count = 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6     & ON", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "y     Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "g     G BELL", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "v     V LEFT$", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "c     C STOP", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f     F DATA", KEYCODE_F, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 5: count = 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "5     % GOTO", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "t     T INPUT", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4     $ THEN", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "r     R RETYP", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "d     D READ", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "x     X DELINE", KEYCODE_X, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 6: count = 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "3     # IF", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "e     E DIM", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "s     S STPLST", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "z     Z CLOAD", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space   CHR$", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a     A CSAVE", KEYCODE_A, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 7: count = 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "2     \" LIST", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "w     W REM", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "1     ! RUN", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "q     Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Ctl", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 8: Machine config */
	PORT_DIPNAME(0x03, 3, "RAM Size")
	PORT_DIPSETTING(0, "4 Kb")
	PORT_DIPSETTING(1, "20 Kb")
	PORT_DIPSETTING(2, "56 Kb")
INPUT_PORTS_END

/* Sound output */

static struct Speaker_interface aquarius_speaker =
{
	1,			/* one speaker */
	{ 100 },	/* mixing levels */
	{ 0 },		/* optional: number of different levels */
	{ NULL }	/* optional: level lookup table */
};

static INTERRUPT_GEN( aquarius_interrupt )
{
	cpu_set_irq_line(0, 0, PULSE_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( aquarius )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3500000)
	MDRV_CPU_MEMORY(aquarius_readmem, aquarius_writemem)
	MDRV_CPU_PORTS(aquarius_readport, aquarius_writeport)
	MDRV_CPU_VBLANK_INT( aquarius_interrupt, 1 )
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( aquarius )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40 * 8, 24 * 8)
	MDRV_VISIBLE_AREA(0, 40 * 8 - 1, 0, 24 * 8 - 1)
	MDRV_GFXDECODE( aquarius_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (aquarius_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (aquarius_colortable))
	MDRV_PALETTE_INIT( aquarius )

	MDRV_VIDEO_START( aquarius )
	MDRV_VIDEO_UPDATE( aquarius )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, aquarius_speaker)
MACHINE_DRIVER_END


ROM_START(aquarius)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("aq2.rom", 0x0000, 0x2000, CRC(a2d15bcf))
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("aq2.chr", 0x0000, 0x0800, BAD_DUMP CRC(0b3edeed))
ROM_END

/*		YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY		FULLNAME */
COMP(	1983,	aquarius,	0,		0,		aquarius,	aquarius,	0,		NULL,		"Mattel",	"Aquarius" )
