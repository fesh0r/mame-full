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

ADDRESS_MAP_START( jupiter_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0xfefe, 0xfefe) AM_READ( jupiter_port_fefe_r )
	AM_RANGE( 0xfdfe, 0xfdfe) AM_READ( jupiter_port_fdfe_r )
	AM_RANGE( 0xfbfe, 0xfbfe) AM_READ( jupiter_port_fbfe_r )
	AM_RANGE( 0xf7fe, 0xf7fe) AM_READ( jupiter_port_f7fe_r )
	AM_RANGE( 0xeffe, 0xeffe) AM_READ( jupiter_port_effe_r )
	AM_RANGE( 0xdffe, 0xdffe) AM_READ( jupiter_port_dffe_r )
	AM_RANGE( 0xbffe, 0xbffe) AM_READ( jupiter_port_bffe_r )
	AM_RANGE( 0x7ffe, 0x7ffe) AM_READ( jupiter_port_7ffe_r )
ADDRESS_MAP_END

ADDRESS_MAP_START( jupiter_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00fe, 0xfffe) AM_WRITE( jupiter_port_fe_w )
ADDRESS_MAP_END

/* memory w/r functions */

ADDRESS_MAP_START( jupiter_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_READ( MRA8_ROM )
	AM_RANGE( 0x2000, 0x22ff) AM_READ( MRA8_NOP )
	AM_RANGE( 0x2300, 0x23ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x2400, 0x26ff) AM_READ( videoram_r )
	AM_RANGE( 0x2700, 0x27ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x2800, 0x2bff) AM_READ( MRA8_NOP )
	AM_RANGE( 0x2c00, 0x2fff) AM_READ( MRA8_RAM )	/* char RAM */
	AM_RANGE( 0x3000, 0x3bff) AM_READ( MRA8_NOP )
	AM_RANGE( 0x3c00, 0x47ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x4800, 0x87ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x8800, 0xffff) AM_READ( MRA8_RAM )
ADDRESS_MAP_END

ADDRESS_MAP_START( jupiter_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0x2000, 0x22ff) AM_WRITE( MWA8_NOP )
	AM_RANGE( 0x2300, 0x23ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x2400, 0x26ff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size )
	AM_RANGE( 0x2700, 0x27ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x2800, 0x2bff) AM_WRITE( MWA8_NOP )
	AM_RANGE( 0x2c00, 0x2fff) AM_WRITE( jupiter_vh_charram_w) AM_BASE( &jupiter_charram) AM_SIZE( &jupiter_charram_size )
	AM_RANGE( 0x3000, 0x3bff) AM_WRITE( MWA8_NOP )
	AM_RANGE( 0x3c00, 0x47ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x4800, 0x87ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x8800, 0xffff) AM_WRITE( MWA8_RAM )
ADDRESS_MAP_END

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
	{-1}
};								   /* end of array */

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
	cpunum_set_input_line(0, 0, PULSE_LINE);
}

/* machine definition */
static MACHINE_DRIVER_START( jupiter )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3250000)        /* 3.25 Mhz */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_PROGRAM_MAP(jupiter_readmem, jupiter_writemem)
	MDRV_CPU_IO_MAP(jupiter_readport, jupiter_writeport)
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
	ROM_LOAD ("jupiter.lo", 0x0000, 0x1000, CRC(dc8438a5) SHA1(8fa97eb71e5dd17c7d190c6587ee3840f839347c))
	ROM_LOAD ("jupiter.hi", 0x1000, 0x1000, CRC(4009f636) SHA1(98c5d4bcd74bcf014268cf4c00b2007ea5cc21f3))
ROM_END

SYSTEM_CONFIG_START(jupiter)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "ace\0", NULL, NULL, device_load_jupiter_ace, NULL, NULL, NULL)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "tap\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_READ, NULL, NULL, device_load_jupiter_tap, device_unload_jupiter_tap, NULL)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
COMP( 1981, jupiter,  0,		0,		jupiter,  jupiter,	0,		  jupiter,	"Cantab",  "Jupiter Ace" )
