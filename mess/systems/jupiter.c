/***************************************************************************
Jupiter Ace memory map

	CPU: Z80
		0000-1fff ROM
		2000-22ff unused
		2300-23ff RAM (cassette buffer)
		2400-26ff RAM (screen)
		2700-27ff RAM (edit buffer)
		2800-2bff unused
		2c00-2fff RAM (char set)
		3000-3bff unused
		3c00-47ff RAM (standard)
		4800-87ff RAM (16K expansion)
		8800-ffff RAM (Expansion)

Interrupts:

	IRQ:
		50Hz vertical sync

Ports:

	Out 0xfe:
		Tape and buzzer

	In 0xfe:
		Keyboard input and buzzer
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/jupiter.h"
#include "devices/cartslot.h"

/* port i/o functions */

PORT_READ_START( jupiter_readport )
	{ 0xfefe, 0xfefe, jupiter_port_fefe_r },
	{ 0xfdfe, 0xfdfe, jupiter_port_fdfe_r },
	{ 0xfbfe, 0xfbfe, jupiter_port_fbfe_r },
	{ 0xf7fe, 0xf7fe, jupiter_port_f7fe_r },
	{ 0xeffe, 0xeffe, jupiter_port_effe_r },
	{ 0xdffe, 0xdffe, jupiter_port_dffe_r },
	{ 0xbffe, 0xbffe, jupiter_port_bffe_r },
	{ 0x7ffe, 0x7ffe, jupiter_port_7ffe_r },
PORT_END

PORT_WRITE_START( jupiter_writeport )
	{ 0x00fe, 0xfffe, jupiter_port_fe_w },
PORT_END

/* memory w/r functions */

MEMORY_READ_START( jupiter_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x22ff, MRA_NOP },
	{ 0x2300, 0x23ff, MRA_RAM },
	{ 0x2400, 0x26ff, videoram_r },
	{ 0x2700, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2bff, MRA_NOP },
	{ 0x2c00, 0x2fff, MRA_RAM },	/* char RAM */
	{ 0x3000, 0x3bff, MRA_NOP },
	{ 0x3c00, 0x47ff, MRA_RAM },
	{ 0x4800, 0x87ff, MRA_RAM },
	{ 0x8800, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( jupiter_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x22ff, MWA_NOP },
	{ 0x2300, 0x23ff, MWA_RAM },
	{ 0x2400, 0x26ff, videoram_w, &videoram, &videoram_size },
	{ 0x2700, 0x27ff, MWA_RAM },
	{ 0x2800, 0x2bff, MWA_NOP },
	{ 0x2c00, 0x2fff, jupiter_vh_charram_w, &jupiter_charram, &jupiter_charram_size },
	{ 0x3000, 0x3bff, MWA_NOP },
	{ 0x3c00, 0x47ff, MWA_RAM },
	{ 0x4800, 0x87ff, MWA_RAM },
	{ 0x8800, 0xffff, MWA_RAM },
MEMORY_END

/* graphics output */

struct GfxLayout jupiter_charlayout =
{
	8, 8,	/* 8x8 characters */
	128,	/* 128 characters */
	1,		/* 1 bits per pixel */
	{0},	/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8 	/* each character takes 8 consecutive bytes */
};

static struct GfxDecodeInfo jupiter_gfxdecodeinfo[] =
{
	{REGION_CPU1, 0x2c00, &jupiter_charlayout, 0, 2},
MEMORY_END								   /* end of array */

static unsigned char jupiter_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static unsigned short jupiter_colortable[] =
{
	0, 1,
	1, 0
};

static PALETTE_INIT( jupiter )
{
	palette_set_colors(0, jupiter_palette, sizeof(jupiter_palette) / 3);
	memcpy (colortable, jupiter_colortable, sizeof (jupiter_colortable));
}

/* keyboard input */

INPUT_PORTS_START (jupiter)
	PORT_START	/* 0: 0xFEFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYM SHFT", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)

	PORT_START	/* 1: 0xFDFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)

	PORT_START	/* 2: 0xFBFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START	/* 3: 0xF7FE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)

	PORT_START	/* 4: 0xEFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)

	PORT_START	/* 5: 0xDFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START	/* 6: 0xBFFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)

	PORT_START	/* 7: 0x7FFE */
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_START	/* 8: machine config */
	PORT_DIPNAME( 0x03, 2, "RAM Size")
	PORT_DIPSETTING(0, "3Kb")
	PORT_DIPSETTING(1, "19Kb")
	PORT_DIPSETTING(2, "49Kb")
INPUT_PORTS_END

/* Sound output */

static struct Speaker_interface speaker_interface =
{
	1,			/* one speaker */
	{ 100 },	/* mixing levels */
	{ 0 },		/* optional: number of different levels */
	{ NULL }	/* optional: level lookup table */
};

static INTERRUPT_GEN( jupiter_interrupt )
{
	cpu_set_irq_line(0, 0, PULSE_LINE);
}

/* machine definition */
static MACHINE_DRIVER_START( jupiter )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3250000)        /* 3.25 Mhz */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(jupiter_readmem, jupiter_writemem)
	MDRV_CPU_PORTS(jupiter_readport, jupiter_writeport)
	MDRV_CPU_VBLANK_INT(jupiter_interrupt,1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( jupiter )
	MDRV_MACHINE_STOP( jupiter )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32 * 8, 24 * 8)
	MDRV_VISIBLE_AREA(0, 32 * 8 - 1, 0, 24 * 8 - 1)
	MDRV_GFXDECODE( jupiter_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(jupiter_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (jupiter_colortable))
	MDRV_PALETTE_INIT( jupiter )

	MDRV_VIDEO_START( jupiter )
	MDRV_VIDEO_UPDATE( jupiter )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
MACHINE_DRIVER_END

ROM_START (jupiter)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("jupiter.lo", 0x0000, 0x1000,CRC( 0xdc8438a5))
	ROM_LOAD ("jupiter.hi", 0x1000, 0x1000,CRC( 0x4009f636))
ROM_END

SYSTEM_CONFIG_START(jupiter)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "ace\0", NULL, NULL, device_load_jupiter_ace, NULL, NULL, NULL)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "tap\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_READ, NULL, NULL, device_load_jupiter_tap, device_unload_jupiter_tap, NULL)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
COMP( 1981, jupiter,  0,		0,		jupiter,  jupiter,	0,		  jupiter,	"Cantab",  "Jupiter Ace" )
